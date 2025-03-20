//using SimHub.Plugins.OutputPlugins.Dash.GLCDTemplating;
using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Runtime.InteropServices;
using System.Windows;
using System.Windows.Controls;

using System.Windows.Media.TextFormatting;
using System.Text.Json;
using FMOD;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Json;
using System.IO;
using System.Text;
using System.Web;
using MahApps.Metro.Controls;
using System.Runtime.CompilerServices;
using System.CodeDom.Compiler;
using System.Windows.Forms;
using static System.Net.Mime.MediaTypeNames;
using System.Runtime.InteropServices.ComTypes;
using Microsoft.Win32;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;
using System.Windows.Input;
using System.Windows.Shapes;
using MouseEventArgs = System.Windows.Input.MouseEventArgs;
using SimHub.Plugins.OutputPlugins.GraphicalDash.PSE;
using SimHub.Plugins.Styles;
using System.Windows.Media;
using System.Runtime.Remoting.Messaging;
using SimHub.Plugins.OutputPlugins.GraphicalDash.Behaviors.DoubleText.Imp;
using System.Reflection;
using System.Text.Json.Nodes;
using System.Text.Json.Serialization;
using Newtonsoft.Json;
using System.Threading;
using System.Text.RegularExpressions;
using SimHub.Plugins;
using log4net.Plugin;
//using System.Drawing;

using vJoyInterfaceWrap;
//using vJoy.Wrapper;
using System.Runtime;
using SimHub.Plugins.DataPlugins.ShakeItV3.Settings;
using System.Windows.Media.Effects;
using System.Diagnostics;
using System.Collections;
using System.Linq;
using Windows.UI.Notifications;
//using System.Diagnostics;
using System.Windows.Navigation;
using System.CodeDom;
using System.Media;
using System.Windows.Threading;
using System.Net.Http;
using System.Threading.Tasks;
using static User.PluginSdkDemo.DIY_FFB_Pedal;
using User.PluginSdkDemo.UIFunction;
using Windows.UI.ViewManagement;



// Win 11 install, see https://github.com/jshafer817/vJoy/releases
//using vJoy.Wrapper;



