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
        // highest oscillator frequency; the pedal plays 500 Hz samples, more makes no sense
        public const double MaxFrequency_Hz = 150.0;

        public const int OutputTypes = 3; // Force_N, Angle_deg, ForceRelative
        private const int SamplesPerPacket = Constants.effectPipelineSamplesPerPacket;
        private const double SamplePeriod_s = Constants.effectPipelineSamplePeriod_s;
        private const double MinSendInterval_s = 0.010;

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
            public double[] LastPhases = new double[0];
            public double Prev;
            public double T;
            public double LastOutput;
            public string Error;
            public double EvalTime_ms; // per second of samples, smoothed
            // raw values of the last packet, so prev continues from the sample before the next one
            public uint LastStartIndex;
            public double[] LastValues = new double[0];
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

        // Called from DataUpdate.
        public void Update(PluginManager pluginManager, bool gameRunning)
        {
            lock (_sync)
            {
                UpdateLocked(pluginManager, gameRunning);
            }
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
                        continue;
                    }

                    long ticks = Stopwatch.GetTimestamp();
                    bool ok = ComputeSlot(slot, state, startIndex, values);
                    double ms = (Stopwatch.GetTimestamp() - ticks) * 1000.0 / Stopwatch.Frequency;
                    // one packet covers SamplesPerPacket * dt seconds of samples
                    double msPerSecond = ms / (SamplesPerPacket * SamplePeriod_s);
                    state.EvalTime_ms += 0.1 * (msPerSecond - state.EvalTime_ms);
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
            if (!_propertyCache.TryGetValue(name, out value))
            {
                value = _pluginManager != null ? _pluginManager.GetPropertyValue(name) : null;
                _propertyCache[name] = value;
            }
            return value;
        }

        // values: raw script results (before gain) for the samples of the next packet
        private bool ComputeSlot(PipelineScriptSlot slot, SlotState state, uint startIndex, double[] values)
        {
            if (!Compile(slot, state)) return false;

            // the packets overlap: continue from the value computed for the sample right before
            uint before = startIndex - 1;
            if (state.LastValues.Length > 0 && before >= state.LastStartIndex &&
                before - state.LastStartIndex < (uint)state.LastValues.Length)
                state.Prev = state.LastValues[before - state.LastStartIndex];

            double[] phases = new double[values.Length];
            try
            {
                // oscillator: frequency evaluated once per packet, phase integrated per sample
                // and continued from the previous packet, so frequency changes don't jump
                double phaseStep = 0.0;
                if (HasFrequency(state))
                {
                    state.T = startIndex * SamplePeriod_s;
                    double freq = EvaluateFrequency(state);
                    if (double.IsNaN(freq) || double.IsInfinity(freq)) freq = 0.0;
                    freq = Math.Max(0.0, Math.Min(MaxFrequency_Hz, freq));
                    phaseStep = 2.0 * Math.PI * freq * SamplePeriod_s;
                    if (state.LastPhases.Length > 0 && before >= state.LastStartIndex &&
                        before - state.LastStartIndex < (uint)state.LastPhases.Length)
                        state.Phase = state.LastPhases[before - state.LastStartIndex] + phaseStep;
                }

                for (int i = 0; i < values.Length; i++)
                {
                    state.T = (startIndex + (uint)i) * SamplePeriod_s;
                    if (i > 0) state.Phase += phaseStep;
                    state.Phase %= 2.0 * Math.PI;
                    phases[i] = state.Phase;
                    double value = EvaluateMain(state);
                    if (double.IsNaN(value) || double.IsInfinity(value)) value = 0.0;
                    state.Prev = value;
                    values[i] = value;
                }
                state.LastOutput = values[0] * slot.Gain;
                state.LastStartIndex = startIndex;
                state.LastValues = (double[])values.Clone();
                state.LastPhases = phases;
                state.Error = null;
                return true;
            }
            catch (Exception ex)
            {
                state.Error = ex.Message;
                state.LastOutput = 0.0;
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
