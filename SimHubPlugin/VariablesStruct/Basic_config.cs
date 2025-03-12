using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    [Serializable]
    public class BasicConfig
    {
        public int MaxForce { get; set; }
        public int PreloadForce { get; set; }
        public int Damping { get; set; }
        public int Travel { get; set; }
        public int relativeForce_p000 { get; set; }
        public int relativeForce_p020 { get; set; }
        public int relativeForce_p040 { get; set; }
        public int relativeForce_p060 { get; set; }
        public int relativeForce_p080 { get; set; }
        public int relativeForce_p100 { get; set; }
    };
}
