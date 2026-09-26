using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Media;
using System.Net.Http;
using System.Reflection;
using System.Runtime.Serialization.Json;
using System.Security.Policy;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Forms;
using User.PluginSdkDemo.UIFunction;
using static User.PluginSdkDemo.DIY_FFB_Pedal;
using MessageBox = System.Windows.MessageBox;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        unsafe public void ReadConfigFromPedal_click(object sender, RoutedEventArgs e)
        {
            Reading_config_auto(indexOfSelectedPedal_u);
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
            tmp_2.payloadBridgeState_.Bridge_action = (byte)bridgeAction.BRIDGE_ACTION_DOWNLOAD_MODE; //restart bridge into boot mode
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
                    TextBox2.Text = errorMessage;
                }
            }
        }



        unsafe private void btn_Bridge_OTA_Click(object sender, RoutedEventArgs e)
        {
            UpdateSettingWindow sideWindow = new UpdateSettingWindow(Plugin.Settings, Plugin._calculations);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {
                Basic_WIfi_info tmp_2;
                int length;
                string SSID = Plugin.Settings.SSID_string;
                string PASS = Plugin.Settings.PASS_string;
                bool SSID_PASS_check = true;

                if (Plugin._calculations.ForceUpdate_b == true)
                {
                    tmp_2.wifi_action = 1;
                }
                if (Plugin.Settings.updateChannel == 0)
                {
                    tmp_2.mode_select = 1;
                }
                if (Plugin.Settings.updateChannel == 1)
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
                    TextBox_serialMonitor_bridge.Text += "\nSending wifi info to Bridge for OTA.\n\r";

                    length = sizeof(Basic_WIfi_info);
                    //TextBox_serialMonitor_bridge.Text += "\nLength:" + length;
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
                            TextBox2.Text = errorMessage;
                        }
                    }
                }
            }
            

        }

        private async void btn_OnlineProfile_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBox.Show("Please make sure you already set the correct Pedal kinematics and Pedal start position(min pos)", "Warning", MessageBoxButton.OK, MessageBoxImage.Warning);
            OnlineProfile sideWindow = new OnlineProfile();
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {

                string jsonUrl = "https://raw.githubusercontent.com/tcfshcrw/FFB_PEDAL_PROFILE/master/Profiles/" + sideWindow.SelectedFileName;

                try
                {
                    bool compatibleMode = false;
                    DAP_config_st tmp_config;
                    int version = 0;
                    byte[] compatibleForce = new byte[6];
                    tmp_config = await GetProfileDataAsync(jsonUrl);
                    if (tmp_config.payloadHeader_.version < 150)
                    {
                        compatibleMode = true;
                        System.Windows.MessageBox.Show($"This config created in DAP{tmp_config.payloadHeader_.version}, compatible mode is on", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

                        try
                        {
                            string jsonString;
                            using (HttpClient client = new HttpClient())
                            {
                                jsonString = await client.GetStringAsync(jsonUrl);
                                //return JsonConvert.DeserializeObject<Profile_Online>(jsonString);
                            }
                            dynamic data = JsonConvert.DeserializeObject(jsonString);
                            version = (int)data["payloadHeader_"]["version"];

                            if (version < 150)
                            {
                                //MessageBox.Show($"This config is created in DAP{version}, Compatible Mode on");
                                compatibleMode = true;
                                compatibleForce[0] = (byte)data["payloadPedalConfig_"]["relativeForce_p000"];
                                compatibleForce[1] = (byte)data["payloadPedalConfig_"]["relativeForce_p020"];
                                compatibleForce[2] = (byte)data["payloadPedalConfig_"]["relativeForce_p040"];
                                compatibleForce[3] = (byte)data["payloadPedalConfig_"]["relativeForce_p060"];
                                compatibleForce[4] = (byte)data["payloadPedalConfig_"]["relativeForce_p080"];
                                compatibleForce[5] = (byte)data["payloadPedalConfig_"]["relativeForce_p100"];
                            }

                        }
                        catch (Exception ex)
                        {

                        }
                    } 
                    float travel = (tmp_config.payloadPedalConfig_.pedalEndPosition - tmp_config.payloadPedalConfig_.pedalStartPosition) / 100.0f * (float)tmp_config.payloadPedalConfig_.lengthPedal_travel;
                    byte max_pos = (byte)(dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.pedalStartPosition + (travel / (float)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_travel * 100.0f));
                    if (max_pos > 95)
                    {
                        System.Windows.MessageBox.Show("Pedal max position calculation error(max position out of travel), please adjust Pedal min position.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                    }
                    else
                    {
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.maxForce = tmp_config.payloadPedalConfig_.maxForce;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.preloadForce = tmp_config.payloadPedalConfig_.preloadForce;
                        /*
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000 = tmp_config.payloadPedalConfig_.relativeForce_p000;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020 = tmp_config.payloadPedalConfig_.relativeForce_p020;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040 = tmp_config.payloadPedalConfig_.relativeForce_p040;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060 = tmp_config.payloadPedalConfig_.relativeForce_p060;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080 = tmp_config.payloadPedalConfig_.relativeForce_p080;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100 = tmp_config.payloadPedalConfig_.relativeForce_p100;
                        */
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
                        if (tmp_config.payloadPedalConfig_.quantityOfControl < 6) tmp_config.payloadPedalConfig_.quantityOfControl = 6;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.quantityOfControl = tmp_config.payloadPedalConfig_.quantityOfControl;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = tmp_config.payloadPedalConfig_.relativeForce00;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = tmp_config.payloadPedalConfig_.relativeForce01;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = tmp_config.payloadPedalConfig_.relativeForce02;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = tmp_config.payloadPedalConfig_.relativeForce03;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = tmp_config.payloadPedalConfig_.relativeForce04;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = tmp_config.payloadPedalConfig_.relativeForce05;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce06 = tmp_config.payloadPedalConfig_.relativeForce06;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce07 = tmp_config.payloadPedalConfig_.relativeForce07;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce08 = tmp_config.payloadPedalConfig_.relativeForce08;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce09 = tmp_config.payloadPedalConfig_.relativeForce09;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce10 = tmp_config.payloadPedalConfig_.relativeForce10;

                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel00 = tmp_config.payloadPedalConfig_.relativeTravel00;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel01 = tmp_config.payloadPedalConfig_.relativeTravel01;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel02 = tmp_config.payloadPedalConfig_.relativeTravel02;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel03 = tmp_config.payloadPedalConfig_.relativeTravel03;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel04 = tmp_config.payloadPedalConfig_.relativeTravel04;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel05 = tmp_config.payloadPedalConfig_.relativeTravel05;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel06 = tmp_config.payloadPedalConfig_.relativeTravel06;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel07 = tmp_config.payloadPedalConfig_.relativeTravel07;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel08 = tmp_config.payloadPedalConfig_.relativeTravel08;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel09 = tmp_config.payloadPedalConfig_.relativeTravel09;
                        dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel10 = tmp_config.payloadPedalConfig_.relativeTravel10;

                        if (compatibleMode)
                        {
                            //get old verison file, auto convert to new config
                            compatibleMode = false;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.quantityOfControl = 6;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = compatibleForce[0];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = compatibleForce[1];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = compatibleForce[2];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = compatibleForce[3];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = compatibleForce[4];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = compatibleForce[5];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel00 = 0;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel01 = 20;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel02 = 40;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel03 = 60;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel04 = 80;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel05 = 100;
                        }
                        updateTheGuiFromConfig();
                    }

                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show($"Error loading JSON: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

                }
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
                        //TextBox_debugOutput.Text = "Serialport open";
                        ConnectToPedal.IsChecked = true;
                        btn_pedal_connect.Content = "Disconnect";

                        // register a callback that is triggered when serial data is received
                        // see https://gist.github.com/mini-emmy/9617732
                        //Plugin._serialPort[indexOfSelectedPedal_u].DataReceived += new SerialDataReceivedEventHandler(sp_DataReceived);

                        System.Threading.Thread.Sleep(100);

                        Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 1;

                    }
                    catch (Exception ex)
                    {
                        TextBox2.Text = ex.Message;
                        ConnectToPedal.IsChecked = false;
                    }

                }
                else
                {
                    closeSerialAndStopReadCallback(indexOfSelectedPedal_u);

                    //Plugin._serialPort[indexOfSelectedPedal_u].DataReceived -= sp_DataReceived;

                    ConnectToPedal.IsChecked = false;
                    TextBox2.Text = "Serialport already open, close it";
                    Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 0;
                    Plugin.Settings.connect_flag[indexOfSelectedPedal_u] = 0;
                    Plugin.connectSerialPort[indexOfSelectedPedal_u] = false;
                    btn_pedal_connect.Content = "Connect";
                }
            }
            else
            {
                ConnectToPedal.IsChecked = false;
                closeSerialAndStopReadCallback(indexOfSelectedPedal_u);
                TextBox2.Text = "Serialport close";
                Plugin.connectSerialPort[indexOfSelectedPedal_u] = false;
                Plugin.Settings.connect_status[indexOfSelectedPedal_u] = 0;
                Plugin.Settings.connect_flag[indexOfSelectedPedal_u] = 0;
                btn_pedal_connect.Content = "Connect";

            }

            ////reading config from pedal

            if (Plugin.Settings.reading_config == 1)
            {
                Reading_config_auto(indexOfSelectedPedal_u);
            }
            updateTheGuiFromConfig();
        }

        public void UpdateSerialPortList_click(object sender, RoutedEventArgs e)
        {
            UpdateSerialPortList_click();
        }

        unsafe private void RestartPedal_click(object sender, RoutedEventArgs e)
        {
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
                        TextBox2.Text = errorMessage;
                    }
                }
            }
            else
            {
                //if (Plugin.Settings.USING_ESP32S3[Plugin.Settings.table_selected])
                //{
                    DAP_action_st tmp;
                    tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    tmp.payloadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
                    tmp.payloadPedalAction_.system_action_u8 = 2; //1=reset pedal position, 2 =restart esp.

                    DAP_action_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    Plugin.SendPedalAction(tmp , (byte)Plugin.Settings.table_selected);
                //}
                //else
                //{
                //    Plugin._serialPort[indexOfSelectedPedal_u].DtrEnable = true;
                //    Plugin._serialPort[indexOfSelectedPedal_u].RtsEnable = true;
                //    System.Threading.Thread.Sleep(100);
                //    Plugin._serialPort[indexOfSelectedPedal_u].DtrEnable = false;
                //    Plugin._serialPort[indexOfSelectedPedal_u].RtsEnable = false;
                //}
            }

        }

        private void OpenButton_Click(object sender, EventArgs e)
        {
            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";
                bool compatibleMode = false;
                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string content = (string)openFileDialog.FileName;
                    //TextBox_debugOutput.Text = content;

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
                        int version = 0;
                        byte[] compatibleForce = new byte[6];
                        try
                        {
                            version = (int)data["payloadHeader_"]["version"];
                            
                            if (version < 150)
                            {
                                MessageBox.Show($"This config is created in DAP{version}, Compatible Mode on");
                                compatibleMode = true;
                                compatibleForce[0]= (byte)data["payloadPedalConfig_"]["relativeForce_p000"];
                                compatibleForce[1] = (byte)data["payloadPedalConfig_"]["relativeForce_p020"];
                                compatibleForce[2] = (byte)data["payloadPedalConfig_"]["relativeForce_p040"];
                                compatibleForce[3] = (byte)data["payloadPedalConfig_"]["relativeForce_p060"];
                                compatibleForce[4] = (byte)data["payloadPedalConfig_"]["relativeForce_p080"];
                                compatibleForce[5] = (byte)data["payloadPedalConfig_"]["relativeForce_p100"];
                            }
                        }
                        catch (Exception ex)
                        {
                            
                        }


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
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_a == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_a = 205;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_b == 0)
                        {
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_b = 220;
                        }
                        if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.lengthPedal_d < 0)
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

                        if (compatibleMode)
                        {
                            //get old verison file, auto convert to new config
                            compatibleMode = false;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.quantityOfControl = 6;
                            /*
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100;
                            */
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = compatibleForce[0];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = compatibleForce[1];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = compatibleForce[2];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = compatibleForce[3];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = compatibleForce[4];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = compatibleForce[5];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel00 = 0;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel01 = 20;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel02 = 40;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel03 = 60;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel04 = 80;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel05 = 100;
                        }
                    }

                    updateTheGuiFromConfig();
                    //TextBox_debugOutput.Text = "Config new imported!";
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
                    //TextBox_debugOutput.Text = "Config new exported!";
                    TextBox2.Text = "Save " + saveFileDialog.FileName;
                }
            }
        }
        private void btn_reset_default_Click(object sender, RoutedEventArgs e)
        {
            DAP_config_set_default(indexOfSelectedPedal_u);
            updateTheGuiFromConfig();
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
                            /*
                            if (Plugin.Settings.Using_CDC_bridge)
                            {
                                Plugin.ESPsync_serialPort.RtsEnable = false;
                                Plugin.ESPsync_serialPort.DtrEnable = true;
                            }
                            */
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

        unsafe private void btn_OTA_enable_Click(object sender, RoutedEventArgs e)
        {
            UpdateSettingWindow sideWindow = new UpdateSettingWindow(Plugin.Settings, Plugin._calculations);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {
                Basic_WIfi_info tmp_2;
                int length;
                string SSID = Plugin.Settings.SSID_string;
                string PASS = Plugin.Settings.PASS_string;
                string MSG_tmp = "";
                bool SSID_PASS_check = true;
                if (Plugin._calculations.ForceUpdate_b == true)
                {
                    tmp_2.wifi_action = 1;
                }
                if (Plugin.Settings.updateChannel == 0)
                {
                    tmp_2.mode_select = 1;
                }
                if (Plugin.Settings.updateChannel == 1)
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
                MSG_tmp += "OTA-Pull function only support V4/V5/Gilphilbert pcba v1.2/pcba v2.0 board.\nAfter enable OTA-Pull, you need wait for 3-5 minutes and Pedal will auto restart after update.\nThe OTA log will showed in serial monitor with USB connection.\nV3/Speedcrafter board user, please connect to ";
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

                //System.Windows.MessageBox.Show(MSG_tmp, "OTA warning", MessageBoxButton.OK, MessageBoxImage.Warning);
                var result = System.Windows.MessageBox.Show(MSG_tmp, "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                if (result == MessageBoxResult.OK)
                {
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
                        TextBox_serialMonitor_bridge.Text += "\nSending WIfi info for OTA to Pedal\n\r";

                        length = sizeof(Basic_WIfi_info);
                        //TextBox_serialMonitor_bridge.Text += "\nLength:" + length;
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
            tmp_2.payloadBridgeState_.Bridge_action = (byte)bridgeAction.BRIDGE_ACTION_RESTART; //restart bridge
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
                    TextBox2.Text = errorMessage;
                }
            }
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
                if (Pedal_connect_status == (byte)PedalAvailability.TwoPedalConnectBrakeThrottle || Pedal_connect_status == (byte)PedalAvailability.ThreePedalConnect)
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

                            CurveRudderForce_Tab.text_rudder_log.Text += "Send Original config back to" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[0]] + "\n";

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

                            CurveRudderForce_Tab.text_rudder_log.Text += "Send Original config back to" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[1]] + "\n";

                        });
                        DelayCall(1600, () =>
                        {
                            CurveRudderForce_Tab.text_rudder_log.Visibility = Visibility.Hidden;
                        });

                    }
                    else
                    {


                        if (Plugin.MSFS_Plugin_Status == false && Plugin.Version_Check_Simhub_MSFS == false)
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


        private void btn_Rudder_load_config_Click(object sender, RoutedEventArgs e)
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
                    //TextBox_debugOutput.Text = content;

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
                        bool compatibleMode = false;
                        int version = 0;
                        byte[] compatibleForce = new byte[6];
                        try
                        {
                            version = (int)data["payloadHeader_"]["version"];

                            if (version < 150)
                            {
                                MessageBox.Show($"This config is created in DAP{version}, Compatible Mode on");
                                compatibleMode = true;
                                compatibleForce[0] = (byte)data["payloadPedalConfig_"]["relativeForce_p000"];
                                compatibleForce[1] = (byte)data["payloadPedalConfig_"]["relativeForce_p020"];
                                compatibleForce[2] = (byte)data["payloadPedalConfig_"]["relativeForce_p040"];
                                compatibleForce[3] = (byte)data["payloadPedalConfig_"]["relativeForce_p060"];
                                compatibleForce[4] = (byte)data["payloadPedalConfig_"]["relativeForce_p080"];
                                compatibleForce[5] = (byte)data["payloadPedalConfig_"]["relativeForce_p100"];
                            }
                        }
                        catch (Exception ex)
                        {

                        }


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
                        if (dap_config_st_rudder.payloadPedalConfig_.lengthPedal_d < 0)
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

                        if (compatibleMode)
                        {
                            //get old verison file, auto convert to new config
                            compatibleMode = false;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.quantityOfControl = 6;
                            /*
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100;
                            */
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = compatibleForce[0];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = compatibleForce[1];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = compatibleForce[2];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = compatibleForce[3];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = compatibleForce[4];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = compatibleForce[5];
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel00 = 0;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel01 = 20;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel02 = 40;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel03 = 60;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel04 = 80;
                            dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeTravel05 = 100;
                        }
                    }
                    writeRudderConfigToSetting();
                    updateTheGuiFromConfig();
                    /*
                    TextBox_debugOutput.Text = "Config new imported!";
                    TextBox2.Text = "Open " + openFileDialog.FileName;
                    */
                }
            }
        }
        unsafe public void SendConfigToPedal_click(object sender, RoutedEventArgs e)
        {
            Sendconfig(indexOfSelectedPedal_u);
        }

        private void btn_serial_clear_bridge_Click(object sender, RoutedEventArgs e)
        {
            TextBox_serialMonitor_bridge.Clear();
        }

        unsafe private void btn_Bridge_print_debug_Click(object sender, RoutedEventArgs e)
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
            tmp_2.payloadBridgeState_.Bridge_action = (byte)bridgeAction.BRIDGE_ACTION_DEBUG; //print out debug message
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
                    TextBox2.Text = errorMessage;
                }
            }
        }
        unsafe private void btn_Bridge_joystick_flashing_Click(object sender, RoutedEventArgs e)
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
            tmp_2.payloadBridgeState_.Bridge_action = (byte)bridgeAction.BRIDGE_ACTION_JOYSTICK_FLASHING_MODE; 
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
                    TextBox2.Text = errorMessage;
                }
            }
        }

        unsafe private void btn_Bridge_joysitck_debug_Click(object sender, RoutedEventArgs e)
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
            tmp_2.payloadBridgeState_.Bridge_action = (byte)bridgeAction.BRIDGE_ACTION_JOYSTICK_DEBUG; //print out debug message
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
                    TextBox2.Text = errorMessage;
                }
            }
        }
        unsafe private void btn_Printlog_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.MessageBox.Show("Please check Serial monitor for pedal info");
            DAP_action_st tmp;
            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.PRINT_PEDAL_INFO;
            tmp.payloadHeader_.PedalTag = (byte)indexOfSelectedPedal_u;
            DAP_action_st* v = &tmp;
            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = Plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
            Plugin.SendPedalAction(tmp, (byte)indexOfSelectedPedal_u);
        }

        private void btn_Plugin_OTA_Click(object sender, RoutedEventArgs e)
        {
            UpdateSettingWindow sideWindow = new UpdateSettingWindow(Plugin.Settings, Plugin._calculations);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {
                string downloadUrl;
                string MSG_tmp = "Plugin will update from ";
                switch (Plugin.Settings.updateChannel)
                {
                    case 0:
                        downloadUrl = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/Plugin/DiyActivePedal.dll";
                        MSG_tmp += "Mainline release channel. ";
                        break;
                    case 1:
                        downloadUrl = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/OTA/DailyBuild/plugin/DiyActivePedal.dll";
                        MSG_tmp += "Nightly-Build channel. ";
                        break;
                    default:
                        downloadUrl = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/main/OTA/Plugin/DiyActivePedal.dll";
                        MSG_tmp += "Mainline release channel. ";
                        break;
                }

                string targetPath = Directory.GetCurrentDirectory() + "\\";
                //System.Windows.MessageBox.Show(targetPath);
                //targetPath = "C:\\Program Files (x86)\\SimHub\\";



                MSG_tmp += "The update requires administrators permission to delete the original plugin and download the new one. If you agree, please click OK.";
                var result = System.Windows.MessageBox.Show(MSG_tmp, "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                if (result == MessageBoxResult.OK)
                {
                    string exeName = "SimHubWPF.exe";
                    string exePath = targetPath + exeName;
                    string targetDllPath = targetPath + "DiyActivePedal.dll";


                    string psScript = $@"
                $processName = 'SimHubWPF'
                $downloadUrl = '{downloadUrl}'
                $targetDllPath = '{targetDllPath}'
                $exePath = '{exePath}'
                $tempPath = $env:TEMP + '\plugin_temp.dll'
                Write-Host 'Closing Simhub...'
                $procs = Get-Process -Name $processName -ErrorAction SilentlyContinue
                foreach ($proc in $procs) {{
                    Stop-Process -Id $proc.Id -Force
                    $proc.WaitForExit()
                }}
                Start-Sleep -Seconds 2

                Write-Host 'Download new Plugin...'
                Invoke-WebRequest -Uri $downloadUrl -OutFile $tempPath -UseBasicParsing

                Write-Host 'Backup .dll file...'
                if (Test-Path $targetDllPath) {{
                    Copy-Item -Path $targetDllPath -Destination ($targetDllPath + '.bak') -Force
                }}
                Write-Host 'Copy plugin to folder...'
                Copy-Item -Path $tempPath -Destination $targetDllPath -Force
                Write-Host 'Restart Simhub...'
                Start-Process -FilePath $exePath
                ";


                    string escapedScript = psScript.Replace("\"", "`\"").Replace("`r", "").Replace("`n", "; ");

                    var psi = new ProcessStartInfo
                    {
                        FileName = "powershell.exe",
                        Arguments = $"-NoProfile -ExecutionPolicy Bypass -Command \"{escapedScript}\"",
                        Verb = "runas", // force run with admin
                        UseShellExecute = true
                    };

                    try
                    {
                        Process.Start(psi);
                    }
                    catch (Exception ex)
                    {

                    }
                }
            }
            
            
        }

        private void btn_rudder_export_config_Click(object sender, RoutedEventArgs e)
        {
            using (System.Windows.Forms.SaveFileDialog saveFileDialog = new System.Windows.Forms.SaveFileDialog())
            {
                readRudderSettingToConfig();
                saveFileDialog.Title = "Datei speichern";
                saveFileDialog.Filter = "Textdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                saveFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (saveFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string fileName = saveFileDialog.FileName;
                    dap_config_st_rudder.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    var stream1 = new MemoryStream();
                    var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                    var serializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                    serializer.WriteObject(writer, dap_config_st_rudder);
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
                    TextBox2.Text = "Save " + saveFileDialog.FileName;
                }
            }
        }
    }
}
