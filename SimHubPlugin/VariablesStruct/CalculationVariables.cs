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

        public double[] Force_curve_Y = new double[100];

        public int current_pedal_travel_state { get; set; }
        public byte pedal_state_in_ratio { get; set; }
        public bool isDragging { get; set; }

        public Point offset;

        public SolidColorBrush lightcolor;
        public SolidColorBrush defaultcolor;
        public SolidColorBrush Red_Warning;
        public SolidColorBrush White_Default;
        public string btn_SendConfig_Content;
        public string btn_SendConfig_tooltip;
        public bool[] dumpPedalToResponseFile_clearFile;
        public bool[] dumpPedalToResponseFile;
        public bool Update_Profile_Checkbox_b;
        public string current_profile = "NA";
        public uint profile_index;
        public bool ForceUpdate_b;
        public uint UpdateChannel;
        public uint _rssi_value;
        public byte[,] PedalFirmwareVersion;
        public byte[] BridgeFirmwareVersion;
        public bool[] PedalAvailability;//wireless
        public bool[] PedalSerialAvailability;
        public bool Rudder_status;
        public bool BridgeSerialAvailability;
        public bool OTASettingUpdate_b;
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
            Update_Profile_Checkbox_b = false;
            profile_index = 0;
            ForceUpdate_b = false;
            UpdateChannel = 0;
            _rssi_value = 0;
            _systemStatusString = "";
            _bridgeSerialConnecitonStatus = false;
            _pedalStatusString = "";
            PedalFirmwareVersion = new byte[3, 3] { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
            BridgeFirmwareVersion = new byte[3] { 0, 0, 0 };
            PedalAvailability = new bool[3] { false, false, false };
            PedalSerialAvailability = new bool[3] { false, false, false };
            Rudder_status = false;
            BridgeSerialAvailability = false;
            _pedalConnectingString = "";
            _bridgeConnectingString = "";
            OTASettingUpdate_b = false;
            
    }
        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
    }
}
