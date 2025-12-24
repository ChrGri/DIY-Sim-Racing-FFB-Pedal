using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_action_ota_st
    {
        public payloadHeader payloadHeader_;
        public payloadOtaInfo payloadOtaInfo_;
        public payloadFooter payloadFooter_;
    }
}
