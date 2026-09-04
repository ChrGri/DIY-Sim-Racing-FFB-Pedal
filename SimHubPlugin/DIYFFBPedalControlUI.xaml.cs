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
using static DiyFfbPedal.DIY_FFB_Pedal;
using DiyFfbPedal.UIFunction;
using Windows.UI.ViewManagement;
using WoteverLocalization;



// Win 11 install, see https://github.com/jshafer817/vJoy/releases
//using vJoy.Wrapper;



namespace DiyFfbPedal
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
        public DAP_config_st[] dap_config_st = new DAP_config_st[3];
        public DAP_config_st dap_config_st_rudder;


        public DAP_bridge_state_st dap_bridge_state_st;
        public Basic_WIfi_info _basic_wifi_info;
        //private string stringValue;
        public bool[] waiting_for_pedal_config = new bool[3];
        public System.Windows.Forms.Timer[] pedal_serial_read_timer = new System.Windows.Forms.Timer[3];
        public System.Windows.Forms.Timer connect_timer;
        public System.Windows.Forms.Timer ESP_host_serial_timer;
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
        //public byte Bridge_RSSI = 0;
        public bool[] Pedal_wireless_connection_update_b = new bool[3] { false,false,false};
        public int Bridge_baudrate = 3000000;
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
        //public byte[,] PedalFirmwareVersion = new byte[3, 3] { { 0, 0, 0}, { 0, 0, 0 }, { 0, 0, 0 } };
        public bool PedalTabChange = false;
        private bool firstAssignPlugin = true;
        private bool manualDisconnect_b = false;


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

            for (uint i = 0; i < 30; i++)
            {
                _basic_wifi_info.WIFI_PASS[i] = 0;
                _basic_wifi_info.WIFI_SSID[i] = 0;
            }
            InitializeComponent();
            this.Loaded += RootLayout_Loaded;
            this.SizeChanged += RootLayout_SizeChanged;
            InitRudderTelemetryTimer();

            //setting drawing color with Simhub theme workaround
            //SolidColorBrush buttonBackground_ = btn_update.Background as SolidColorBrush;
            SolidColorBrush buttonBackground_ = btn_pedal_connect.Background as SolidColorBrush;
            

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
            this.DataContext = this;
            CheckForUpdateAsync();
        }

        private const double RootScale_DesignWidth_d = 810.0;
        private const double RootScale_DesignHeight_d = 910.0;
        private const double RootScale_MaxScale_d = 1.75;
        private const double RootScale_Deadband_d = 0.005;

        private void RootLayout_Loaded(object sender, RoutedEventArgs e)
        {
            UpdateRootScale();
        }

        private void RootLayout_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            UpdateRootScale();
        }

        private void UpdateRootScale()
        {
            if (ScaleTransform_RootScale == null) return;

            double availableWidth_d = this.ActualWidth;
            double availableHeight_d = this.ActualHeight;
            if (double.IsNaN(availableWidth_d) || availableWidth_d <= 0) return;
            if (double.IsNaN(availableHeight_d) || availableHeight_d <= 0) return;

            double scale_d = availableWidth_d / RootScale_DesignWidth_d;
            double scaleFromHeight_d = availableHeight_d / RootScale_DesignHeight_d;
            if (scaleFromHeight_d < scale_d) scale_d = scaleFromHeight_d;

            if (scale_d > RootScale_MaxScale_d) scale_d = RootScale_MaxScale_d;

            if (scale_d >= 1.0 && Math.Abs(scale_d - ScaleTransform_RootScale.ScaleX) < RootScale_Deadband_d) return;
            if (scale_d <= 1.0) return;
            ScaleTransform_RootScale.ScaleX = scale_d;
            ScaleTransform_RootScale.ScaleY = scale_d;
        }



        



        public DIYFFBPedalControlUI(DIY_FFB_Pedal plugin) : this()
        {
            this.Plugin = plugin;
            if (CurveRudderForce_Tab != null && plugin?.Settings != null) CurveRudderForce_Tab.Settings = plugin.Settings;
            if (RudderDynamics_Tab != null && plugin?.Settings != null) RudderDynamics_Tab.Settings = plugin.Settings;
            plugin.testValue = 1;
            plugin.wpfHandle = this;
            UpdateSerialPortList_click();
            
            indexOfSelectedPedal_u = plugin.Settings.table_selected;
            MyTab.SelectedIndex = (int)indexOfSelectedPedal_u;
            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                DAP_config_set_default(pedalIdx);

            }

            // WICHTIG: Hier abonnieren wir die neuen Batch-Events für den Servo-Tab,
            // damit die UI-Events auch wirklich an die C#-Methoden weitergeleitet werden!
            if (Servo_Tab != null)
            {
                Servo_Tab.ServoModbusBatchReadRequested -= Servo_Tab_ServoModbusBatchReadRequested;
                Servo_Tab.ServoModbusBatchReadRequested += Servo_Tab_ServoModbusBatchReadRequested;
                Servo_Tab.ServoBatchWriteRequested -= Servo_Tab_ServoBatchWriteRequested;
                Servo_Tab.ServoBatchWriteRequested += Servo_Tab_ServoBatchWriteRequested;
                Servo_Tab.FlashToServoRequested -= Servo_Tab_FlashToServoRequested;
                Servo_Tab.FlashToServoRequested += Servo_Tab_FlashToServoRequested;
                Servo_Tab.ResetToFactoryRequested -= Servo_Tab_ResetToFactoryRequested;
                Servo_Tab.ResetToFactoryRequested += Servo_Tab_ResetToFactoryRequested;
            }

            //auto connection with timmer
            if (connect_timer != null)
            {
                connect_timer.Dispose();
                connect_timer.Stop();
            }

            connect_timer = new System.Windows.Forms.Timer();
            connect_timer.Tick += new EventHandler(connection_timmer_tick);
            connect_timer.Interval = 1000; // in miliseconds try connect every 1s
            connect_timer.Start();
            System.Threading.Thread.Sleep(50);
            Plugin.BridgeHidService.OnDataReceived += HidRecieveCallback;
            updateTheGuiFromConfig();
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



        private void SerialPortSelection_DropDownOpened(object sender, EventArgs e)
        {
            // 1. Store the currently selected value, if any.
            var currentSelectedValue = SerialPortSelection.SelectedValue;

            // 2. Your logic to get the updated list of items.
            //    For example, querying for available serial ports.
            var updatedPortList = GetAvailableSerialPorts(); // This is your custom method.
            //Plugin.comportList.Clear();
            //Plugin.comportList = updatedPortList;
            //UpdateSerialPortList_click();

            // 3. Assign the new list to the ComboBox's ItemsSource.
            SerialPortSelection.ItemsSource = updatedPortList;

            // 4. (Optional but recommended) Restore the previous selection 
            //    if it still exists in the new list.
            if (currentSelectedValue != null)
            {
                SerialPortSelection.SelectedValue = currentSelectedValue;
            }
        }

        private void ESPNow_SerialPortSelection_DropDownOpened(object sender, EventArgs e)
        {
            // 1. Store the currently selected value, if any.
            var currentSelectedValue = SerialPortSelection.SelectedValue;

            // 2. Your logic to get the updated list of items.
            //    For example, querying for available serial ports.
            var updatedPortList = GetAvailableSerialPorts(); // This is your custom method.

            //UpdateSerialPortList_click();

            // 3. Assign the new list to the ComboBox's ItemsSource.
            SerialPortSelection_ESPNow.ItemsSource = updatedPortList;

            // 4. (Optional but recommended) Restore the previous selection 
            //    if it still exists in the new list.
            if (currentSelectedValue != null)
            {
                SerialPortSelection_ESPNow.SelectedValue = currentSelectedValue;
            }
        }

        // The method that gets called from the DropDownOpened event
        private List<SerialPortChoice> GetAvailableSerialPorts()
        {
            // This is the list we will return
            var portChoices = new List<SerialPortChoice>();

            // Your logic starts here:
            //string[] comPorts = System.IO.Ports.SerialPort.GetPortNames();

            // After (guaranteed to be unique)
            string[] comPorts = System.IO.Ports.SerialPort.GetPortNames().Distinct().ToArray();

            // 🌟 MODIFIED SECTION STARTS HERE 🌟
            // Use LINQ to sort the COM ports numerically.
            comPorts = comPorts
                .Select(port => new
                {
                    Name = port,
                    // Use Regex to extract the number from the string (e.g., "COM17" -> 17)
                    Number = int.TryParse(
                        System.Text.RegularExpressions.Regex.Match(port, @"\d+").Value,
                        out int num) ? num : int.MaxValue
                })
                // Order by the extracted number
                .OrderBy(p => p.Number)
                // Select just the port name string back
                .Select(p => p.Name)
                .ToArray();
            // 🌟 MODIFIED SECTION ENDS HERE 🌟

            if (comPorts.Length > 0)
            {
                // Use a simple loop, Distinct() is good but GetPortNames()
                // usually doesn't return duplicates anyway.
                foreach (string portName in comPorts)
                {
                    // Get additional details about the port (your helper method)
                    // Example: ComPortHelper.GetVidPidFromComPort(portName) might return
                    // an object with a DeviceName property like "USB-SERIAL CH340".
                    var parseResult = ComPortHelper.GetVidPidFromComPort(portName);

                    // Create a user-friendly display name, e.g., "COM3 USB-SERIAL CH340"
                    string friendlyName = $"{portName} ({parseResult.DeviceName})";

                    // Add the new object to our list.
                    // The first parameter is what the user sees.
                    // The second parameter is the value used by the program.
                    portChoices.Add(new SerialPortChoice(friendlyName, portName));
                }
            }
            else
            {
                // Handle the case where no ports are found
                portChoices.Add(new SerialPortChoice("No ports found", "NA"));
            }

            return portChoices;
        }


        // NOTE: You will also need your ComPortHelper class and any other
        // dependencies like the `Plugin.comportList` if you still need it for other purposes.

        public void SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection.SelectedValue;
            //string tmp_2= Plugin.comportList[SerialPortSelection.SelectedIndex].ComPortName;
            //System.Windows.MessageBox.Show("connect to " + tmp_2);
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
                //TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.selectedComPortNames[indexOfSelectedPedal_u];

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox2.Text = errorMessage;
            }
        }

        public void ESPNow_SerialPortSelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string tmp = (string)SerialPortSelection_ESPNow.SelectedValue;
            try
            {
                //if (Plugin.Settings.connect_status[indexOfSelectedPedal_u] == 0)
                if (Plugin.ESPsync_serialPort.IsOpen == false)
                {
                    Plugin.Settings.ESPNow_port = tmp;
                    Plugin.ESPsync_serialPort.PortName = tmp;
                }
                //TextBox_debugOutput.Text = "COM port selected: " + Plugin.Settings.ESPNow_port;

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                TextBox2.Text = errorMessage;
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
                if (!Plugin.Rudder_status && !Plugin._calculations.Rudder_status)
                {
                    PedalParameterLiveUpdate();
                }
            }
            
        }

        private void Tab_SettingsChanged(object sender, DIYFFBPedalSettings e)
        {
            if (Plugin != null)
            {
                Plugin.Settings = e;
                if (Plugin.Rudder_status || Plugin._calculations.Rudder_status)
                {
                    RudderParameterLiveUpdate();
                }
                else
                {
                    updateTheGuiFromConfig();
                }
            }
        }

        private void Tab_CalculationChanged(object sender, CalculationVariables e)
        {
            if (Plugin != null)
            {
                Plugin._calculations = e;
                updateTheGuiFromConfig();
            }
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
                if (Plugin.Rudder_status || Plugin._calculations.Rudder_status)
                {
                    RudderParameterLiveUpdate();
                }
            }
        }




        private void SystemLicense_Tab_btn_test_Click_event(object sender, EventArgs e)
        {
            //uint hash =Plugin.ConfigService.ConfigHashMap.Fnv1aHash("RudderConfig");
            ToastNotification("Debug", "Print All parameter and available com portin Serial log\n"+"current Plugin version:" + Plugin._calculations.pluginVersionReading[0]);
            //readRudderSettingToConfig();
            //PrintUnknownStructParameters(dap_config_st_rudder.payloadPedalConfig_);
            if (_serial_monitor_window != null)
            {
                //_serial_monitor_window.TextBox_SerialMonitor.Text += "\n\nDefaultConfig Hash:" + hash+"\n";
                PrintUnknownStructParameters(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_);
                PrintUnknownStructParameters(dap_config_st_rudder.payloadPedalConfig_);
                UpdateSerialPortList_click();
                _serial_monitor_window.TextBox_SerialMonitor.Text += "\nCom port count: " + Plugin.comportList.Count;
                foreach (var items in Plugin.comportList)
                {              
                    _serial_monitor_window.TextBox_SerialMonitor.Text += "\ndevice name:" + items.DeviceName + "\nVID:" + items.Vid + " PID:" + items.Pid+"\n";
                }
                
                    
            }
            


        }
		
		

        private void btn_RudderDocs_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/rudder_test/docs/rudder_modes_flight_dynamics.md") { UseShellExecute = true });
            }
            catch { }
        }

        #region Rudder ESP-NOW Latency, Stability & Pedal RSSI Telemetry Monitor
        private struct RudderTelemetrySample
        {
            public DateTime Timestamp;
            public double DelayMs;
            public double ClutchRssi;
            public double BrakeRssi;
            public double ThrottleRssi;
        }

        private readonly List<RudderTelemetrySample> _telemetryHistory = new List<RudderTelemetrySample>();
        private const double TELEMETRY_WINDOW_SECONDS = 5.0;
        private const int MAX_TELEMETRY_POINTS = 600;
        private DateTime _lastLatencyPacketTime = DateTime.MinValue;
        private DateTime _lastSampleHistoryTime = DateTime.MinValue;
        private double _smoothedJitter_ms = 0.0;
        private double _smoothedRate_hz = 0.0;
        private double _prevDelay_ms = 0.0;
        private System.Windows.Threading.DispatcherTimer _rudderTelemetryTimer;

        public void UpdateRudderLatency(byte delay_ms)
        {
            int c = 0, b = 0, t = 0;
            try
            {
                if (Plugin != null && Plugin._calculations != null && Plugin._calculations.rssi != null && Plugin._calculations.rssi.Length >= 3)
                {
                    c = Plugin._calculations.rssi[0];
                    b = Plugin._calculations.rssi[1];
                    t = Plugin._calculations.rssi[2];
                }
            }
            catch { }

            if (Dispatcher.CheckAccess())
            {
                UpdateRudderTelemetryInternal(delay_ms, c, b, t, isPacket: true);
            }
            else
            {
                Dispatcher.BeginInvoke(new Action(() => UpdateRudderTelemetryInternal(delay_ms, c, b, t, isPacket: true)));
            }
        }

        private void InitRudderTelemetryTimer()
        {
            if (_rudderTelemetryTimer != null) return;
            _rudderTelemetryTimer = new System.Windows.Threading.DispatcherTimer();
            _rudderTelemetryTimer.Interval = TimeSpan.FromMilliseconds(50); // 20 Hz periodic refresh
            _rudderTelemetryTimer.Tick += (s, e) =>
            {
                if (Tab_Rudder != null && Tab_Rudder.IsSelected)
                {
                    // If no live packets received in last 100ms, push current RSSI reading to keep plot moving smoothly
                    if ((DateTime.UtcNow - _lastLatencyPacketTime).TotalMilliseconds > 100)
                    {
                        int c = 0, b = 0, t = 0;
                        try
                        {
                            if (Plugin != null && Plugin._calculations != null && Plugin._calculations.rssi != null && Plugin._calculations.rssi.Length >= 3)
                            {
                                c = Plugin._calculations.rssi[0];
                                b = Plugin._calculations.rssi[1];
                                t = Plugin._calculations.rssi[2];
                            }
                        }
                        catch { }
                        UpdateRudderTelemetryInternal(0, c, b, t, isPacket: false);
                    }
                }
            };
            _rudderTelemetryTimer.Start();
        }

        private void UpdateRudderTelemetryInternal(byte delay_ms, int clutchRssi, int brakeRssi, int throttleRssi, bool isPacket)
        {
            if (poly_rudder_latency_trace == null || canvas_rudder_latency_graph == null) return;

            DateTime now = DateTime.UtcNow;
            double d = (double)delay_ms;

            if (isPacket)
            {
                // Packet rate calculation
                if (_lastLatencyPacketTime != DateTime.MinValue)
                {
                    double dtSec = (now - _lastLatencyPacketTime).TotalSeconds;
                    if (dtSec > 0.001 && dtSec < 1.0)
                    {
                        double instRate = 1.0 / dtSec;
                        _smoothedRate_hz = (_smoothedRate_hz == 0.0) ? instRate : (_smoothedRate_hz * 0.9 + instRate * 0.1);
                    }
                }
                _lastLatencyPacketTime = now;

                // Jitter calculation
                double delta = Math.Abs(d - _prevDelay_ms);
                _smoothedJitter_ms = (_smoothedJitter_ms == 0.0) ? delta : (_smoothedJitter_ms * 0.92 + delta * 0.08);
                _prevDelay_ms = d;

                // Update Text Badges
                if (tb_rudder_sync_delay != null)
                {
                    tb_rudder_sync_delay.Text = $"Delay: {d:F0} ms";
                    if (d <= 10.0)
                        tb_rudder_sync_delay.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xFF, 0xCC));
                    else if (d <= 20.0)
                        tb_rudder_sync_delay.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xD7, 0x00));
                    else
                        tb_rudder_sync_delay.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0x55, 0x55));
                }

                if (tb_rudder_sync_rate != null)
                {
                    tb_rudder_sync_rate.Text = $"Rate: {_smoothedRate_hz:F0} Hz";
                }

                if (tb_rudder_sync_jitter != null)
                {
                    tb_rudder_sync_jitter.Text = $"Jitter: \u00B1{_smoothedJitter_ms:F1} ms";
                }
            }
            else
            {
                // Decay rate if packets paused
                if ((now - _lastLatencyPacketTime).TotalSeconds > 1.0)
                {
                    _smoothedRate_hz = Math.Max(0.0, _smoothedRate_hz * 0.85);
                    if (tb_rudder_sync_rate != null) tb_rudder_sync_rate.Text = $"Rate: {_smoothedRate_hz:F0} Hz";
                    if (tb_rudder_sync_delay != null && _smoothedRate_hz < 1.0) tb_rudder_sync_delay.Text = "Delay: -- ms";
                }
            }

            // Update Live RSSI text readouts in legend
            if (tb_rssi_clutch_val != null) tb_rssi_clutch_val.Text = FormatRssiString(clutchRssi);
            if (tb_rssi_brake_val != null) tb_rssi_brake_val.Text = FormatRssiString(brakeRssi);
            if (tb_rssi_throttle_val != null) tb_rssi_throttle_val.Text = FormatRssiString(throttleRssi);

            // Throttle adding history points to ~66Hz (at most once every 15ms) so high packet rate (e.g. 267Hz)
            // does NOT fill or crop the 5-second buffer prematurely!
            double msSinceLastSample = (now - _lastSampleHistoryTime).TotalMilliseconds;
            if (msSinceLastSample >= 15.0 || _telemetryHistory.Count == 0)
            {
                _lastSampleHistoryTime = now;
                _telemetryHistory.Add(new RudderTelemetrySample
                {
                    Timestamp = now,
                    DelayMs = (isPacket ? d : _prevDelay_ms),
                    ClutchRssi = (double)clutchRssi,
                    BrakeRssi = (double)brakeRssi,
                    ThrottleRssi = (double)throttleRssi
                });
            }

            // Purge samples older than TELEMETRY_WINDOW_SECONDS (5.0s)
            DateTime cutoff = now.AddSeconds(-TELEMETRY_WINDOW_SECONDS);
            while (_telemetryHistory.Count > 1 && _telemetryHistory[1].Timestamp < cutoff)
            {
                _telemetryHistory.RemoveAt(0);
            }
            if (_telemetryHistory.Count > MAX_TELEMETRY_POINTS)
            {
                _telemetryHistory.RemoveAt(0);
            }

            // Draw Graphs
            double canvasWidth = canvas_rudder_latency_graph.ActualWidth;
            if (canvasWidth <= 0) canvasWidth = 520;

            double latHeight = canvas_rudder_latency_graph.ActualHeight;
            if (latHeight <= 0) latHeight = 70;

            double rssiHeight = (canvas_rudder_rssi_graph != null && canvas_rudder_rssi_graph.ActualHeight > 0)
                ? canvas_rudder_rssi_graph.ActualHeight : 70;

            const double MAX_DISPLAY_MS = 30.0;
            const double RSSI_MAX = -30.0;
            const double RSSI_MIN = -100.0;

            PointCollection latPoints = new PointCollection();
            PointCollection fillPoints = new PointCollection();
            PointCollection clutchPoints = new PointCollection();
            PointCollection brakePoints = new PointCollection();
            PointCollection throttlePoints = new PointCollection();

            for (int i = 0; i < _telemetryHistory.Count; i++)
            {
                var pt = _telemetryHistory[i];
                double ageSec = (now - pt.Timestamp).TotalSeconds;
                if (ageSec < 0) ageSec = 0;
                // 0s is at canvasWidth (right edge), 5s is at 0 (left edge)
                double x = canvasWidth * (1.0 - (ageSec / TELEMETRY_WINDOW_SECONDS));
                x = Math.Max(0.0, Math.Min(canvasWidth, x));

                // Latency Y coordinate
                double clampedLat = Math.Min(Math.Max(pt.DelayMs, 0.0), MAX_DISPLAY_MS);
                double yLat = latHeight - 5.0 - (clampedLat / MAX_DISPLAY_MS * (latHeight - 12.0));
                Point pLat = new Point(x, yLat);
                latPoints.Add(pLat);

                // RSSI Y coordinates (Clutch Red, Brake Green, Throttle Blue)
                clutchPoints.Add(new Point(x, MapRssiToY(pt.ClutchRssi, rssiHeight, RSSI_MIN, RSSI_MAX)));
                brakePoints.Add(new Point(x, MapRssiToY(pt.BrakeRssi, rssiHeight, RSSI_MIN, RSSI_MAX)));
                throttlePoints.Add(new Point(x, MapRssiToY(pt.ThrottleRssi, rssiHeight, RSSI_MIN, RSSI_MAX)));
            }

            // Form clean fill polygon under the latency curve only where data exists
            if (latPoints.Count > 0)
            {
                fillPoints.Add(new Point(latPoints[0].X, latHeight));
                for (int i = 0; i < latPoints.Count; i++)
                {
                    fillPoints.Add(latPoints[i]);
                }
                fillPoints.Add(new Point(latPoints[latPoints.Count - 1].X, latHeight));
            }

            poly_rudder_latency_trace.Points = latPoints;
            if (poly_rudder_latency_fill != null) poly_rudder_latency_fill.Points = fillPoints;
            if (poly_rssi_clutch != null) poly_rssi_clutch.Points = clutchPoints;
            if (poly_rssi_brake != null) poly_rssi_brake.Points = brakePoints;
            if (poly_rssi_throttle != null) poly_rssi_throttle.Points = throttlePoints;
        }

        private static double MapRssiToY(double val, double h, double minRssi, double maxRssi)
        {
            if (val >= 0 || val < -110.0) val = minRssi; // Disconnected or invalid mapped to bottom
            else if (val > maxRssi) val = maxRssi;
            else if (val < minRssi) val = minRssi;
            double norm = (val - minRssi) / (maxRssi - minRssi); // 0.0 at -100dBm, 1.0 at -30dBm
            return h - 5.0 - (norm * (h - 12.0));
        }

        private static string FormatRssiString(int val)
        {
            if (val < -20 && val > -105) return $"{val}dBm";
            return "--";
        }
        #endregion
    }
}