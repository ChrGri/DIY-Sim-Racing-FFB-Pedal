using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    public class ProfileScriptBinding
    {
        public int Pedal { get; set; } = 1; // 0: Clutch, 1: Brake, 2: Throttle
        public string ScriptFile { get; set; } = string.Empty;
        public bool Enabled { get; set; } = true;
        public double Gain { get; set; } = 1.0;

        public ProfileScriptBinding() { }

        public ProfileScriptBinding Clone()
        {
            return new ProfileScriptBinding
            {
                Pedal = Pedal,
                ScriptFile = ScriptFile ?? string.Empty,
                Enabled = Enabled,
                Gain = Gain,
            };
        }
    }

    public class DAP_system_profile_cls
    {
        public string BindGameOrCar { get; set; } = string.Empty;

        public string[] ConfigPath { get; set; }

        public bool[][] Effects { get; set; }

        public bool HasScriptBindings { get; set; } = false;

        public List<ProfileScriptBinding> ScriptBindings { get; set; } = new List<ProfileScriptBinding>();

        public DAP_system_profile_cls()
        {
            ConfigPath = new string[3] { string.Empty, string.Empty, string.Empty };
            Effects = new bool[3][];
            for (int i = 0; i < 3; i++)
            {
                Effects[i] = new bool[10];
            }
            ScriptBindings = new List<ProfileScriptBinding>();
        }
        [OnDeserialized]
        internal void OnDeserializedMethod(StreamingContext context)
        {
            if (ConfigPath == null)
            {
                ConfigPath = new string[3];
            }
            else if (ConfigPath.Length < 3)
            {
                string[] configPath = ConfigPath;
                Array.Resize(ref configPath, 3);
                ConfigPath = configPath;
            }
            for (int i = 0; i < ConfigPath.Length; i++)
            {
                if (ConfigPath[i] == null) ConfigPath[i] = string.Empty;
            }

            if (Effects == null)
            {
                Effects = new bool[3][];
            }
            else if (Effects.Length < 3)
            {
                bool[][] effects = Effects;
                Array.Resize(ref effects, 3);
                Effects = effects;
            }
            for (int i = 0; i < Effects.Length; i++)
            {
                if (Effects[i] == null)
                {
                    Effects[i] = new bool[10];
                }
                else if (Effects[i].Length < 10)
                {
                    Array.Resize(ref Effects[i], 10);
                }
            }

            if (ScriptBindings == null)
            {
                ScriptBindings = new List<ProfileScriptBinding>();
            }
        }
    }
}
