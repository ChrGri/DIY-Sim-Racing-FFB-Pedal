using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadBridgeState
    {
        public byte unassignedPedalCount;
        public byte Pedal_availability_0;
        public byte Pedal_availability_1;
        public byte Pedal_availability_2;
        public byte Bridge_action;//0=none, 1=enable pairing
        public fixed byte Bridge_firmware_version_u8[3];
        public fixed Int32 Pedal_RSSI_realtime[3];
        public fixed byte macAddressDetection[18];
    };
}
