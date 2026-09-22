using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadWifiChannel
    {
        public const int ChannelCount = 13; // channels 1-13, index 0 = channel 1

        public byte command_u8;            // 1=ScanReq, 2=ScanRes, 3=SetReq, 4=SetAck
        public byte currentChannel_u8;     // Current active Wi-Fi channel (1-13)
        public byte recommendedChannel_u8; // Recommended clean channel (any of 1-13)
        // Per-channel scan results, index 0 = channel 1 .. index 12 = channel 13.
        public fixed sbyte channelRssi_ai8[13];    // Strongest AP RSSI seen on this channel (0 = none seen)
        public fixed byte channelApCount_au8[13];  // AP count on this channel
        public fixed byte channelApScore_au8[13];  // Congestion score (0-100, lower = cleaner)

        public sbyte GetRssi(int channel1Based)
        {
            int idx = channel1Based - 1;
            if (idx < 0 || idx >= ChannelCount) return 0;
            fixed (sbyte* p = channelRssi_ai8) { return p[idx]; }
        }

        public byte GetApCount(int channel1Based)
        {
            int idx = channel1Based - 1;
            if (idx < 0 || idx >= ChannelCount) return 0;
            fixed (byte* p = channelApCount_au8) { return p[idx]; }
        }

        public byte GetApScore(int channel1Based)
        {
            int idx = channel1Based - 1;
            if (idx < 0 || idx >= ChannelCount) return 0;
            fixed (byte* p = channelApScore_au8) { return p[idx]; }
        }
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_wifi_channel_st
    {
        public payloadHeader payloadHeader_;
        public payloadWifiChannel payloadWifiChannel_;
        public payloadFooter payloadFooter_;
    }
}
