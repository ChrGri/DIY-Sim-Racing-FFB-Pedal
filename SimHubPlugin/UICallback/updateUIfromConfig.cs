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
        public void CheckSerialAvailability()
        {
            //check the port availability
            if (Plugin != null)
            {
                if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    Plugin._calculations.BridgeSerialConnectionStatus = true;
                    Plugin._calculations.BridgeSerialAvailability = true;
                }
                else
                {
                    Plugin._calculations.BridgeSerialConnectionStatus = false;
                    Plugin._calculations.BridgeSerialAvailability = false;
                    Plugin._calculations.RSSI_Value = 0;
                    for (int i = 0; i < 3; i++)
                    {
                        if (Plugin.Settings.Pedal_ESPNow_Sync_flag[i])
                        {
                            Plugin._calculations.PedalAvailability[i] = false;
                            Plugin._calculations.PedalFirmwareVersion[i, 2] = 0;
                        }
                    }
                    Plugin._calculations.BridgeFirmwareVersion[2] = 0;
                }
                for (int i = 0; i < 3; i++)
                {
                    Plugin._calculations.PedalSerialAvailability[i] = Plugin._serialPort[indexOfSelectedPedal_u].IsOpen;
                }
            }

        }

        unsafe public void updateTheGuiFromConfig()
        {

            CheckSerialAvailability();

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
                SystemInfo.Settings = Plugin.Settings;
                PedalInfo.Settings = Plugin.Settings;

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
                SystemInfo.calculation = Plugin._calculations;
                PedalInfo.calculation = Plugin._calculations;
                RudderInfo.calculation = Plugin._calculations;

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
                if (Plugin._calculations.Rudder_status)
                {
                    btn_rudder_initialize.Content = "Disable Rudder";
                    //text_rudder_log.Visibility= Visibility.Visible;
                }
                else
                {
                    btn_rudder_initialize.Content = "Enable Rudder";
                    //text_rudder_log.Visibility = Visibility.Hidden;
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
