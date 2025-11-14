using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    static class Constants
    {
        // payload revisiom
        public const uint pedalConfigPayload_version = 162;


        // pyload types
        public const uint pedalConfigPayload_type = 100;
        public const uint pedalActionPayload_type = 110;
        public const uint pedalStateBasicPayload_type = 120;
        public const uint pedalStateExtendedPayload_type = 130;
        public const uint bridgeStatePayloadType = 210;
        public const uint OtaPayloadType = 220;
        public const string pluginVersion = "0.90.24";
        public const string version_control_url = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/version_control.json";
        public const int DEFAULTBAUD = 921600;
        public const int BAUD3M = 3000000;

    }

    public enum enumServoStatus
    {
        Off,
        On,
        Idle,
        ForceStop
    }
    public enum bridgeAction
    {
        BRIDGE_ACTION_NONE,
        BRIDGE_ACTION_ENABLE_PAIRING,
        BRIDGE_ACTION_RESTART,
        BRIDGE_ACTION_DOWNLOAD_MODE,
        BRIDGE_ACTION_DEBUG,
        BRIDGE_ACTION_JOYSTICK_FLASHING_MODE,
        BRIDGE_ACTION_JOYSTICK_DEBUG
    };
    public enum otaAction
    {
        OTA_ACTION_NORMAL,
        OTA_ACTION_FORCE_UPDATE,
        OTA_ACTION_UPLOAD_FROM_PLATFORMIO
    };

    public enum PedalSystemAction
    {
        NONE,
        RESET_PEDAL_POSITION,//not in use
        PEDAL_RESTART,
        ENABLE_OTA,//not in use
        ENABLE_PAIRING,//not in use
        ESP_BOOT_INTO_DOWNLOAD_MODE,
        PRINT_PEDAL_INFO,
        SET_ASSIGNMENT_0,
        SET_ASSIGNMENT_1,
        SET_ASSIGNMENT_2,
        CLEAR_ASSIGNMENT,
        ASSIGNMENT_CHECK_BEEP
    };
    public enum RudderAction
    {
        None,
        EnableRudderTwoPedals,
        ClearRudderStatus,
        EnableRudderThreePedals,
        EnableHeliRudderTwoPedals,
        EnableHeliRudderThreePedals

    };

    public enum TrackConditionEnum
    {
        Dry,
        MostlyDry,
        VeryLightWet,
        LightWet,
        ModeratelyWet,
        VeryWet,
        ExtremelyWet,
        DIRT,
        ICED
    };

    public enum PedalIdEnum
    {
        PEDAL_ID_CLUTCH,
        PEDAL_ID_BRAKE,
        PEDAL_ID_THROTTLE,
        PEDAL_ID_ASSIGNMENT_ERROR,
        PEDAL_ID_UNKNOWN,
        PEDAL_ID_TEMP_1,
        PEDAL_ID_TEMP_2,
        PEDAL_ID_TEMP_3
    };

    public enum ConnectStateEnum:uint
    {
        PEDAL_DISCONNECT,
        PEDAL_ENTRY_CONNECT,
        PEDAL_GET_BASIC_PACKETS,
        PEDAL_IS_READY
    };
    public enum WirelessConnectStateEnum:uint
    {
        PEDAL_DISCONNECT,
        PEDAL_BRIDGE_ENTRY_CONNECT,
        PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW,
        PEDAL_WIRELESS_IS_READY
    };
    public enum BridgeConnectStateEnum:uint
    {
        BRIDGE_DISCONNECT,
        BRIDGE_ENTRY_CONNECT,
        BRIDGE_IS_READY
    };

    static class PedalConstStrings 
    {
        public static readonly string[] ConnectState = new string[] { "Disconnected", "Connecting", "Read Pedal Config", "Connected" };
        public static readonly string[] WirelessConnectState = new string[] { "Disconnected", "Connecting", "Read Pedal Config", "Wireless" };
        public static readonly string[] BridgeConnectState = new string[] { "Disconnected", "Connecting", "Connneted"};
        public static readonly string[] PedalID = new string[] { "Clutch", "Brake", "Throttle", "None", "Bridge" };
        public static readonly string[] ServoStatus = new string[] { "Off", "On", "Idle", "Force Stop" };
    }
}
