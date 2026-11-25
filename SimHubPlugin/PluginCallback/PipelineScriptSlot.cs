using System.Collections.Generic;

namespace DiyFfbPedal
{
    public enum PipelineScriptLanguage
    {
        NCalc = 0,
        JavaScript = 1,
    }

    // One script effect. Any number of slots per pedal; the plugin sums all enabled
    // slots of a pedal per output type and streams one channel per type to the pedal.
    // Stored in DIYFFBPedalSettings.Pipeline_slots (JSON).
    public class PipelineScriptSlot
    {
        public int Pedal = 1;                       // 0 clutch, 1 brake, 2 throttle
        public string Name = "";
        public bool Enabled = false;
        public byte OffsetType = (byte)EffectPipelineOffsetType.Angle_deg;
        public double Gain = 1.0;
        public PipelineScriptLanguage Language = PipelineScriptLanguage.NCalc;
        public string Script = "";
        public string FreqScript = "";               // Hz, drives the phase variable; empty = no oscillator

        public PipelineScriptSlot Clone()
        {
            return (PipelineScriptSlot)MemberwiseClone();
        }

        public string DisplayName(int number)
        {
            string name = string.IsNullOrWhiteSpace(Name) ? Loc.T("TextScriptDefaultName", "Script") + " " + number : Name.Trim();
            string type;
            switch ((EffectPipelineOffsetType)OffsetType)
            {
                case EffectPipelineOffsetType.Force_N: type = "N"; break;
                case EffectPipelineOffsetType.ForceRelative: type = "rel"; break;
                default: type = "deg"; break;
            }
            string lang = Language == PipelineScriptLanguage.JavaScript ? ", JS" : "";
            return number + ": " + name + " (" + type + lang + ")" + (Enabled ? "" : " - " + Loc.T("TextScriptOff", "off"));
        }

        // The first version had two fixed script channels per pedal in arrays.
        // Moves them into slots once; afterwards the arrays are ignored.
        public static void MigrateLegacySettings(DIYFFBPedalSettings settings)
        {
            if (settings == null) return;
            if (settings.Pipeline_slots == null) settings.Pipeline_slots = new List<PipelineScriptSlot>();
            if (settings.Pipeline_slots_migrated) return;
            settings.Pipeline_slots_migrated = true;
            if (settings.Pipeline_scripts == null) return;

            for (int idx = 0; idx < settings.Pipeline_scripts.Length && idx < 6; idx++)
            {
                string script = settings.Pipeline_scripts[idx];
                if (string.IsNullOrWhiteSpace(script)) continue;
                var slot = new PipelineScriptSlot
                {
                    Pedal = idx / 2,
                    Name = "SCRIPT" + (idx % 2 + 1),
                    Script = script,
                };
                if (settings.Pipeline_enable_flag != null && idx < settings.Pipeline_enable_flag.Length)
                    slot.Enabled = settings.Pipeline_enable_flag[idx];
                if (settings.Pipeline_offset_type != null && idx < settings.Pipeline_offset_type.Length)
                    slot.OffsetType = settings.Pipeline_offset_type[idx];
                if (settings.Pipeline_gain != null && idx < settings.Pipeline_gain.Length)
                    slot.Gain = settings.Pipeline_gain[idx];
                if (settings.Pipeline_freq_scripts != null && idx < settings.Pipeline_freq_scripts.Length)
                    slot.FreqScript = settings.Pipeline_freq_scripts[idx] ?? "";
                settings.Pipeline_slots.Add(slot);
            }
        }
    }
}
