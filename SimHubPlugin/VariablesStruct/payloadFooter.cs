using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    
    public struct payloadFooter
    {
        // To check if structure is valid
        public UInt16 checkSum;
    }
}
