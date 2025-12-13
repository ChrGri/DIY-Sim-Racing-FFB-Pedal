using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadOtaInfo
    {
        public byte device_ID;
        public byte ota_action;
        public byte mode_select;
        public byte SSID_Length;
        public byte PASS_Length;
        public fixed byte WIFI_SSID[64];
        public fixed byte WIFI_PASS[64];
    }
}
