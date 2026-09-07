using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct payloadWifiChannel
    {
        public byte command_u8;            // 1=ScanReq, 2=ScanRes, 3=SetReq, 4=SetAck
        public byte currentChannel_u8;     // Current active Wi-Fi channel (1-13)
        public byte recommendedChannel_u8; // Recommended clean channel (1, 6, 11)
        public sbyte channel1Rssi_i8;      // Strongest AP RSSI on Ch 1
        public sbyte channel6Rssi_i8;      // Strongest AP RSSI on Ch 6
        public sbyte channel11Rssi_i8;     // Strongest AP RSSI on Ch 11
        public byte channel1ApCount_u8;    // AP count on Ch 1
        public byte channel6ApCount_u8;    // AP count on Ch 6
        public byte channel11ApCount_u8;   // AP count on Ch 11
        public byte channel1ApScore_u8;    // Congestion score (0-100) on Ch 1
        public byte channel6ApScore_u8;    // Congestion score (0-100) on Ch 6
        public byte channel11ApScore_u8;   // Congestion score (0-100) on Ch 11
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_wifi_channel_st
    {
        public payloadHeader payloadHeader_;
        public payloadWifiChannel payloadWifiChannel_;
        public payloadFooter payloadFooter_;
    }
}
