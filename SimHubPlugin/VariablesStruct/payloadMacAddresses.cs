using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadMacAddresses
    {
        public fixed byte assignmentState_au8[4];
        public fixed byte macAddress_aau8[24]; // 4 * 6 = 24 bytes
        public fixed byte ownMacAddress_au8[6];
        public byte ownNodeType_u8; // 0=Clutch, 1=Brake, 2=Throttle, 3=Bridge
        public byte wifiChannel_u8;
        public fixed byte reserved_au8[2];

        public void Initialize()
        {
            fixed (byte* p = assignmentState_au8)
            {
                for (int i = 0; i < 4; i++) p[i] = 0;
            }
            fixed (byte* p = macAddress_aau8)
            {
                for (int i = 0; i < 24; i++) p[i] = 0;
            }
            fixed (byte* p = ownMacAddress_au8)
            {
                for (int i = 0; i < 6; i++) p[i] = 0;
            }
            fixed (byte* p = reserved_au8)
            {
                p[0] = 0; p[1] = 0;
            }
        }

        public byte[] GetMacAddress(int nodeIdx)
        {
            byte[] mac = new byte[6];
            if (nodeIdx >= 0 && nodeIdx < 4)
            {
                fixed (byte* p = macAddress_aau8)
                {
                    for (int i = 0; i < 6; i++) mac[i] = p[nodeIdx * 6 + i];
                }
            }
            return mac;
        }

        public string GetMacAddressString(int nodeIdx)
        {
            byte[] mac = GetMacAddress(nodeIdx);
            bool allZero = true;
            for (int i = 0; i < 6; i++)
            {
                if (mac[i] != 0) { allZero = false; break; }
            }
            if (allZero) return "--";
            return string.Format("{0:X2}:{1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        }

        public string GetOwnMacAddressString()
        {
            bool allZero = true;
            fixed (byte* p = ownMacAddress_au8)
            {
                for (int i = 0; i < 6; i++)
                {
                    if (p[i] != 0) { allZero = false; break; }
                }
                if (allZero) return "--";
                return string.Format("{0:X2}:{1:X2}:{2:X2}:{3:X2}:{4:X2}:{5:X2}", p[0], p[1], p[2], p[3], p[4], p[5]);
            }
        }

        public void SetMacAddress(int nodeIdx, byte[] mac)
        {
            if (nodeIdx >= 0 && nodeIdx < 4 && mac != null && mac.Length == 6)
            {
                fixed (byte* p = macAddress_aau8)
                {
                    for (int i = 0; i < 6; i++) p[nodeIdx * 6 + i] = mac[i];
                }
            }
        }

        public bool SetMacAddressFromString(int nodeIdx, string macStr)
        {
            if (string.IsNullOrWhiteSpace(macStr)) return false;
            string clean = macStr.Replace(":", "").Replace("-", "").Replace(" ", "").Trim();
            if (clean.Length != 12) return false;
            try
            {
                byte[] mac = new byte[6];
                for (int i = 0; i < 6; i++)
                {
                    mac[i] = Convert.ToByte(clean.Substring(i * 2, 2), 16);
                }
                SetMacAddress(nodeIdx, mac);
                return true;
            }
            catch
            {
                return false;
            }
        }
    }
}
