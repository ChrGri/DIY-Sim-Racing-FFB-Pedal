using NCalc;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    // Effect pipelines: user scripts (NCalc or JavaScript) compute offset samples
    // from SimHub properties; the samples are streamed to the pedal, which plays
    // them back on its own clock (EffectPipeline.h in the firmware).
    //
    // Any number of script slots per pedal (DIYFFBPedalSettings.Pipeline_slots).
    // All enabled slots of a pedal are summed per output type; each type has its
    // own channel to the pedal (channel = type - 1).
    //
    // Every telemetry update produces one packet per used channel with the
    // samples for the next ~42 ms on a 500 Hz sample clock. The packets overlap,
    // so jitter and single lost packets don't reach the pedal.
    //
    // Script variables (in addition to every SimHub property):
    //   t     time of the sample in seconds (continuous)
    //   dt    sample period in seconds (0.002)
    //   prev  previous result of this slot (before gain), for filters
    //   phase oscillator phase in radians, integrated from the optional frequency
    //         script, so Sin(phase) stays continuous when the frequency changes
    //   pi    3.14159...
    // NCalc: properties as [DataCorePlugin.GameData.Rpms]
    // JavaScript: $prop('DataCorePlugin.GameData.Rpms'), root keeps values between calls
    // Output unit depends on the offset type: N, degrees pedal angle or relative (0.1 = 10 %).
    public class EffectPipelineEngine
    {
        // limits of the sum per pedal and type (the pedal limits again)
        public const double MaxForce_N = 60.0;
        public const double MaxAngle_deg = 2.0;
        public const double MaxForceRelative = 0.8;
        // highest frequency of the phase oscillator (frequency field): the pedal damps
        // vibrations above ~40 Hz strongly (servo, 5 ms effect smoothing in the firmware)
        public const double MaxFrequency_Hz = 40.0;

        public const int OutputTypes = 3; // Force_N, Angle_deg, ForceRelative
        private const int SamplesPerPacket = Constants.effectPipelineSamplesPerPacket;
        private const double SamplePeriod_s = Constants.effectPipelineSamplePeriod_s;
        private const double MinSendInterval_s = 0.010;
        // samples computed ahead of now (~one telemetry update); the rest of a packet repeats
        // the last computed value and is replaced by the next packets before the pedal plays it
        private const int LeadSamples = 8;
        private const int RingSize = 128;

        private readonly DIY_FFB_Pedal _plugin;
        private readonly Stopwatch _clock = Stopwatch.StartNew();
        private double _lastSend_s = -1.0;

        private class SlotState
        {
            // compiled script, rebuilt when text or language change
            public string Script;
            public string FreqScript;
            public PipelineScriptLanguage Language;
            public bool Compiled;
            public Expression Expression;
            public Expression FreqExpression;
            public JintScriptHost Js;

            public double Phase;
            // every sample is computed exactly once, in time order (scripts keep state in
            // prev, phase and the JavaScript root object). Ring of the computed raw values.
            public bool HasNext;
            public uint NextIndex;
            public readonly double[] Ring = new double[RingSize];
            public readonly uint[] RingIndex = new uint[RingSize];
            public double Prev;
            public double T;
            public double LastOutput;
            public string Error;
            public double EvalTime_ms; // per second of samples, smoothed
        }

        private readonly Dictionary<PipelineScriptSlot, SlotState> _states = new Dictionary<PipelineScriptSlot, SlotState>();

        // History of the sent samples per pedal and output type for the live plot, 5 s at 500 Hz.
        // The packets overlap; a later packet overwrites its predicted samples with newer values.
        public const int HistoryLength = 2500;
        private class History
        {
            public readonly double[] Values = new double[HistoryLength];
            public readonly uint[] Indices = new uint[HistoryLength];
            public readonly bool[] Valid = new bool[HistoryLength];
        }
        private readonly History[] _history = new History[3 * OutputTypes];

        // SimHub property values, read once per telemetry update
        private readonly Dictionary<string, object> _propertyCache = new Dictionary<string, object>();
        private PluginManager _pluginManager;
        // Update() runs on the telemetry thread, the UI calls from its thread
        private readonly object _sync = new object();

        public EffectPipelineEngine(DIY_FFB_Pedal plugin)
        {
            _plugin = plugin;
            for (int i = 0; i < _history.Length; i++) _history[i] = new History();
        }

        public static int TypeIndex(EffectPipelineOffsetType type)
        {
            switch (type)
            {
                case EffectPipelineOffsetType.Force_N: return 0;
                case EffectPipelineOffsetType.Angle_deg: return 1;
                case EffectPipelineOffsetType.ForceRelative: return 2;
                default: return -1;
            }
        }

        public static EffectPipelineOffsetType TypeFromIndex(int index)
        {
            return index == 0 ? EffectPipelineOffsetType.Force_N
                 : index == 1 ? EffectPipelineOffsetType.Angle_deg
                 : EffectPipelineOffsetType.ForceRelative;
        }

        // ---- test mode: simulated property values instead of a game ----
        // Scripts run without a game and see the override values instead of the real
        // properties (only the scripts, SimHub itself is not changed). The pedal values
        // (force, travel) stay real. Ends by itself after TestTimeout_s without input,
        // and as soon as a game sends telemetry.

        public const double TestTimeout_s = 60.0;
        private bool _testActive;
        private double _testLastTouch_s;
        private readonly Dictionary<string, object> _overrides = new Dictionary<string, object>();
        private EffectTestScenario _scenario;
        private double _scenarioStart_s;
        private System.Threading.Timer _testTimer;

        public bool TestActive { get { lock (_sync) return _testActive; } }

        public string ScenarioName { get { lock (_sync) return _scenario != null ? _scenario.LocalName : null; } }

        // seconds until the test mode switches itself off
        public double TestRemaining_s
        {
            get { lock (_sync) return _testActive ? Math.Max(0.0, TestTimeout_s - (_clock.Elapsed.TotalSeconds - _testLastTouch_s)) : 0.0; }
        }

        // SimHub properties a script reads: [Name] (NCalc), $prop('Name') and quoted
        // property names (JavaScript). Pedal values of this plugin are left out, they stay real.
        public static List<string> FindProperties(PipelineScriptSlot slot)
        {
            var found = new List<string>();
            if (slot == null) return found;
            string text = (slot.Script ?? "") + "\n" + (slot.FreqScript ?? "");
            var patterns = slot.Language == PipelineScriptLanguage.JavaScript
                ? new[] { @"\$prop\(\s*['""]([^'""]+)['""]\s*\)", @"['""]([A-Za-z]\w*Plugin\.[\w\.\-]+)['""]" }
                : new[] { @"\[([A-Za-z_][\w\.\-]*)\]" };
            foreach (string pattern in patterns)
            {
                foreach (System.Text.RegularExpressions.Match m in System.Text.RegularExpressions.Regex.Matches(text, pattern))
                {
                    string name = m.Groups[1].Value;
                    if (name.StartsWith("DIY_FFB_Pedal.PedalStatus.")) continue;
                    if (!found.Contains(name)) found.Add(name);
                }
            }
            return found;
        }

        public void StartTest()
        {
            lock (_sync)
            {
                _testLastTouch_s = _clock.Elapsed.TotalSeconds;
                if (_testActive) return;
                _testActive = true;
                // without a game DataUpdate does not drive the engine: own ~60 Hz clock
                _testTimer = new System.Threading.Timer(_ => TestTick(), null, 0, 16);
            }
        }

        public void StopTest()
        {
            lock (_sync)
            {
                _testActive = false;
                _scenario = null;
                if (_testTimer != null)
                {
                    _testTimer.Dispose();
                    _testTimer = null;
                }
                foreach (SlotState st in _states.Values)
                {
                    st.LastOutput = 0.0;
                    st.Prev = 0.0;
                }
            }
        }

        // user input in the test window, restarts the timeout
        public void TouchTest()
        {
            lock (_sync) _testLastTouch_s = _clock.Elapsed.TotalSeconds;
        }

        public void SetOverride(string name, object value)
        {
            lock (_sync)
            {
                _overrides[name] = value;
                _testLastTouch_s = _clock.Elapsed.TotalSeconds;
            }
        }

        public object GetOverride(string name)
        {
            lock (_sync)
            {
                object value;
                return _overrides.TryGetValue(name, out value) ? value : null;
            }
        }

        public void ClearOverrides()
        {
            lock (_sync) _overrides.Clear();
        }

        public void StartScenario(EffectTestScenario scenario)
        {
            lock (_sync)
            {
                _scenario = scenario;
                _scenarioStart_s = _clock.Elapsed.TotalSeconds;
                _testLastTouch_s = _scenarioStart_s;
            }
            StartTest();
        }

        public void StopScenario()
        {
            lock (_sync) _scenario = null;
        }

        private void TestTick()
        {
            try
            {
                PluginManager pm = _plugin.PluginManager;
                if (pm != null) Update(pm, false, true);
            }
            catch (Exception ex)
            {
                SimHub.Logging.Current.Error("Script effects test mode: " + ex.Message);
            }
        }

        // ---- slot list, changed from the UI ----

        // template null: empty script; keepName: from the effect library (else: "... copy")
        public PipelineScriptSlot AddSlot(int pedal, PipelineScriptSlot template, bool keepName = false)
        {
            lock (_sync)
            {
                DIYFFBPedalSettings settings = _plugin.Settings;
                var slot = template != null ? template.Clone() : new PipelineScriptSlot();
                slot.Pedal = pedal;
                if (template == null)
                {
                    int count = 0;
                    foreach (var s in settings.Pipeline_slots) if (s.Pedal == pedal) count++;
                    slot.Name = Loc.T("TextScriptDefaultName", "Script") + " " + (count + 1);
                }
                else
                {
                    if (!keepName) slot.Name = (template.Name ?? "") + " copy";
                    slot.Enabled = false;
                }
                settings.Pipeline_slots.Add(slot);
                return slot;
            }
        }

        public void RemoveSlot(PipelineScriptSlot slot)
        {
            lock (_sync)
            {
                _plugin.Settings.Pipeline_slots.Remove(slot);
                _states.Remove(slot);
            }
        }

        public List<PipelineScriptSlot> GetSlots(int pedal)
        {
            lock (_sync)
            {
                var list = new List<PipelineScriptSlot>();
                DIYFFBPedalSettings settings = _plugin.Settings;
                if (settings == null || settings.Pipeline_slots == null) return list;
                foreach (var s in settings.Pipeline_slots) if (s.Pedal == pedal) list.Add(s);
                return list;
            }
        }

        // Replaces all slots of a single pedal (e.g. when applying a Profile) and drops stale states.
        public void ReplacePedalSlots(int pedal, IEnumerable<PipelineScriptSlot> newSlots)
        {
            lock (_sync)
            {
                DIYFFBPedalSettings settings = _plugin.Settings;
                if (settings == null) return;
                if (settings.Pipeline_slots == null) settings.Pipeline_slots = new List<PipelineScriptSlot>();

                for (int i = settings.Pipeline_slots.Count - 1; i >= 0; i--)
                {
                    if (settings.Pipeline_slots[i].Pedal == pedal)
                    {
                        _states.Remove(settings.Pipeline_slots[i]);
                        settings.Pipeline_slots.RemoveAt(i);
                    }
                }
                if (newSlots != null)
                {
                    foreach (var s in newSlots)
                    {
                        if (s == null) continue;
                        s.Pedal = pedal;
                        settings.Pipeline_slots.Add(s);
                    }
                }
            }
        }

        // Syncs all mounted slots referencing fileName across all pedals after an edit in ScriptEditorWindow.
        public void ReloadFromPresetFile(string fileName, EffectPreset preset)
        {
            if (string.IsNullOrWhiteSpace(fileName) || preset == null) return;
            lock (_sync)
            {
                DIYFFBPedalSettings settings = _plugin.Settings;
                if (settings == null || settings.Pipeline_slots == null) return;
                foreach (var slot in settings.Pipeline_slots)
                {
                    if (string.Equals(slot.ScriptFile, fileName, StringComparison.OrdinalIgnoreCase))
                    {
                        slot.ApplyFromPreset(preset);
                    }
                }
            }
        }

        // ---- status for the UI ----

        public double GetLastOutput(PipelineScriptSlot slot)
        {
            lock (_sync)
            {
                SlotState st;
                return _states.TryGetValue(slot, out st) ? st.LastOutput : 0.0;
            }
        }

        // null when the script is fine
        public string GetError(PipelineScriptSlot slot)
        {
            lock (_sync)
            {
                SlotState st;
                return _states.TryGetValue(slot, out st) ? st.Error : null;
            }
        }

        // evaluation time in ms per second, i.e. 10 = 1 % of one CPU core
        public double GetEvalTime(PipelineScriptSlot slot)
        {
            lock (_sync)
            {
                SlotState st;
                return _states.TryGetValue(slot, out st) ? st.EvalTime_ms : 0.0;
            }
        }

        // Current index of the plugin sample clock (500 Hz).
        public uint CurrentSampleIndex
        {
            get { return (uint)(_clock.Elapsed.TotalSeconds / SamplePeriod_s); }
        }

        // Copies the sent samples of one pedal and output type for the sample indices
        // [endIndex - dest.Length + 1, endIndex] into dest (0 where nothing was sent).
        public void CopyHistory(int pedal, EffectPipelineOffsetType type, uint endIndex, double[] dest)
        {
            int t = TypeIndex(type);
            if (pedal < 0 || pedal > 2 || t < 0) return;
            lock (_sync)
            {
                History h = _history[pedal * OutputTypes + t];
                uint first = endIndex - (uint)dest.Length + 1;
                for (int i = 0; i < dest.Length; i++)
                {
                    uint sampleIndex = first + (uint)i;
                    int slot = (int)(sampleIndex % HistoryLength);
                    dest[i] = (h.Valid[slot] && h.Indices[slot] == sampleIndex) ? h.Values[slot] : 0.0;
                }
            }
        }

        // ---- streaming ----

        // Called from DataUpdate (and from the test mode clock).
        public void Update(PluginManager pluginManager, bool gameRunning)
        {
            Update(pluginManager, gameRunning, false);
        }

        private void Update(PluginManager pluginManager, bool gameRunning, bool fromTestClock)
        {
            bool stopTest = false;
            lock (_sync)
            {
                if (_testActive)
                {
                    double now_s = _clock.Elapsed.TotalSeconds;
                    // a running game has priority; no input for a while: switch off
                    if (gameRunning || now_s - _testLastTouch_s > TestTimeout_s) stopTest = true;
                    else if (_scenario != null)
                    {
                        double t = (now_s - _scenarioStart_s) % _scenario.Duration_s;
                        _scenario.Values(t, _overrides);
                    }
                }
                if (!stopTest)
                {
                    if (_testActive) gameRunning = true;
                    else if (fromTestClock) return;
                    UpdateLocked(pluginManager, gameRunning);
                }
            }
            if (stopTest) StopTest();
        }

        private void UpdateLocked(PluginManager pluginManager, bool gameRunning)
        {
            DIYFFBPedalSettings settings = _plugin.Settings;
            if (settings == null || settings.Pipeline_slots == null) return;
            // scripts move the pedal actively: nothing runs before the user accepted the warning
            if (!settings.Pipeline_warning_ack) return;

            double now_s = _clock.Elapsed.TotalSeconds;
            if (_lastSend_s >= 0.0 && (now_s - _lastSend_s) < MinSendInterval_s) return;
            _lastSend_s = now_s;

            _pluginManager = pluginManager;
            _propertyCache.Clear();
            uint startIndex = (uint)(now_s / SamplePeriod_s);

            // forget slots that were removed
            if (_states.Count > settings.Pipeline_slots.Count)
            {
                var keep = new HashSet<PipelineScriptSlot>(settings.Pipeline_slots);
                var removed = new List<PipelineScriptSlot>();
                foreach (var key in _states.Keys) if (!keep.Contains(key)) removed.Add(key);
                foreach (var key in removed) _states.Remove(key);
            }

            for (int pedal = 0; pedal < 3; pedal++)
            {
                bool pedalActive = gameRunning && UIFunction.ServoLedIndicator.IsPedalConnected(_plugin._calculations, pedal);
                double[][] sums = new double[OutputTypes][];
                double[] values = new double[SamplesPerPacket];

                foreach (PipelineScriptSlot slot in settings.Pipeline_slots)
                {
                    if (slot.Pedal != pedal) continue;
                    SlotState state = GetState(slot);
                    int t = TypeIndex((EffectPipelineOffsetType)slot.OffsetType);
                    if (!pedalActive || !slot.Enabled || string.IsNullOrWhiteSpace(slot.Script) || t < 0)
                    {
                        state.LastOutput = 0.0;
                        state.Prev = 0.0;
                        state.HasNext = false;
                        continue;
                    }

                    long ticks = Stopwatch.GetTimestamp();
                    int computed;
                    bool ok = ComputeSlot(slot, state, startIndex, values, out computed);
                    double ms = (Stopwatch.GetTimestamp() - ticks) * 1000.0 / Stopwatch.Frequency;
                    if (computed > 0)
                    {
                        // cost per second of samples
                        double msPerSecond = ms / (computed * SamplePeriod_s);
                        state.EvalTime_ms += 0.1 * (msPerSecond - state.EvalTime_ms);
                    }
                    if (!ok) continue;

                    if (sums[t] == null) sums[t] = new double[SamplesPerPacket];
                    for (int i = 0; i < SamplesPerPacket; i++) sums[t][i] += values[i] * slot.Gain;
                }

                for (int t = 0; t < OutputTypes; t++)
                {
                    // no slot for this type: nothing is sent, the pedal fades the channel out
                    if (sums[t] == null) continue;
                    EffectPipelineOffsetType type = TypeFromIndex(t);
                    double limit, lsb;
                    GetLimit(type, out limit, out lsb);
                    short[] samples = new short[SamplesPerPacket];
                    for (int i = 0; i < SamplesPerPacket; i++)
                        samples[i] = (short)Math.Round(Math.Max(-limit, Math.Min(limit, sums[t][i])) / lsb);
                    RecordHistory(pedal, t, lsb, startIndex, samples);
                    SendPacket((byte)pedal, (byte)t, type, startIndex, samples);
                }
            }
        }

        private SlotState GetState(PipelineScriptSlot slot)
        {
            SlotState state;
            if (!_states.TryGetValue(slot, out state))
            {
                state = new SlotState();
                _states[slot] = state;
            }
            return state;
        }

        private static void GetLimit(EffectPipelineOffsetType type, out double limit, out double lsb)
        {
            switch (type)
            {
                case EffectPipelineOffsetType.Force_N: limit = MaxForce_N; lsb = 0.01; break;
                case EffectPipelineOffsetType.Angle_deg: limit = MaxAngle_deg; lsb = 0.001; break;
                default: limit = MaxForceRelative; lsb = 0.0001; break;
            }
        }

        // ---- script editor test ----

        // Evaluates a script once at t = 0 for the editor; returns the value or an error text.
        public string TestScript(PipelineScriptSlot slot, PluginManager pluginManager)
        {
            lock (_sync)
            {
                var state = new SlotState();
                _pluginManager = pluginManager;
                _propertyCache.Clear();
                try
                {
                    if (!Compile(slot, state)) return "Error: " + state.Error;
                    state.T = 0.0;
                    return EvaluateMain(state).ToString("0.###");
                }
                catch (Exception ex)
                {
                    return "Error: " + ex.Message;
                }
            }
        }

        // ---- evaluation ----

        private bool Compile(PipelineScriptSlot slot, SlotState state)
        {
            string script = slot.Script ?? "";
            string freq = slot.FreqScript ?? "";
            if (state.Compiled && state.Script == script && state.FreqScript == freq && state.Language == slot.Language)
                return state.Error == null || !state.Error.StartsWith("compile: ");

            state.Compiled = true;
            state.Script = script;
            state.FreqScript = freq;
            state.Language = slot.Language;
            state.Expression = null;
            state.FreqExpression = null;
            state.Js = null;
            state.Prev = 0.0;
            state.Error = null;

            try
            {
                if (slot.Language == PipelineScriptLanguage.JavaScript)
                {
                    state.Js = new JintScriptHost(script, freq, GetPropertyRaw);
                }
                else
                {
                    state.Expression = CompileNCalc(state, script, "");
                    if (!string.IsNullOrWhiteSpace(freq))
                        state.FreqExpression = CompileNCalc(state, freq, "frequency: ");
                }
                return true;
            }
            catch (Exception ex)
            {
                state.Error = "compile: " + ex.Message;
                return false;
            }
        }

        private Expression CompileNCalc(SlotState state, string script, string errorPrefix)
        {
            var exp = new Expression(script);
            if (exp.HasErrors())
                throw new InvalidOperationException(errorPrefix + (exp.Error != null ? exp.Error.ToString() : "syntax error"));
            exp.ResolveParameter += delegate (string name, ParameterResolveArgs rarg)
            {
                rarg.Result = () => ResolveVariable(state, name);
            };
            return exp;
        }

        private bool HasFrequency(SlotState state)
        {
            return state.Js != null ? state.Js.HasFrequency : state.FreqExpression != null;
        }

        private double EvaluateMain(SlotState state)
        {
            if (state.Js != null) return state.Js.CallMain(state.T, SamplePeriod_s, state.Prev, state.Phase);
            return Convert.ToDouble(state.Expression.Evaluate());
        }

        private double EvaluateFrequency(SlotState state)
        {
            if (state.Js != null) return state.Js.CallFrequency(state.T, SamplePeriod_s, state.Prev, state.Phase);
            return Convert.ToDouble(state.FreqExpression.Evaluate());
        }

        private object ResolveVariable(SlotState state, string name)
        {
            switch (name)
            {
                case "t": return state.T;
                case "dt": return SamplePeriod_s;
                case "prev": return state.Prev;
                case "phase": return state.Phase;
                case "pi": return Math.PI;
            }
            object value = GetPropertyRaw(name);
            // NCalc can't calculate with null: missing properties count as 0
            if (value == null) return 0.0;
            if (value is bool b) return b ? 1.0 : 0.0;
            return value;
        }

        // raw property value (JavaScript gets null, strings and arrays as they are)
        private object GetPropertyRaw(string name)
        {
            if (name == null) return null;
            object value;
            if (_testActive && _overrides.TryGetValue(name, out value)) return value;
            if (!_propertyCache.TryGetValue(name, out value))
            {
                value = _pluginManager != null ? _pluginManager.GetPropertyValue(name) : null;
                _propertyCache[name] = value;
            }
            return value;
        }

        // values: raw script results (before gain) for the samples [nowIndex, nowIndex + values.Length).
        // Computes the new samples up to nowIndex + LeadSamples in time order, each one once:
        // recomputing overlapping samples would run stateful scripts (root, filters,
        // oscillators) ahead of time and make them jump at every packet.
        private bool ComputeSlot(PipelineScriptSlot slot, SlotState state, uint nowIndex, double[] values, out int computed)
        {
            computed = 0;
            if (!Compile(slot, state)) return false;

            uint endIndex = nowIndex + LeadSamples;
            int behind = (int)(endIndex - state.NextIndex);
            if (!state.HasNext || behind > RingSize / 2 || behind < -RingSize / 2)
            {
                // first run or after a pause: start at now
                state.HasNext = true;
                state.NextIndex = nowIndex;
                for (int r = 0; r < RingSize; r++) state.RingIndex[r] = uint.MaxValue;
            }

            try
            {
                if ((int)(endIndex - state.NextIndex) >= 0)
                {
                    // oscillator: frequency evaluated once per update, phase integrated per sample
                    double phaseStep = 0.0;
                    if (HasFrequency(state))
                    {
                        state.T = state.NextIndex * SamplePeriod_s;
                        double freq = EvaluateFrequency(state);
                        if (double.IsNaN(freq) || double.IsInfinity(freq)) freq = 0.0;
                        freq = Math.Max(0.0, Math.Min(MaxFrequency_Hz, freq));
                        phaseStep = 2.0 * Math.PI * freq * SamplePeriod_s;
                    }

                    for (uint idx = state.NextIndex; (int)(endIndex - idx) >= 0; idx++)
                    {
                        state.T = idx * SamplePeriod_s;
                        state.Phase = (state.Phase + phaseStep) % (2.0 * Math.PI);
                        double value = EvaluateMain(state);
                        if (double.IsNaN(value) || double.IsInfinity(value)) value = 0.0;
                        state.Prev = value;
                        int r = (int)(idx % RingSize);
                        state.Ring[r] = value;
                        state.RingIndex[r] = idx;
                        computed++;
                    }
                    state.NextIndex = endIndex + 1;
                }

                double last = 0.0;
                for (int i = 0; i < values.Length; i++)
                {
                    uint idx = nowIndex + (uint)i;
                    int r = (int)(idx % RingSize);
                    if (i <= LeadSamples && state.RingIndex[r] == idx) last = state.Ring[r];
                    values[i] = last; // beyond the lead: hold, overwritten by the next packets
                }
                state.LastOutput = values[0] * slot.Gain;
                state.Error = null;
                return true;
            }
            catch (Exception ex)
            {
                state.Error = ex.Message;
                state.LastOutput = 0.0;
                state.HasNext = false;
                return false;
            }
        }

        private void RecordHistory(int pedal, int typeIndex, double lsb, uint startIndex, short[] samples)
        {
            History h = _history[pedal * OutputTypes + typeIndex];
            for (int i = 0; i < samples.Length; i++)
            {
                uint sampleIndex = startIndex + (uint)i;
                int slot = (int)(sampleIndex % HistoryLength);
                h.Values[slot] = samples[i] * lsb;
                h.Indices[slot] = sampleIndex;
                h.Valid[slot] = true;
            }
        }

        private unsafe void SendPacket(byte pedal, byte channel, EffectPipelineOffsetType type,
                                       uint startIndex, short[] samples)
        {
            DAP_effect_pipeline_st pkt = default;
            pkt.payloadHeader_.startOfFrame0_u8 = _plugin.STARTOFFRAMCHAR[0];
            pkt.payloadHeader_.startOfFrame1_u8 = _plugin.STARTOFFRAMCHAR[1];
            pkt.payloadHeader_.payloadType = (byte)Constants.effectPipelinePayload_type;
            pkt.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            pkt.payloadHeader_.storeToEeprom = 0;
            pkt.payloadHeader_.PedalTag = pedal;
            pkt.payloadEffectPipeline_.startSampleIndex_u32 = startIndex;
            pkt.payloadEffectPipeline_.channel_u8 = channel;
            pkt.payloadEffectPipeline_.offsetType_u8 = (byte)type;
            pkt.payloadEffectPipeline_.sampleCount_u8 = (byte)samples.Length;
            for (int i = 0; i < samples.Length && i < SamplesPerPacket; i++)
                pkt.payloadEffectPipeline_.samples_ai16[i] = samples[i];
            pkt.payloadFooter_.enfOfFrame0_u8 = _plugin.ENDOFFRAMCHAR[0];
            pkt.payloadFooter_.enfOfFrame1_u8 = _plugin.ENDOFFRAMCHAR[1];

            byte* p = (byte*)&pkt;
            pkt.payloadFooter_.checkSum = _plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadEffectPipeline));

            byte[] buffer = new byte[sizeof(DAP_effect_pipeline_st)];
            fixed (byte* dst = buffer) { *(DAP_effect_pipeline_st*)dst = pkt; }

            try
            {
                if (_plugin.Settings.Pedal_ESPNow_Sync_flag[pedal])
                {
                    if (_plugin.BridgeHidService.IsConnected)
                    {
                        // fits into a single HID report; streamed, a lost one is covered by the next
                        Task.Run(() => _plugin.BridgeHidService.SendLargeDataAsync(buffer));
                    }
                    else if (_plugin.ESPsync_serialPort.IsOpen)
                    {
                        _plugin.ESPsync_serialPort.Write(buffer, 0, buffer.Length);
                    }
                }
                else if (_plugin._serialPort[pedal].IsOpen)
                {
                    _plugin._serialPort[pedal].Write(buffer, 0, buffer.Length);
                }
            }
            catch (Exception ex)
            {
                SimHub.Logging.Current.Error("Effect pipeline send error: " + ex.Message);
            }
        }
    }
}