namespace User.PluginSdkDemo
{
    /// <summary>
    /// Logique d'interaction pour SettingsControlDemo.xaml
    /// </summary>
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {


        // payload revisiom
        //public uint pedalConfigPayload_version = 110;
        //public uint pedalConfigPayload_type = 100;
        //public uint pedalActionPayload_type = 110;

        public uint indexOfSelectedPedal_u = 1;
        public uint profile_select = 0;
        public DIY_FFB_Pedal Plugin { get; }
        public static DAP_config_st[] dap_config_st = new DAP_config_st[3];
        public static DAP_config_st dap_config_st_rudder;


        public DAP_bridge_state_st dap_bridge_state_st;
        public Basic_WIfi_info _basic_wifi_info;
        private string stringValue;


        public bool[] waiting_for_pedal_config = new bool[3];
        public System.Windows.Forms.Timer[] pedal_serial_read_timer = new System.Windows.Forms.Timer[3];
        public System.Windows.Forms.Timer connect_timer;
        public System.Windows.Forms.Timer ESP_host_serial_timer;
        //public System.Timers.Timer[] pedal_serial_read_timer = new System.Timers.Timer[3];
        //int printCtr = 0;

        //public double[] Force_curve_Y = new double[100];
        public bool debug_flag = false;

        //public VirtualJoystick joystick;
        


        public bool[] dumpPedalToResponseFile = new bool[3];
        public bool[] dumpPedalToResponseFile_clearFile = new bool[3];

        private SolidColorBrush defaultcolor;
        private SolidColorBrush lightcolor;
        private SolidColorBrush redcolor;
        private SolidColorBrush color_RSSI_1;
        private SolidColorBrush color_RSSI_2;
        private SolidColorBrush color_RSSI_3;
        private SolidColorBrush color_RSSI_4;
        private SolidColorBrush Red_Warning;
        private SolidColorBrush White_Default;
        private string info_text_connection;
        private string system_info_text_connection;
        private int current_pedal_travel_state= 0;
        private int gridline_kinematic_count_original = 0;
        private double[] Pedal_position_reading=new double[3];
        private bool[] Serial_connect_status = new bool[3] { false,false,false};
        public byte Bridge_RSSI = 0;
        public bool[] Pedal_wireless_connection_update_b = new bool[3] { false,false,false};
        public int Bridge_baudrate = 3000000;
        //public bool Fanatec_mode = false;
        public bool Update_Profile_Checkbox_b = false;
        //public bool Update_CV_textbox = false;
        public bool[] Version_error_warning_b = new bool[3] { false, false, false };
        public bool[] Version_warning_first_show_b= new bool[3] { false, false, false };
        public bool Version_warning_first_show_b_bridge = false;
        public byte[] Pedal_version = new byte[3];
        private SerialMonitor_Window _serial_monitor_window;
        public bool Pedal_Log_warning_1st_show_b = true;
        private string[] Rudder_Pedal_idx_Name= new string[3] {"Clutch", "Brake","Throttle"};
        public byte Pedal_connect_status = 0;
        DateTime ConfigLiveSending_last = DateTime.Now;
        DateTime PedalTabChange_last = DateTime.Now;
        public byte[,] PedalFirmwareVersion = new byte[3, 3] { { 0, 0, 0}, { 0, 0, 0 }, { 0, 0, 0 } };
        public bool PedalTabChange = false;
        private bool isDragging = false;
        private Point offset;
        public enum PedalAvailability        
        {
            NopedalConnect,
            SinglePedalClutch,
            SinglePedalBrake,
            SinglePedalThrottle,
            TwoPedalConnectClutchBrake,
            TwoPedalConnectClutchThrottle,
            TwoPedalConnectBrakeThrottle,
            ThreePedalConnect
        }
        //public int Bridge_baudrate = 921600;
        /*
        private double kinematicDiagram_zeroPos_OX = 100;
        private double kinematicDiagram_zeroPos_OY = 20;
        private double kinematicDiagram_zeroPos_scale = 1;
        */



        // read config from JSON on startup
        //ReadStructFromJson();


        // read JSON config from JSON file
        //private void ReadStructFromJson()
        //{



        //    try
        //    {
        //        // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/how-to?pivots=dotnet-8-0
        //        // https://www.educative.io/answers/how-to-read-a-json-file-in-c-sharp

        //        string currentDirectory = Directory.GetCurrentDirectory();
        //        string dirName = currentDirectory + "\\PluginsData\\Common";
        //        //string jsonFileName = ComboBox_JsonFileSelected.Text;
        //        string jsonFileName = ((ComboBoxItem)ComboBox_JsonFileSelected.SelectedItem).Content.ToString();
        //        string fileName = dirName + "\\" + jsonFileName + ".json";

        //        string text = System.IO.File.ReadAllText(fileName);

        //        DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
        //        var ms = new MemoryStream(Encoding.UTF8.GetBytes(text));
        //        dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
        //        //TextBox_debugOutput.Text = "Config loaded!";
        //        //TextBox_debugOutput.Text += ComboBox_JsonFileSelected.Text;
        //        //TextBox_debugOutput.Text += "    ";
        //        //TextBox_debugOutput.Text += ComboBox_JsonFileSelected.SelectedIndex;

        //        updateTheGuiFromConfig();

        //    }
        //    catch (Exception caughtEx)
        //    {

        //        string errorMessage = caughtEx.Message;
        //        TextBox_debugOutput.Text = errorMessage;
        //    }


        //}
        private void ToastNotification(string message1, string message2)
        {
            
            var xml = ToastNotificationManager.GetTemplateContent(ToastTemplateType.ToastText02);
            var text = xml.GetElementsByTagName("text");
            text[0].AppendChild(xml.CreateTextNode(message1));
            text[1].AppendChild(xml.CreateTextNode(message2));
            var toast = new ToastNotification(xml);
            toast.ExpirationTime = DateTime.Now.AddSeconds(1);
            toast.Tag = "Pedal_notification";
            ToastNotificationManager.CreateToastNotifier("FFB Pedal Dashboard").Show(toast);



        }
        private void InitReadStructFromJson()
        {



            try
            {
                // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/how-to?pivots=dotnet-8-0
                // https://www.educative.io/answers/how-to-read-a-json-file-in-c-sharp
                string jsonFileName = "NA";

                string currentDirectory = Directory.GetCurrentDirectory();
                string dirName = currentDirectory + "\\PluginsData\\Common";
                //string jsonFileName = ComboBox_JsonFileSelected.Text;
                if (indexOfSelectedPedal_u == 0)
                {
                    jsonFileName = ("DiyPedalConfig_Clutch_Default");
                }
                else if (indexOfSelectedPedal_u == 1)
                {
                    jsonFileName = ("DiyPedalConfig_Brake_Default");
                }
                else if (indexOfSelectedPedal_u == 2)
                {
                    jsonFileName = ("DiyPedalConfig_Accelerator_Default");
                }

                string fileName = dirName + "\\" + jsonFileName + ".json";
                string text = System.IO.File.ReadAllText(fileName);



                DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                var ms = new MemoryStream(Encoding.UTF8.GetBytes(text));
                dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
                TextBox_debugOutput.Text = "Config loaded!" + jsonFileName;
                //TextBox_debugOutput.Text += ComboBox_JsonFileSelected.Text;
                //TextBox_debugOutput.Text += "    ";
                //TextBox_debugOutput.Text += ComboBox_JsonFileSelected.SelectedIndex;

                updateTheGuiFromConfig();

            }
            catch (Exception caughtEx)
            {

                string errorMessage = caughtEx.Message;
                TextBox_debugOutput.Text = errorMessage;
            }


        }

        private void UpdateSerialPortList_click()
        {

            var SerialPortSelectionArray = new List<SerialPortChoice>();
            string[] comPorts = SerialPort.GetPortNames();

            comPorts = comPorts.Distinct().ToArray(); // unique

            if (comPorts.Length > 0)
            {

                foreach (string portName in comPorts)
                {
                    SerialPortSelectionArray.Add(new SerialPortChoice(portName, portName));
                }
            }
            else
            {
                SerialPortSelectionArray.Add(new SerialPortChoice("NA", "NA"));
            }

            SerialPortSelection.DataContext = SerialPortSelectionArray;
            SerialPortSelection_ESPNow.DataContext = SerialPortSelectionArray;
        }



        public void DAP_config_set_default(uint pedalIdx)
        {
            dumpPedalToResponseFile[pedalIdx] = false;
            dumpPedalToResponseFile_clearFile[pedalIdx] = false;
            dap_config_st[pedalIdx].payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_st[pedalIdx].payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedalStartPosition = 35;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedalEndPosition = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.maxForce = 50;
            dap_config_st[pedalIdx].payloadPedalConfig_.preloadForce = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.dampingPress = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.dampingPull = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.absFrequency = 5;
            dap_config_st[pedalIdx].payloadPedalConfig_.absAmplitude = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.absPattern = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.absForceOrTarvelBit = 0;

            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_a = 205;
            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_b = 220;
            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_d = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_c_horizontal = 215;
            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_c_vertical = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.lengthPedal_travel = 100;

            dap_config_st[pedalIdx].payloadPedalConfig_.Simulate_ABS_trigger = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.Simulate_ABS_value = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.RPM_max_freq = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.RPM_min_freq = 10;
            dap_config_st[pedalIdx].payloadPedalConfig_.RPM_AMP = 30;
            dap_config_st[pedalIdx].payloadPedalConfig_.BP_trigger_value = 50;
            dap_config_st[pedalIdx].payloadPedalConfig_.BP_amp = 1;
            dap_config_st[pedalIdx].payloadPedalConfig_.BP_freq = 15;
            dap_config_st[pedalIdx].payloadPedalConfig_.BP_trigger = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.G_multi = 50;
            dap_config_st[pedalIdx].payloadPedalConfig_.G_window = 10;
            dap_config_st[pedalIdx].payloadPedalConfig_.WS_amp = 1;
            dap_config_st[pedalIdx].payloadPedalConfig_.WS_freq = 15;
            dap_config_st[pedalIdx].payloadPedalConfig_.Impact_multi = 50;
            dap_config_st[pedalIdx].payloadPedalConfig_.Impact_window = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_amp_1 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_freq_1 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_amp_2 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_freq_2 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.maxGameOutput = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelNoise = 128;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelOrder = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_a_0 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_a_1 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_a_2 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_a_3 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_a_4 = 0;

            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_b_0 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_b_1 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_b_2 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_b_3 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.cubic_spline_param_b_4 = 0;

            dap_config_st[pedalIdx].payloadPedalConfig_.PID_p_gain = 0.1f;
            dap_config_st[pedalIdx].payloadPedalConfig_.PID_i_gain = 1.0f;
            dap_config_st[pedalIdx].payloadPedalConfig_.PID_d_gain = 0.0f;
            dap_config_st[pedalIdx].payloadPedalConfig_.PID_velocity_feedforward_gain = 0.0f;

            dap_config_st[pedalIdx].payloadPedalConfig_.MPC_0th_order_gain = 10.0f;
            dap_config_st[pedalIdx].payloadPedalConfig_.MPC_1st_order_gain = 0.0f;

            dap_config_st[pedalIdx].payloadPedalConfig_.control_strategy_b = 2;

            dap_config_st[pedalIdx].payloadPedalConfig_.loadcell_rating = 150;

            dap_config_st[pedalIdx].payloadPedalConfig_.travelAsJoystickOutput_u8 = 0;

            dap_config_st[pedalIdx].payloadPedalConfig_.invertLoadcellReading_u8 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.invertMotorDirection_u8 = 0;

            dap_config_st[pedalIdx].payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedal_type = (byte)pedalIdx;
            //dap_config_st[pedalIdx].payloadPedalConfig_.OTA_flag = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.stepLossFunctionFlags_u8 = 0b11;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelNoise_joystick = 1;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_Joystick_u8 = 0;
        }

        public void DAP_config_set_default_rudder()
        {
            
            dap_config_st_rudder.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_st_rudder.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_st_rudder.payloadPedalConfig_.pedalStartPosition = 5;
            dap_config_st_rudder.payloadPedalConfig_.pedalEndPosition = 95;
            dap_config_st_rudder.payloadPedalConfig_.maxForce = 10;
            dap_config_st_rudder.payloadPedalConfig_.preloadForce = 1.0f;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st_rudder.payloadPedalConfig_.dampingPress = 0;
            dap_config_st_rudder.payloadPedalConfig_.dampingPull = 0;
            dap_config_st_rudder.payloadPedalConfig_.absFrequency = 5;
            dap_config_st_rudder.payloadPedalConfig_.absAmplitude = 20;
            dap_config_st_rudder.payloadPedalConfig_.absPattern = 0;
            dap_config_st_rudder.payloadPedalConfig_.absForceOrTarvelBit = 0;

            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_a = 205;
            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_b = 220;
            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_d = 60;
            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_horizontal = 215;
            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_vertical = 60;
            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_travel = 60;

            dap_config_st_rudder.payloadPedalConfig_.Simulate_ABS_trigger = 0;
            dap_config_st_rudder.payloadPedalConfig_.Simulate_ABS_value = 80;
            dap_config_st_rudder.payloadPedalConfig_.RPM_max_freq = 45;
            dap_config_st_rudder.payloadPedalConfig_.RPM_min_freq = 15;
            dap_config_st_rudder.payloadPedalConfig_.RPM_AMP = 1;
            dap_config_st_rudder.payloadPedalConfig_.BP_trigger_value = 50;
            dap_config_st_rudder.payloadPedalConfig_.BP_amp = 1;
            dap_config_st_rudder.payloadPedalConfig_.BP_freq = 15;
            dap_config_st_rudder.payloadPedalConfig_.BP_trigger = 0;
            dap_config_st_rudder.payloadPedalConfig_.G_multi = 50;
            dap_config_st_rudder.payloadPedalConfig_.G_window = 10;
            dap_config_st_rudder.payloadPedalConfig_.WS_amp = 1;
            dap_config_st_rudder.payloadPedalConfig_.WS_freq = 15;
            dap_config_st_rudder.payloadPedalConfig_.Impact_multi = 50;
            dap_config_st_rudder.payloadPedalConfig_.Impact_window = 60;
            dap_config_st_rudder.payloadPedalConfig_.CV_amp_1 = 0;
            dap_config_st_rudder.payloadPedalConfig_.CV_freq_1 = 0;
            dap_config_st_rudder.payloadPedalConfig_.CV_amp_2 = 0;
            dap_config_st_rudder.payloadPedalConfig_.CV_freq_2 = 0;

            dap_config_st_rudder.payloadPedalConfig_.maxGameOutput = 100;
            dap_config_st_rudder.payloadPedalConfig_.kf_modelNoise = 10;
            dap_config_st_rudder.payloadPedalConfig_.kf_modelOrder = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_a_0 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_a_1 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_a_2 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_a_3 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_a_4 = 0;

            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_b_0 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_b_1 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_b_2 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_b_3 = 0;
            dap_config_st_rudder.payloadPedalConfig_.cubic_spline_param_b_4 = 0;

            dap_config_st_rudder.payloadPedalConfig_.PID_p_gain = 0.3f;
            dap_config_st_rudder.payloadPedalConfig_.PID_i_gain = 50.0f;
            dap_config_st_rudder.payloadPedalConfig_.PID_d_gain = 0.0f;
            dap_config_st_rudder.payloadPedalConfig_.PID_velocity_feedforward_gain = 0.0f;

            dap_config_st_rudder.payloadPedalConfig_.MPC_0th_order_gain = 3.0f;
            dap_config_st_rudder.payloadPedalConfig_.MPC_1st_order_gain = 0.0f;

            dap_config_st_rudder.payloadPedalConfig_.control_strategy_b = 2;

            dap_config_st_rudder.payloadPedalConfig_.loadcell_rating = 100;

            dap_config_st_rudder.payloadPedalConfig_.travelAsJoystickOutput_u8 = 1;

            dap_config_st_rudder.payloadPedalConfig_.invertLoadcellReading_u8 = 0;
            dap_config_st_rudder.payloadPedalConfig_.invertMotorDirection_u8 = 0;

            dap_config_st_rudder.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
            dap_config_st_rudder.payloadPedalConfig_.pedal_type = (byte)4;
            //dap_config_st[pedalIdx].payloadPedalConfig_.OTA_flag = 0;
            dap_config_st_rudder.payloadPedalConfig_.stepLossFunctionFlags_u8 = 0b11;
            dap_config_st_rudder.payloadPedalConfig_.kf_modelNoise_joystick = 1;
            dap_config_st_rudder.payloadPedalConfig_.kf_Joystick_u8 = 1;
        }
        System.Windows.Controls.CheckBox[,] Effect_status_profile=new System.Windows.Controls.CheckBox[3,8];
        unsafe public DIYFFBPedalControlUI()
        {
            
            DAP_config_set_default_rudder();
            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                DAP_config_set_default(pedalIdx);
                
            }
            for (uint i = 0; i < 30; i++)
            {
                _basic_wifi_info.WIFI_PASS[i] = 0;
                _basic_wifi_info.WIFI_SSID[i] = 0;
            }
            InitializeComponent();
            //Misc_Tab.dap_config_st = new DAP_config_st();
            //initialize profile effect status
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    Effect_status_profile[i, j] = new System.Windows.Controls.CheckBox();
                    Effect_status_profile[i, j].Width = 45;
                    Effect_status_profile[i, j].Height = 20;
                    Effect_status_profile[i, j].FontSize = 8;
                    switch (j)
                    {
                        case 0:
                            
                            Effect_status_profile[i, j].Margin = new Thickness(20, 0, 0, 0);
                            if (i == 0 || i == 2)
                            {
                                Effect_status_profile[i, j].Content = "TC";
                                
                            }
                            else
                            {
                                Effect_status_profile[i, j].Content = "ABS";
                            }
                            
                            break;
                        case 1:
                            Effect_status_profile[i, j].Content = "RPM";
                            break;
                        case 2:
                            Effect_status_profile[i, j].Content = "B.P";
                            Effect_status_profile[i, j].Width = 40;
                            break;
                        case 3:
                            Effect_status_profile[i, j].Content = "G-F";
                            Effect_status_profile[i, j].Width = 40;
                            if (i == 0 || i == 2)
                            {
                                Effect_status_profile[i, j].IsEnabled = false;
                            }
                            break;
                        case 4:
                            Effect_status_profile[i, j].Content = "W.S";
                            Effect_status_profile[i, j].Width = 40;
                            break;
                        case 5:
                            Effect_status_profile[i, j].Content = "IMAPCT";
                            Effect_status_profile[i, j].Width = 60;
                            break;
                        case 6:
                            Effect_status_profile[i, j].Content = "CUS-1";
                            Effect_status_profile[i, j].Width = 50;
                            break;
                        case 7:
                            Effect_status_profile[i, j].Content = "CUS-2";
                            Effect_status_profile[i, j].Width = 50;
                            break;
                    }
                    switch (i)
                    {
                        case 0:
                            StackPanel_Effects_Status_0.Children.Add(Effect_status_profile[i, j]);
                            break;
                        case 1:
                            StackPanel_Effects_Status_1.Children.Add(Effect_status_profile[i, j]);
                            break;
                        case 2:
                            StackPanel_Effects_Status_2.Children.Add(Effect_status_profile[i, j]);
                            break;
                    }
                }
            }

            // debug mode invisiable
            //text_debug_flag.Visibility = Visibility.Hidden;
            //text_serial.Visibility = Visibility.Hidden;
            //TextBox_serialMonitor.Visibility = System.Windows.Visibility.Hidden;
            //InvertLoadcellReading_check.Visibility = Visibility.Hidden;
            //InvertMotorDir_check.Visibility = Visibility.Hidden;
            //textBox_debug_Flag_0.Visibility = Visibility.Hidden;
            //Border_serial_monitor.Visibility=Visibility.Hidden;
            
           

            //Label_reverse_LC.Visibility=Visibility.Hidden;
            //Label_reverse_servo.Visibility=Visibility.Hidden;
            btn_test.Visibility=Visibility.Hidden;
            //setting drawing color with Simhub theme workaround
            SolidColorBrush buttonBackground_ = btn_update.Background as SolidColorBrush;

            Color color = Color.FromArgb(150, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color color_2 = Color.FromArgb(200, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color color_3 = Color.FromArgb(255, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B);
            Color RED_color = Color.FromArgb(60, 139, 0, 0);
            redcolor = new SolidColorBrush(RED_color);
            SolidColorBrush Line_fill = new SolidColorBrush(color_2);
            
            //SolidColorBrush rect_fill = new SolidColorBrush(color);
            defaultcolor = new SolidColorBrush(color);
            
            lightcolor = new SolidColorBrush(color_3);
            
            color_RSSI_1 = new SolidColorBrush(Color.FromArgb(150, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_2 = new SolidColorBrush(Color.FromArgb(180, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_3 = new SolidColorBrush(Color.FromArgb(210, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            color_RSSI_4 = new SolidColorBrush(Color.FromArgb(255, buttonBackground_.Color.R, buttonBackground_.Color.G, buttonBackground_.Color.B));
            Red_Warning = new SolidColorBrush(Color.FromArgb(255, 244, 67, 67));
            White_Default = new SolidColorBrush(Color.FromArgb(255, 255, 255, 255));
            RSSI_2.Fill = color_RSSI_2;
            RSSI_3.Fill = color_RSSI_3;
            RSSI_4.Fill = color_RSSI_4;
            //Plugin.simhub_theme_color=defaultcolor.ToString();            
            // Call this method to generate gridlines on the Canvas            
            Label_RSSI.Visibility= Visibility.Hidden;
            TextBox_debug_count.Visibility= Visibility.Hidden;
        }



        public byte[] getBytesPayload(payloadPedalConfig aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }


        unsafe public byte[] getBytes(DAP_config_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);

            //int length = sizeof(DAP_config_st);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);


            //DAP_config_st* v = &aux;
            //for (UInt16 ptrIdx = 0; ptrIdx < length; ptrIdx++)
            //{
            //    myBuffer[ptrIdx] = *((byte*)v + ptrIdx);
            //}

            return myBuffer;
        }


        //public byte[] getBytes_Action(DAP_action_st aux)
        //{
        //    int length = Marshal.SizeOf(aux);
        //    IntPtr ptr = Marshal.AllocHGlobal(length);
        //    byte[] myBuffer = new byte[length];

        //    Marshal.StructureToPtr(aux, ptr, true);
        //    Marshal.Copy(ptr, myBuffer, 0, length);
        //    Marshal.FreeHGlobal(ptr);

        //    return myBuffer;
        //}


        public DAP_config_st getConfigFromBytes(byte[] myBuffer)
        {
            DAP_config_st aux;

            // see https://stackoverflow.com/questions/31045358/how-do-i-copy-bytes-into-a-struct-variable-in-c
            int size = Marshal.SizeOf(typeof(DAP_config_st));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(myBuffer, 0, ptr, size);

            aux = (DAP_config_st)Marshal.PtrToStructure(ptr, typeof(DAP_config_st));
            Marshal.FreeHGlobal(ptr);

            return aux;
        }


        public DAP_state_basic_st getStateFromBytes(byte[] myBuffer)
        {
            DAP_state_basic_st aux;

            // see https://stackoverflow.com/questions/31045358/how-do-i-copy-bytes-into-a-struct-variable-in-c
            int size = Marshal.SizeOf(typeof(DAP_state_basic_st));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(myBuffer, 0, ptr, size);

            aux = (DAP_state_basic_st)Marshal.PtrToStructure(ptr, typeof(DAP_state_basic_st));
            Marshal.FreeHGlobal(ptr);

            return aux;
        }

        public DAP_state_extended_st getStateExtFromBytes(byte[] myBuffer)
        {
            DAP_state_extended_st aux;

            // see https://stackoverflow.com/questions/31045358/how-do-i-copy-bytes-into-a-struct-variable-in-c
            int size = Marshal.SizeOf(typeof(DAP_state_extended_st));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(myBuffer, 0, ptr, size);

            aux = (DAP_state_extended_st)Marshal.PtrToStructure(ptr, typeof(DAP_state_extended_st));
            Marshal.FreeHGlobal(ptr);

            return aux;
        }
        public DAP_bridge_state_st getStateBridgeFromBytes(byte[] myBuffer)
        {
            DAP_bridge_state_st aux;

            // see https://stackoverflow.com/questions/31045358/how-do-i-copy-bytes-into-a-struct-variable-in-c
            int size = Marshal.SizeOf(typeof(DAP_bridge_state_st));
            IntPtr ptr = Marshal.AllocHGlobal(size);

            Marshal.Copy(myBuffer, 0, ptr, size);

            aux = (DAP_bridge_state_st)Marshal.PtrToStructure(ptr, typeof(DAP_bridge_state_st));
            Marshal.FreeHGlobal(ptr);

            return aux;
        }


        //unsafe private UInt16 checksumCalc(byte* data, int length)
        //{

        //    UInt16 curr_crc = 0x0000;
        //    byte sum1 = (byte)curr_crc;
        //    byte sum2 = (byte)(curr_crc >> 8);
        //    int index;
        //    for (index = 0; index < length; index = index + 1)
        //    {
        //        int v = (sum1 + (*data));
        //        sum1 = (byte)v;
        //        sum1 = (byte)(v % 255);

        //        int w = (sum1 + sum2) % 255;
        //        sum2 = (byte)w;

        //        data++;// = data++;
        //    }

        //    int x = (sum2 << 8) | sum1;
        //    return (UInt16)x;
        //}


        public DIYFFBPedalControlUI(DIY_FFB_Pedal plugin) : this()
        {
            this.Plugin = plugin;
            plugin.testValue = 1;
            plugin.wpfHandle = this;
            UpdateSerialPortList_click();
            
            indexOfSelectedPedal_u = plugin.Settings.table_selected;
            MyTab.SelectedIndex = (int)indexOfSelectedPedal_u;


            //auto connection with timmer
            if (connect_timer != null)
            {
                connect_timer.Dispose();
                connect_timer.Stop();
            }

            connect_timer = new System.Windows.Forms.Timer();
            connect_timer.Tick += new EventHandler(connection_timmer_tick);
            connect_timer.Interval = 5000; // in miliseconds try connect every 5s
            connect_timer.Start();
            System.Threading.Thread.Sleep(50);

        }



        //manually fresh the UI element
        unsafe public void updateTheGuiFromConfig()
        {

            info_label.Content = "State:\nDAP Version:\nPlugin Version:\nPedal Version:";
            info_label_system.Content = "Bridge:\nDAP Version:\nPlugin Version:\nBridge Verison:";
            //RSSI canvas
            if (Plugin != null)
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    RSSI_canvas.Visibility = Visibility.Visible;
                    //Label_RSSI.Visibility = Visibility.Visible;
                }
                else
                {
                    RSSI_canvas.Visibility = Visibility.Hidden;
                    Label_RSSI.Visibility = Visibility.Hidden;
                }
            }

            //string plugin_version = Assembly.GetExecutingAssembly().GetName().Version.ToString();

            string plugin_version = Constants.pluginVersion;
            string info_text;
            string system_info_text;
            info_text = "Waiting...";
            system_info_text = "Waiting...";
            if (Plugin != null)
            {
                
                if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
                {
                    info_text = "Connected";
                }
                else
                {
                    if (Plugin.Settings.auto_connect_flag[indexOfSelectedPedal_u] == 1)
                    {
                        info_text = info_text_connection;
                    }
                }
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    system_info_text = "Connected";
                    if (Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u])
                    {
                        
                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1 && indexOfSelectedPedal_u == 0)
                        {
                            info_text = "Wireless";
                        }
                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1 && indexOfSelectedPedal_u == 1)
                        {
                            info_text = "Wireless";
                        }
                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1 && indexOfSelectedPedal_u == 2)
                        {
                            info_text = "Wireless";
                        }

                    }
                }
                else
                {
                    if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                    {
                        system_info_text = system_info_text_connection;
                    }
                    if (Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u])
                    {
                        if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                        {
                            info_text = info_text_connection;
                        }
                        
                    }

                }
                info_text += "\n" + Constants.pedalConfigPayload_version + "\n" + plugin_version;
                system_info_text += "\n" + Constants.pedalConfigPayload_version + "\n" + plugin_version;
                if (PedalFirmwareVersion[indexOfSelectedPedal_u, 2] != 0)
                {
                    info_text += "\n" + PedalFirmwareVersion[indexOfSelectedPedal_u, 0] + "." + PedalFirmwareVersion[indexOfSelectedPedal_u, 1] + "." ;
                    if (PedalFirmwareVersion[indexOfSelectedPedal_u, 2] < 10)
                    {
                        info_text += "0" + PedalFirmwareVersion[indexOfSelectedPedal_u, 2];
                    }
                    else
                    {
                        info_text += PedalFirmwareVersion[indexOfSelectedPedal_u, 2];
                    }
                }
                else
                {
                    info_text += "\n" + "No data";
                }

                if (dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2] != 0)
                {
                    system_info_text += "\n" + dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0] + "." + dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1] + "." ;
                    if (dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2] < 10)
                    {
                        system_info_text += "0" + dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2];
                    }
                    else
                    {
                        system_info_text += dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[2];
                    }
                }
                else
                {
                    system_info_text += "\n" + "No data";
                }
                
                info_label_2.Content = info_text;
                info_label_2_system.Content = system_info_text;
            }
            if (Plugin != null)
            {                
                var tmp_struct = dap_config_st[indexOfSelectedPedal_u];
                Misc_Tab.dap_config_st = tmp_struct;
                KF_Tab.dap_config_st=tmp_struct;
                ControlStrategy_Tab.dap_config_st=tmp_struct;
                PID_Tab.dap_config_st = tmp_struct;
                MPC_tab.dap_config_st = tmp_struct;
                EffectsABS_Tab.dap_config_st = tmp_struct;
                EffectsRPM_Tab.dap_config_st = tmp_struct;
                EffectsBitePoint_Tab.dap_config_st = tmp_struct;
                EffectsGFroce_Tab.dap_config_st = tmp_struct;
                EffectsWheelSlip_Tab.dap_config_st = tmp_struct;
                EffectsRoadImpact_Tab.dap_config_st = tmp_struct;
                EffectsCustom1_tab.dap_config_st=tmp_struct; 
                EffectsCustom2_Tab.dap_config_st=tmp_struct;
                PedalForceTravel_Tab.dap_config_st = tmp_struct;
                PedalKinematics_Tab.dap_config_st= tmp_struct;
                PedalSettingsSection.dap_config_st = tmp_struct;
                var tmp_rudder = dap_config_st_rudder;
                CurveRudderForce_Tab.dap_config_st = tmp_rudder;
                RudderSetting_Tab.dap_config_st = tmp_rudder;
                EffectsRPMRudder_Tab.dap_config_st = tmp_rudder;


                Misc_Tab.Settings = Plugin.Settings;
                EffectsABS_Tab.Settings = Plugin.Settings;
                EffectsRPM_Tab.Settings = Plugin.Settings;
                EffectsGFroce_Tab.Settings=Plugin.Settings;
                EffectsWheelSlip_Tab.Settings = Plugin.Settings;
                EffectsRoadImpact_Tab.Settings=Plugin.Settings;
                EffectsCustom1_tab.Settings=Plugin.Settings;
                EffectsCustom2_Tab.Settings=Plugin.Settings;
                PedalForceTravel_Tab.Settings=Plugin.Settings;
                PedalKinematics_Tab.Settings = Plugin.Settings;
                PedalSettingsSection.Settings=Plugin.Settings;
                EffectsRPMRudder_Tab.Settings= Plugin.Settings;
                EffectRudderACC_Tab.Settings= Plugin.Settings;

                EffectsABS_Tab.calculation = Plugin._calculaitons;
                EffectsBitePoint_Tab.calculation=Plugin._calculaitons;
                EffectsCustom1_tab.calculation = Plugin._calculaitons;
                EffectsCustom2_Tab.calculation = Plugin._calculaitons;
                Misc_Tab.calculation = Plugin._calculaitons;
                PedalForceTravel_Tab.calculation=Plugin._calculaitons;
                PedalSettingsSection.calculation=Plugin._calculaitons;
                CurveRudderForce_Tab.calculation=Plugin._calculaitons;


                EffectsCustom1_tab.Plugin = Plugin;
                EffectsCustom2_Tab.Plugin = Plugin;


                btn_SendConfig.Content= Plugin._calculaitons.btn_SendConfig_Content;
                btn_SendConfig.ToolTip = Plugin._calculaitons.btn_SendConfig_tooltip;
            }
            

            
            if (Plugin != null)
            {
                
                

                if (Plugin.Sync_esp_connection_flag)
                {
                    btn_connect_espnow_port.Content = "Disconnect";
                }
                else
                {
                    btn_connect_espnow_port.Content = "Connect";
                }


                if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                {
                    CheckBox_Pedal_ESPNow_autoconnect.IsChecked = true;
                }
                else
                { 
                    CheckBox_Pedal_ESPNow_autoconnect.IsChecked= false;
                }

                if (Plugin.Settings.Serial_auto_clean)
                {
                    //Checkbox_auto_remove_serial_line.IsChecked = true;
                }
                else
                { 
                    //Checkbox_auto_remove_serial_line.IsChecked= false;
                }

                if (Plugin.Settings.Using_CDC_bridge)
                {
                    CheckBox_using_CDC_for_bridge.IsChecked = true;
                }
                else
                {
                    CheckBox_using_CDC_for_bridge.IsChecked = false;
                }


                
                
            }


            


            
            //// Select serial port accordingly
            string tmp = (string)Plugin._serialPort[indexOfSelectedPedal_u].PortName;
            try
            {
                SerialPortSelection.SelectedValue = tmp;
                TextBox_debugOutput.Text = "Serial port selected: " + SerialPortSelection.SelectedValue;

            }
            catch (Exception caughtEx)
            {
            }


            if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == true)
            {
                ConnectToPedal.IsChecked = true;
                btn_pedal_connect.Content = "Disconnect From Pedal";
            }
            else
            {
                ConnectToPedal.IsChecked = false;
                btn_pedal_connect.Content = "Connect To Pedal";
            }

            if (Plugin.Settings.file_enable_check[profile_select, 0] == 1)
            {
                Label_clutch_file.Content = Plugin.Settings.Pedal_file_string[profile_select,0];
                Clutch_file_check.IsChecked = true;
            }
            else 
            {
                Label_clutch_file.Content = "";
                Clutch_file_check.IsChecked = false;
            }
            if (Plugin.Settings.file_enable_check[profile_select, 1] == 1)
            {
                Label_brake_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 1];
                Brake_file_check.IsChecked = true;
            }
            else
            {
                Label_brake_file.Content = "";
                Brake_file_check.IsChecked = false;
            }

            if (Plugin.Settings.file_enable_check[profile_select,2] == 1)
            {
                Label_gas_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 2];
                Gas_file_check.IsChecked = true;
            }
            else
            {
                Label_gas_file.Content = "";
                Gas_file_check.IsChecked = false;
            }
            

            textbox_profile_name.Text = Plugin.Settings.Profile_name[profile_select];


            //Canvas.SetRight(TextBlock_Warning, 0);


            //Rudder UI initialized
            if (Plugin != null)
            {
  
                if (Plugin.Rudder_status)
                {
                    btn_rudder_initialize.Content = "Disable Rudder";
                    //text_rudder_log.Visibility= Visibility.Visible;
                }
                else
                {
                    btn_rudder_initialize.Content = "Enable Rudder";
                    //text_rudder_log.Visibility = Visibility.Hidden;
                }


                //Rudder text
                info_rudder_label.Content = "Bridge State:\nClutch:\nBrake:\nThrottle:\nRudder:";
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    info_rudder_label_2.Content = "Online\n";
                }
                else
                {
                    info_rudder_label_2.Content = "Offline\n";
                }
                if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1)
                {
                    info_rudder_label_2.Content += "Online\n";
                }
                else
                {
                    info_rudder_label_2.Content += "Offline\n";
                }
                if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1)
                {
                    info_rudder_label_2.Content += "Online\n";
                }
                else
                {
                    info_rudder_label_2.Content += "Offline\n";
                }
                if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                {
                    info_rudder_label_2.Content += "Online\n";
                }
                else
                {
                    info_rudder_label_2.Content += "Offline\n";
                }
                if (Plugin.Rudder_status)
                {
                    info_rudder_label_2.Content += "In action";
                }
                else
                {
                    info_rudder_label_2.Content += "Off";
                }
            }

            //system UI

            if (Plugin != null)
            {
                if (Plugin.Settings.advanced_b)
                {
                    Debug_check.IsChecked = true;
                    debug_flag=Plugin.Settings.advanced_b;
                }
                else
                { 
                    Debug_check.IsChecked= false;
                    debug_flag = Plugin.Settings.advanced_b;
                }

                if (Plugin.Settings.Serial_auto_clean_bridge)
                {
                    Checkbox_auto_remove_serial_line_bridge.IsChecked = true;
                }
                else
                {
                    Checkbox_auto_remove_serial_line_bridge.IsChecked = false;
                }
                //effect profile reading
                if (Update_Profile_Checkbox_b)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        for (int k = 0; k < 8; k++)
                        {
                            if (Plugin.Settings.Effect_status_prolife[profile_select, j, k])
                            {
                                Effect_status_profile[j, k].IsChecked = true;
                            }
                            else
                            {
                                Effect_status_profile[j, k].IsChecked = false;
                            }
                        }
                    }
                    Update_Profile_Checkbox_b = false;
                }

                textbox_SSID.Text = Plugin.Settings.SSID_string;
                textbox_PASS.Password = Plugin.Settings.PASS_string;
            }

        }

        public class SerialPortChoice
        {
            public SerialPortChoice(string display, string value)
            {
                Display = display;
                Value = value;
            }

            public string Value { get; set; }
            public string Display { get; set; }
        }

        private void PedalParameterLiveUpdate()
        {
            if (Plugin != null)
            {
                if (Plugin.Settings.LivePreview[indexOfSelectedPedal_u] && Plugin.PedalConfigRead_b[indexOfSelectedPedal_u])
                {
                    DateTime ConfigLiveSending_now = DateTime.Now;
                    TimeSpan diff = ConfigLiveSending_now - ConfigLiveSending_last;
                    int millisceonds = (int)diff.TotalMilliseconds;
                    bool live_preview_b = true;
                    
                    if (PedalTabChange)
                    {
                        diff = ConfigLiveSending_now-PedalTabChange_last ;
                        int millseconds_pedaltabchange = (int)diff.TotalMilliseconds;
                        if (millseconds_pedaltabchange > 100)
                        {
                            PedalTabChange = false;
                            PedalTabChange_last=DateTime.Now;
                            
                        }
                        else
                        {
                            live_preview_b = false;
                        }
                    }
                    
                    if (millisceonds > Plugin.Settings.Pedal_action_interval[indexOfSelectedPedal_u] && live_preview_b)
                    {
                        //live_preview_b = true;
                        Plugin.SendConfigWithoutSaveToEEPROM(dap_config_st[indexOfSelectedPedal_u], (byte)indexOfSelectedPedal_u);
                        ConfigLiveSending_last = DateTime.Now;
                    }





                }
            }

        }


        // Select which pedal to config
        // see https://stackoverflow.com/questions/772841/is-there-selected-tab-changed-event-in-the-standard-wpf-tab-control
        private void TabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            // update the sliders & serial port selection accordingly
            if (Plugin != null)
            {
                indexOfSelectedPedal_u = (uint)MyTab.SelectedIndex;
                Plugin.Settings.table_selected = (uint)MyTab.SelectedIndex;
                //Update_CV_textbox = true;
                Plugin._calculaitons.Update_CV1_textbox = true;
                Plugin._calculaitons.Update_CV2_textbox = true;
                PedalTabChange =true;
                PedalTabChange_last=DateTime.Now;
                updateTheGuiFromConfig();
            }
        }


        private void NumericTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex("^[.][0-9]+$|^[0-9]*[.]{0,4}[0-9]*$");

            System.Windows.Controls.TextBox textBox = (System.Windows.Controls.TextBox)sender;

            e.Handled = !regex.IsMatch(textBox.Text + e.Text);

        }


        unsafe public void Sendconfig(uint pedalIdx)
        {
            // compute checksum
            //getBytes(this.dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_)
            dap_config_st[pedalIdx].payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_st[pedalIdx].payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_st[pedalIdx].payloadHeader_.PedalTag = (byte)pedalIdx;
            dap_config_st[pedalIdx].payloadHeader_.storeToEeprom = 1;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedal_type = (byte)pedalIdx;

            DAP_config_st tmp = dap_config_st[pedalIdx];
            //prevent read default config from pedal without assignement
            DAP_config_st* v = &tmp;

            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));


            int length = sizeof(DAP_config_st);
            //int val = this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.checkSum;
            //string msg = "CRC value: " + val.ToString();
            byte[] newBuffer = new byte[length];
            newBuffer = getBytes(tmp);

            //TextBox_debugOutput.Text = "CRC simhub calc: " + this.dap_config_st[indexOfSelectedPedal_u].payloadFooter_.checkSum + "    ";

            TextBox_debugOutput.Text = String.Empty;
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        TextBox2.Text = "Buffer sent size:" + length;
                        Plugin.ESPsync_serialPort.DiscardInBuffer();
                        Plugin.ESPsync_serialPort.DiscardOutBuffer();
                        // send data
                        Plugin.ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);


                        //Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                        System.Threading.Thread.Sleep(100);
                    }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        TextBox_debugOutput.Text = errorMessage;
                    }
                }
            }
            else
            {
                //int length2 = sizeof(DAP_config_st);
                if (Plugin._serialPort[pedalIdx].IsOpen)
                {

                    try
                    {
                        //TextBox_debugOutput.Text = "ConfigLength" + length;
                        // clear inbuffer 
                        Plugin._serialPort[pedalIdx].DiscardInBuffer();
                        Plugin._serialPort[pedalIdx].DiscardOutBuffer();
                        // send data
                        Plugin._serialPort[pedalIdx].Write(newBuffer, 0, newBuffer.Length);
                        //Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                    }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        TextBox_debugOutput.Text = errorMessage;
                    }

                }
            }
        }

        unsafe public void Sendconfig_Rudder(uint pedalIdx)
        {

            dap_config_st_rudder.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_st_rudder.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_st_rudder.payloadHeader_.PedalTag = (byte)pedalIdx;
            dap_config_st_rudder.payloadHeader_.storeToEeprom = 0;
            dap_config_st_rudder.payloadPedalConfig_.pedal_type = (byte)pedalIdx;
            DAP_config_st tmp = dap_config_st_rudder;

            DAP_config_st* v = &tmp;
            byte* p = (byte*)v;
            dap_config_st_rudder.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
            Plugin.SendConfig(dap_config_st_rudder, (byte)pedalIdx);
            

        }
        unsafe public void Sendconfigtopedal_shortcut()
        {

            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                if (Plugin.Settings.file_enable_check[profile_select, pedalIdx] == 1)
                {
                    Sendconfig(pedalIdx);
                    TextBox_debugOutput.Text = "config was sent to pedal";
                }
            }

        }
        unsafe public void SendConfigToPedal_click(object sender, RoutedEventArgs e)
        {

            Sendconfig(indexOfSelectedPedal_u);
 
        }


        unsafe public void Reading_config_auto(uint i)
        {
            // compute checksum
            DAP_action_st tmp;
            tmp.payloadPedalAction_.returnPedalConfig_u8 = 1;
            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp.payloadHeader_.PedalTag = (byte)i;
            DAP_action_st* v = &tmp;
            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
            int length = sizeof(DAP_action_st);
            byte[] newBuffer = new byte[length];
            newBuffer = Plugin.getBytes_Action(tmp);
            // tell the plugin that we expect config data
            waiting_for_pedal_config[i] = true;
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[i])
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    // try N times and check whether config has been received
                    for (int rep = 0; rep < 1; rep++)
                    {
                        // send query command
                        Plugin.ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);

                        // wait some time and check whether data has been received
                        System.Threading.Thread.Sleep(50);

                        if (waiting_for_pedal_config[i] == false)
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                if (Plugin._serialPort[i].IsOpen)
                {
                    // try N times and check whether config has been received
                    for (int rep = 0; rep < 1; rep++)
                    {
                        // send query command
                        Plugin._serialPort[i].Write(newBuffer, 0, newBuffer.Length);

                        // wait some time and check whether data has been received
                        System.Threading.Thread.Sleep(50);

                        if (waiting_for_pedal_config[i] == false)
                        {
                            break;
                        }
                    }
                }
            }

        }

        public string[] STOPCHAR = { "\r\n" };
        public bool EndsWithStop(string incomingData)
        {
            for (int i = 0; i < STOPCHAR.Length; i++)
            {
                if (incomingData.EndsWith(STOPCHAR[i]))
                {
                    return true;
                }
            }
            return false;
        }

        unsafe public void ReadConfigFromPedal_click(object sender, RoutedEventArgs e)
        {
            Reading_config_auto(indexOfSelectedPedal_u);
        }


        public string[] _data = { "", "", "" };// = "";

        //unsafe private void sp_DataReceived(object sender, object e)
        unsafe private void sp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {

            SerialPort sp = (SerialPort)sender;
            //string _type = (string)e;

            //if (Plugin._serialPort[indexOfSelectedPedal_u].PortName = sp.PortName)

            // identify which pedal has send the data
            int pedalSelected = 255;
            for (int pedalIdx_i = 0; pedalIdx_i < 3; pedalIdx_i++)
            {
                if ((Plugin._serialPort[pedalIdx_i].PortName == sp.PortName) && (Plugin._serialPort[pedalIdx_i].IsOpen))
                {
                    pedalSelected = pedalIdx_i;
                }
            }

            // once the pedal has identified, go ahead
            if (pedalSelected < 3)
            //if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
            {
                // https://stackoverflow.com/questions/9732709/the-calling-thread-cannot-access-this-object-because-a-different-thread-owns-it


                int length = sizeof(DAP_config_st);
                byte[] newBuffer_config = new byte[length];

                int receivedLength = sp.BytesToRead;


                string incomingData = sp.ReadExisting();
                //if the data doesn't end with a stop char this will signal to keep it in _data 
                //for appending to the following read of data
                bool endsWithStop = EndsWithStop(incomingData);

                //each array object will be sent separately to the callback
                string[] dataArray = incomingData.Split(STOPCHAR, StringSplitOptions.None);

                for (int i = 0; i < dataArray.Length; i++)
                {
                    string newData = dataArray[i];

                    //if you are at the last object in the array and this hasn't got a stopchar after
                    //it will be saved in _data
                    if (!endsWithStop && i == dataArray.Length - 1)
                    {
                        _data[pedalSelected] += newData;
                    }
                    else
                    {
                        string dataToSend = _data[pedalSelected] + newData;
                        _data[pedalSelected] = "";


                        // decode into config struct
                        if (dataToSend.Length == length)
                        {
                            DAP_config_st tmp;

                            // transform string into byte
                            fixed (byte* p = Encoding.ASCII.GetBytes(dataToSend))
                            {
                                // create a fixed size buffer
                                length = sizeof(DAP_config_st);
                                byte[] newBuffer_config_2 = new byte[length];

                                // copy the received bytes into byte array
                                for (int j = 0; j < length; j++)
                                {
                                    newBuffer_config_2[j] = p[j];
                                }

                                // parse byte array as config struct
                                DAP_config_st pedalConfig_read_st = getConfigFromBytes(newBuffer_config_2);

                                // check whether receive struct is plausible
                                DAP_config_st* v_config = &pedalConfig_read_st;
                                byte* p_config = (byte*)v_config;

                                // payload type check
                                bool check_payload_config_b = false;
                                if (pedalConfig_read_st.payloadHeader_.payloadType == Constants.pedalConfigPayload_type)
                                {
                                    check_payload_config_b = true;
                                }

                                // CRC check
                                bool check_crc_config_b = false;
                                if (Plugin.checksumCalc(p_config, sizeof(payloadHeader) + sizeof(payloadPedalConfig)) == pedalConfig_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_config_b = true;
                                }


                                // when all checks are passed, accept the config. Otherwise discard and trow error
                                Dispatcher.Invoke(
                                new Action<DAP_config_st>((t) => dap_config_st[pedalSelected] = t),
                                pedalConfig_read_st);


                                this.Dispatcher.Invoke(() =>
                                {
                                    // update pedal config
                                    if (check_payload_config_b)
                                    {
                                        //this.dap_config_st[indexOfSelectedPedal_u] = pedalConfig_read_st;
                                        updateTheGuiFromConfig();
                                    }

                                    TextBox_debugOutput.Text = "Payload config test 1: " + check_payload_config_b;
                                    TextBox_debugOutput.Text += "Payload config test 2: " + check_crc_config_b;
                                });

                            }

                        }
                        else
                        {
                            this.Dispatcher.Invoke(() =>
                            {
                                //TextBox_serialMonitor.Text += "DataArrayLength: " + dataArray.Length + "\n";
                                //TextBox_serialMonitor.Text += "DataLength: " + dataToSend.Length + "\n";
                                if (_serial_monitor_window != null)
                                {
                                    _serial_monitor_window.TextBox_SerialMonitor.Text += dataToSend + "\n";
                                    _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
                                }
                                //TextBox_serialMonitor.Text += dataToSend + "\n";
                                //TextBox_serialMonitor.ScrollToEnd();
                                //TextBox_serialMonitor.Text += receivedLength + "\n";
                            });
                        }


                    }











                    //limits the data stored to 1000 to avoid using up all the memory in case of 
                    //failure to register callback or include stopchar

                    if (_data[pedalSelected].Length > 1000)
                    {
                        _data[pedalSelected] = "";
                    }


                    //////this.Dispatcher.Invoke(() =>
                    //////    {
                    //////        TextBox_serialMonitor.Text += incomingData;

                    //////        TextBox_serialMonitor.ScrollToEnd();
                    //////        //TextBox_serialMonitor.Text += receivedLength + "\n";
                    //////    });
                }

                // obtain data and check whether it is from known payload type or just debug info

            }
        }



        /********************************************************************************************************************/
        /*							read serial stream																		*/
        /********************************************************************************************************************/
        public void openSerialAndAddReadCallback(uint pedalIdx)
        {
            try
            {
                // serial port settings
                Plugin._serialPort[pedalIdx].Handshake = Handshake.None;
                Plugin._serialPort[pedalIdx].Parity = Parity.None;
                //_serialPort[pedalIdx].StopBits = StopBits.None;


                Plugin._serialPort[pedalIdx].ReadTimeout = 2000;
                Plugin._serialPort[pedalIdx].WriteTimeout = 500;

                // https://stackoverflow.com/questions/7178655/serialport-encoding-how-do-i-get-8-bit-ascii
                Plugin._serialPort[pedalIdx].Encoding = System.Text.Encoding.GetEncoding(28591);

                // regular ESP
                //Plugin._serialPort[pedalIdx].DtrEnable = false;

                // ESP32 S3
                //Plugin._serialPort[pedalIdx].RtsEnable = false;
                //Plugin._serialPort[pedalIdx].DtrEnable = true;


                Plugin._serialPort[pedalIdx].NewLine = "\r\n";
                Plugin._serialPort[pedalIdx].ReadBufferSize = 10000;
                if (Plugin.Settings.auto_connect_flag[pedalIdx] == 1 & Plugin.Settings.connect_flag[pedalIdx] == 1 )
                {
                    if (Plugin.Settings.autoconnectComPortNames[pedalIdx] == "NA")
                    {
                        Plugin._serialPort[pedalIdx].PortName = Plugin.Settings.autoconnectComPortNames[pedalIdx];
                    }
                    else
                    {
                        Plugin._serialPort[pedalIdx].PortName = Plugin.Settings.selectedComPortNames[pedalIdx];
                        Plugin.Settings.autoconnectComPortNames[pedalIdx] = Plugin.Settings.selectedComPortNames[pedalIdx];
                    }
                    
                }
                else
                {
                    Plugin._serialPort[pedalIdx].PortName = Plugin.Settings.selectedComPortNames[pedalIdx];
                    Plugin.Settings.autoconnectComPortNames[pedalIdx]= Plugin.Settings.selectedComPortNames[pedalIdx];
                }
                
                if (Plugin.PortExists(Plugin._serialPort[pedalIdx].PortName))
                {
                    try
                    {
                        Plugin._serialPort[pedalIdx].Open();

                        // ESP32 S3
                        /*
                        if (Plugin.Settings.RTSDTR_False[pedalIdx] == true)
                        {
                            Plugin._serialPort[pedalIdx].RtsEnable = false;
                            Plugin._serialPort[pedalIdx].DtrEnable = false;
                        }
                        */
                        //

                        if (Plugin.Settings.USING_ESP32S3[pedalIdx] == true)
                        {
                            // ESP32 S3
                            Plugin._serialPort[pedalIdx].RtsEnable = false;
                            Plugin._serialPort[pedalIdx].DtrEnable = true;
                        }
                        
                        

                        System.Threading.Thread.Sleep(200);

                       

                        Plugin.Settings.connect_status[pedalIdx] = 1;
                        // read callback
                        if (pedal_serial_read_timer[pedalIdx] != null)
                        {
                            pedal_serial_read_timer[pedalIdx].Stop();
                            pedal_serial_read_timer[pedalIdx].Dispose();
                        }
                        pedal_serial_read_timer[pedalIdx] = new System.Windows.Forms.Timer();
                        pedal_serial_read_timer[pedalIdx].Tick += new EventHandler(timerCallback_serial);
                        pedal_serial_read_timer[pedalIdx].Tag = pedalIdx;
                        pedal_serial_read_timer[pedalIdx].Interval = 16; // in miliseconds
                        pedal_serial_read_timer[pedalIdx].Start();
                        System.Threading.Thread.Sleep(100);
                        Serial_connect_status[pedalIdx] = true;
                    }
                    catch(Exception ex)
                    { 
                        TextBox2.Text = ex.Message;
                        Serial_connect_status[pedalIdx] = false;
                    }
                    

                }
                else
                {
                    Plugin.Settings.connect_status[pedalIdx] = 0;
                    Plugin.connectSerialPort[pedalIdx] = false;
                    Serial_connect_status[pedalIdx] = false;

                }
            }
            catch (Exception ex)
            { }




        }
        private uint count_timmer_count = 0;
        private string Toast_tmp;
        public void connection_timmer_tick(object sender, EventArgs e)
        {
            //simhub action for debug
            Simhub_action_update();
            string tmp = "Connecting";
            int count_connection = ((int)count_timmer_count) % 4;
            
            switch (count_connection) 
            {
                case 0:
                    break;
                case 1:
                    tmp = tmp + ".";
                    break;
                case 2:
                    tmp = tmp + "..";
                    break;
                case 3:
                    tmp = tmp + "...";
                    break;
            }
            info_text_connection = tmp;
            system_info_text_connection=tmp;
            for (uint pedal_idex = 0; pedal_idex < 3; pedal_idex++)
            {
                if (Pedal_wireless_connection_update_b[pedal_idex])
                {
                    Pedal_wireless_connection_update_b[pedal_idex] = false;
                    if (Plugin.Settings.reading_config == 1)
                    {
                        Reading_config_auto(pedal_idex);
                    }
                }
            }

            

            count_timmer_count++;
            if (count_timmer_count > 1)
            {
                if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                {
                    if (Plugin.PortExists(Plugin.Settings.ESPNow_port))
                    {
                        if (Plugin.ESPsync_serialPort.IsOpen == false)
                        {
                            Plugin.ESPsync_serialPort.PortName = Plugin.Settings.ESPNow_port;
                            try
                            {
                                // serial port settings
                                Plugin.ESPsync_serialPort.Handshake = Handshake.None;
                                Plugin.ESPsync_serialPort.Parity = Parity.None;
                                //_serialPort[pedalIdx].StopBits = StopBits.None;
                                Plugin.ESPsync_serialPort.ReadTimeout = 2000;
                                Plugin.ESPsync_serialPort.WriteTimeout = 500;
                                Plugin.ESPsync_serialPort.BaudRate = Bridge_baudrate;
                                // https://stackoverflow.com/questions/7178655/serialport-encoding-how-do-i-get-8-bit-ascii
                                Plugin.ESPsync_serialPort.Encoding = System.Text.Encoding.GetEncoding(28591);
                                Plugin.ESPsync_serialPort.NewLine = "\r\n";
                                Plugin.ESPsync_serialPort.ReadBufferSize = 40960;
                                try
                                {
                                    Plugin.ESPsync_serialPort.Open();
                                    System.Threading.Thread.Sleep(200);
                                    // ESP32 S3
                                    if (Plugin.Settings.Using_CDC_bridge)
                                    {
                                        Plugin.ESPsync_serialPort.RtsEnable = false;
                                        Plugin.ESPsync_serialPort.DtrEnable = true;
                                    }
                                    //SystemSounds.Beep.Play();
                                    Plugin.Sync_esp_connection_flag = true;
                                    btn_connect_espnow_port.Content = "Disconnect";
                                    ESP_host_serial_timer = new System.Windows.Forms.Timer();
                                    ESP_host_serial_timer.Tick += new EventHandler(timerCallback_serial_esphost);
                                    ESP_host_serial_timer.Tag = 3;
                                    ESP_host_serial_timer.Interval = 8; // in miliseconds
                                    ESP_host_serial_timer.Start();
                                    System.Threading.Thread.Sleep(100);
                                    ToastNotification("Pedal Wireless Bridge", "Connected");
                                    updateTheGuiFromConfig();
                                }
                                catch (Exception ex)
                                {
                                    TextBox2.Text = ex.Message;
                                    //Serial_connect_status[3] = false;
                                }
                            }
                            catch (Exception ex)
                            {
                                TextBox2.Text = ex.Message;
                            }
                        }
                    }
                    else
                    {
                        if (Plugin.Sync_esp_connection_flag)
                        {
                            Plugin.Sync_esp_connection_flag = false;
                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 = 0;
                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 = 0;
                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 = 0;
                            for (int i = 0; i < 3; i++)
                            {
                                Plugin.PedalConfigRead_b[i] = false;
                            }
                            
                        }
                        
                        btn_connect_espnow_port.Content = "Connect";
                        if (ESP_host_serial_timer != null)
                        {
                            ESP_host_serial_timer.Stop();
                            ESP_host_serial_timer.Dispose();
                            updateTheGuiFromConfig();
                        }
                            
                    }
                                            
                }

                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    if (Plugin.Settings.auto_connect_flag[pedalIdx] == 1)
                    {

                        if (Plugin.Settings.connect_flag[pedalIdx] == 1)
                        {
                            if (Plugin.PortExists(Plugin._serialPort[pedalIdx].PortName))
                            {
                                if (Plugin._serialPort[pedalIdx].IsOpen == false)
                                {
                                    //UpdateSerialPortList_click();
                                    openSerialAndAddReadCallback(pedalIdx);
                                    //Plugin.Settings.autoconnectComPortNames[pedalIdx] = Plugin._serialPort[pedalIdx].PortName;
                                    System.Threading.Thread.Sleep(200);
                                    if (Serial_connect_status[pedalIdx])
                                    {
                                        if (Plugin.Settings.reading_config == 1)
                                        {
                                            Reading_config_auto(pedalIdx);
                                        }
                                        System.Threading.Thread.Sleep(100);
                                        //add toast notificaiton
                                        switch (pedalIdx)
                                        {
                                            case 0:
                                                Toast_tmp = "Clutch Pedal:" + Plugin.Settings.autoconnectComPortNames[pedalIdx];
                                                break;
                                            case 1:
                                                Toast_tmp = "Brake Pedal:" + Plugin.Settings.autoconnectComPortNames[pedalIdx];
                                                break;
                                            case 2:
                                                Toast_tmp = "Throttle Pedal:" + Plugin.Settings.autoconnectComPortNames[pedalIdx];
                                                break;
                                        }
                                        ToastNotification(Toast_tmp, "Connected");
                                        updateTheGuiFromConfig();
                                        //System.Threading.Thread.Sleep(2000);
                                        //ToastNotificationManager.History.Clear("FFB Pedal Dashboard");
                                    }



                                }
                            }
                            else
                            {
                                Plugin.connectSerialPort[pedalIdx] = false;
                                Plugin.Settings.connect_status[pedalIdx] = 0;
                                Plugin.PedalConfigRead_b[pedalIdx] = false;
                                updateTheGuiFromConfig();
                            }




                        }
                    }
                }

                
            }
            if (count_timmer_count > 200)
            {
                count_timmer_count = 2;
            }
            updateTheGuiFromConfig();

        }

        public void closeSerialAndStopReadCallback(uint pedalIdx)
        {
            
            if (pedal_serial_read_timer[pedalIdx] != null)
            {
                pedal_serial_read_timer[pedalIdx].Stop();
                pedal_serial_read_timer[pedalIdx].Dispose();
            }
            connect_timer.Dispose();
            connect_timer.Stop();
            if (ESP_host_serial_timer != null)
            {
                ESP_host_serial_timer.Stop();
                ESP_host_serial_timer.Dispose();
            }
            System.Threading.Thread.Sleep(300);
            
            
            if (Plugin._serialPort[pedalIdx].IsOpen)
            {
                // ESP32 S3
                /*
                if (Plugin.Settings.RTSDTR_False[pedalIdx] == true)
                {
                    Plugin._serialPort[pedalIdx].RtsEnable = false;
                    Plugin._serialPort[pedalIdx].DtrEnable = false;
                }
                */


                Plugin._serialPort[pedalIdx].DiscardInBuffer();
                Plugin._serialPort[pedalIdx].DiscardOutBuffer();
                Plugin._serialPort[pedalIdx].Close();
                Plugin.Settings.connect_status[pedalIdx] = 0;
            }
            if (Plugin.ESPsync_serialPort.IsOpen)
            {
                Plugin.ESPsync_serialPort.DiscardInBuffer();
                Plugin.ESPsync_serialPort.DiscardOutBuffer();
                Plugin.ESPsync_serialPort.Close();
                Plugin.Sync_esp_connection_flag = false;
            }
        }

        Int64 writeCntr = 0;

        int[] timeCntr = { 0, 0, 0,0 };

        double[] timeCollector = { 0, 0, 0,0 };


        static List<int> FindAllOccurrences(byte[] source, byte[] sequence, int maxLength)
        {
            List<int> indices = new List<int>();

            int len = source.Length - sequence.Length;
            if (len > maxLength)
            {
                len = maxLength;
            }

            for (int i = 0; i <= len; i++)
            {
                bool found = true;
                for (int j = 0; j < sequence.Length; j++)
                {
                    if (source[i + j] != sequence[j])
                    {
                        found = false;
                        break;
                    }
                }
                if (found)
                {
                    indices.Add(i); // Sequence found, add index to the list
                }
            }



            //int i = 0;
            //while (i < len)
            //{
            //    bool found = true;
            //    for (int j = 0; j < sequence.Length; j++)
            //    {
            //        if (source[i + j] != sequence[j])
            //        {
            //            found = false;
            //            break;
            //        }
            //    }
            //    if (found)
            //    {
            //        indices.Add(i); // Sequence found, add index to the list
            //        i += sequence.Length;
            //    }
            //    else { i++; } 
            //}



            return indices;
        }

        public void Simhub_action_update()
        {
            if (Plugin.Page_update_flag == true)
            {
                Profile_change(Plugin.profile_index);
                Plugin.Page_update_flag = false;
                MyTab.SelectedIndex = (int)Plugin.Settings.table_selected;
                Plugin.pedal_select_update_flag = false;
                Plugin.simhub_theme_color = defaultcolor.ToString();
                switch (Plugin.Settings.table_selected)
                {
                    case 0:
                        Plugin.current_pedal = "Clutch";
                        break;
                    case 1:
                        Plugin.current_pedal = "Brake";
                        break;
                    case 2:
                        Plugin.current_pedal = "Throttle";
                        break;
                }
                updateTheGuiFromConfig();
            }

            if (Plugin.sendconfig_flag == 1)
            {
                Sendconfigtopedal_shortcut();
                Plugin.sendconfig_flag = 0;
            }
        }

        int[] appendedBufferOffset = { 0, 0, 0,0 };

        static int bufferSize = 10000;
        static int destBufferSize = 1000;
        byte[][] buffer_appended = { new byte[bufferSize], new byte[bufferSize], new byte[bufferSize], new byte[bufferSize] };

        unsafe public void timerCallback_serial(object sender, EventArgs e)
        {

            //action here 
            Simhub_action_update();
            
            


            int pedalSelected = Int32.Parse((sender as System.Windows.Forms.Timer).Tag.ToString());
            //int pedalSelected = (int)(sender as System.Windows.Forms.Timer).Tag;

            bool pedalStateHasAlreadyBeenUpdated_b = false;

            // once the pedal has identified, go ahead
            if (pedalSelected < 3)
            //if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
            {



                // Create a Stopwatch instance
                Stopwatch stopwatch = new Stopwatch();

                // Start the stopwatch
                stopwatch.Start();



                SerialPort sp = Plugin._serialPort[pedalSelected];



                // https://stackoverflow.com/questions/9732709/the-calling-thread-cannot-access-this-object-because-a-different-thread-owns-it


                //int length = sizeof(DAP_config_st);




                if (sp.IsOpen)
                {
                    if (Plugin.Settings.Serial_auto_clean)
                    {
                        /*
                        if (TextBox_serialMonitor.LineCount > 300)
                        {
                            TextBox_serialMonitor.Clear();
                        }
                        */
                        if (_serial_monitor_window != null && _serial_monitor_window.TextBox_SerialMonitor.LineCount > 300)
                        {
                            _serial_monitor_window.TextBox_SerialMonitor.Clear();
                        }
                    }

                    int receivedLength = 0;
                    try 
                    {
                        receivedLength = sp.BytesToRead;
                    }
                    catch (Exception ex)
                    {
                        TextBox_debugOutput.Text = ex.Message;
                        //ConnectToPedal.IsChecked = false;
                        return;
                    }

                

                    if (receivedLength > 0)
                    {

                        //TextBox_serialMonitor.Text += "Received:" + receivedLength + "\n";
                        //TextBox_serialMonitor.ScrollToEnd();


                        timeCntr[pedalSelected] += 1;


                        // determine byte sequence which is defined as message end --> crlf
                        byte[] byteToFind = System.Text.Encoding.GetEncoding(28591).GetBytes(STOPCHAR[0].ToCharArray());
                        int stop_char_length = byteToFind.Length;


                        // calculate current buffer length
                        int currentBufferLength = appendedBufferOffset[pedalSelected] + receivedLength;


                        // check if buffer is large enough otherwise discard in buffer and set offset to 0
                        if ((bufferSize > currentBufferLength) && (appendedBufferOffset[pedalSelected] >= 0))
                        {
                            sp.Read(buffer_appended[pedalSelected], appendedBufferOffset[pedalSelected], receivedLength);
                        }
                        else
                        {
                            sp.DiscardInBuffer();
                            appendedBufferOffset[pedalSelected] = 0;
                            return;
                        }


                        


                        // copy to local buffer
                        //byte[] localBuffer = new byte[currentBufferLength];
                        
                        //Buffer.BlockCopy(buffer_appended[pedalSelected], 0, localBuffer, 0, currentBufferLength);


                        // find all occurences of crlf as they indicate message end
                        List<int> indices = FindAllOccurrences(buffer_appended[pedalSelected], byteToFind, currentBufferLength);




                        // Destination array
                        byte[] destinationArray = new byte[destBufferSize];

                        





                        int srcBufferOffset = 0;
                        // decode every message
                        //foreach (int number in indices)
                        for (int msgId = 0; msgId < indices.Count; msgId++)
                        {
                            // computes the length of bytes to read
                            int destBuffLength = 0; //number - srcBufferOffset;

                            if (msgId == 0)
                            {
                                srcBufferOffset = 0;
                                destBuffLength = indices.ElementAt(msgId);
                            }
                            else 
                            {
                                srcBufferOffset = indices.ElementAt(msgId - 1) + stop_char_length;
                                destBuffLength = indices.ElementAt(msgId) - srcBufferOffset;
                            }

                            // check if dest buffer length is within valid length
                            if ( (destBuffLength <= 0) | (destBuffLength > destBufferSize) )
                            {
                                continue;
                            }


                 


                            // copy bytes to subarray
                            Buffer.BlockCopy(buffer_appended[pedalSelected], srcBufferOffset, destinationArray, 0, destBuffLength);


                            // check for pedal state struct
                            if ((destBuffLength == sizeof(DAP_state_basic_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_basic_st pedalState_read_st = getStateFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_basic_st* v_state = &pedalState_read_st;
                                byte* p_state = (byte*)v_state;
                                
                                // payload type check
                                bool check_payload_state_b = false;
                                if (pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                {
                                    check_payload_state_b = true;
                                }

                                //Pedal version and Plugin DAP version check
                                Pedal_version[pedalSelected] = pedalState_read_st.payloadHeader_.version;
                                if (Pedal_version[pedalSelected] != Constants.pedalConfigPayload_version && pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                {
                                    if (!Version_warning_first_show_b[pedalSelected])
                                    {
                                        Version_warning_first_show_b[pedalSelected] = true;
                                        if (Pedal_version[pedalSelected] > Constants.pedalConfigPayload_version)
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Simhub Plugin.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                        else
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Pedal Firmware.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                    }
                                }


                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Basic)) == pedalState_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {

                                    // write vJoy data
                                    Pedal_position_reading[pedalSelected] = pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16;
                                    //if (Plugin.Rudder_enable_flag == false)
                                    //{
                                        if (Plugin.Settings.vjoy_output_flag == 1)
                                        {
                                            switch (pedalSelected)
                                            {

                                                case 0:
                                                //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RX);  // Center X axis
                                                    PedalSettingsSection._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RX);   // HID_USAGES Enums
                                                    break;
                                                case 1:
                                                //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RY);  // Center X axis
                                                    PedalSettingsSection._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RY);   // HID_USAGES Enums
                                                    break;
                                                case 2:
                                                //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RZ);  // Center X axis
                                                    PedalSettingsSection._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RZ);   // HID_USAGES Enums
                                                    break;
                                                default:
                                                    break;
                                            }

                                        }
                                   



                                    // GUI update
                                    if ((pedalStateHasAlreadyBeenUpdated_b == false) && (indexOfSelectedPedal_u == pedalSelected))
                                    {
                                        //TextBox_debugOutput.Text = "Pedal pos: " + pedalState_read_st.payloadPedalState_.pedalPosition_u16;
                                        //TextBox_debugOutput.Text += "Pedal force: " + pedalState_read_st.payloadPedalState_.pedalForce_u16;
                                        //TextBox_debugOutput.Text += ",  Servo pos targe: " + pedalState_read_st.payloadPedalState_.servoPosition_i16;
                                        //TextBox_debugOutput.Text += ",  Servo pos: " + pedalState_read_st.payloadPedalState_.servoPosition_i16;

                                        PedalForceTravel_Tab.updatePedalState(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16, pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);

                                        pedalStateHasAlreadyBeenUpdated_b = true;

                                        double control_rect_value_max = 65535;


                                        if (debug_flag)
                                        {
                                            int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                            int x_showed = round_x + 1;
                                            
                                            current_pedal_travel_state = x_showed;
                                            Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;

                                        }
                                        else
                                        {
                                            int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                            int x_showed = round_x + 1;
                                            round_x = Math.Max(0, Math.Min(round_x, 99));
                                            current_pedal_travel_state = x_showed;
                                            Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;
                                        }
                                        for (int i = 0; i < 3; i++)
                                        {
                                            PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                        }
                                        

                                    }


                                    continue;
                                }

                            }






                            // check for pedal extended state struct
                            if ((destBuffLength == sizeof(DAP_state_extended_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_extended_st pedalState_ext_read_st = getStateExtFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_extended_st* v_state = &pedalState_ext_read_st;
                                byte* p_state = (byte*)v_state;

                                // payload type check
                                bool check_payload_state_b = false;
                                if (pedalState_ext_read_st.payloadHeader_.payloadType == Constants.pedalStateExtendedPayload_type)
                                {
                                    check_payload_state_b = true;
                                }

                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Extended)) == pedalState_ext_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {




                                    if (indexOfSelectedPedal_u == pedalSelected)
                                    {
                                        
                                        if (Plugin._calculaitons.dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                        //if (dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                        {
                                            // Specify the path to the file
                                            string currentDirectory = Directory.GetCurrentDirectory();
                                            string filePath = currentDirectory + "\\PluginsData\\Common" + "\\DiyFfbPedalStateLog_" + indexOfSelectedPedal_u.ToString() + ".txt";


                                            // delete file 
                                            if (true == dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u])
                                            {
                                                dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u] = false;
                                                File.Delete(filePath);
                                            }


                                            // write header
                                            if (!File.Exists(filePath))
                                            {
                                                using (StreamWriter writer = new StreamWriter(filePath, true))
                                                {
                                                    // Write the content to the file
                                                    writer.Write("cycleCtr, ");
                                                    writer.Write("time_InMs, ");
                                                    writer.Write("forceRaw_InKg, ");
                                                    writer.Write("forceFiltered_InKg, ");
                                                    writer.Write("forceVelocity_InKgPerSec, ");
                                                    writer.Write("servoPos_InSteps, ");
                                                    writer.Write("servoPosEsp_InSteps, ");
                                                    writer.Write("servoCurrent_InPercent, ");
                                                    writer.Write("servoVoltage_InV, ");
                                                    writer.Write("angleSensorOutput");
                                                    writer.Write("\n");
                                                }

                                            }


                                            // Use StreamWriter to write to the file
                                            using (StreamWriter writer = new StreamWriter(filePath, true))
                                            {
                                                // Write the content to the file
                                                writeCntr++;
                                                writer.Write(writeCntr);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.timeInMs_u32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_raw_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_filtered_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.forceVel_est_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPosition_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPositionTarget_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servo_current_percent_i16);
                                                writer.Write(", ");
                                                writer.Write(((float)pedalState_ext_read_st.payloadPedalExtendedState_.servo_voltage_0p1V_i16) / 10.0);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.angleSensorOutput_ui16);
                                                writer.Write("\n");
                                            }
                                        }
                                    }




                                    continue;
                                }
                            }








                            // decode into config struct
                            if ((waiting_for_pedal_config[pedalSelected]) && (destBuffLength == sizeof(DAP_config_st)))
                            {

                                // parse byte array as config struct
                                DAP_config_st pedalConfig_read_st = getConfigFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_config_st* v_config = &pedalConfig_read_st;
                                byte* p_config = (byte*)v_config;

                                // payload type check
                                bool check_payload_config_b = false;
                                if (pedalConfig_read_st.payloadHeader_.payloadType == Constants.pedalConfigPayload_type)
                                {
                                    check_payload_config_b = true;
                                }

                                // CRC check
                                bool check_crc_config_b = false;
                                if (Plugin.checksumCalc(p_config, sizeof(payloadHeader) + sizeof(payloadPedalConfig)) == pedalConfig_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_config_b = true;
                                }

                                if ((check_payload_config_b) && check_crc_config_b)
                                {
                                    waiting_for_pedal_config[pedalSelected] = false;
                                    dap_config_st[pedalSelected] = pedalConfig_read_st;
                                    Plugin.PedalConfigRead_b[pedalSelected] = true;
                                    updateTheGuiFromConfig();

                                    continue;
                                }
                                else
                                {
                                    TextBox_debugOutput.Text = "Payload config test 1: " + check_payload_config_b;
                                    TextBox_debugOutput.Text += "Payload config test 2: " + check_crc_config_b;
                                }

                            }


                            // If non known array datatype was received, assume a text message was received and print it
                            // only print debug messages when debug mode is active as it degrades performance
                            if (/*Debug_check.IsChecked == true|| */_serial_monitor_window != null)
                            {
                                byte[] destinationArray_sub = new byte[destBuffLength];
                                Buffer.BlockCopy(destinationArray, 0, destinationArray_sub, 0, destBuffLength);
                                string resultString = Encoding.GetEncoding(28591).GetString(destinationArray_sub);
                                if (_serial_monitor_window != null)
                                {
                                    _serial_monitor_window.TextBox_SerialMonitor.Text += resultString + "\n";
                                    _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
                                }
                                /*
                                TextBox_serialMonitor.Text += resultString + "\n";
                                TextBox_serialMonitor.ScrollToEnd();
                                */
                            }

                            




                            // When only a few messages are received, make the counter greater than N thus every message is printed
                            //if (destBuffLength < 100)
                            //{
                            //    printCtr = 600;
                            //}

                            //if (printCtr++ > 200)
                            //{
                            //    printCtr = 0;
                            //    TextBox_serialMonitor.Text += dataToSend + "\n";
                            //    TextBox_serialMonitor.ScrollToEnd();
                            //}





                        }







                        // copy the last not finished buffer element to begining of next cycles buffer
                        // and determine buffer offset
                        if (indices.Count > 0)
                        {
                            // If at least one crlf was detected, check whether it arrieved at the last bytes
                            int lastElement = indices.Last<int>();
                            int remainingMessageLength = currentBufferLength - (lastElement + stop_char_length);
                            if (remainingMessageLength > 0)
                            {
                                appendedBufferOffset[pedalSelected] = remainingMessageLength;

                                Buffer.BlockCopy(buffer_appended[pedalSelected], lastElement + stop_char_length, buffer_appended[pedalSelected], 0, remainingMessageLength);
                            }
                            else
                            {
                                appendedBufferOffset[pedalSelected] = 0;
                            }
                        }
                        else
                        {
                            appendedBufferOffset[pedalSelected] += receivedLength;
                        }






                        // Stop the stopwatch
                        stopwatch.Stop();

                        // Get the elapsed time
                        TimeSpan elapsedTime = stopwatch.Elapsed;

                        timeCollector[pedalSelected] += elapsedTime.TotalMilliseconds;

                        if (timeCntr[pedalSelected] >= 50)
                        {


                            double avgTime = timeCollector[pedalSelected] / timeCntr[pedalSelected];
                            if (debug_flag)
                            {
                                TextBox_debugOutput.Text = "Serial callback time in ms: " + avgTime.ToString();
                            }
                            timeCntr[pedalSelected] = 0;
                            timeCollector[pedalSelected] = 0;
                        }
                    }

                }
            }
        }



        unsafe public void ConnectToPedal_click(object sender, RoutedEventArgs e)
        {

            Plugin.Settings.connect_flag[indexOfSelectedPedal_u] = 1;
            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedal_type = (byte)indexOfSelectedPedal_u;
            if (ConnectToPedal.IsChecked == false)
            {
                if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == false)
                {
                    try
                    {
                        openSerialAndAddReadCallback(indexOfSelectedPedal_u);
                        TextBox_debugOutput.Text = "Serialport open";
                        ConnectToPedal.IsChecked = true;
                        btn_pedal_connect.Content = "Disconnect From Pedal";

                        // register a callback that is triggered when serial data is received
                        // see https://gist.github.com/mini-emmy/9617732
                        //Plugin._serialPort[indexOfSelectedPedal_u].DataReceived += new SerialDataReceivedEventHandler(sp_DataReceived);

                        System.Threading.Thread.Sleep(100);

                        Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 1;

                    }
                    catch (Exception ex)
                    {
                        TextBox_debugOutput.Text = ex.Message;
                        ConnectToPedal.IsChecked = false;
                    }

                }
                else
                {
                    closeSerialAndStopReadCallback(indexOfSelectedPedal_u);

                    //Plugin._serialPort[indexOfSelectedPedal_u].DataReceived -= sp_DataReceived;

                    ConnectToPedal.IsChecked = false;
                    TextBox_debugOutput.Text = "Serialport already open, close it";
                    Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 0;
                    Plugin.Settings.connect_flag[indexOfSelectedPedal_u] = 0;
                    Plugin.connectSerialPort[indexOfSelectedPedal_u] = false;
                    btn_pedal_connect.Content = "Connect To Pedal";
                }
            }
            else
            {
                ConnectToPedal.IsChecked = false;
                closeSerialAndStopReadCallback(indexOfSelectedPedal_u);
                TextBox_debugOutput.Text = "Serialport close";
                Plugin.connectSerialPort[indexOfSelectedPedal_u] = false;
                Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 0;
                Plugin.Settings.connect_flag[indexOfSelectedPedal_u] = 0;
                btn_pedal_connect.Content = "Connect To Pedal";

            }

            ////reading config from pedal

            if (Plugin.Settings.reading_config==1)
            {
                Reading_config_auto(indexOfSelectedPedal_u);
            }
            updateTheGuiFromConfig();
        }

        public void UpdateSerialPortList_click(object sender, RoutedEventArgs e)
        {
            UpdateSerialPortList_click();
        }

        public void SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection.SelectedValue;
            //Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;


            //try 
            //{
            //    TextBox_debugOutput.Text = "Debug: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];
            //}
            //catch (Exception caughtEx)
            //{
            //    string errorMessage = caughtEx.Message;
            //    TextBox_debugOutput.Text = errorMessage;
            //}

            try
            {
                //if (Plugin.Settings.connect_status[indexOfSelectedPedal_u] == 0)
                if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen == false)
                {
                    Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u] = tmp;
                    Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;
                }
                TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox_debugOutput.Text = errorMessage;
            }
        }

        unsafe private void RestartPedal_click(object sender, RoutedEventArgs e)
        {
            Plugin._serialPort[indexOfSelectedPedal_u].DtrEnable = true;
            Plugin._serialPort[indexOfSelectedPedal_u].RtsEnable = true;
            System.Threading.Thread.Sleep(100);
            Plugin._serialPort[indexOfSelectedPedal_u].DtrEnable = false;
            Plugin._serialPort[indexOfSelectedPedal_u].RtsEnable = false;
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u])
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        // compute checksum
                        DAP_action_st tmp;
                        tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                        tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                        tmp.payloadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
                        tmp.payloadPedalAction_.system_action_u8 = 2; //1=reset pedal position, 2 =restart esp.

                        DAP_action_st* v = &tmp;
                        byte* p = (byte*)v;
                        tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                        int length = sizeof(DAP_action_st);
                        byte[] newBuffer = new byte[length];
                        newBuffer = Plugin.getBytes_Action(tmp);
                        // clear inbuffer 
                        Plugin.ESPsync_serialPort.DiscardInBuffer();

                        // send query command
                        Plugin.ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                    }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        TextBox_debugOutput.Text = errorMessage;
                    }
                }
            }
            
        }

        public void Read_for_slot(object sender, EventArgs e)
        {
            var Button = sender as SHButtonPrimary;
            
            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string content = (string)openFileDialog.FileName;


                    string filePath = openFileDialog.FileName;
                    //TextBox_debugOutput.Text =  Button.Name;
                    //
                    uint i = profile_select;
                    uint j = 0;
                    if (Button.Name == "Reading_clutch")
                    {

                        Plugin.Settings.Pedal_file_string[profile_select,0] = filePath;
                        Label_clutch_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 0];
                        Plugin.Settings.file_enable_check[profile_select, 0] = 1;
                        Clutch_file_check.IsChecked = true;                       
                        j = 0;                        
                    }
                    if (Button.Name == "Reading_brake")
                    {
                        Plugin.Settings.Pedal_file_string[profile_select, 1] = filePath;
                        Label_brake_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 1];
                        Plugin.Settings.file_enable_check[profile_select, 1] = 1;
                        Brake_file_check.IsChecked = true;
                        j = 1;
                    }
                    if (Button.Name == "Reading_gas")
                    {
                        Plugin.Settings.Pedal_file_string[profile_select, 2] = filePath;
                        Label_gas_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 2];
                        Plugin.Settings.file_enable_check[profile_select, 2] = 1;
                        Gas_file_check.IsChecked = true;
                        j = 2;
                    }

                    //write to setting
                    for (int k = 0; k < 8; k++)
                    {
                        if (Effect_status_profile[j, k].IsChecked == true)
                        {
                            Plugin.Settings.Effect_status_prolife[i, j, k] = true;
                        }
                        else
                        {
                            Plugin.Settings.Effect_status_prolife[i, j, k] = false;
                        }

                    }


                }
            }
        }

        public void Clear_slot(object sender, EventArgs e)
        {
            var Button = sender as SHButtonPrimary;
            uint i = profile_select;
            uint j = 0;
            if (Button.Name == "Clear_clutch")
            {

                Plugin.Settings.Pedal_file_string[profile_select, 0] = "";
                Label_clutch_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 0];
                Plugin.Settings.file_enable_check[profile_select, 0] = 0;
                Clutch_file_check.IsChecked = false;
                j = 0;

            }
            if (Button.Name == "Clear_brake")
            {

                Plugin.Settings.Pedal_file_string[profile_select, 1] = "";
                Label_brake_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 1];
                Plugin.Settings.file_enable_check[profile_select, 1] = 0;
                Brake_file_check.IsChecked = false;
                j = 1;

            }
            if (Button.Name == "Clear_gas")
            {

                Plugin.Settings.Pedal_file_string[profile_select, 2] = "";
                Label_gas_file.Content = Plugin.Settings.Pedal_file_string[profile_select, 2];
                Plugin.Settings.file_enable_check[profile_select, 2] = 0;
                Gas_file_check.IsChecked = false;
                j = 2;

            }
            //write to setting
            for (int k = 0; k < 8; k++)
            {
                Plugin.Settings.Effect_status_prolife[i, j, k] = false;
                Effect_status_profile[j, k].IsChecked = false;
            }
            //updateTheGuiFromConfig();
        }
        void Parsefile(uint profile_index)
        {
            // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/deserialization


            // c# code to iterate over all fields of struct and set values from json file
            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                if (Plugin.Settings.file_enable_check[profile_select, pedalIdx] == 1)
                {
                    payloadPedalConfig payloadPedalConfig_fromJson_st = dap_config_st[pedalIdx].payloadPedalConfig_;
                    // Read the entire JSON file
                    string jsonString = File.ReadAllText(Plugin.Settings.Pedal_file_string[profile_index, pedalIdx]);
                    // Parse all of the JSON.
                    //JsonNode forecastNode = JsonNode.Parse(jsonString);
                    dynamic data = JsonConvert.DeserializeObject(jsonString);
                    //var s = default(payloadPedalConfig);
                    Object obj = payloadPedalConfig_fromJson_st;// s;
                    FieldInfo[] fi = payloadPedalConfig_fromJson_st.GetType().GetFields(BindingFlags.Public | BindingFlags.Instance);
                    // Iterate over each field and print its name and value
                    foreach (var field in fi)
                    {

                        if (data["payloadPedalConfig_"][field.Name] != null)
                        //if (forecastNode["payloadPedalConfig_"][field.Name] != null)
                        {
                            try
                            {
                                if (field.FieldType == typeof(float))
                                {
                                    //float value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<float>();
                                    float value = (float)data["payloadPedalConfig_"][field.Name];
                                    field.SetValue(obj, value);
                                }

                                if (field.FieldType == typeof(byte))
                                {
                                    //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                    byte value = (byte)data["payloadPedalConfig_"][field.Name];
                                    field.SetValue(obj, value);
                                }
                                if (field.FieldType == typeof(Int16))
                                {
                                    //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                    Int16 value = (Int16)data["payloadPedalConfig_"][field.Name];
                                    field.SetValue(obj, value);
                                }

                            }
                            catch (Exception)
                            {

                            }

                        }
                    }

                    // set values in global structure
                    dap_config_st[pedalIdx].payloadPedalConfig_ = (payloadPedalConfig)obj;// payloadPedalConfig_fromJson_st;
                    if (dap_config_st[pedalIdx].payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                    {
                        dap_config_st[pedalIdx].payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
                    }
                    if (dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelNoise == 0)
                    {
                        dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelNoise = 5;
                    }
                    dap_config_st[pedalIdx].payloadPedalConfig_.pedal_type = (byte)pedalIdx;

                }
                
            }
            


            updateTheGuiFromConfig();
        }

        private void OpenButton_Click(object sender, EventArgs e)
        {
            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string content = (string)openFileDialog.FileName;
                    TextBox_debugOutput.Text = content;

                    string filePath = openFileDialog.FileName;


                    if (false)
                    {
                        string text1 = System.IO.File.ReadAllText(filePath);
                        DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                        var ms = new MemoryStream(Encoding.UTF8.GetBytes(text1));
                        dap_config_st[indexOfSelectedPedal_u] = (DAP_config_st)deserializer.ReadObject(ms);
                    }
                    else
                    {
                        // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/deserialization


                        // c# code to iterate over all fields of struct and set values from json file

                        // Read the entire JSON file
                        string jsonString = File.ReadAllText(filePath);

                        // Parse all of the JSON.
                        //JsonNode forecastNode = JsonNode.Parse(jsonString);
                        dynamic data = JsonConvert.DeserializeObject(jsonString);



                        payloadPedalConfig payloadPedalConfig_fromJson_st = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_;
                        //var s = default(payloadPedalConfig);
                        Object obj = payloadPedalConfig_fromJson_st;// s;



                        FieldInfo[] fi = payloadPedalConfig_fromJson_st.GetType().GetFields(BindingFlags.Public | BindingFlags.Instance);

                        // Iterate over each field and print its name and value
                        foreach (var field in fi)
                        {

                            if (data["payloadPedalConfig_"][field.Name] != null)
                            //if (forecastNode["payloadPedalConfig_"][field.Name] != null)
                            {
                                try
                                {
                                    if (field.FieldType == typeof(float))
                                    {
                                        //float value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<float>();
                                        float value = (float)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }

                                    if (field.FieldType == typeof(byte))
                                    {
                                        //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                        byte value = (byte)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }

                                    if (field.FieldType == typeof(Int16))
                                    {
                                        //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                        Int16 value = (Int16)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }


                                }
                                catch (Exception)
                                {

                                }

                            }
                        }

                        // set values in global structure
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_ = (payloadPedalConfig)obj;// payloadPedalConfig_fromJson_st;
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.kf_modelNoise == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.kf_modelNoise = 5;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedal_type != indexOfSelectedPedal_u)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedal_type = (byte)indexOfSelectedPedal_u;

                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_a==0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_a = 205;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_b == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_b = 220;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_d == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_d = 60;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_c_horizontal == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_c_horizontal = 215;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_c_vertical == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_c_vertical = 60;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_travel == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_travel = 100;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition < 5)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition = 5;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition > 95)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition = 95;
                        }
                    }

                    updateTheGuiFromConfig();
                    TextBox_debugOutput.Text = "Config new imported!";
                    TextBox2.Text = "Open " + openFileDialog.FileName;
                }
            }

        }

        private void SaveButton_Click(object sender, RoutedEventArgs e)
        {
            using (System.Windows.Forms.SaveFileDialog saveFileDialog = new System.Windows.Forms.SaveFileDialog())
            {
                saveFileDialog.Title = "Datei speichern";
                saveFileDialog.Filter = "Textdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                saveFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                     string fileName = saveFileDialog.FileName;


                    dap_config_st[indexOfSelectedPedal_u].payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;

                    // https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
                    // https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
                    var stream1 = new MemoryStream();
                    //var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
                    //ser.WriteObject(stream1, dap_config_st[indexOfSelectedPedal_u]);


                    // formatted JSON see https://stackoverflow.com/a/38538454
                    var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                    var serializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                    serializer.WriteObject(writer, dap_config_st[indexOfSelectedPedal_u]);
                    writer.Flush();

                    stream1.Position = 0;
                    StreamReader sr = new StreamReader(stream1);
                    string jsonString = sr.ReadToEnd();

                    // Check if file already exists. If yes, delete it.     
                    if (File.Exists(fileName))
                    {
                        File.Delete(fileName);
                    }


                    System.IO.File.WriteAllText(fileName, jsonString);
                    TextBox_debugOutput.Text = "Config new exported!";
                    TextBox2.Text = "Save " + saveFileDialog.FileName;
                    }
            }
        }

        private void Debug_checkbox_Checked(object sender, RoutedEventArgs e)
        {


            debug_flag = true;
            if (Plugin != null)
            {
                Plugin.Settings.advanced_b = debug_flag;
            }

            btn_test.Visibility = Visibility.Visible;
            
            TextBox_debug_count.Visibility=Visibility.Visible;
            updateTheGuiFromConfig();


        }
        private void Debug_checkbox_Unchecked(object sender, RoutedEventArgs e)
        {
            debug_flag = false;
            if (Plugin != null)
            {
                Plugin.Settings.advanced_b = debug_flag;
            }
            btn_test.Visibility = Visibility.Hidden;
            TextBox_debug_count.Visibility = Visibility.Hidden;
            updateTheGuiFromConfig();
        }







 



        




        private void btn_reset_default_Click(object sender, RoutedEventArgs e)
        {
            DAP_config_set_default(indexOfSelectedPedal_u);
            updateTheGuiFromConfig();
        }

        // RPM effect select


        private void TabControl_file_path(object sender, SelectionChangedEventArgs e)
        {
            profile_select = (uint)ProfileTab.SelectedIndex;
            Plugin.profile_index = profile_select;
            //Profile_change(profile_select);
            //Plugin.Settings.table_selected = (uint)MyTab.SelectedIndex;
            // update the sliders & serial port selection accordingly
            Update_Profile_Checkbox_b = true;
            updateTheGuiFromConfig();
            
        }

        private void file_check_Checked(object sender, RoutedEventArgs e)
        {
            var checkbox = sender as System.Windows.Controls.CheckBox;
            if (checkbox.Name == "Clutch_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 0] = 1;
            }

            if (checkbox.Name == "Brake_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 1] = 1;
            }

            if (checkbox.Name == "Gas_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 2] = 1;
            }
        }

        private void file_check_Unchecked(object sender, RoutedEventArgs e)
        {
            var checkbox = sender as System.Windows.Controls.CheckBox;
            if (checkbox.Name == "Clutch_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 0] = 0;
            }

            if (checkbox.Name == "Brake_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 1] = 0;
            }

            if (checkbox.Name == "Gas_file_check")
            {
                Plugin.Settings.file_enable_check[profile_select, 2] = 0;
            }
        }

        public void Profile_change(uint profile_index)
        {
            profile_select = profile_index;
            ProfileTab.SelectedIndex = (int)profile_index;
            //if (Plugin.Settings.file_enable_check[profile_select])
            Parsefile(profile_index);
            string tmp;
            switch (profile_index)
            {
                case 0:
                    tmp = "A:" +Plugin.Settings.Profile_name[profile_index];
                    break;
                case 1:
                    tmp = "B:" + Plugin.Settings.Profile_name[profile_index];
                    break;
                case 2:
                    tmp = "C:" + Plugin.Settings.Profile_name[profile_index];
                    break;
                case 3:
                    tmp = "D:" + Plugin.Settings.Profile_name[profile_index];
                    break;
                case 4:
                    tmp = "E:" + Plugin.Settings.Profile_name[profile_index];
                    break;
                case 5:
                    tmp = "F:" + Plugin.Settings.Profile_name[profile_index];
                    break;
                default:
                    tmp = "No Profile";
                    break;
            }
            Plugin.current_profile = tmp;
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    if (Plugin.Settings.Effect_status_prolife[profile_select, j, k])
                    {
                        switch (k)
                        {
                            case 0:
                                Plugin.Settings.ABS_enable_flag[j] = 1;
                                break;
                            case 1:
                                Plugin.Settings.RPM_enable_flag[j] = 1;
                                break;
                            case 2:
                                //Plugin.Settings. = 1;
                                break;
                            case 3:
                                Plugin.Settings.G_force_enable_flag[j] = 1;
                                break;
                            case 4:
                                Plugin.Settings.WS_enable_flag[j] = 1;
                                break;
                            case 5:
                                Plugin.Settings.Road_impact_enable_flag[j] = 1;
                                break;
                            case 6:
                                Plugin.Settings.CV1_enable_flag[j] = true;
                                break;
                            case 7:
                                Plugin.Settings.CV2_enable_flag[j] = true;
                                break;
                        }
                    }
                    else
                    {
                        switch (k)
                        {
                            case 0:
                                Plugin.Settings.ABS_enable_flag[j] = 0;
                                break;
                            case 1:
                                Plugin.Settings.RPM_enable_flag[j] = 0;
                                break;
                            case 2:
                                //Plugin.Settings. = 1;
                                break;
                            case 3:
                                Plugin.Settings.G_force_enable_flag[j] = 0;
                                break;
                            case 4:
                                Plugin.Settings.WS_enable_flag[j] = 0;
                                break;
                            case 5:
                                Plugin.Settings.Road_impact_enable_flag[j] = 0;
                                break;
                            case 6:
                                Plugin.Settings.CV1_enable_flag[j] = false;
                                break;
                            case 7:
                                Plugin.Settings.CV2_enable_flag[j] = false;
                                break;
                        }
                    }

                }
            }
            //effect profile change

        }



        private void btn_apply_profile_Click(object sender, RoutedEventArgs e)
        {
            Profile_change((uint)ProfileTab.SelectedIndex);
            Parsefile((uint)ProfileTab.SelectedIndex);
        }

        private void btn_send_profile_Click(object sender, RoutedEventArgs e)
        {
            Sendconfigtopedal_shortcut();
        }

        private void textbox_profile_name_TextChanged(object sender, TextChangedEventArgs e)
        {
            var textbox = sender as System.Windows.Controls.TextBox;
            Plugin.Settings.Profile_name[profile_select]= textbox.Text;
        }

        unsafe private void btn_toast_Click(object sender, RoutedEventArgs e)
        {

            ToastNotification("Debug","Print All parameter in Serial log");
            //Plugin.Rudder_brake_enable_flag = true;
            //TestSlider.SliderValue = TestSlider.SliderValue + 1;
            //Check1.IsChecked = true;
            PrintUnknownStructParameters(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_);


        }
        private void Hyperlink_RequestNavigate(object sender, RequestNavigateEventArgs e)
        {
            // for .NET Core you need to add UseShellExecute = true
            // see https://learn.microsoft.com/dotnet/api/system.diagnostics.processstartinfo.useshellexecute#property-value
            Process.Start(new ProcessStartInfo(e.Uri.AbsoluteUri));
            e.Handled = true;
        }
        






        

        

        private void Tab_main_1_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            updateTheGuiFromConfig();
        }

        public void ESPNow_SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection_ESPNow.SelectedValue;
            //Plugin._serialPort[indexOfSelectedPedal_u].PortName = tmp;


            //try 
            //{
            //    TextBox_debugOutput.Text = "Debug: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];
            //}
            //catch (Exception caughtEx)
            //{
            //    string errorMessage = caughtEx.Message;
            //    TextBox_debugOutput.Text = errorMessage;
            //}

            try
            {
                //if (Plugin.Settings.connect_status[indexOfSelectedPedal_u] == 0)
                if (Plugin.ESPsync_serialPort.IsOpen == false)
                {
                    Plugin.Settings.ESPNow_port = tmp;
                    Plugin.ESPsync_serialPort.PortName = tmp;
                }
                TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.ESPNow_port;

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox_debugOutput.Text = errorMessage;
            }



        }

        private void btn_connect_espnow_port_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin.Sync_esp_connection_flag)
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    if (ESP_host_serial_timer != null)
                    {
                        ESP_host_serial_timer.Stop();
                        ESP_host_serial_timer.Dispose();
                    }
                    Plugin.ESPsync_serialPort.DiscardInBuffer();
                    Plugin.ESPsync_serialPort.DiscardOutBuffer();
                    Plugin.ESPsync_serialPort.Close();
                    Plugin.Sync_esp_connection_flag = false;
                    btn_connect_espnow_port.Content = "Connect";
                    SystemSounds.Beep.Play();
                    Plugin.Settings.Pedal_ESPNow_auto_connect_flag = false;
                    updateTheGuiFromConfig();
                }
            }
            else
            {
                try
                {
                    // serial port settings
                    Plugin.ESPsync_serialPort.Handshake = Handshake.None;
                    Plugin.ESPsync_serialPort.Parity = Parity.None;
                    //_serialPort[pedalIdx].StopBits = StopBits.None;
                    Plugin.ESPsync_serialPort.BaudRate = Bridge_baudrate;

                    Plugin.ESPsync_serialPort.ReadTimeout = 2000;
                    Plugin.ESPsync_serialPort.WriteTimeout = 500;

                    // https://stackoverflow.com/questions/7178655/serialport-encoding-how-do-i-get-8-bit-ascii
                    Plugin.ESPsync_serialPort.Encoding = System.Text.Encoding.GetEncoding(28591);
                    Plugin.ESPsync_serialPort.NewLine = "\r\n";
                    Plugin.ESPsync_serialPort.ReadBufferSize = 40960;
                    if (Plugin.PortExists(Plugin.ESPsync_serialPort.PortName))
                    {
                        try
                        {
                            Plugin.ESPsync_serialPort.Open();
                            System.Threading.Thread.Sleep(200);
                            // ESP32 S3
                            if (Plugin.Settings.Using_CDC_bridge)
                            {
                                Plugin.ESPsync_serialPort.RtsEnable = false;
                                Plugin.ESPsync_serialPort.DtrEnable = true;
                            }
                            Plugin.ESPsync_serialPort.RtsEnable = false;
                            Plugin.ESPsync_serialPort.DtrEnable = false;

                            SystemSounds.Beep.Play();
                            Plugin.Sync_esp_connection_flag = true;
                            btn_connect_espnow_port.Content = "Disconnect";
                            //Plugin.Settings.connect_status[3] = 1;
                            // read callback
                            /*
                            if (pedal_serial_read_timer[3] != null)
                            {
                                pedal_serial_read_timer[3].Stop();
                                pedal_serial_read_timer[3].Dispose();
                            }
                            */
                            ESP_host_serial_timer = new System.Windows.Forms.Timer();
                            ESP_host_serial_timer.Tick += new EventHandler(timerCallback_serial_esphost);
                            ESP_host_serial_timer.Tag = 3;
                            ESP_host_serial_timer.Interval = 8; // in miliseconds
                            ESP_host_serial_timer.Start();
                            System.Threading.Thread.Sleep(100);
                            if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                            {
                                Plugin.Settings.ESPNow_port = Plugin.ESPsync_serialPort.PortName;
                            }
                            updateTheGuiFromConfig();
                            
                            
                        }
                        catch (Exception ex)
                        {
                            TextBox2.Text = ex.Message;
                            //Serial_connect_status[3] = false;
                        }


                    }
                    else
                    {
                        //Plugin.Settings.connect_status[pedalIdx] = 0;
                        //Plugin.connectSerialPort[pedalIdx] = false;
                        //Serial_connect_status[pedalIdx] = false;

                    }
                }
                catch (Exception ex)
                {
                    TextBox2.Text = ex.Message;
                }
            }
            
        }



        private void CheckBox_Pedal_ESPNow_SyncFlag_Checked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u] = true;
            }
        }

        private void CheckBox_Pedal_ESPNow_SyncFlag_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u] = false;
            }
        }


        unsafe public void timerCallback_serial_esphost(object sender, EventArgs e)
        {

            //action here 
            Simhub_action_update();




            //int pedalSelected = Int32.Parse((sender as System.Windows.Forms.Timer).Tag.ToString());
            //int pedalSelected = (int)(sender as System.Windows.Forms.Timer).Tag;

            bool pedalStateHasAlreadyBeenUpdated_b = false;
            if (Plugin.Settings.Serial_auto_clean_bridge)
            {
                if (TextBox_serialMonitor_bridge.LineCount > 100)
                {
                    TextBox_serialMonitor_bridge.Clear();
                }
                /*
                if (TextBox_serialMonitor.LineCount > 100)
                {
                    TextBox_serialMonitor.Clear();
                }*/
                if (_serial_monitor_window != null && _serial_monitor_window.TextBox_SerialMonitor.LineCount>100)
                {
                    _serial_monitor_window.TextBox_SerialMonitor.Clear();
                }
            }
            try 
            {
                // Create a Stopwatch instance
                Stopwatch stopwatch = new Stopwatch();
                // Start the stopwatch
                stopwatch.Start();
                SerialPort sp = Plugin.ESPsync_serialPort;
                // https://stackoverflow.com/questions/9732709/the-calling-thread-cannot-access-this-object-because-a-different-thread-owns-it
                if (sp.IsOpen)
                {
                    int receivedLength = 0;
                    try
                    {
                        receivedLength = sp.BytesToRead;
                    }
                    catch (Exception ex)
                    {
                        TextBox_debugOutput.Text = ex.Message;
                        //ConnectToPedal.IsChecked = false;
                        return;
                    }

                    if (receivedLength > 0)
                    {

                        //TextBox_serialMonitor.Text += "Received:" + receivedLength + "\n";
                        //TextBox_serialMonitor.ScrollToEnd();


                        timeCntr[3] += 1;


                        // determine byte sequence which is defined as message end --> crlf
                        byte[] byteToFind = System.Text.Encoding.GetEncoding(28591).GetBytes(STOPCHAR[0].ToCharArray());
                        int stop_char_length = byteToFind.Length;


                        // calculate current buffer length
                        int currentBufferLength = appendedBufferOffset[3] + receivedLength;


                        // check if buffer is large enough otherwise discard in buffer and set offset to 0
                        if ((bufferSize > currentBufferLength) && (appendedBufferOffset[3] >= 0))
                        {
                            sp.Read(buffer_appended[3], appendedBufferOffset[3], receivedLength);
                        }
                        else
                        {
                            sp.DiscardInBuffer();
                            appendedBufferOffset[3] = 0;
                            return;
                        }
                        // copy to local buffer
                        //byte[] localBuffer = new byte[currentBufferLength];

                        //Buffer.BlockCopy(buffer_appended[pedalSelected], 0, localBuffer, 0, currentBufferLength);


                        // find all occurences of crlf as they indicate message end
                        List<int> indices = FindAllOccurrences(buffer_appended[3], byteToFind, currentBufferLength);
                        // Destination array
                        byte[] destinationArray = new byte[destBufferSize];
                        int srcBufferOffset = 0;
                        // decode every message
                        //foreach (int number in indices)
                        for (int msgId = 0; msgId < indices.Count; msgId++)
                        {
                            // computes the length of bytes to read
                            int destBuffLength = 0; //number - srcBufferOffset;

                            if (msgId == 0)
                            {
                                srcBufferOffset = 0;
                                destBuffLength = indices.ElementAt(msgId);
                            }
                            else
                            {
                                srcBufferOffset = indices.ElementAt(msgId - 1) + stop_char_length;
                                destBuffLength = indices.ElementAt(msgId) - srcBufferOffset;
                            }

                            // check if dest buffer length is within valid length
                            if ((destBuffLength <= 0) | (destBuffLength > destBufferSize))
                            {
                                continue;
                            }
                            // copy bytes to subarray
                            Buffer.BlockCopy(buffer_appended[3], srcBufferOffset, destinationArray, 0, destBuffLength);
                            // check for pedal state struct
                            if ((destBuffLength == sizeof(DAP_state_basic_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_basic_st pedalState_read_st = getStateFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_basic_st* v_state = &pedalState_read_st;
                                byte* p_state = (byte*)v_state;
                                UInt16 pedalSelected = pedalState_read_st.payloadHeader_.PedalTag;
                                // payload type check
                                bool check_payload_state_b = false;
                                if (pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                {
                                    check_payload_state_b = true;
                                }
                                Pedal_version[pedalSelected] = pedalState_read_st.payloadHeader_.version;
                                if (Pedal_version[pedalSelected] != Constants.pedalConfigPayload_version && pedalState_read_st.payloadHeader_.payloadType== Constants.pedalStateBasicPayload_type)
                                {
                                    if (!Version_warning_first_show_b[pedalSelected])
                                    {
                                        Version_warning_first_show_b[pedalSelected] = true;
                                        if (Pedal_version[pedalSelected] > Constants.pedalConfigPayload_version)
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Simhub Plugin.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                        else
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Pedal Firmware.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning); 
                                        }
                                    }
                                }
                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Basic)) == pedalState_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {

                                    // write vJoy data
                                    Pedal_position_reading[pedalSelected] = pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16;
                                    // GUI update
                                    if (pedalState_read_st.payloadPedalBasicState_.error_code_u8 != 0)
                                    {
                                        Plugin.PedalErrorCode = pedalState_read_st.payloadPedalBasicState_.error_code_u8;
                                        Plugin.PedalErrorIndex = pedalState_read_st.payloadHeader_.PedalTag;
                                        string errorcodetext = "";
                                        //errorcode
                                        switch (Plugin.PedalErrorCode)
                                        {
                                            case 12:
                                                errorcodetext = "Servo Lost connection";
                                                break;
                                            case 101:
                                                errorcodetext = "Config payload type error";
                                                break;
                                            case 102:
                                                errorcodetext = "Config version error";
                                                break;
                                            case 103:
                                                errorcodetext = "Config CRC error";
                                                break;
                                            case 111:
                                                errorcodetext = "Action payload type error";
                                                break;
                                            case 122:
                                                errorcodetext = "Action version error";
                                                break;
                                            case 123:
                                                errorcodetext = "Action CRC error";
                                                break;
                                        }
                                        string temp_str= "Pedal:" + pedalState_read_st.payloadHeader_.PedalTag + " E:" + pedalState_read_st.payloadPedalBasicState_.error_code_u8 + "-" + errorcodetext;
                                        //TextBox2.Text = temp_str;
                                        SimHub.Logging.Current.Info("DIYFFBPedal "+temp_str);
                                        TextBox_serialMonitor_bridge.Text = temp_str;

                                    }
                                    if ((pedalStateHasAlreadyBeenUpdated_b == false) && (indexOfSelectedPedal_u == pedalSelected))
                                    {
                                        pedalStateHasAlreadyBeenUpdated_b = true;
                                        PedalForceTravel_Tab.updatePedalState(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16, pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);
                                        double control_rect_value_max = 65535;

                                        if (debug_flag)
                                        {
                                            int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                            int x_showed = round_x + 1;

                                            current_pedal_travel_state = x_showed;
                                            Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;
                                        }
                                        else
                                        {
                                            int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                            int x_showed = round_x + 1;
                                            round_x = Math.Max(0, Math.Min(round_x, 99));
                                            current_pedal_travel_state = x_showed;
                                            Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;
                                        }

                                    }
                                    for (int i = 0; i < 3; i++)
                                    {
                                        PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                    }

                                    continue;
                                }

                            }
                            // check for pedal extended state struct
                            if ((destBuffLength == sizeof(DAP_state_extended_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_extended_st pedalState_ext_read_st = getStateExtFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_extended_st* v_state = &pedalState_ext_read_st;
                                byte* p_state = (byte*)v_state;
                                UInt16 pedalSelected = pedalState_ext_read_st.payloadHeader_.PedalTag;
                                // payload type check
                                bool check_payload_state_b = false;
                                if (pedalState_ext_read_st.payloadHeader_.payloadType == Constants.pedalStateExtendedPayload_type)
                                {
                                    check_payload_state_b = true;
                                }

                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Extended)) == pedalState_ext_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {
                                    //if (indexOfSelectedPedal_u == pedalSelected)
                                    {
                                        if (Plugin._calculaitons.dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                        {
                                            // Specify the path to the file
                                            string currentDirectory = Directory.GetCurrentDirectory();
                                            string filePath = currentDirectory + "\\PluginsData\\Common" + "\\DiyFfbPedalStateLog_" + pedalSelected + ".txt";

                                            // delete file 
                                            if (true == dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u])
                                            {
                                                dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u] = false;
                                                File.Delete(filePath);
                                            }

                                            // write header
                                            if (!File.Exists(filePath))
                                            {
                                                using (StreamWriter writer = new StreamWriter(filePath, true))
                                                {
                                                    // Write the content to the file
                                                    writer.Write("cycleCtr, ");
                                                    writer.Write("time_InMs, ");
                                                    writer.Write("forceRaw_InKg, ");
                                                    writer.Write("forceFiltered_InKg, ");
                                                    writer.Write("forceVelocity_InKgPerSec, ");
                                                    writer.Write("servoPos_InSteps, ");
                                                    writer.Write("servoPosEsp_InSteps, ");
                                                    writer.Write("servoCurrent_InPercent, ");
                                                    writer.Write("servoVoltage_InV");
                                                    writer.Write("\n");
                                                }

                                            }
                                            // Use StreamWriter to write to the file
                                            using (StreamWriter writer = new StreamWriter(filePath, true))
                                            {
                                                // Write the content to the file
                                                writeCntr++;
                                                writer.Write(writeCntr);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.timeInMs_u32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_raw_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_filtered_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.forceVel_est_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPosition_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPositionTarget_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servo_current_percent_i16);
                                                writer.Write(", ");
                                                writer.Write(((float)pedalState_ext_read_st.payloadPedalExtendedState_.servo_voltage_0p1V_i16) / 10.0);
                                                writer.Write("\n");
                                            }
                                        }
                                    }
                                    continue;
                                }
                            }


                            if ((destBuffLength == sizeof(DAP_bridge_state_st)))
                            {

                                // parse byte array as config struct
                                DAP_bridge_state_st bridge_state = getStateBridgeFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_bridge_state_st* v_state = &bridge_state;
                                byte* p_state = (byte*)v_state;

                                // payload type check
                                bool check_payload_state_b = false;
                                if (bridge_state.payLoadHeader_.payloadType == Constants.bridgeStatePayloadType)
                                {
                                    check_payload_state_b = true;
                                }

                                if (bridge_state.payLoadHeader_.version != Constants.pedalConfigPayload_version && bridge_state.payLoadHeader_.payloadType== Constants.bridgeStatePayloadType)
                                {
                                    if (!Version_warning_first_show_b_bridge)
                                    {
                                        Version_warning_first_show_b_bridge = true;
                                        if (bridge_state.payLoadHeader_.version > Constants.pedalConfigPayload_version)
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Bridge Dap version: " + bridge_state.payLoadHeader_.version + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Simhub Plugin.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                        else
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Bridge Dap version: " + bridge_state.payLoadHeader_.version + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Bridge Firmware.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning); 
                                        }
                                    }
                                }
                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadBridgeState)) == bridge_state.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {
                                    Bridge_RSSI = bridge_state.payloadBridgeState_.Pedal_RSSI;
                                    Label_RSSI.Content = "" + (Bridge_RSSI - 100) + "dBm";
                                    if (Bridge_RSSI < 25)
                                    {
                                        RSSI_1.Visibility = Visibility.Visible;
                                        RSSI_1.Fill = redcolor;
                                        RSSI_2.Visibility = Visibility.Hidden;
                                        RSSI_3.Visibility = Visibility.Hidden;
                                        RSSI_4.Visibility = Visibility.Hidden;
                                    }
                                    if (Bridge_RSSI > 25 && Bridge_RSSI < 30)
                                    {
                                        RSSI_1.Visibility = Visibility.Visible;
                                        RSSI_1.Fill = color_RSSI_1;
                                        RSSI_2.Visibility = Visibility.Visible;

                                        RSSI_3.Visibility = Visibility.Hidden;
                                        RSSI_4.Visibility = Visibility.Hidden;
                                    }
                                    if (Bridge_RSSI > 30 && Bridge_RSSI < 35)
                                    {
                                        RSSI_1.Visibility = Visibility.Visible;
                                        RSSI_1.Fill = color_RSSI_1;
                                        RSSI_2.Visibility = Visibility.Visible;

                                        RSSI_3.Visibility = Visibility.Visible;
                                        RSSI_4.Visibility = Visibility.Hidden;
                                    }
                                    if (Bridge_RSSI > 35)
                                    {
                                        RSSI_1.Visibility = Visibility.Visible;
                                        RSSI_1.Fill = defaultcolor;
                                        RSSI_2.Visibility = Visibility.Visible;
                                        RSSI_3.Visibility = Visibility.Visible;
                                        RSSI_4.Visibility = Visibility.Visible;
                                    }
                                    if (debug_flag)
                                    {
                                        Label_RSSI.Visibility = Visibility.Visible;
                                    }
                                    else
                                    {
                                        Label_RSSI.Visibility = Visibility.Hidden;
                                    }

                                    dap_bridge_state_st.payloadBridgeState_.Pedal_RSSI = bridge_state.payloadBridgeState_.Pedal_RSSI;
                                    string connection_tmp = "";
                                    bool wireless_connection_update = false;
                                    if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 != bridge_state.payloadBridgeState_.Pedal_availability_0)
                                    {

                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 0)
                                        {
                                            //ToastNotification("Wireless Clutch", "Connected");
                                            connection_tmp += "Clutch Connected";
                                            wireless_connection_update = true;
                                            Pedal_wireless_connection_update_b[0] = true;

                                        }
                                        else
                                        {
                                            ///ToastNotification("Wireless Clutch", "Disconnected");
                                            connection_tmp += "Clutch Disconnected";
                                            wireless_connection_update = true;
                                            Plugin.PedalConfigRead_b[0] = false;
                                        }
                                        dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 = bridge_state.payloadBridgeState_.Pedal_availability_0;
                                        //updateTheGuiFromConfig();
                                    }


                                    if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 != bridge_state.payloadBridgeState_.Pedal_availability_1)
                                    {

                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 0)
                                        {
                                            //ToastNotification("Wireless Brake", "Connected");
                                            connection_tmp += " Brake Connected";
                                            wireless_connection_update = true;
                                            Pedal_wireless_connection_update_b[1] = true;


                                        }
                                        else
                                        {
                                            //ToastNotification("Wireless Brake", "Disconnected");
                                            connection_tmp += " Brake Disconnected";
                                            wireless_connection_update = true;
                                            Plugin.PedalConfigRead_b[1] = false;
                                        }
                                        dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 = bridge_state.payloadBridgeState_.Pedal_availability_1;
                                        //updateTheGuiFromConfig();
                                    }

                                    if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 != bridge_state.payloadBridgeState_.Pedal_availability_2)
                                    {

                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 0)
                                        {
                                            //ToastNotification("Wireless Throttle", "Connected");
                                            connection_tmp += " Throttle Connected";
                                            wireless_connection_update = true;
                                            Pedal_wireless_connection_update_b[2] = true;

                                        }
                                        else
                                        {
                                            //ToastNotification("Wireless Throttle", "Disconnected");
                                            connection_tmp += " Throttle Disconnected";
                                            wireless_connection_update = true;
                                            Plugin.PedalConfigRead_b[2] = false;
                                        }
                                        dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 = bridge_state.payloadBridgeState_.Pedal_availability_2;

                                    }

                                    //Pedal availability status update
                                    int PedalAvailabilityCheck = dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 + dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 + dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2;
                                    if (PedalAvailabilityCheck == 3)
                                    {
                                        Pedal_connect_status = (byte)PedalAvailability.ThreePedalConnect;
                                    }

                                    if (PedalAvailabilityCheck == 2)
                                    {
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectClutchBrake;
                                        }
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectClutchThrottle;
                                        }
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectBrakeThrottle;
                                        }
                                    }

                                    if (PedalAvailabilityCheck == 1)
                                    {
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.SinglePedalClutch;
                                        }
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.SinglePedalBrake;
                                        }
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.SinglePedalThrottle;
                                        }
                                    }

                                    if (wireless_connection_update)
                                    {
                                        ToastNotification("Wireless Connection", connection_tmp);
                                        updateTheGuiFromConfig();
                                        wireless_connection_update = false;
                                    }
                                    //fill the version info
                                    for (int i = 0; i < 3; i++)
                                    {
                                        dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[i] = bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                    }
                                    


                                    //updateTheGuiFromConfig();
                                    continue;
                                }
                            }
                            // decode into config struct

                            if (destBuffLength == sizeof(DAP_config_st))
                            {

                                // parse byte array as config struct
                                DAP_config_st pedalConfig_read_st = getConfigFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_config_st* v_config = &pedalConfig_read_st;
                                byte* p_config = (byte*)v_config;

                                // payload type check
                                bool check_payload_config_b = false;
                                if (pedalConfig_read_st.payloadHeader_.payloadType == Constants.pedalConfigPayload_type)
                                {
                                    check_payload_config_b = true;
                                }

                                // CRC check
                                bool check_crc_config_b = false;
                                if (Plugin.checksumCalc(p_config, sizeof(payloadHeader) + sizeof(payloadPedalConfig)) == pedalConfig_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_config_b = true;
                                }
                                UInt16 pedalSelected = pedalConfig_read_st.payloadHeader_.PedalTag;
                                if (waiting_for_pedal_config[pedalSelected])
                                {
                                    if ((check_payload_config_b) && check_crc_config_b)
                                    {
                                        waiting_for_pedal_config[pedalSelected] = false;
                                        dap_config_st[pedalSelected] = pedalConfig_read_st;
                                        updateTheGuiFromConfig();
                                        Plugin.PedalConfigRead_b[pedalSelected] = true;
                                        continue;
                                    }
                                    else
                                    {
                                        TextBox_debugOutput.Text = "Payload config test 1: " + check_payload_config_b;
                                        TextBox_debugOutput.Text += "Payload config test 2: " + check_crc_config_b;
                                    }
                                }
                            }
                            // If non known array datatype was received, assume a text message was received and print it
                            // only print debug messages when debug mode is active as it degrades performance
                            if (Debug_check.IsChecked == true||_serial_monitor_window != null)
                            {
                                byte[] destinationArray_sub = new byte[destBuffLength];
                                Buffer.BlockCopy(destinationArray, 0, destinationArray_sub, 0, destBuffLength);
                                string resultString = Encoding.GetEncoding(28591).GetString(destinationArray_sub);
                                if (resultString.Length > 3)
                                {
                                    string str_chk = resultString.Substring(0, 3);
                                    if (String.Equals(str_chk, "[L]"))
                                    {
                                        string temp = resultString.Substring(3, resultString.Length - 3);
                                        //TextBox_serialMonitor.Text += str_chk + "\n";
                                        TextBox_serialMonitor_bridge.Text += temp + "\n";
                                        //TextBox_serialMonitor.Text += temp + "\n";
                                        if (_serial_monitor_window != null)
                                        {
                                            _serial_monitor_window.TextBox_SerialMonitor.Text += temp + "\n";
                                        }
                                        TextBox_serialMonitor_bridge.ScrollToEnd();

                                        SimHub.Logging.Current.Info(temp);
                                    }
                                    if ( String.Equals(str_chk, "E ("))
                                    {
                                        TextBox_serialMonitor_bridge.Text += resultString + "\n";
                                        //TextBox_serialMonitor.Text += resultString + "\n";
                                        SimHub.Logging.Current.Info(resultString);
                                        if (_serial_monitor_window != null)
                                        {
                                            _serial_monitor_window.TextBox_SerialMonitor.Text += resultString + "\n";
                                        }
                                        TextBox_serialMonitor_bridge.ScrollToEnd();
                                    }
                                }

                            }
                        }
                        // copy the last not finished buffer element to begining of next cycles buffer
                        // and determine buffer offset
                        if (indices.Count > 0)
                        {
                            // If at least one crlf was detected, check whether it arrieved at the last bytes
                            int lastElement = indices.Last<int>();
                            int remainingMessageLength = currentBufferLength - (lastElement + stop_char_length);
                            if (remainingMessageLength > 0)
                            {
                                appendedBufferOffset[3] = remainingMessageLength;

                                Buffer.BlockCopy(buffer_appended[3], lastElement + stop_char_length, buffer_appended[3], 0, remainingMessageLength);
                            }
                            else
                            {
                                appendedBufferOffset[3] = 0;
                            }
                        }
                        else
                        {
                            appendedBufferOffset[3] += receivedLength;
                        }
                        // Stop the stopwatch
                        stopwatch.Stop();

                        // Get the elapsed time
                        TimeSpan elapsedTime = stopwatch.Elapsed;

                        timeCollector[3] += elapsedTime.TotalMilliseconds;

                        if (timeCntr[3] >= 50)
                        {


                            double avgTime = timeCollector[3] / timeCntr[3];
                            if (debug_flag)
                            {
                                TextBox_debugOutput.Text = "Serial callback time in ms: " + avgTime.ToString();
                            }
                            timeCntr[3] = 0;
                            timeCollector[3] = 0;
                        }
                    }

                }
            }
            catch (Exception caughtEx)
            {

                string errorMessage = caughtEx.Message;
                TextBox_debug_count.Text += errorMessage;
                SimHub.Logging.Current.Error(errorMessage);
            }
      
        }

        private void CheckBox_Pedal_ESPNow_autoconnect_Checked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            { 
                Plugin.Settings.Pedal_ESPNow_auto_connect_flag = true;
            }
        }

        private void CheckBox_Pedal_ESPNow_autoconnect_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Pedal_ESPNow_auto_connect_flag = false;
            }
        }



        unsafe private void btn_OTA_enable_Click(object sender, RoutedEventArgs e)
        {
            
            Basic_WIfi_info tmp_2;
            int length;
            string SSID = textbox_SSID.Text;
            string PASS = textbox_PASS.Password;
            string MSG_tmp="";
            bool SSID_PASS_check = true;
            if (Checkbox_Force_flash.IsChecked == true)
            {
                tmp_2.wifi_action = 1;
            }
            if (OTAChannel_Sel_1.IsChecked == true)
            {
                tmp_2.mode_select = 1;
            }
            if (OTAChannel_Sel_2.IsChecked == true)
            {
                tmp_2.mode_select = 2;
            }
            if (SSID.Length > 30 || PASS.Length > 30)
            {
                SSID_PASS_check = false;
                
                MSG_tmp = "ERROR! SSID or Password length must less than 30 bytes";
                System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }
            MSG_tmp += "OTA-Pull function only support V4/Gilphilbert board, for V3/Speedcrafter board user, please connect";
            if (indexOfSelectedPedal_u == 0)
            {
                MSG_tmp += "FFBPedalClutch";
            }
            if (indexOfSelectedPedal_u == 1)
            {
                MSG_tmp += "FFBPedalBrake";
            }
            if (indexOfSelectedPedal_u == 2)
            {
                MSG_tmp += "FFBPedalGas";
            }
            MSG_tmp += " wifi hotspot, open 192.168.2.1 in web browser to upload firmware.bin";

            System.Windows.MessageBox.Show(MSG_tmp, "OTA warning", MessageBoxButton.OK, MessageBoxImage.Warning);

            if (SSID_PASS_check)
            {
                tmp_2.SSID_Length = (byte)SSID.Length;
                tmp_2.PASS_Length = (byte)PASS.Length;
                tmp_2.device_ID = (byte)indexOfSelectedPedal_u;
                tmp_2.payload_Type = (Byte)Constants.Basic_Wifi_info_type;

                byte[] array_ssid = Encoding.ASCII.GetBytes(SSID);
                //TextBox_serialMonitor_bridge.Text += "SSID:";
                for (int i = 0; i < SSID.Length; i++)
                {
                    tmp_2.WIFI_SSID[i] = array_ssid[i];
                    //TextBox_serialMonitor_bridge.Text += tmp_2.WIFI_SSID[i] + ",";
                }
                //TextBox_serialMonitor_bridge.Text += "\nPASS:";
                byte[] array_pass = Encoding.ASCII.GetBytes(PASS);
                for (int i = 0; i < PASS.Length; i++)
                {
                    tmp_2.WIFI_PASS[i] = array_pass[i];
                    //TextBox_serialMonitor_bridge.Text += tmp_2.WIFI_PASS[i] + ",";
                }

                Basic_WIfi_info* v_2 = &tmp_2;
                byte* p_2 = (byte*)v_2;
                TextBox_serialMonitor_bridge.Text += "\nwifi info sent\n\r";

                length = sizeof(Basic_WIfi_info);
                TextBox_serialMonitor_bridge.Text += "\nLength:" + length;
                byte[] newBuffer_2 = new byte[length];
                newBuffer_2 = Plugin.getBytes_Basic_Wifi_info(tmp_2);
                if (Plugin.Settings.Pedal_ESPNow_Sync_flag[indexOfSelectedPedal_u])
                {
                    if (Plugin.ESPsync_serialPort.IsOpen)
                    {
                        try
                        {
                            // clear inbuffer 
                            Plugin.ESPsync_serialPort.DiscardInBuffer();

                            // send query command
                            Plugin.ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                        }
                        catch (Exception caughtEx)
                        {
                            string errorMessage = caughtEx.Message;
                            //TextBox_debugOutput.Text = errorMessage;
                            if (_serial_monitor_window != null)
                            {
                                _serial_monitor_window.TextBox_SerialMonitor.Text += errorMessage + "\n";
                                _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
                            }
                            //TextBox_serialMonitor.Text+= errorMessage+"\n";
                        }
                    }
                }
                else
                {
                    if (Plugin._serialPort[indexOfSelectedPedal_u].IsOpen)
                    {
                        try
                        {
                            // clear inbuffer 
                            Plugin._serialPort[indexOfSelectedPedal_u].DiscardInBuffer();

                            // send query command
                            Plugin._serialPort[indexOfSelectedPedal_u].Write(newBuffer_2, 0, newBuffer_2.Length);
                        }
                        catch (Exception caughtEx)
                        {
                            string errorMessage = caughtEx.Message;
                            //TextBox_debugOutput.Text = errorMessage;
                            if (_serial_monitor_window != null)
                            {
                                _serial_monitor_window.TextBox_SerialMonitor.Text += errorMessage + "\n";
                            }
                            //TextBox_serialMonitor.Text += errorMessage + "\n";
                        }
                    }
                }
            }
        }

        unsafe private void btn_Bridge_restart_Click(object sender, RoutedEventArgs e)
        {
            /*
            Plugin.ESPsync_serialPort.DtrEnable = true;
            Plugin.ESPsync_serialPort.RtsEnable = true;
            System.Threading.Thread.Sleep(100);
            Plugin.ESPsync_serialPort.DtrEnable = false;
            Plugin.ESPsync_serialPort.RtsEnable = false;
            */
            //write to bridge
            DAP_bridge_state_st tmp_2;
            int length;
            tmp_2.payLoadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp_2.payLoadHeader_.payloadType = (byte)Constants.bridgeStatePayloadType;
            tmp_2.payLoadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
            tmp_2.payloadBridgeState_.Pedal_RSSI = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_0 = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_1 = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_2 = 0;
            tmp_2.payloadBridgeState_.Bridge_action = 2; //restart bridge
            DAP_bridge_state_st* v_2 = &tmp_2;
            byte* p_2 = (byte*)v_2;
            tmp_2.payloadFooter_.checkSum = Plugin.checksumCalc(p_2, sizeof(payloadHeader) + sizeof(payloadBridgeState));
            length = sizeof(DAP_bridge_state_st);
            byte[] newBuffer_2 = new byte[length];
            newBuffer_2 = Plugin.getBytes_Bridge(tmp_2);
            if (Plugin.ESPsync_serialPort.IsOpen)
            {
                try
                {
                    // clear inbuffer 
                    Plugin.ESPsync_serialPort.DiscardInBuffer();
                    // send query command
                    Plugin.ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    TextBox_debugOutput.Text = errorMessage;
                }
            }
        }
        private void CheckBox_using_CDC_for_bridge_Checked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            { 
                Plugin.Settings.Using_CDC_bridge = true;
            }
        }

        private void CheckBox_using_CDC_for_bridge_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Using_CDC_bridge = false;
            }
        }

        //Rudder initialize procee
        public void DelayCall(int msec, Action fn)
        {
            // Grab the dispatcher from the current executing thread
            Dispatcher d = Dispatcher.CurrentDispatcher;

            // Tasks execute in a thread pool thread
            new System.Threading.Tasks.Task(() =>
            {
                System.Threading.Thread.Sleep(msec);   // delay

                // use the dispatcher to asynchronously invoke the action 
                // back on the original thread
                d.BeginInvoke(fn);
            }).Start();
        }
        private void Rudder_Initialized()
        {

            DelayCall(400, () =>
            {
                Reading_config_auto(Plugin.Rudder_Pedal_idx[0]);//read brk config from pedal
                CurveRudderForce_Tab.text_rudder_log.Text += "Read Config from" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[0]] +"\n";              
            });

            DelayCall(600, () =>
            {
                Reading_config_auto(Plugin.Rudder_Pedal_idx[1]);//read gas config from pedal
                CurveRudderForce_Tab.text_rudder_log.Text += "Read Config from"+ Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[1]] + "\n";               
            });
                //System.Threading.Thread.Sleep(200);
                DelayCall((int)(900), () =>
                {
                    for (uint idx = 0; idx < 2; idx++)
                    {
                        uint i = Plugin.Rudder_Pedal_idx[idx];
                        CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                        //read pedal kinematic
                        CurveRudderForce_Tab.text_rudder_log.Text += "Create Rudder config for Pedal: " + i + "\n";
                        dap_config_st_rudder.payloadPedalConfig_.lengthPedal_a = dap_config_st[i].payloadPedalConfig_.lengthPedal_a;
                        dap_config_st_rudder.payloadPedalConfig_.lengthPedal_b = dap_config_st[i].payloadPedalConfig_.lengthPedal_b;
                        dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_horizontal = dap_config_st[i].payloadPedalConfig_.lengthPedal_c_horizontal;
                        dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_vertical = dap_config_st[i].payloadPedalConfig_.lengthPedal_c_vertical;
                        dap_config_st_rudder.payloadPedalConfig_.lengthPedal_travel = dap_config_st[i].payloadPedalConfig_.lengthPedal_travel;
                        dap_config_st_rudder.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = dap_config_st[i].payloadPedalConfig_.spindlePitch_mmPerRev_u8;
                        dap_config_st_rudder.payloadPedalConfig_.invertLoadcellReading_u8 = dap_config_st[i].payloadPedalConfig_.invertLoadcellReading_u8;
                        dap_config_st_rudder.payloadPedalConfig_.invertMotorDirection_u8 = dap_config_st[i].payloadPedalConfig_.invertMotorDirection_u8;
                        dap_config_st_rudder.payloadPedalConfig_.loadcell_rating = dap_config_st[i].payloadPedalConfig_.loadcell_rating;
                        dap_config_st_rudder.payloadPedalConfig_.stepLossFunctionFlags_u8 = dap_config_st[i].payloadPedalConfig_.stepLossFunctionFlags_u8;
                        //dap_config_st_rudder.payloadPedalConfig_.Simulate_ABS_trigger = 0;
                        dap_config_st_rudder.payloadPedalConfig_.Simulate_ABS_value = dap_config_st[i].payloadPedalConfig_.Simulate_ABS_value;
                        Sendconfig_Rudder(i);
                        System.Threading.Thread.Sleep(200);
                        CurveRudderForce_Tab.text_rudder_log.Text += "Send Rudder config to Pedal: " + i + "\n";
                    }
                });

        }



        unsafe private void btn_rudder_initialize_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin.ESPsync_serialPort.IsOpen)
            {
                
                if (Pedal_connect_status == (byte)PedalAvailability.ThreePedalConnect)
                {
                    Plugin.Rudder_Pedal_idx[0] = 0;
                }
                else
                {
                    Plugin.Rudder_Pedal_idx[0] = 1;
                }
                if (Pedal_connect_status==(byte)PedalAvailability.TwoPedalConnectBrakeThrottle || Pedal_connect_status == (byte)PedalAvailability.ThreePedalConnect)
                {
                    if (Plugin.Rudder_status)
                    {
                        

                        CurveRudderForce_Tab.text_rudder_log.Clear();
                        CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                        Plugin.Rudder_enable_flag = true;
                        //Plugin.Rudder_status = false;
                        CurveRudderForce_Tab.text_rudder_log.Text += "Disabling Rudder\n";
                        btn_rudder_initialize.Content = "Enable Rudder";
                                             
                        DelayCall(300, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                            
                            DAP_config_st tmp = dap_config_st[Plugin.Rudder_Pedal_idx[0]];
                            tmp.payloadHeader_.storeToEeprom = 0;
                            DAP_config_st* v = &tmp;
                            byte* p = (byte*)v;
                            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
                            Plugin.SendConfig(tmp, Plugin.Rudder_Pedal_idx[0]);
                            //Sendconfig(Plugin.Rudder_Pedal_idx[0]);

                            CurveRudderForce_Tab.text_rudder_log.Text += "Send Original config back to" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[0]] +"\n";
                            
                        });
                        DelayCall(600, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                            DAP_config_st tmp = dap_config_st[Plugin.Rudder_Pedal_idx[1]];
                            tmp.payloadHeader_.storeToEeprom = 0;
                            DAP_config_st* v = &tmp;
                            byte* p = (byte*)v;
                            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
                            Plugin.SendConfig(tmp, Plugin.Rudder_Pedal_idx[1]);
                            //Sendconfig(Plugin.Rudder_Pedal_idx[1]);

                            CurveRudderForce_Tab.text_rudder_log.Text += "Send Original config back to"+ Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[1]] + "\n";
                            
                        });
                        DelayCall(1600, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Hidden;
                        });

                    }
                    else
                    {
                        

                        if (Plugin.MSFS_Plugin_Status == false && Plugin.Version_Check_Simhub_MSFS==false)
                        {
                            String MSG_tmp;
                            MSG_tmp = "No MSFS simconnect plugin detected, please install the plugin or update simhub verison above 9.6.0 then try again.";
                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        }

                        CurveRudderForce_Tab.text_rudder_log.Clear();
                        CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                        DelayCall(100, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                            CurveRudderForce_Tab.text_rudder_log.Text += "Initializing Rudder\n";
                        });
                        Rudder_Initialized();
                        DelayCall(1300, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Visible;
                            CurveRudderForce_Tab.text_rudder_log.Text += "Rudder initialized\n";
                            Plugin.Rudder_enable_flag = true;
                            //Plugin.Rudder_status = true;
                            btn_rudder_initialize.Content = "Disable Rudder";
                        });
                        

                    }
                }
                else
                {
                    String MSG_tmp;
                    MSG_tmp = "Pedal didnt connect to Bridge, please connect pedal to via Bridge then try again.";
                    System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                }

            }
            else
            {
                String MSG_tmp;
                MSG_tmp = "No Bridge conneted, please connect to Bridge and try again.";
                System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

            }

            
        }


        private void SHButtonPrimary_Click(object sender, RoutedEventArgs e)
        {
            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string content = (string)openFileDialog.FileName;
                    TextBox_debugOutput.Text = content;

                    string filePath = openFileDialog.FileName;


                    if (false)
                    {
                        string text1 = System.IO.File.ReadAllText(filePath);
                        DataContractJsonSerializer deserializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                        var ms = new MemoryStream(Encoding.UTF8.GetBytes(text1));
                        dap_config_st_rudder = (DAP_config_st)deserializer.ReadObject(ms);
                    }
                    else
                    {
                        // https://learn.microsoft.com/en-us/dotnet/standard/serialization/system-text-json/deserialization


                        // c# code to iterate over all fields of struct and set values from json file

                        // Read the entire JSON file
                        string jsonString = File.ReadAllText(filePath);

                        // Parse all of the JSON.
                        //JsonNode forecastNode = JsonNode.Parse(jsonString);
                        dynamic data = JsonConvert.DeserializeObject(jsonString);



                        payloadPedalConfig payloadPedalConfig_fromJson_st = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_;
                        //var s = default(payloadPedalConfig);
                        Object obj = payloadPedalConfig_fromJson_st;// s;



                        FieldInfo[] fi = payloadPedalConfig_fromJson_st.GetType().GetFields(BindingFlags.Public | BindingFlags.Instance);

                        // Iterate over each field and print its name and value
                        foreach (var field in fi)
                        {

                            if (data["payloadPedalConfig_"][field.Name] != null)
                            //if (forecastNode["payloadPedalConfig_"][field.Name] != null)
                            {
                                try
                                {
                                    if (field.FieldType == typeof(float))
                                    {
                                        //float value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<float>();
                                        float value = (float)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }

                                    if (field.FieldType == typeof(byte))
                                    {
                                        //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                        byte value = (byte)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }

                                    if (field.FieldType == typeof(Int16))
                                    {
                                        //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                                        Int16 value = (Int16)data["payloadPedalConfig_"][field.Name];
                                        field.SetValue(obj, value);
                                    }


                                }
                                catch (Exception)
                                {

                                }

                            }
                        }

                        // set values in global structure
                        dap_config_st_rudder.payloadPedalConfig_ = (payloadPedalConfig)obj;// payloadPedalConfig_fromJson_st;
                        if (dap_config_st_rudder.payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.kf_modelNoise == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.kf_modelNoise = 5;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_a == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_a = 205;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_b == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_b = 220;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_d == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_d = 60;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_horizontal == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_horizontal = 215;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_vertical == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_c_vertical = 60;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_travel == 0)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.lengthPedal_travel = 100;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.pedalStartPosition < 5)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.pedalStartPosition = 5;
                        }
                        if (dap_config_st_rudder.payloadPedalConfig_.pedalEndPosition > 95)
                        {
                            dap_config_st_rudder.payloadPedalConfig_.pedalEndPosition = 95;
                        }
                    }

                    updateTheGuiFromConfig();
                    /*
                    TextBox_debugOutput.Text = "Config new imported!";
                    TextBox2.Text = "Open " + openFileDialog.FileName;
                    */
                }
            }
        }

        

        private void btn_serial_clear_bridge_Click(object sender, RoutedEventArgs e)
        {
            TextBox_serialMonitor_bridge.Clear();
        }

        private void Checkbox_auto_remove_serial_line_bridge_Checked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Serial_auto_clean_bridge = true;
            }
        }

        private void Checkbox_auto_remove_serial_line_bridge_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Plugin != null)
            {
                Plugin.Settings.Serial_auto_clean_bridge = false;
            }
        }

        unsafe private void btn_Bridge_boot_restart_Click(object sender, RoutedEventArgs e)
        {
            DAP_bridge_state_st tmp_2;
            int length;
            tmp_2.payLoadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp_2.payLoadHeader_.payloadType = (byte)Constants.bridgeStatePayloadType;
            tmp_2.payLoadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
            tmp_2.payloadBridgeState_.Pedal_RSSI = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_0 = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_1 = 0;
            tmp_2.payloadBridgeState_.Pedal_availability_2 = 0;
            tmp_2.payloadBridgeState_.Bridge_action = 3; //restart bridge into boot mode
            DAP_bridge_state_st* v_2 = &tmp_2;
            byte* p_2 = (byte*)v_2;
            tmp_2.payloadFooter_.checkSum = Plugin.checksumCalc(p_2, sizeof(payloadHeader) + sizeof(payloadBridgeState));
            length = sizeof(DAP_bridge_state_st);
            byte[] newBuffer_2 = new byte[length];
            newBuffer_2 = Plugin.getBytes_Bridge(tmp_2);
            if (Plugin.ESPsync_serialPort.IsOpen)
            {
                try
                {
                    // clear inbuffer 
                    Plugin.ESPsync_serialPort.DiscardInBuffer();
                    // send query command
                    Plugin.ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    TextBox_debugOutput.Text = errorMessage;
                }
            }
        }

        

        unsafe private void btn_Bridge_OTA_Click(object sender, RoutedEventArgs e)
        {
            Basic_WIfi_info tmp_2;
            int length;
            string SSID = textbox_SSID.Text;
            string PASS = textbox_PASS.Password;
            bool SSID_PASS_check = true;
            if (Checkbox_Force_flash.IsChecked == true)
            {
                tmp_2.wifi_action = 1;
            }
            if (OTAChannel_Sel_1.IsChecked == true)
            {
                tmp_2.mode_select = 1;
            }
            if (OTAChannel_Sel_2.IsChecked == true)
            {
                tmp_2.mode_select = 2;
            }
            if (SSID.Length > 30 || PASS.Length > 30)
            { 
                SSID_PASS_check = false;
                String MSG_tmp;
                MSG_tmp = "ERROR! SSID or Password length larger than 30 bytes";
                System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            }

            if (SSID_PASS_check)
            {
                tmp_2.SSID_Length = (byte)SSID.Length;
                tmp_2.PASS_Length = (byte)PASS.Length;
                tmp_2.device_ID = 99;
                tmp_2.payload_Type = (Byte)Constants.Basic_Wifi_info_type;

                byte[] array_ssid = Encoding.ASCII.GetBytes(SSID);
                //TextBox_serialMonitor_bridge.Text += "SSID:";
                for (int i = 0; i < SSID.Length; i++)
                {
                    tmp_2.WIFI_SSID[i] = array_ssid[i];
                    //TextBox_serialMonitor_bridge.Text += tmp_2.WIFI_SSID[i] + ",";
                }
                //TextBox_serialMonitor_bridge.Text += "\nPASS:";
                byte[] array_pass = Encoding.ASCII.GetBytes(PASS);
                for (int i = 0; i < PASS.Length; i++)
                {
                    tmp_2.WIFI_PASS[i] = array_pass[i];
                    //TextBox_serialMonitor_bridge.Text += tmp_2.WIFI_PASS[i] + ",";
                }

                Basic_WIfi_info* v_2 = &tmp_2;
                byte* p_2 = (byte*)v_2;
                TextBox_serialMonitor_bridge.Text += "\nwifi info sent\n\r";

                length = sizeof(Basic_WIfi_info);
                TextBox_serialMonitor_bridge.Text += "\nLength:" + length;
                byte[] newBuffer_2 = new byte[length];
                newBuffer_2 = Plugin.getBytes_Basic_Wifi_info(tmp_2);
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        // clear inbuffer 
                        Plugin.ESPsync_serialPort.DiscardInBuffer();
                        // send query command
                        Plugin.ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                    }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        TextBox_debugOutput.Text = errorMessage;
                    }
                }
            }
            
        }

        private void textbox_SSID_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (textbox_SSID.Text.Length > 30)
            {
                Label_SSID.Content = "Error! SSID length >30";
            }
            else
            {
                if (Plugin != null)
                { 
                    Plugin.Settings.SSID_string = textbox_SSID.Text;
                }
                Label_SSID.Content = "";
            }
        }


        private void textbox_PASS_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (textbox_PASS.Password.Length > 30)
            {
                Label_PASS.Content = "Error! Password length >30";
            }
            else
            {
                if (Plugin != null)
                {
                    Plugin.Settings.PASS_string = textbox_PASS.Password;
                }
                Label_PASS.Content = "";
            }
        }

        private void Function_Tab_seleciton_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (Function_Tab_seleciton.SelectedIndex==2)
            {
                Update_Profile_Checkbox_b = true;
                updateTheGuiFromConfig();
            }
        }


        private void TabControl_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            if (Plugin != null)
            {
                //Update_CV_textbox = true;
                Plugin._calculaitons.Update_CV1_textbox = true;
                Plugin._calculaitons.Update_CV2_textbox = true;
                updateTheGuiFromConfig();
            }

        }



        private async void btn_OnlineProfile_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBox.Show("Please make sure you already set the correct Pedal kinematics and Pedal start position(min pos)", "Warning", MessageBoxButton.OK, MessageBoxImage.Warning);
            OnlineProfile sideWindow = new OnlineProfile();
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left=screenWidth/2-sideWindow.Width/2;
            sideWindow.Top=screenHeight/2-sideWindow.Height/2;
            if (sideWindow.ShowDialog() == true)
            {

                string jsonUrl = "https://raw.githubusercontent.com/tcfshcrw/FFB_PEDAL_PROFILE/master/Profiles/"+sideWindow.SelectedFileName;

                try
                {
                    DAP_config_st tmp_config;
                    tmp_config = await GetProfileDataAsync(jsonUrl);
                    float travel = (tmp_config.payloadPedalConfig_.pedalEndPosition - tmp_config.payloadPedalConfig_.pedalStartPosition) / 100.0f * (float)tmp_config.payloadPedalConfig_.lengthPedal_travel;
                    byte max_pos= (byte)(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition + (travel / (float)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_travel * 100.0f));
                    if (max_pos > 95)
                    {
                        System.Windows.MessageBox.Show("Pedal max position calculation error(max position out of travel), please adjust Pedal min position.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    }
                    else
                    {
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce = tmp_config.payloadPedalConfig_.maxForce;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce = tmp_config.payloadPedalConfig_.preloadForce;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000 = tmp_config.payloadPedalConfig_.relativeForce_p000;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020 = tmp_config.payloadPedalConfig_.relativeForce_p020;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040 = tmp_config.payloadPedalConfig_.relativeForce_p040;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060 = tmp_config.payloadPedalConfig_.relativeForce_p060;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080 = tmp_config.payloadPedalConfig_.relativeForce_p080;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100 = tmp_config.payloadPedalConfig_.relativeForce_p100;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.dampingPress = tmp_config.payloadPedalConfig_.dampingPress;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.dampingPull = tmp_config.payloadPedalConfig_.dampingPull;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalEndPosition = max_pos;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.MPC_0th_order_gain = tmp_config.payloadPedalConfig_.MPC_0th_order_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.MPC_1st_order_gain = tmp_config.payloadPedalConfig_.MPC_1st_order_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.MPC_2nd_order_gain = tmp_config.payloadPedalConfig_.MPC_2nd_order_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_d_gain = tmp_config.payloadPedalConfig_.PID_d_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_i_gain = tmp_config.payloadPedalConfig_.PID_i_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_p_gain = tmp_config.payloadPedalConfig_.PID_p_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.PID_velocity_feedforward_gain = tmp_config.payloadPedalConfig_.PID_velocity_feedforward_gain;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.control_strategy_b = tmp_config.payloadPedalConfig_.control_strategy_b;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.kf_modelNoise = tmp_config.payloadPedalConfig_.kf_modelNoise;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.kf_modelOrder = tmp_config.payloadPedalConfig_.kf_modelOrder;
                        updateTheGuiFromConfig();
                    }

                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show($"Error loading JSON: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    
                }
            }
        }

        
        private async Task<DAP_config_st> GetProfileDataAsync(string url)
        {
            using (HttpClient client = new HttpClient())
            {
                string jsonString = await client.GetStringAsync(url);
                //return JsonConvert.DeserializeObject<Profile_Online>(jsonString);
                return JsonConvert.DeserializeObject<DAP_config_st>(jsonString);
            }
        }
        private void btn_SerialMonitorWindow_Click(object sender, RoutedEventArgs e)
        {
            if (_serial_monitor_window == null || !_serial_monitor_window.IsVisible)
            {
                if (Pedal_Log_warning_1st_show_b)
                {
                    System.Windows.MessageBox.Show("Please connect Pedal via USB to Simhub to get Logs");
                    Pedal_Log_warning_1st_show_b = false;
                }
                
                _serial_monitor_window = new SerialMonitor_Window(this); // Create a new side window
                double screenWidth = SystemParameters.PrimaryScreenWidth;
                double screenHeight = SystemParameters.PrimaryScreenHeight;
                _serial_monitor_window.Left = screenWidth / 2 - _serial_monitor_window.Width / 2;
                _serial_monitor_window.Top = screenHeight / 2 - _serial_monitor_window.Height / 2;
                _serial_monitor_window.Show(); // Show the side window

            }
        }

        unsafe private void btn_rudder_status_clear_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin.ESPsync_serialPort.IsOpen)
            {
                System.Windows.MessageBox.Show("Clear Rudder status, please also send the config in.");
                try
                {
                    // compute checksum
                    DAP_action_st tmp;
                    tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    
                    tmp.payloadPedalAction_.system_action_u8 = 0; //1=reset pedal position, 2 =restart esp.
                    tmp.payloadPedalAction_.Rudder_action = 2;
                    for (int i = 1; i < 3; i++)
                    {
                        tmp.payloadHeader_.PedalTag = (byte)i;
                        DAP_action_st* v = &tmp;
                        byte* p = (byte*)v;
                        tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                        int length = sizeof(DAP_action_st);
                        byte[] newBuffer = new byte[length];
                        newBuffer = Plugin.getBytes_Action(tmp);
                        // clear inbuffer 
                        Plugin.ESPsync_serialPort.DiscardInBuffer();
                        // send query command
                        Plugin.ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                        System.Threading.Thread.Sleep(200);
                        //SystemSounds.Beep.Play();
                    }
                    Plugin.Rudder_status = false;
                    Plugin.Rudder_brake_status = false;
                    CurveRudderForce_Tab.text_rudder_log.Clear();
                    CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Hidden;
                    btn_rudder_initialize.Content = "Enable Rudder";


                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    TextBox_debugOutput.Text = errorMessage;
                }
            }
        }

        void PrintUnknownStructParameters(object obj)
        {
            if (obj == null) throw new ArgumentNullException(nameof(obj));

            Type type = obj.GetType();
            _serial_monitor_window.TextBox_SerialMonitor.Text += $"Structure: {type.Name}" + "\n";
            _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
            

            // Get and print all fields
            foreach (FieldInfo field in type.GetFields(BindingFlags.Public | BindingFlags.Instance))
            {
                _serial_monitor_window.TextBox_SerialMonitor.Text += $"Field: {field.Name}, Value: {field.GetValue(obj)}" + "\n";
                _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
                
            }

            // Get and print all properties
            foreach (PropertyInfo property in type.GetProperties(BindingFlags.Public | BindingFlags.Instance))
            {
                if (property.CanRead) // Ensure the property is readable
                {
                    _serial_monitor_window.TextBox_SerialMonitor.Text += $"Property: {property.Name}, Value: {property.GetValue(obj)}" + "\n";
                    _serial_monitor_window.TextBox_SerialMonitor.ScrollToEnd();
                    
                }
            }
        }

        unsafe private void btn_PedalBootMode_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBox.Show("Pedal will restart into download mode, only work with V4/V5 control board(This function will support GIlphilbert board V2 in the future, but not V1.2).");
            DAP_action_st tmp;
            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.ESP_BOOT_INTO_DOWNLOAD_MODE;
            tmp.payloadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
            DAP_action_st* v = &tmp;
            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
            Plugin.SendPedalAction(tmp, (byte)indexOfSelectedPedal_u);


        }



        

        private void Tab_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st[indexOfSelectedPedal_u] = e;
                if (Plugin._calculaitons.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculaitons.IsUIRefreshNeeded = false;
                }
                PedalParameterLiveUpdate();
            }
            
        }

        private void Tab_SettingsChanged(object sender, DIYFFBPedalSettings e)
        {
            Plugin.Settings = e;
            updateTheGuiFromConfig();
        }

        private void Tab_CalculationChanged(object sender, CalculationVariables e)
        {
            Plugin._calculaitons = e;
            updateTheGuiFromConfig();
        }

        private void Rudder_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st_rudder = e;
                if (Plugin._calculaitons.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculaitons.IsUIRefreshNeeded = false;
                }
            }
        }
    }
    
}
