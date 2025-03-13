using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    unsafe public struct Basic_WIfi_info
    {
        public byte payload_Type;
        public byte device_ID;
        public byte wifi_action;
        public byte mode_select;
        public byte SSID_Length;
        public byte PASS_Length;
        public fixed byte WIFI_SSID[30];
        public fixed byte WIFI_PASS[30];
    };
}
