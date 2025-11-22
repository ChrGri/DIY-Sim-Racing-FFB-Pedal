using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
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
                Effects[i] = new bool[8];
            }
        }
    }
}
