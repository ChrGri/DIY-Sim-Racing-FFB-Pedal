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
        //public bool debug_flag = false;

        //public VirtualJoystick joystick;
        


        //public bool[] dumpPedalToResponseFile = new bool[3];
        //public bool[] dumpPedalToResponseFile_clearFile = new bool[3];

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
        //private int gridline_kinematic_count_original = 0;
        private double[] Pedal_position_reading=new double[3];
        private bool[] Serial_connect_status = new bool[3] { false,false,false};
        public byte Bridge_RSSI = 0;
        public bool[] Pedal_wireless_connection_update_b = new bool[3] { false,false,false};
        public int Bridge_baudrate = 3000000;
        //public bool Fanatec_mode = false;
        //public bool Update_Profile_Checkbox_b = false;
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
            
        }



        



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

        



        

        Int64 writeCntr = 0;

        int[] timeCntr = { 0, 0, 0,0 };

        double[] timeCollector = { 0, 0, 0,0 };


        
        

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

        
        private void Tab_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st[indexOfSelectedPedal_u] = e;
                if (Plugin._calculations.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculations.IsUIRefreshNeeded = false;
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
            Plugin._calculations = e;
            updateTheGuiFromConfig();
        }

        private void Rudder_ConfigChanged(object sender, DAP_config_st e)
        {
            if (Plugin != null)
            {
                dap_config_st_rudder = e;
                if (Plugin._calculations.IsUIRefreshNeeded)
                {
                    updateTheGuiFromConfig();
                    Plugin._calculations.IsUIRefreshNeeded = false;
                }
            }
        }

        private void SystemProfile_Tab_btn_send_profile_Click_event(object sender, EventArgs e)
        {
            Sendconfigtopedal_shortcut();
            
        }

        private void SystemProfile_Tab_btn_apply_profile_Click_event(object sender, EventArgs e)
        {
            Profile_change((uint)Plugin._calculations.profile_index);
            Parsefile((uint)Plugin._calculations.profile_index);
        }

        private void SystemLicense_Tab_btn_test_Click_event(object sender, EventArgs e)
        {
            ToastNotification("Debug", "Print All parameter in Serial log");
            PrintUnknownStructParameters(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_);
        }
    }
    
}
