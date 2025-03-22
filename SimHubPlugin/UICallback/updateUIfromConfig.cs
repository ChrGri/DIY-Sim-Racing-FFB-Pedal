using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
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
                    info_text += "\n" + PedalFirmwareVersion[indexOfSelectedPedal_u, 0] + "." + PedalFirmwareVersion[indexOfSelectedPedal_u, 1] + ".";
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
                    system_info_text += "\n" + dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[0] + "." + dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[1] + ".";
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
                KF_Tab.dap_config_st = tmp_struct;
                ControlStrategy_Tab.dap_config_st = tmp_struct;
                PID_Tab.dap_config_st = tmp_struct;
                MPC_tab.dap_config_st = tmp_struct;
                EffectsABS_Tab.dap_config_st = tmp_struct;
                EffectsRPM_Tab.dap_config_st = tmp_struct;
                EffectsBitePoint_Tab.dap_config_st = tmp_struct;
                EffectsGFroce_Tab.dap_config_st = tmp_struct;
                EffectsWheelSlip_Tab.dap_config_st = tmp_struct;
                EffectsRoadImpact_Tab.dap_config_st = tmp_struct;
                EffectsCustom1_tab.dap_config_st = tmp_struct;
                EffectsCustom2_Tab.dap_config_st = tmp_struct;
                PedalForceTravel_Tab.dap_config_st = tmp_struct;
                PedalKinematics_Tab.dap_config_st = tmp_struct;
                PedalSettingsSection.dap_config_st = tmp_struct;
                var tmp_rudder = dap_config_st_rudder;
                CurveRudderForce_Tab.dap_config_st = tmp_rudder;
                RudderSetting_Tab.dap_config_st = tmp_rudder;
                EffectsRPMRudder_Tab.dap_config_st = tmp_rudder;


                Misc_Tab.Settings = Plugin.Settings;
                EffectsABS_Tab.Settings = Plugin.Settings;
                EffectsRPM_Tab.Settings = Plugin.Settings;
                EffectsGFroce_Tab.Settings = Plugin.Settings;
                EffectsWheelSlip_Tab.Settings = Plugin.Settings;
                EffectsRoadImpact_Tab.Settings = Plugin.Settings;
                EffectsCustom1_tab.Settings = Plugin.Settings;
                EffectsCustom2_Tab.Settings = Plugin.Settings;
                PedalForceTravel_Tab.Settings = Plugin.Settings;
                PedalKinematics_Tab.Settings = Plugin.Settings;
                PedalSettingsSection.Settings = Plugin.Settings;
                EffectsRPMRudder_Tab.Settings = Plugin.Settings;
                EffectRudderACC_Tab.Settings = Plugin.Settings;
                SystemProfile_Tab.Settings = Plugin.Settings;
                SettingOTA_Tab.Settings = Plugin.Settings;
                SystemLicense_Tab.Settings = Plugin.Settings;
                SystemSetting_Section.Settings = Plugin.Settings;

                EffectsABS_Tab.calculation = Plugin._calculations;
                EffectsBitePoint_Tab.calculation = Plugin._calculations;
                EffectsCustom1_tab.calculation = Plugin._calculations;
                EffectsCustom2_Tab.calculation = Plugin._calculations;
                Misc_Tab.calculation = Plugin._calculations;
                PedalForceTravel_Tab.calculation = Plugin._calculations;
                PedalSettingsSection.calculation = Plugin._calculations;
                CurveRudderForce_Tab.calculation = Plugin._calculations;
                SystemProfile_Tab.calculation = Plugin._calculations;
                SettingOTA_Tab.calculation = Plugin._calculations;

                EffectsCustom1_tab.Plugin = Plugin;
                EffectsCustom2_Tab.Plugin = Plugin;


                btn_SendConfig.Content = Plugin._calculations.btn_SendConfig_Content;
                btn_SendConfig.ToolTip = Plugin._calculations.btn_SendConfig_tooltip;
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

                if (Plugin.Settings.Serial_auto_clean_bridge)
                {
                    Checkbox_auto_remove_serial_line_bridge.IsChecked = true;
                }
                else
                {
                    Checkbox_auto_remove_serial_line_bridge.IsChecked = false;
                }

            }

        }
    }
}
