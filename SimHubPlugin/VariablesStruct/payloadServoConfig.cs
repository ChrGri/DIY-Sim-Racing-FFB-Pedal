using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct payloadServoConfig
    {
        public byte readWriteFlag;      // 0 = Read from Servo, 1 = Write to Servo
        public byte numValidFields;     // Anzahl der validen Eintraege (1 bis 10)
        
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
        public ushort[] registerAddresses;
        
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 10)]
        public ushort[] registerValues;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_servo_config_st
    {
        public payloadHeader payloadHeader_st;
        public payloadServoConfig payloadServoConfig_st;
        public payloadFooter payloadFooter_st;

        public byte[] toBytes()
        {
            int size = Marshal.SizeOf(this);
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.StructureToPtr(this, ptr, false);
                Marshal.Copy(ptr, arr, 0, size);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
            return arr;
        }
    }
}
