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
    public class Profile_Online
    {
        public BasicConfig Basic_Config { get; set; }
    };
}
