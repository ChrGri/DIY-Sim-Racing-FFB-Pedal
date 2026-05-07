using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_bridge_state_st
    {
        public payloadHeader payLoadHeader_;
        public payloadBridgeState payloadBridgeState_;
        public payloadFooter payloadFooter_;
    };
}
