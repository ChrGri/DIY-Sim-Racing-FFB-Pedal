using NCalc;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    // Effect pipelines: user scripts (NCalc) compute offset samples from SimHub
    // properties; the samples are streamed to the pedal, which plays them back on
    // its own clock (EffectPipeline.h in the firmware).
    //
    // Every telemetry update produces one packet per enabled channel with the
    // samples for the next ~42 ms on a 500 Hz sample clock. The packets overlap,
    // so jitter and single lost packets don't reach the pedal.
    //
    // Script variables (in addition to every SimHub property, e.g.
    // [DataCorePlugin.GameData.Rpms]):
    //   t     time of the sample in seconds (continuous)
    //   dt    sample period in seconds (0.002)
    //   prev  previous output of this channel (before gain), for filters
    //   phase oscillator phase in radians, integrated from the optional frequency
    //         script, so Sin(phase) stays continuous when the frequency changes
    //   pi    3.14159...
    // Output unit depends on the offset type: N, mm or relative (0.1 = 10 %).
    public class EffectPipelineEngine
    {
        // limits of a single channel (the pedal limits the sum as well)
        public const double MaxForce_N = 60.0;
        public const double MaxPosition_mm = 5.0;
        public const double MaxForceRelative = 0.8;
        // highest oscillator frequency; the pedal plays 500 Hz samples, more makes no sense
        public const double MaxFrequency_Hz = 150.0;

        private const int Channels = Constants.effectPipelineChannels;
        private const int SamplesPerPacket = Constants.effectPipelineSamplesPerPacket;
        private const double SamplePeriod_s = Constants.effectPipelineSamplePeriod_s;
        private const double MinSendInterval_s = 0.010;

        private readonly DIY_FFB_Pedal _plugin;
        private readonly Stopwatch _clock = Stopwatch.StartNew();
        private double _lastSend_s = -1.0;

        private class ChannelState
        {
            public string Script = null;
            public Expression Expression;
            public string FreqScript = null;
            public Expression FreqExpression;
            public double Phase;
            public double[] LastPhases = new double[0];
            public double Prev;
            public double T;
            public double LastOutput;
            public string Error;
            // raw values of the last packet, so prev continues from the sample before the next one
            public uint LastStartIndex;
            public double[] LastValues = new double[0];
        }

        private readonly ChannelState[] _channels = new ChannelState[3 * Channels];

        // History of the computed samples (after gain and limit) for the live plot, 5 s at 500 Hz.
        // The packets overlap; a later packet overwrites its predicted samples with newer values.
        public const int HistoryLength = 2500;
        private class History
        {
            public readonly double[] Values = new double[HistoryLength];
            public readonly uint[] Indices = new uint[HistoryLength];
            public readonly bool[] Valid = new bool[HistoryLength];
            public EffectPipelineOffsetType Type = EffectPipelineOffsetType.Off;
        }
        private readonly History[] _history = new History[3 * Channels];
        // SimHub property values, read once per telemetry update
        private readonly Dictionary<string, object> _propertyCache = new Dictionary<string, object>();
        private PluginManager _pluginManager;
        // Update() runs on the telemetry thread, TestScript() on the UI thread
        private readonly object _sync = new object();

        public EffectPipelineEngine(DIY_FFB_Pedal plugin)
        {
            _plugin = plugin;
            for (int i = 0; i < _channels.Length; i++) _channels[i] = new ChannelState();
            for (int i = 0; i < _history.Length; i++) _history[i] = new History();
        }

        // last output (after gain) per pedal/channel, for the UI
        public double GetLastOutput(int pedal, int channel)
        {
            int idx = pedal * Channels + channel;
            return (idx >= 0 && idx < _channels.Length) ? _channels[idx].LastOutput : 0.0;
        }

        // null when the script is fine
        public string GetError(int pedal, int channel)
        {
            int idx = pedal * Channels + channel;
            return (idx >= 0 && idx < _channels.Length) ? _channels[idx].Error : null;
        }

        // Current index of the plugin sample clock (500 Hz).
        public uint CurrentSampleIndex
        {
            get { return (uint)(_clock.Elapsed.TotalSeconds / SamplePeriod_s); }
        }

        // Copies the plotted history of one pedal channel for the sample indices
        // [endIndex - dest.Length + 1, endIndex] into dest (0 where nothing was sent).
        public EffectPipelineOffsetType CopyHistory(int pedal, int channel, uint endIndex, double[] dest)
        {
            int idx = pedal * Channels + channel;
            if (idx < 0 || idx >= _history.Length) return EffectPipelineOffsetType.Off;
            lock (_sync)
            {
                History h = _history[idx];
                uint first = endIndex - (uint)dest.Length + 1;
                for (int i = 0; i < dest.Length; i++)
                {
                    uint sampleIndex = first + (uint)i;
                    int slot = (int)(sampleIndex % HistoryLength);
                    dest[i] = (h.Valid[slot] && h.Indices[slot] == sampleIndex) ? h.Values[slot] : 0.0;
                }
                return h.Type;
            }
        }

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
            if (settings == null || settings.Pipeline_enable_flag == null) return;

            double now_s = _clock.Elapsed.TotalSeconds;
            if (_lastSend_s >= 0.0 && (now_s - _lastSend_s) < MinSendInterval_s) return;
            _lastSend_s = now_s;

            _pluginManager = pluginManager;
            _propertyCache.Clear();
            uint startIndex = (uint)(now_s / SamplePeriod_s);

            for (int pedal = 0; pedal < 3; pedal++)
            {
                for (int channel = 0; channel < Channels; channel++)
                {
                    int idx = pedal * Channels + channel;
                    ChannelState state = _channels[idx];
                    bool enabled = gameRunning && idx < settings.Pipeline_enable_flag.Length &&
                                   settings.Pipeline_enable_flag[idx] &&
                                   !string.IsNullOrWhiteSpace(settings.Pipeline_scripts[idx]) &&
                                   UIFunction.ServoLedIndicator.IsPedalConnected(_plugin._calculations, pedal);
                    if (!enabled)
                    {
                        // nothing sent: the pedal fades the channel out by itself
                        state.LastOutput = 0.0;
                        state.Prev = 0.0;
                        continue;
                    }

                    var type = (EffectPipelineOffsetType)settings.Pipeline_offset_type[idx];
                    double gain = settings.Pipeline_gain[idx];
                    short[] samples = new short[SamplesPerPacket];
                    string freqScript = (settings.Pipeline_freq_scripts != null && idx < settings.Pipeline_freq_scripts.Length)
                        ? settings.Pipeline_freq_scripts[idx] : null;
                    if (!ComputeSamples(state, settings.Pipeline_scripts[idx], freqScript, type, gain, startIndex, samples))
                        continue;
                    RecordHistory(idx, type, startIndex, samples);
                    SendPacket((byte)pedal, (byte)channel, type, startIndex, samples);
                }
            }
        }

        // Evaluates a script once at t = 0 for the script editor; returns the value or an error text.
        public string TestScript(string script, PluginManager pluginManager)
        {
            lock (_sync)
            {
                return TestScriptLocked(script, pluginManager);
            }
        }

        private string TestScriptLocked(string script, PluginManager pluginManager)
        {
            var state = new ChannelState();
            _pluginManager = pluginManager;
            _propertyCache.Clear();
            try
            {
                if (!Prepare(state, script)) return "Error: " + state.Error;
                state.T = 0.0;
                return Convert.ToDouble(state.Expression.Evaluate()).ToString("0.###");
            }
            catch (Exception ex)
            {
                return "Error: " + ex.Message;
            }
        }

        private bool Prepare(ChannelState state, string script)
        {
            if (state.Script == script && state.Expression != null) return true;
            state.Script = script;
            state.Expression = null;
            state.Prev = 0.0;
            var exp = new Expression(script);
            if (exp.HasErrors())
            {
                state.Error = exp.Error != null ? exp.Error.ToString() : "syntax error";
                return false;
            }
            exp.ResolveParameter += delegate (string name, ParameterResolveArgs rarg)
            {
                rarg.Result = () => ResolveVariable(state, name);
            };
            state.Expression = exp;
            state.Error = null;
            return true;
        }

        // Frequency script (Hz) for the phase variable; empty = no oscillator.
        private bool PrepareFrequency(ChannelState state, string script)
        {
            if (state.FreqScript == script && (state.FreqExpression != null || string.IsNullOrWhiteSpace(script))) return true;
            state.FreqScript = script;
            state.FreqExpression = null;
            if (string.IsNullOrWhiteSpace(script)) return true;
            var exp = new Expression(script);
            if (exp.HasErrors())
            {
                state.Error = "frequency: " + (exp.Error != null ? exp.Error.ToString() : "syntax error");
                return false;
            }
            exp.ResolveParameter += delegate (string name, ParameterResolveArgs rarg)
            {
                rarg.Result = () => ResolveVariable(state, name);
            };
            state.FreqExpression = exp;
            return true;
        }

        private object ResolveVariable(ChannelState state, string name)
        {
            switch (name)
            {
                case "t": return state.T;
                case "dt": return SamplePeriod_s;
                case "prev": return state.Prev;
                case "phase": return state.Phase;
                case "pi": return Math.PI;
            }
            object value;
            if (!_propertyCache.TryGetValue(name, out value))
            {
                value = _pluginManager != null ? _pluginManager.GetPropertyValue(name) : null;
                // NCalc can't calculate with null: missing properties count as 0
                if (value == null) value = 0.0;
                else if (value is bool b) value = b ? 1.0 : 0.0;
                _propertyCache[name] = value;
            }
            return value;
        }

        private void RecordHistory(int idx, EffectPipelineOffsetType type, uint startIndex, short[] samples)
        {
            double lsb = type == EffectPipelineOffsetType.Force_N ? 0.01
                : type == EffectPipelineOffsetType.Position_mm ? 0.001 : 0.0001;
            History h = _history[idx];
            h.Type = type;
            for (int i = 0; i < samples.Length; i++)
            {
                uint sampleIndex = startIndex + (uint)i;
                int slot = (int)(sampleIndex % HistoryLength);
                h.Values[slot] = samples[i] * lsb;
                h.Indices[slot] = sampleIndex;
                h.Valid[slot] = true;
            }
        }

        private bool ComputeSamples(ChannelState state, string script, string freqScript, EffectPipelineOffsetType type,
                                    double gain, uint startIndex, short[] samples)
        {
            if (!Prepare(state, script)) return false;
            if (!PrepareFrequency(state, freqScript)) return false;

            double limit, lsb;
            switch (type)
            {
                case EffectPipelineOffsetType.Force_N: limit = MaxForce_N; lsb = 0.01; break;
                case EffectPipelineOffsetType.Position_mm: limit = MaxPosition_mm; lsb = 0.001; break;
                case EffectPipelineOffsetType.ForceRelative: limit = MaxForceRelative; lsb = 0.0001; break;
                default: return false;
            }

            // the packets overlap: continue from the value computed for the sample right before
            uint before = startIndex - 1;
            if (state.LastValues.Length > 0 && before >= state.LastStartIndex &&
                before - state.LastStartIndex < (uint)state.LastValues.Length)
                state.Prev = state.LastValues[before - state.LastStartIndex];

            double[] values = new double[samples.Length];
            double[] phases = new double[samples.Length];
            try
            {
                // oscillator: frequency evaluated once per packet, phase integrated per sample
                // and continued from the previous packet, so frequency changes don't jump
                double phaseStep = 0.0;
                if (state.FreqExpression != null)
                {
                    state.T = startIndex * SamplePeriod_s;
                    double freq = Convert.ToDouble(state.FreqExpression.Evaluate());
                    if (double.IsNaN(freq) || double.IsInfinity(freq)) freq = 0.0;
                    freq = Math.Max(0.0, Math.Min(MaxFrequency_Hz, freq));
                    phaseStep = 2.0 * Math.PI * freq * SamplePeriod_s;
                    if (state.LastPhases.Length > 0 && before >= state.LastStartIndex &&
                        before - state.LastStartIndex < (uint)state.LastPhases.Length)
                        state.Phase = state.LastPhases[before - state.LastStartIndex] + phaseStep;
                }

                for (int i = 0; i < samples.Length; i++)
                {
                    state.T = (startIndex + (uint)i) * SamplePeriod_s;
                    if (i > 0) state.Phase += phaseStep;
                    state.Phase %= 2.0 * Math.PI;
                    phases[i] = state.Phase;
                    double value = Convert.ToDouble(state.Expression.Evaluate());
                    if (double.IsNaN(value) || double.IsInfinity(value)) value = 0.0;
                    state.Prev = value;
                    values[i] = value;
                    double output = Math.Max(-limit, Math.Min(limit, value * gain));
                    samples[i] = (short)Math.Round(output / lsb);
                    if (i == 0) state.LastOutput = output;
                }
                state.LastStartIndex = startIndex;
                state.LastValues = values;
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
