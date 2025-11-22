using log4net.Core;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Media;

namespace User.PluginSdkDemo
{
    public class CalculationVariables : INotifyPropertyChanged
    {
        public int PedalCurrentTravel { get; set; }
        public int PedalCurrentForce { get; set; }
        public bool SendAbsSignal { get; set; }
        public bool IsUIRefreshNeeded { get; set; }
        public bool Update_CV1_textbox { get; set; }
        public bool Update_CV2_textbox { get; set; }

        public double[] Force_curve_Y = new double[101];

        public int current_pedal_travel_state { get; set; }
        public byte pedal_state_in_ratio { get; set; }
        public bool isDragging { get; set; }
        public int unassignedPedalCount { get; set; }
        public Point offset;

        public SolidColorBrush lightcolor;
        public SolidColorBrush defaultcolor;
        public SolidColorBrush Red_Warning;
        public SolidColorBrush White_Default;
        public string btn_SendConfig_Content;
        public string btn_SendConfig_tooltip;
        public bool[] dumpPedalToResponseFile_clearFile;
        public bool[] dumpPedalToResponseFile;
        public string current_profile = "NA";
        public bool ForceUpdate_b;
        public uint UpdateChannel;
        public uint _rssi_value;
        public byte[,] PedalFirmwareVersion;
        public byte[] BridgeFirmwareVersion;
        //public bool[] PedalAvailability;//wireless
        //public bool[] PedalSerialAvailability;
        public bool Rudder_status;
        public bool BridgeSerialAvailability;
        public bool OTASettingUpdate_b;
        public int[] rssi;
        public byte[] ServoStatus;
        public string[] pluginVersionReading;
        public string[] updateChannelString = new[] { "main", "dev-build", "daily-build" };
        public bool versionCheck_b;
        public bool verisonCreate_b;
        public Version pluginVersion;
        public Version updateVerison;
        public vJoyInterfaceWrap.vJoy _joystick;
        public bool IsJoystickInitialized = false;
        public uint rudderType;
        public bool IsTestBuild = false;
        public bool IsOtaUploadFromPlatformIO = false;
        public byte[][] unassignedPedalMacaddress;
        public BridgeConnectStateEnum bridgeConnectionStatus= BridgeConnectStateEnum.BRIDGE_DISCONNECT;
        public WirelessConnectStateEnum[] pedalWirelessStatus = new WirelessConnectStateEnum[3] { WirelessConnectStateEnum.PEDAL_DISCONNECT, WirelessConnectStateEnum.PEDAL_DISCONNECT, WirelessConnectStateEnum.PEDAL_DISCONNECT};
        public ConnectStateEnum[] pedalSerialStatus = new ConnectStateEnum[3] { ConnectStateEnum.PEDAL_DISCONNECT, ConnectStateEnum.PEDAL_DISCONNECT, ConnectStateEnum.PEDAL_DISCONNECT };
        public DateTime[] pedalWirelessConnetionlastTime = new DateTime[3];
        public DateTime[] pedalSerialConnetionlastTime = new DateTime[3];
        public DateTime bridgeConnetionlastTime = DateTime.Now;
        public bool[] configPreviewLock = new bool[3] { false, false, false};
        public DateTime[] configPreviewLockLast = new DateTime[3];
        public string[] ConfigEditing = new string[3] { string.Empty, string.Empty, string.Empty};
        public string ProfileEditing = string.Empty;
        public string ProfileSelected = string.Empty;
        public int ProfileIndex = -1;
        public string logDateTime =  string.Empty;
        public uint RSSI_Value
        {
            get => _rssi_value;
            set { _rssi_value = value; OnPropertyChanged(nameof(RSSI_Value)); }
        }

        private string _systemStatusString;
        public string SystemStatusString
        {
            get => _systemStatusString;
            set { _systemStatusString = value; OnPropertyChanged(nameof(SystemStatusString)); }
        }
        private string _pedalStatusString;
        public string PedalStatusString
        {
            get => _pedalStatusString;
            set { _pedalStatusString = value; OnPropertyChanged(nameof(PedalStatusString)); }
        }
        private string _rudderStatusString;
        public string RudderStatusString
        {
            get => _rudderStatusString;
            set { _rudderStatusString = value; OnPropertyChanged(nameof(RudderStatusString)); }
        }
        private bool _bridgeSerialConnecitonStatus;
        public bool BridgeSerialConnectionStatus
        {
            get => _bridgeSerialConnecitonStatus;
            set { _bridgeSerialConnecitonStatus = value; OnPropertyChanged(nameof(BridgeSerialConnectionStatus)); }
        }
        private string _bridgeConnectingString;
        public string BridgeConnetingString
        {
            get => _bridgeConnectingString;
            set { _bridgeConnectingString = value; OnPropertyChanged(nameof(BridgeConnetingString)); }
        }
        private string _pedalConnectingString;
        public string PedalConnetingString
        {
            get => _pedalConnectingString;
            set { _pedalConnectingString = value; OnPropertyChanged(nameof(PedalConnetingString)); }
        }

        public CalculationVariables()
        {
            PedalCurrentForce = 0;
            PedalCurrentTravel = 0;
            SendAbsSignal = false;
            IsUIRefreshNeeded = false;
            Update_CV1_textbox = false;
            Update_CV2_textbox = false;
            pedal_state_in_ratio = 0;
            current_pedal_travel_state = 0;
            isDragging = false;
            lightcolor = new SolidColorBrush();
            defaultcolor = new SolidColorBrush();
            Red_Warning = new SolidColorBrush(Color.FromArgb(255, 244, 67, 67));
            White_Default = new SolidColorBrush(Color.FromArgb(255, 255, 255, 255));
            offset = new Point();
            btn_SendConfig_Content = "Send Config to Pedal";
            btn_SendConfig_tooltip = "Send Config to Pedal and save in storage";
            dumpPedalToResponseFile_clearFile = new bool[3] { false, false, false };
            dumpPedalToResponseFile = new bool[3] { false, false, false };
            ForceUpdate_b = false;
            UpdateChannel = 0;
            _rssi_value = 0;
            _systemStatusString = "";
            _bridgeSerialConnecitonStatus = false;
            _pedalStatusString = "";
            PedalFirmwareVersion = new byte[3, 3] { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
            BridgeFirmwareVersion = new byte[3] { 0, 0, 0 };
            //PedalAvailability = new bool[3] { false, false, false };
            //PedalSerialAvailability = new bool[3] { false, false, false };
            Rudder_status = false;
            BridgeSerialAvailability = false;
            _pedalConnectingString = "";
            _bridgeConnectingString = "";
            OTASettingUpdate_b = false;
            rssi = new int[3] { 0, 0, 0 };
            ServoStatus = new byte[3] { 0, 0, 0 };
            pluginVersionReading = new string[3] { "", "", ""};
            versionCheck_b = false;
            verisonCreate_b = false;
            _joystick = new vJoyInterfaceWrap.vJoy();
            rudderType = 0;
            unassignedPedalCount = 0;
            unassignedPedalMacaddress = new byte[3][];
            for (int i = 0; i < 3; i++)
            {
                unassignedPedalMacaddress[i]=new byte[6] { 0, 0, 0, 0, 0, 0};
            }
            

        }
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }


    }
}
