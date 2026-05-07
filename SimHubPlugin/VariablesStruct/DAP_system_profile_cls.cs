using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    public class DAP_system_profile_cls
    {
        public string BindGameOrCar { get; set; } = string.Empty;

        public string[] ConfigPath { get; set; }

        public bool[][] Effects { get; set; }

        public DAP_system_profile_cls()
        {
            ConfigPath = new string[3] { string.Empty, string.Empty, string.Empty };
            Effects = new bool[3][];
            for (int i = 0; i < 3; i++)
            {
                Effects[i] = new bool[10];
            }
        }
        [OnDeserialized]
        internal void OnDeserializedMethod(StreamingContext context)
        {
            if (Effects == null)
            {
                Effects = new bool[3][];
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
        }
    }
}
