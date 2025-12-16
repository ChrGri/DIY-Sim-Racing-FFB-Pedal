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
        private WirelessConnectStateEnum[] pedalWirelessStatusLast = new WirelessConnectStateEnum[3] { WirelessConnectStateEnum.PEDAL_DISCONNECT, WirelessConnectStateEnum.PEDAL_DISCONNECT, WirelessConnectStateEnum.PEDAL_DISCONNECT };
        private ConnectStateEnum[] pedalSerialStatusLast = new ConnectStateEnum[3] { ConnectStateEnum.PEDAL_DISCONNECT, ConnectStateEnum.PEDAL_DISCONNECT, ConnectStateEnum.PEDAL_DISCONNECT };
        public void connection_timmer_tick(object sender, EventArgs e)
        {
            //simhub action for debug
            Simhub_action_update();
           

            for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            {
                if (Plugin._calculations.pedalWirelessStatus[pedalIdx] == WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW)
                {
                    Reading_config_auto(pedalIdx);
                    
                }
                if (Plugin._calculations.pedalSerialStatus[pedalIdx] == ConnectStateEnum.PEDAL_GET_BASIC_PACKETS && !Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
                {
                    Reading_config_auto(pedalIdx);
                    
                }
                
                
            }




            count_timmer_count++;
            if (count_timmer_count > 1)
            {
                if (Plugin.Settings.Pedal_ESPNow_auto_connect_flag)
                {
                    if (Plugin.PortExists(Plugin.Settings.ESPNow_port))
                    {
                        if (OpenBridgeSerialConnection())
                        {
                            Plugin._calculations.bridgeConnectionStatus = BridgeConnectStateEnum.BRIDGE_ENTRY_CONNECT;
                            for (uint i = 0; i < 3; i++)
                            {
                                if (Plugin._calculations.pedalWirelessStatus[(uint)i] == WirelessConnectStateEnum.PEDAL_DISCONNECT)
                                {
                                    Plugin._calculations.pedalWirelessStatus[(uint)i] = WirelessConnectStateEnum.PEDAL_BRIDGE_ENTRY_CONNECT;
                                }

                            }
                            ToastNotification("Pedal Wireless Bridge", "Connection initialized");
                            updateTheGuiFromConfig();

                            btn_connect_espnow_port.Content = "Disconnect";

                        }
                    }
                    else
                    {
                        /*
                        if (Plugin.Sync_esp_connection_flag)
                        {
                            Plugin.Sync_esp_connection_flag = false;
                        }
                        */


                        if (ESP_host_serial_timer != null)
                        {
                            ESP_host_serial_timer.Stop();
                            ESP_host_serial_timer.Dispose();

                        }
                        btn_connect_espnow_port.Content = "Connect";
                        if (Plugin._calculations.bridgeConnectionStatus != BridgeConnectStateEnum.BRIDGE_DISCONNECT)
                        {
                            updateTheGuiFromConfig();
                            Plugin._calculations.bridgeConnectionStatus = BridgeConnectStateEnum.BRIDGE_DISCONNECT;
                            for (int i = 0; i < 3; i++) Plugin._calculations.pedalWirelessStatus[i] = WirelessConnectStateEnum.PEDAL_DISCONNECT;
                            
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
                                    /*
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
                                        Toast_tmp = PedalConstStrings.PedalID[pedalIdx] + " Connected";
                                        ToastNotification(Toast_tmp, "Connected");
                                        updateTheGuiFromConfig();
                                        //System.Threading.Thread.Sleep(2000);
                                        //ToastNotificationManager.History.Clear("FFB Pedal Dashboard");
                                    }
                                    */



                                }
                            }
                            else
                            {
                                Plugin.connectSerialPort[pedalIdx] = false;
                                Plugin.Settings.connect_status[pedalIdx] = 0;
                                Plugin._calculations.ServoStatus[pedalIdx] = 0;
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
            // game config auto switch
            bool gameProfileChanged = false;
            for (int i = 0; i < 3; i++)
            {
                if (Plugin.ProfileServicePlugin.GamePofileConfigChange_b[i]
                    && (Plugin._calculations.pedalWirelessStatus[i] == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY ||
                    Plugin._calculations.pedalSerialStatus[i] == ConnectStateEnum.PEDAL_IS_READY))
                {
                    Plugin.SendConfigWithoutSaveToEEPROM(Plugin.ProfileServicePlugin.ConfigBuffer[i], (byte)i);
                    Plugin._calculations.ConfigEditing[i] = Plugin.ConfigService.ConfigList.FirstOrDefault(item => item.FullPath == Plugin.ProfileServicePlugin.GameConfigPathBuffer[i]).FileName;
                    Plugin.ProfileServicePlugin.GamePofileConfigChange_b[i] = false;
                    if (!Plugin.ProfileServicePlugin.GamePofileConfigChange_b[0] && !Plugin.ProfileServicePlugin.GamePofileConfigChange_b[1] && !Plugin.ProfileServicePlugin.GamePofileConfigChange_b[2])
                    {
                        gameProfileChanged = true;
                    }
                }
            }
            if(gameProfileChanged)
            {
                ToastNotification("Pedal Profile Switch", $"Profile switched to: {Plugin.ProfileServicePlugin.CurrentGameProfile} according to the game: {Plugin.currentGame}");
            }


            updateTheGuiFromConfig();
            string tmpPedalStatusChange = "";
            bool pedalConnectedToast = false;
            
            for (int i = 0; i < 3; i++)
            {
                if (pedalWirelessStatusLast[i] != WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY && Plugin._calculations.pedalWirelessStatus[i] == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY)
                {
                    pedalConnectedToast = true;
                    tmpPedalStatusChange += PedalConstStrings.PedalID[i] + " ";
                }
                if (pedalSerialStatusLast[i] != ConnectStateEnum.PEDAL_IS_READY && Plugin._calculations.pedalSerialStatus[i] == ConnectStateEnum.PEDAL_IS_READY)
                {
                    pedalConnectedToast = true;
                    tmpPedalStatusChange += PedalConstStrings.PedalID[i] + " ";
                }
            }
            if (pedalConnectedToast)
            {
                tmpPedalStatusChange += "Connected";
                ToastNotification("Pedal connection status", tmpPedalStatusChange);
            }
            tmpPedalStatusChange = "";
            pedalConnectedToast = false;
            for (int i = 0; i < 3; i++)
            {    
                if (pedalSerialStatusLast[i] == ConnectStateEnum.PEDAL_IS_READY && Plugin._calculations.pedalSerialStatus[i] == ConnectStateEnum.PEDAL_DISCONNECT)
                {
                    pedalConnectedToast = true;
                    tmpPedalStatusChange += PedalConstStrings.PedalID[i] + " ";
                }
                pedalWirelessStatusLast[i] = Plugin._calculations.pedalWirelessStatus[i];
                pedalSerialStatusLast[i] = Plugin._calculations.pedalSerialStatus[i];
            }
            if (pedalConnectedToast)
            {
                tmpPedalStatusChange += "Disconnected";
                ToastNotification("Pedal connection status", tmpPedalStatusChange);
            }

        }
    }
}
