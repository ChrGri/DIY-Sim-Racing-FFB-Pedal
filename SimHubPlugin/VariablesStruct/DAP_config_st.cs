using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_config_st
    {
        public payloadHeader payloadHeader_;
        public payloadPedalConfig payloadPedalConfig_;
        public payloadFooter payloadFooter_;
    }
}

