using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_mac_addresses_st
    {
        public payloadHeader payloadHeader_;
        public payloadMacAddresses payloadMacAddresses_;
        public payloadFooter payloadFooter_;
    }
}
