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
    unsafe public struct payloadPedalState_Basic
    {
        public UInt16 pedalPosition_u16;
        public UInt16 pedalForce_u16;
        public UInt16 joystickOutput_u16;
        public byte error_code_u8;
        public fixed byte pedalFirmwareVersion_u8[3];

    };
}
