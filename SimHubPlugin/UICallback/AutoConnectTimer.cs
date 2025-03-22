using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
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
            system_info_text_connection = tmp;
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
    }
}
