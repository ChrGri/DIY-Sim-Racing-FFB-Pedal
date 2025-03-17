using GameReaderCommon;
using log4net.Plugin;
using NCalc;

//using log4net.Plugin;
using SimHub.Plugins;
using SimHub.Plugins.DataPlugins.DataCore;
using SimHub.Plugins.DataPlugins.RGBMatrixDriver.Settings;
using SimHub.Plugins.DataPlugins.ShakeItV3.UI.Effects;
using SimHub.Plugins.OutputPlugins.Dash.GLCDTemplating;
using System;
using System.IO.Ports;
using System.Media;
using System.Runtime;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Controls;
using System.Windows.Media;
using Windows.UI.Notifications;
using static System.Net.Mime.MediaTypeNames;
using IPlugin = SimHub.Plugins.IPlugin;






namespace User.PluginSdkDemo
{
    [PluginDescription("The Plugin was for FFB pedal, To tune the pedal parameters and communicates with the pedal over USB.")]
    [PluginAuthor("OpenSource")]
    [PluginName("DIY active pedal plugin")]
    public class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public CalculationVariables _calculaitons;
        public PluginManager pluginHandle;// = this;

        public bool sendAbsSignal = false;
		public DAP_config_st dap_config_initial_st;
        public byte rpm_last_value = 0 ;
        public double g_force_last_value = 128;
        public byte Road_impact_last = 0;
        public byte game_running_index = 0 ;
        public uint testValue = 0;
        public uint[] profile_flag = new uint[4] { 0,0,0,0};
        public uint[] select_button_flag = new uint[2] { 0, 0, };// define the up and down selection
        public uint slotA_flag = 0;
        public uint slotB_flag = 0;
        public uint slotC_flag = 0;
        public uint slotD_flag = 0;
        public uint sendconfig_flag = 0;
        public DIYFFBPedalControlUI wpfHandle;
        public uint in_game_flag = 0; // check current game is off or pause
        public string current_profile = "NA" ;
        public uint profile_index = 0;
        //public uint Page_update_flag = 0;
        public bool binding_check=false;
        public bool pedal_select_update_flag = false;
        public string current_pedal = "NA";
        public string current_action = "NA";
        public bool Page_update_flag =false;
        public uint overlay_display = 0;
        public string simhub_theme_color = "#7E87CEFA";
        public uint debug_value = 0;
        public bool Rudder_enable_flag=false;
        public bool clear_action = false;
        public bool Rudder_status = false;
        public bool Rudder_brake_enable_flag = false;
        public bool Rudder_brake_status = false;
        public byte pedal_state_in_ratio = 0;
        public bool Sync_esp_connection_flag=false;
        public byte PedalErrorCode = 0;
        public byte PedalErrorIndex = 0;
        public byte[] random_pedal_action_interval=new byte[3] { 50,51,53};
        public byte Rudder_RPM_Effect_last_value = 0;
        public byte Rudder_G_last_value = 0;
        public bool MSFS_status = false;
        public byte Rudder_Wind_Force_last_value = 0;
        public bool MSFS_Plugin_Status = false;
        public string Simhub_version = "";
        public bool Version_Check_Simhub_MSFS = false;
        public byte[] Rudder_Pedal_idx = new byte[2] { 1, 2 };
        public string Current_Game = "";
        public byte TrackSurfaceCondition = 0;
        public bool[] PedalConfigRead_b = new bool[3] { false, false, false };
        //public vJoyInterfaceWrap.vJoy joystick;
        //effect trigger timer
        DateTime[] Action_currentTime = new DateTime[3];
        DateTime[] Action_lastTime = new DateTime[3];
        

        // ABS trigger timer
        DateTime absTrigger_currentTime = DateTime.Now;
        DateTime absTrigger_lastTime = DateTime.Now;

        //G force timer
        DateTime GTrigger_currentTime = DateTime.Now;
        DateTime GTrigger_lastTime = DateTime.Now;

        //Road effect
        DateTime RoadTrigger_currentTime = DateTime.Now;
        DateTime RoadTrigger_lastTime = DateTime.Now;
        //Rudder update
        DateTime Rudder_Action_currentTime = DateTime.Now;
        DateTime Rudder_Action_lastTime = DateTime.Now;

        public enum RudderAction
        {
            None,
            EnableRudderTwoPedals,
            ClearRudderStatus,
            EnableRudderThreePedals

        };

        public enum PedalSystemAction
        {
            NONE,
            RESET_PEDAL_POSITION,//not in use
            PEDAL_RESTART,
            ENABLE_OTA,//not in use
            ENABLE_PAIRING,//not in use
            ESP_BOOT_INTO_DOWNLOAD_MODE
        };

        public enum TrackConditionEnum
        {
            Dry,
            MostlyDry,
            VeryLightWet,
            LightWet,
            ModeratelyWet,
            VeryWet,
            ExtremelyWet,
            DIRT,
            ICED
        };

        //https://www.c-sharpcorner.com/uploadfile/eclipsed4utoo/communicating-with-serial-port-in-C-Sharp/
        public SerialPort[] _serialPort = new SerialPort[4] {new SerialPort("COM7", 921600, Parity.None, 8, StopBits.One),
            new SerialPort("COM7", 921600, Parity.None, 8, StopBits.One),
            new SerialPort("COM7", 921600, Parity.None, 8, StopBits.One),new SerialPort("COM7", 921600, Parity.None, 8, StopBits.One)};

        public SerialPort ESPsync_serialPort = new SerialPort("COM7", 3000000, Parity.None, 8, StopBits.One);

        //for (byte pedalIdx_lcl = 0; pedalIdx_lcl< 3; pedalIdx_lcl++)
        //{
        //    _serialPortt[pedalIdx_lcl].RtsEnable = false;
        //    _serialPort[pedalIdx_lcl].DtrEnable = true;
        //}   







        public bool[] connectSerialPort = { false, false, false };


        public DIYFFBPedalSettings Settings;



        /// <summary>
        /// Instance of the current plugin manager
        /// </summary>
        public PluginManager PluginManager { get; set; }

        /// <summary>
        /// Gets the left menu icon. Icon must be 24x24 and compatible with black and white display.
        /// </summary>
        public ImageSource PictureIcon => this.ToIcon(Properties.Resources.sdkmenuicon);

        /// <summary>
        /// Gets a short plugin title to show in left menu. Return null if you want to use the title as defined in PluginName attribute.
        /// </summary>
        public string LeftMenuTitle => "FFB Pedal Dashboard";
        //public string LeftMenuTitle => "DIY FFB Pedal";

        /// <summary>
        /// Called one time per game data update, contains all normalized game data,
        /// raw data are intentionnally "hidden" under a generic object type (A plugin SHOULD NOT USE IT)
        ///
        /// This method is on the critical path, it must execute as fast as possible and avoid throwing any error
        ///
        /// </summary>
        /// <param name="pluginManager"></param>
        /// <param name="data">Current game data, including current and previous data frame.</param>
        /// 
        unsafe public UInt16 checksumCalc(byte* data, int length)
        {

            UInt16 curr_crc = 0x0000;
            byte sum1 = (byte)curr_crc;
            byte sum2 = (byte)(curr_crc >> 8);
            int index;
            for (index = 0; index < length; index = index + 1)
            {
                int v = (sum1 + (*data));
                sum1 = (byte)v;
                sum1 = (byte)(v % 255);

                int w = (sum1 + sum2) % 255;
                sum2 = (byte)w;

                data++;// = data++;
            }

            int x = (sum2 << 8) | sum1;
            return (UInt16)x;
        }


        unsafe public UInt16 checksumCalcArray(byte[] data, int length)
        {

            UInt16 curr_crc = 0x0000;
            byte sum1 = (byte)curr_crc;
            byte sum2 = (byte)(curr_crc >> 8);
            int index;
            for (index = 0; index < length; index = index + 1)
            {
                int v = (sum1 + data[index]);
                sum1 = (byte)v;
                sum1 = (byte)(v % 255);

                int w = (sum1 + sum2) % 255;
                sum2 = (byte)w;

            }

            int x = (sum2 << 8) | sum1;
            return (UInt16)x;
        }

        unsafe public byte[] getBytesConfig(DAP_config_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);

            //int length = sizeof(DAP_config_st);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);
            return myBuffer;
        }
        public byte[] getBytes_Action(DAP_action_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }
        public byte[] getBytes_Bridge(DAP_bridge_state_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }

        public byte[] getBytes_Basic_Wifi_info(Basic_WIfi_info aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }
        public string Ncalc_reading(String expression)
        {
            string value = "";
            try
            {
                NCalc.Expression exp = new NCalc.Expression(expression);
                exp.ResolveParameter += delegate (string name, ParameterResolveArgs rarg)
                {
                    rarg.Result = () => PluginManager.GetPropertyValue(name);
                };

                if (exp.HasErrors() == false)
                {
                    value = exp.Evaluate().ToString();
                }
                else
                {
                    value = "Error";
                }

            }
            catch (Exception ex)
            {
                SimHub.Logging.Current.Error(ex.Message);
            }

            return value;
        }

        unsafe public void SendPedalAction(DAP_action_st action_tmp, Byte PedalID)
        {
            //DAP_action_st* v = &action_tmp;
            //byte* p = (byte*)v;
            int length = sizeof(DAP_action_st);
            byte[] newBuffer = new byte[length];
            newBuffer = getBytes_Action(action_tmp);
            try
            {
                if (Settings.Pedal_ESPNow_Sync_flag[PedalID])
                {
                    if (ESPsync_serialPort.IsOpen)
                    {
                        ESPsync_serialPort.DiscardInBuffer();
                        ESPsync_serialPort.DiscardOutBuffer();
                        // send query command
                        ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                    }
                }
                else
                {
                    
                    if (_serialPort[PedalID].IsOpen)
                    {
                        
                        // clear inbuffer 
                        _serialPort[PedalID].DiscardInBuffer();
                        _serialPort[PedalID].DiscardOutBuffer();
                        // send data
                        _serialPort[PedalID].Write(newBuffer, 0, newBuffer.Length);
                    }
                }
            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                SimHub.Logging.Current.Error("FFB_Pedal_Action_Sending_error:"+errorMessage);
            }

        }

        unsafe public void SendConfig(DAP_config_st tmp, byte PedalIDX)
        {
            int length = sizeof(DAP_config_st);
            //int val = this.dap_config_st[indexOfSelectedPedal_u].payloadHeader_.checkSum;
            //string msg = "CRC value: " + val.ToString();
            byte[] newBuffer = new byte[length];
            newBuffer = getBytesConfig(tmp);
            if (Settings.Pedal_ESPNow_Sync_flag[PedalIDX])
            {
                if (ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        ESPsync_serialPort.DiscardInBuffer();
                        ESPsync_serialPort.DiscardOutBuffer();
                        // send data
                        ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                        //Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                        System.Threading.Thread.Sleep(100);
                    }
                    catch (Exception caughtEx)
                    {
                        string errorMessage = caughtEx.Message;
                        SimHub.Logging.Current.Error("FFB_Pedal_Config_Sending_error:" + errorMessage);
                    }
                }
            }
            else
            {
                if (_serialPort[PedalIDX].IsOpen)
                {

                    // clear inbuffer 
                    _serialPort[PedalIDX].DiscardInBuffer();
                    _serialPort[PedalIDX].DiscardOutBuffer();
                    // send data
                    _serialPort[PedalIDX].Write(newBuffer, 0, newBuffer.Length);
                }
            }
        }

        unsafe public void SendConfigWithoutSaveToEEPROM(DAP_config_st tmp, byte PedalIDX)
        {
            tmp.payloadHeader_.storeToEeprom = 0;
            tmp.payloadHeader_.PedalTag=PedalIDX;
            DAP_config_st* v = &tmp;
            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
            SendConfig(tmp, PedalIDX);
        }
        unsafe public void DataUpdate(PluginManager pluginManager, ref GameData data)
        {
			
			bool sendAbsSignal_local_b = false;
            bool sendTcSignal_local_b = false;
            double RPM_value =0;
            double RPM_MAX = 0;
            double _G_force = 128;
            byte WS_value = 0;
            byte Road_impact_value = 0;
            byte CV1_value = 0;
            byte CV2_value = 0;
            double MSFS_RPM_Value_Simhub = 0;
            double RUDDER_DEFLECTION_Simhub = 0;
            double RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
            double ACCELERATION_BODY_Z_Simhub = 0;
            double ACCELERATION_BODY_Y_Simhub = 0;
            bool Flight_running_simhub = false;
            
            //bool WS_flag = false;

            if (data.GamePaused | (!data.GameRunning))
            {
                in_game_flag = 0;
            }
            else 
            {
                in_game_flag = 1;
            }
            //for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
            //{
            //    if (_serialPort[pedalIdx].IsOpen)
            //    {
            //        int receivedLength = _serialPort[pedalIdx].BytesToRead;

            //        settings.TextBox_debugOutput.Text = "Test";
            //    }
            //}


            // Send ABS signal when triggered by the game
            if (data.GameRunning)
            {
                Current_Game=(string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame");
                //load surface condition
                TrackSurfaceCondition = 0;
                if (Current_Game == "IRacing")
                {
                    byte TrackCondition = Convert.ToByte(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.Telemetry.TrackWetness"));
                    switch (TrackCondition)
                    {
                        case 0:
                            TrackSurfaceCondition = 0;
                            break;
                        case 1:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.Dry;
                            break;
                        case 2:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.MostlyDry;
                            break;
                        case 3:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.VeryLightWet;
                            break;
                        case 4:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.LightWet;
                            break;
                        case 5:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.ModeratelyWet;
                            break;
                        case 6:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.VeryWet;
                            break;
                        case 7:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.ExtremelyWet;
                            break;
                    }
                }
                if (Current_Game == "AssettoCorsaCompetizione")
                {
                    byte TrackCondition;
                    TrackCondition = Convert.ToByte(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.Graphics.trackGripStatus"));
                    switch (TrackCondition)
                    {
                        case 0:
                            TrackSurfaceCondition = 0;
                            break;
                        case 1:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.Dry;
                            break;
                        case 2:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.Dry;
                            break;
                        case 3:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.LightWet;
                            break;
                        case 4:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.ModeratelyWet;
                            break;
                        case 5:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.VeryWet;
                            break;
                        case 6:
                            TrackSurfaceCondition = (byte)TrackConditionEnum.ExtremelyWet;
                            break;
                        default:
                            TrackSurfaceCondition = 0;
                            break;

                    }
                }

                if (data.OldData != null && data.NewData != null)
                {
                    if (data.NewData.ABSActive > 0)
                    {
                        sendAbsSignal_local_b = true;
                    }

                    if (data.NewData.TCActive > 0)
                    {
                        sendTcSignal_local_b = true;
                    }


                    // when test signal is activated, overwrite trigger signal
                    if (sendAbsSignal)
                    {
                        sendAbsSignal_local_b = true;
                        sendTcSignal_local_b = true;
                    }



                    //fill the RPM value
                    if (Settings.RPM_effect_type == 0)
                    {
                        if (data.NewData.CarSettings_MaxRPM == 0)
                        {
                            RPM_MAX = 10000;
                        }
                        else
                        {
                            RPM_MAX = data.NewData.CarSettings_MaxRPM;
                        }

                        RPM_value = (data.NewData.Rpms / RPM_MAX * 100);
                    }
                    else
                    {
                        if (data.NewData.MaxSpeedKmh == 0)
                        {
                            RPM_MAX = 300;
                        }
                        else
                        { 
                            RPM_MAX= data.NewData.MaxSpeedKmh;
                        }
                        RPM_value = (data.NewData.SpeedKmh / RPM_MAX * 100);
                    }

                    
                    if (data.NewData.GlobalAccelerationG != 0)
                    {
                        _G_force = -1 * data.NewData.GlobalAccelerationG + 128;
                    }
                    else
                    {
                        _G_force = 128;
                    }

                    game_running_index = 1;
                    

                }
                else
                {
                    RPM_value = 0;
                    _G_force = 128;
                    


                }
            }
            else
            {
                RPM_value = 0;
                _G_force = 128;
                
            }
			




            absTrigger_currentTime = DateTime.Now;
            TimeSpan diff = absTrigger_currentTime - absTrigger_lastTime;
            int millisceonds = (int)diff.TotalMilliseconds;
            if (millisceonds <= 10)
            {
                sendAbsSignal_local_b = false;
                sendTcSignal_local_b = false;
                

            }
            else
            {
                absTrigger_lastTime = DateTime.Now;
            }




            bool update_flag = false;

            if (data.GameRunning)
            {
                // Send ABS trigger signal via serial
                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    
                    

                        DAP_action_st tmp;
                        tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                        tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                        tmp.payloadHeader_.PedalTag = (byte)pedalIdx;
                        tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                        tmp.payloadPedalAction_.RPM_u8 = (Byte)rpm_last_value;
                        
                        tmp.payloadPedalAction_.WS_u8 = 0;
                        tmp.payloadPedalAction_.impact_value = 0;
                        tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                        tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                        tmp.payloadPedalAction_.Rudder_action = 0;
                        tmp.payloadPedalAction_.Rudder_brake_action = 0;
                        if (Settings.G_force_enable_flag[pedalIdx] == 1)
                        {
                            tmp.payloadPedalAction_.G_value = (Byte)g_force_last_value;
                        }
                        else
                        {
                            tmp.payloadPedalAction_.G_value = 128;
                        }


                        if (Settings.RPM_enable_flag[pedalIdx] == 1)
                        {

                            if (Math.Abs(RPM_value - rpm_last_value) > 3)
                            {
                                tmp.payloadPedalAction_.RPM_u8 = (Byte)RPM_value;
                                update_flag = true;
                                rpm_last_value = (Byte)RPM_value;
                            }

                        }
                        else
                        {
                            tmp.payloadPedalAction_.RPM_u8 = 0;
                        }

                        //G force effect only effect on brake
                        if (pedalIdx == 1)
                        {

                            GTrigger_currentTime = DateTime.Now;
                            TimeSpan diff_G = GTrigger_currentTime - GTrigger_lastTime;
                            int millisceonds_G = (int)diff_G.TotalMilliseconds;
                            if (millisceonds <= 10)
                            {
                                _G_force = g_force_last_value;
                            }
                            else
                            {
                                GTrigger_lastTime = DateTime.Now;
                            }
                            if (Settings.G_force_enable_flag[pedalIdx] == 1)
                            {
                                //double value_check_g = 1 - _G_force / ((double)g_force_last_value);
                                double value_check_g = (_G_force - (double)g_force_last_value);
                                if (Math.Abs(value_check_g) > 2)
                                {
                                    tmp.payloadPedalAction_.G_value = (Byte)_G_force;
                                    update_flag = true;
                                    g_force_last_value = (Byte)_G_force;
                                }

                            }
                        }

                        //Wheel slip
                        
                        if (Settings.WS_enable_flag[pedalIdx] == 1)
                        {
                            if (pluginManager.GetPropertyValue(Settings.WSeffect_bind) != null)
                            {
                                /*object tmp_ws = (pluginManager.GetPropertyValue(Settings.WSeffect_bind));
                                int tmp_ws_number = Int32.Parse(tmp_ws.ToString());
                                WS_value = (byte)tmp_ws_number;
                                */
                                WS_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.WSeffect_bind));
                                //pluginManager.SetPropertyValue("Wheelslip-test", this.GetType(), WS_value);
                                if (WS_value >= (Settings.WS_trigger ))
                                {
                                    tmp.payloadPedalAction_.WS_u8 = 1;
                                    update_flag = true;
                                }
                            }
                        }
                        //Road impact
                        if (Settings.Road_impact_enable_flag[pedalIdx] == 1)
                        {
                            if (pluginManager.GetPropertyValue(Settings.Road_impact_bind) != null)
                            {
                                Road_impact_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.Road_impact_bind));

                                RoadTrigger_currentTime = DateTime.Now;
                                TimeSpan diff_Road = RoadTrigger_currentTime - RoadTrigger_lastTime;
                                int millisceonds_G = (int)diff_Road.TotalMilliseconds;
                                if (millisceonds <= 10)
                                {
                                    Road_impact_value = Road_impact_last;
                                }
                                else
                                {
                                    RoadTrigger_lastTime = DateTime.Now;
                                }
                                if (true)
                                {
                                    //double value_check_g = 1 - _G_force / ((double)g_force_last_value);
                                    double value_check_road = Road_impact_value - Road_impact_last;
                                    if (Math.Abs(value_check_road) > 2)
                                    {
                                        tmp.payloadPedalAction_.impact_value = Road_impact_value;
                                        update_flag = true;
                                        Road_impact_last = Road_impact_value;
                                        debug_value = Road_impact_value;
                                    }

                                }
                            }
                        }
                     //custom effcts
                     if (Settings.CV1_enable_flag[pedalIdx] == true)
                     {
                        //CV1_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV1_bindings[pedalIdx]));
                        string temp_string = Ncalc_reading(Settings.CV1_bindings[pedalIdx]);
                        if (temp_string != "Error")
                        {
                            CV1_value = Convert.ToByte(temp_string);
                        }
                        else
                        {
                            CV1_value = 0;
                            SimHub.Logging.Current.Error("CV1 Reading error");
                        }


                        if (CV1_value > (Settings.CV1_trigger[pedalIdx]))
                        {
                            tmp.payloadPedalAction_.Trigger_CV_1 = 1;
                            update_flag = true;
                        }
                    }
                     if (Settings.CV2_enable_flag[pedalIdx] == true)
                     {

                        //CV2_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV2_bindings[pedalIdx]));
                        string temp_string = Ncalc_reading(Settings.CV2_bindings[pedalIdx]);
                        if (temp_string != "Error")
                        {
                            CV2_value = Convert.ToByte(temp_string);
                        }
                        else
                        {
                            CV2_value = 0;
                            SimHub.Logging.Current.Error("CV2 Reading error");
                        }
                        if (CV2_value > (Settings.CV2_trigger[pedalIdx]))
                        {
                            tmp.payloadPedalAction_.Trigger_CV_2 = 1;
                            update_flag = true;
                        }

                    }
                    //ABS/TC function
                    if (pedalIdx == 1)
                    {
                        if (sendAbsSignal_local_b && Settings.ABS_enable_flag[pedalIdx] == 1)
                        {
                            //_serialPort[1].Write("2");

                            // compute checksum
                            tmp.payloadPedalAction_.triggerAbs_u8 = (byte)(TrackSurfaceCondition +1);
                            update_flag = true;

                        }
                    }
                    if (pedalIdx == 2)
                    {
                        if (sendTcSignal_local_b && Settings.ABS_enable_flag[pedalIdx] == 1)
                        {
                            // compute checksum

                            tmp.payloadPedalAction_.triggerAbs_u8 = 1;
                            //tmp.payloadPedalAction_.RPM_u8 = 20;
                            update_flag = true;

                        }
                    }
                    // check the update interval
                    if (update_flag)
                    {
                        Action_currentTime[pedalIdx] = DateTime.Now;
                        TimeSpan diff_action = Action_currentTime[pedalIdx] - Action_lastTime[pedalIdx];
                        int millisceonds_action = (int)diff_action.TotalMilliseconds;
                        if (millisceonds_action <= Settings.Pedal_action_interval[pedalIdx])
                        {
                            update_flag = false;
                        }
                        else
                        {
                            Action_lastTime[pedalIdx] = DateTime.Now;
                            
                        }

                        
                    }


                    if (update_flag)
                    {

                        DAP_action_st* v = &tmp;
                            byte* p = (byte*)v;
                            tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));


                            int length = sizeof(DAP_action_st);
                            byte[] newBuffer = new byte[length];
                            newBuffer = getBytes_Action(tmp);

                            if (Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
                            {
                                if (ESPsync_serialPort.IsOpen)
                                {
                                    ESPsync_serialPort.DiscardInBuffer();
                                    ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                                    System.Threading.Thread.Sleep(7);
                                }

                            }
                            else
                            {
                                if (_serialPort[pedalIdx].IsOpen)
                                {
                                    // clear inbuffer 
                                    _serialPort[pedalIdx].DiscardInBuffer();

                                    // send query command
                                    _serialPort[pedalIdx].Write(newBuffer, 0, newBuffer.Length);
                                }

                            }



                    }
                    
                }

                if (((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2020" || ((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2024")
                {
                    MSFS_RPM_Value_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.GeneralEngPctMaxRPM1"));
                    //RUDDER_DEFLECTION_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.RUDDER_DEFLECTION")); 
                    RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AircraftWindZ"));
                    ACCELERATION_BODY_Z_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AccelerationBodyZ"));
                    ACCELERATION_BODY_Y_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AccelerationBodyY"));
                    Flight_running_simhub = true;
                }
                else
                {
                    MSFS_RPM_Value_Simhub = 0;
                    //RUDDER_DEFLECTION_Simhub = 0; 
                    RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Y_Simhub = 0;
                    Flight_running_simhub = false;
                }
                if (((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "DCS")
                {
                    MSFS_RPM_Value_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.EngineInfo.Rpm.Left"));
                    //RUDDER_DEFLECTION_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.RUDDER_DEFLECTION")); 
                    RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Y_Simhub = 0;
                    Flight_running_simhub = true;
                }
                else
                {
                    MSFS_RPM_Value_Simhub = 0;
                    //RUDDER_DEFLECTION_Simhub = 0; 
                    RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Z_Simhub = 0;
                    ACCELERATION_BODY_Y_Simhub = 0;
                    Flight_running_simhub = false;
                }
            }
            else
            {
                if (game_running_index == 1)
                {
                    game_running_index = 0;
                    clear_action = true;   
                }
            }



            // Send ABS test signal if requested
            if (sendAbsSignal || _calculaitons.SendAbsSignal)
            {
                sendAbsSignal_local_b = true;
                sendTcSignal_local_b = true;
                DAP_action_st tmp;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                tmp.payloadPedalAction_.triggerAbs_u8 = 1;
                tmp.payloadPedalAction_.RPM_u8 = 0;
                tmp.payloadPedalAction_.G_value = 128;
                tmp.payloadPedalAction_.WS_u8 = 0;
                tmp.payloadPedalAction_.impact_value = 0;
                tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                tmp.payloadPedalAction_.Rudder_action = 0;
                tmp.payloadPedalAction_.Rudder_brake_action = 0;

                for (uint PIDX = 1; PIDX < 3; PIDX++)
                {
                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                    DAP_action_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    int length = sizeof(DAP_action_st);
                    byte[] newBuffer = new byte[length];
                    newBuffer = getBytes_Action(tmp);
                    if (Settings.Pedal_ESPNow_Sync_flag[PIDX])
                    {
                        if (ESPsync_serialPort.IsOpen) 
                        {
                            ESPsync_serialPort.DiscardInBuffer();  
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                            System.Threading.Thread.Sleep(30);
                        }
                    }
                    else
                    {
                        if (_serialPort[PIDX].IsOpen)
                        {
                            // clear inbuffer 
                            _serialPort[PIDX].DiscardInBuffer();

                            // send query command
                            _serialPort[PIDX].Write(newBuffer, 0, newBuffer.Length);
                            System.Threading.Thread.Sleep(50);
                        }
                    }
                }
                    

            }
            if (Rudder_enable_flag)
            {
                if (Rudder_status == false)
                {
                    Rudder_status = true;
                }
                else
                {
                    Rudder_status = false;
                }
                DAP_action_st tmp;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;                    
                tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                tmp.payloadPedalAction_.RPM_u8 = 0;
                tmp.payloadPedalAction_.G_value = 128;
                tmp.payloadPedalAction_.WS_u8 = 0;
                tmp.payloadPedalAction_.impact_value = 0;
                tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                if (Rudder_Pedal_idx[0] == 0)
                {
                    tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableRudderThreePedals;
                }
                else
                {
                    tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableRudderTwoPedals;
                }
                
                tmp.payloadPedalAction_.Rudder_brake_action = 0;

                for (uint i = 0; i < 2; i++)
                {
                    uint PIDX = Rudder_Pedal_idx[i];
                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                    DAP_action_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    int length = sizeof(DAP_action_st);
                    byte[] newBuffer = new byte[length];
                    newBuffer = getBytes_Action(tmp);
                    
                    
                    if (Settings.Pedal_ESPNow_Sync_flag[PIDX])
                    {
                        if (ESPsync_serialPort.IsOpen)
                        {
                            ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                            System.Threading.Thread.Sleep(5);
                        }
                    }

                    
                    Rudder_enable_flag = false;
                    System.Threading.Thread.Sleep(50);
                }
                SystemSounds.Beep.Play();

            }

            //Rudder effect runtine
            //check MSFS plugin version
            if (((string)pluginManager.GetPropertyValue("FlightPlugin.MSFS_PLUGIN_VERSION")) == "1.0.0.0")
            {
                MSFS_Plugin_Status = true;
            }
            else
            {
                MSFS_Plugin_Status = false;
            }

            if (Rudder_status)
            {
                if (MSFS_Plugin_Status || Flight_running_simhub)
                {
                    if (Convert.ToByte(pluginManager.GetPropertyValue("FlightPlugin.IS_MSFS_DATA_UPDATING")) == 1)
                    {
                        MSFS_status = true;

                    }
                    else
                    {
                        if (MSFS_status)
                        {
                            clear_action = true;
                            MSFS_status = false;
                        }
                    }
                    if (MSFS_status || Flight_running_simhub)
                    {
                        Rudder_Action_currentTime = DateTime.Now;
                        TimeSpan diff_action = Rudder_Action_currentTime - Rudder_Action_lastTime;
                        int millisceonds_action = (int)diff_action.TotalMilliseconds;
                        if (millisceonds_action > 40)
                        {
                            bool Rudder_Effect_update_b = false;
                            DAP_action_st tmp;
                            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                            tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                            tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                            tmp.payloadPedalAction_.RPM_u8 = Rudder_RPM_Effect_last_value;
                            tmp.payloadPedalAction_.G_value = 128;
                            tmp.payloadPedalAction_.WS_u8 = 0;
                            tmp.payloadPedalAction_.impact_value = Rudder_G_last_value;
                            //tmp.payloadPedalAction_.impact_value = 0;
                            tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                            tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                            tmp.payloadPedalAction_.Rudder_action = 0;
                            tmp.payloadPedalAction_.Rudder_brake_action = 0;
                            //action here

                            //RPM effect
                            if (Settings.Rudder_RPM_effect_b)
                            {
                                byte Rudder_RPM_value = 0;
                                if (MSFS_Plugin_Status)
                                {
                                    Rudder_RPM_value = Convert.ToByte(pluginManager.GetPropertyValue("FlightPlugin.FlightData.GENERAL_ENG_PCT_MAX_RPM_1"));
                                }
                                if (Flight_running_simhub)
                                {
                                    Rudder_RPM_value = (byte)MSFS_RPM_Value_Simhub;
                                }
                                
                                
                                
                                if (Math.Abs(Rudder_RPM_value - Rudder_RPM_Effect_last_value) > 3)
                                {
                                    tmp.payloadPedalAction_.RPM_u8 = Rudder_RPM_value;
                                    Rudder_Effect_update_b = true;
                                    Rudder_Action_lastTime = DateTime.Now;
                                    Rudder_RPM_Effect_last_value = Rudder_RPM_value;
                                }
                            }

                            if (Settings.Rudder_ACC_effect_b)
                            {
                                double Rudder_Wind_Froce_Ratio = 0;

                                double RELATIVE_WIND_VELOCITY_BODY_Z = 0;
                                double Rudder_Radians = 0;
                                double Rudder_G_value_dz = 0;
                                double Rudder_G_value_dy = 0;
                                if (MSFS_Plugin_Status)
                                {
                                    Rudder_G_value_dz = Convert.ToDouble(pluginManager.GetPropertyValue("FlightPlugin.FlightData.ACCELERATION_BODY_Z"));
                                    Rudder_G_value_dy = Convert.ToDouble(pluginManager.GetPropertyValue("FlightPlugin.FlightData.ACCELERATION_BODY_Y"));
                                    RELATIVE_WIND_VELOCITY_BODY_Z = Math.Abs(Convert.ToDouble(pluginManager.GetPropertyValue("FlightPlugin.FlightData.RELATIVE_WIND_VELOCITY_BODY_Z")));
                                    Rudder_Radians = Math.Abs(Convert.ToDouble(pluginManager.GetPropertyValue("FlightPlugin.FlightData.RUDDER_DEFLECTION")));
                                }
                                if (Flight_running_simhub)
                                {
                                    Rudder_G_value_dz = ACCELERATION_BODY_Z_Simhub;
                                    Rudder_G_value_dy = ACCELERATION_BODY_Y_Simhub;
                                    RELATIVE_WIND_VELOCITY_BODY_Z = RELATIVE_WIND_VELOCITY_BODY_Z_Simhub;
                                    Rudder_Radians = RUDDER_DEFLECTION_Simhub;
                                }
                                if (Settings.Rudder_ACC_WindForce)
                                {

                                    double Rudder_Wind_Force = Math.Sin(Rudder_Radians) * RELATIVE_WIND_VELOCITY_BODY_Z;
                                    Rudder_Wind_Force_last_value = (Byte)Rudder_Wind_Force;
                                    double Max_Wind_Force = 100;
                                    Rudder_Wind_Force = Math.Min(Max_Wind_Force, Rudder_Wind_Force);//clipping max force
                                    Rudder_Wind_Froce_Ratio = 0.5 * 100 * (Rudder_Wind_Force / Max_Wind_Force);
                                }
                                //G-effect
                                double Rudder_G_percent = 0;
                                double max_G = 100;

                                double Rudder_G_value_combined = Math.Sqrt(Rudder_G_value_dz * Rudder_G_value_dz + Rudder_G_value_dy * Rudder_G_value_dy);
                                double Rudder_G_constrain = Math.Min(Rudder_G_value_combined, max_G);
                                Rudder_G_percent = Rudder_G_constrain / max_G * 100.0f;
                                double Rudder_G_Wind_combined = Math.Min(Rudder_G_percent + Rudder_Wind_Froce_Ratio, max_G);

                                if (Math.Abs(Rudder_G_last_value - Rudder_G_percent) > 2)
                                {
                                    tmp.payloadPedalAction_.impact_value = (Byte)Rudder_G_Wind_combined;
                                    Rudder_Effect_update_b = true;
                                    Rudder_Action_lastTime = DateTime.Now;
                                    Rudder_G_last_value = (Byte)Rudder_G_Wind_combined;
                                }
                            }




                            //Write to Pedal
                            if (Rudder_Effect_update_b)
                            {
                                for (uint i = 0; i < 2; i++)
                                {
                                    uint PIDX = Rudder_Pedal_idx[i];
                                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                                    DAP_action_st* v = &tmp;
                                    byte* p = (byte*)v;
                                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                                    int length = sizeof(DAP_action_st);
                                    byte[] newBuffer = new byte[length];
                                    newBuffer = getBytes_Action(tmp);
                                    if (ESPsync_serialPort.IsOpen)
                                    {
                                        ESPsync_serialPort.DiscardInBuffer();
                                        ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                                        System.Threading.Thread.Sleep(7);
                                    }
                                }
                                Rudder_Effect_update_b = false;
                            }
                        }
                    }
                }
                
                

            }



            if (Rudder_brake_enable_flag)
            {
                if (Rudder_brake_status == false)
                {
                    Rudder_brake_status = true;
                    
                }
                else
                {
                    Rudder_brake_status = false;
                }
                SystemSounds.Beep.Play();
                DAP_action_st tmp;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                tmp.payloadPedalAction_.RPM_u8 = 0;
                tmp.payloadPedalAction_.G_value = 128;
                tmp.payloadPedalAction_.WS_u8 = 0;
                tmp.payloadPedalAction_.impact_value = 0;
                tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                tmp.payloadPedalAction_.Rudder_action = 0;
                tmp.payloadPedalAction_.Rudder_brake_action = 1;

                for (uint i = 0; i < 2; i++)
                {
                    uint PIDX = Rudder_Pedal_idx[i];
                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                    DAP_action_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    int length = sizeof(DAP_action_st);
                    byte[] newBuffer = new byte[length];
                    newBuffer = getBytes_Action(tmp);
                    if (Settings.Pedal_ESPNow_Sync_flag[PIDX])
                    {
                        if (ESPsync_serialPort.IsOpen)
                        {
                            ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                            System.Threading.Thread.Sleep(10);
                        }
                    }
                    else
                    {
                        if (_serialPort[PIDX].IsOpen)
                        {
                            // clear inbuffer 
                            _serialPort[PIDX].DiscardInBuffer();

                            // send query command
                            _serialPort[PIDX].Write(newBuffer, 0, newBuffer.Length);
                        }

                    }
                    Rudder_brake_enable_flag = false;
                    System.Threading.Thread.Sleep(50);
                }

            }


            if (clear_action)
            {
                
                DAP_action_st tmp;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                tmp.payloadPedalAction_.RPM_u8 = 0;
                tmp.payloadPedalAction_.G_value = 128;
                tmp.payloadPedalAction_.WS_u8 = 0;
                tmp.payloadPedalAction_.impact_value = 0;
                tmp.payloadPedalAction_.Trigger_CV_1 = 0;
                tmp.payloadPedalAction_.Trigger_CV_2 = 0;
                tmp.payloadPedalAction_.Rudder_action = 0;
                tmp.payloadPedalAction_.Rudder_brake_action = 0;
                rpm_last_value = 0;
                Road_impact_last = 0;
                debug_value = 0;

                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    tmp.payloadHeader_.PedalTag = (byte)pedalIdx;
                    DAP_action_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    int length = sizeof(DAP_action_st);
                    byte[] newBuffer = new byte[length];
                    newBuffer = getBytes_Action(tmp);
                    if (Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
                    {
                        if (ESPsync_serialPort.IsOpen)
                        {
                            ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                            System.Threading.Thread.Sleep(10);
                        }
                    }
                    else
                    {
                        if (_serialPort[pedalIdx].IsOpen)
                        {
                            // clear inbuffer 
                            _serialPort[pedalIdx].DiscardInBuffer();

                            // send query command
                            _serialPort[pedalIdx].Write(newBuffer, 0, newBuffer.Length);
                        }

                    }
                }
                clear_action = false;
            }


            this.AttachDelegate("CurrentProfile", () => current_profile);
            pluginManager.SetPropertyValue("SelectedPedal", this.GetType(), current_pedal);
            pluginManager.SetPropertyValue("Action", this.GetType(), current_action);
            pluginManager.SetPropertyValue("ABS_effect_status", this.GetType(), Settings.ABS_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("RPM_effect_status", this.GetType(), Settings.RPM_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("Gforce_effect_status", this.GetType(), Settings.G_force_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("WheelSlip_effect_status", this.GetType(), Settings.WS_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("RoadImpact_effect_status", this.GetType(), Settings.Road_impact_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("Overlay_display", this.GetType(), overlay_display);
            pluginManager.SetPropertyValue("Theme_color", this.GetType(), simhub_theme_color);
            pluginManager.SetPropertyValue("ProfileIndex", this.GetType(), profile_index);
            pluginManager.SetPropertyValue("debugvalue", this.GetType(), debug_value);
            pluginManager.SetPropertyValue("rudder_status", this.GetType(), Rudder_status);
            pluginManager.SetPropertyValue("rudder_brake_status", this.GetType(), Rudder_brake_status);
            pluginManager.SetPropertyValue("pedal_position", this.GetType(), pedal_state_in_ratio);
            pluginManager.SetPropertyValue("PedalErrorIndex", this.GetType(), PedalErrorIndex);
            pluginManager.SetPropertyValue("PedalErrorCode", this.GetType(), PedalErrorCode);
            pluginManager.SetPropertyValue("FlightRudder_G", this.GetType(), Rudder_G_last_value);
            pluginManager.SetPropertyValue("FlightRudder_Wind_Force", this.GetType(), Rudder_Wind_Force_last_value);
        }












        /// <summary>
        /// Returns the settings control, return null if no settings control is required
        /// </summary>
        /// <param name="pluginManager"></param>
        /// <returns></returns>
        public System.Windows.Controls.Control GetWPFSettingsControl(PluginManager pluginManager)
        {

            return new DIYFFBPedalControlUI(this);
        }


        /// <summary>
        /// Called at plugin manager stop, close/dispose anything needed here !
        /// Plugins are rebuilt at game change
        /// </summary>
        /// <param name="pluginManager"></param>
        public void End(PluginManager pluginManager)
        {           
            // Save settings
            this.SaveCommonSettings("GeneralSettings", Settings);

            // close serial communication
            if (wpfHandle != null)
            {
                
                try
                {
                    
                    if (wpfHandle.PedalSettingsSection._joystick!= null)
                    {
                        wpfHandle.PedalSettingsSection._joystick.RelinquishVJD(Settings.vjoy_order);
                    }
                    
                    
                }
                catch (Exception caughtEx)
                { 
                }
                
                

                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    wpfHandle.closeSerialAndStopReadCallback(pedalIdx);
                }
            }
            
            if (ToastNotificationManager.History.GetHistory("Pedal_notification").Count != 0)
            {
                ToastNotificationManager.History.Remove("Pedal_notification");
            }
            

        }



        public bool PortExists(string portName)
        {
            string[] portNames = SerialPort.GetPortNames();
            return Array.Exists(portNames, name => name.Equals(portName, StringComparison.OrdinalIgnoreCase));
        }





        /// <summary>
        /// Called once after plugins startup
        /// Plugins are rebuilt at game change
        /// </summary>
        /// <param name="pluginManager"></param>
        public void Init(PluginManager pluginManager)
        {

            pluginHandle = pluginManager;
            _calculaitons = new CalculationVariables();
            SimHub.Logging.Current.Info("Starting DIY active pedal plugin");

            // Load settings
            Settings = this.ReadCommonSettings<DIYFFBPedalSettings>("GeneralSettings", () => new DIYFFBPedalSettings());
            Simhub_version = (String)pluginManager.GetPropertyValue("DataCorePlugin.SimHubVersion");
            // Declare a property available in the property list, this gets evaluated "on demand" (when shown or used in formulas)
            //this.AttachDelegate("CurrentDateTime", () => DateTime.Now);
            pluginManager.AddProperty("ProfileIndex", this.GetType(), profile_index);
            pluginManager.AddProperty("SelectedPedal", this.GetType(), current_pedal);
            pluginManager.AddProperty("Action", this.GetType(), current_action);
            pluginManager.AddProperty("ABS_effect_status", this.GetType(), Settings.ABS_enable_flag[Settings.table_selected]);
            pluginManager.AddProperty("RPM_effect_status", this.GetType(), Settings.RPM_enable_flag[Settings.table_selected]);
            pluginManager.AddProperty("Gforce_effect_status", this.GetType(), Settings.G_force_enable_flag[Settings.table_selected]);
            pluginManager.AddProperty("WheelSlip_effect_status", this.GetType(), Settings.WS_enable_flag[Settings.table_selected]);
            pluginManager.AddProperty("RoadImpact_effect_status", this.GetType(), Settings.Road_impact_enable_flag[Settings.table_selected]);
            pluginManager.AddProperty("Overlay_display", this.GetType(), overlay_display);
            pluginManager.AddProperty("Theme_color", this.GetType(), simhub_theme_color);
            pluginManager.AddProperty("debugvalue", this.GetType(), debug_value);
            pluginManager.AddProperty("rudder_status", this.GetType(), Rudder_status);
            pluginManager.AddProperty("rudder_brake_status", this.GetType(), Rudder_brake_status);
            pluginManager.AddProperty("pedal_position", this.GetType(), pedal_state_in_ratio);
            pluginManager.AddProperty("PedalErrorIndex", this.GetType(), PedalErrorIndex);
            pluginManager.AddProperty("PedalErrorCode", this.GetType(), PedalErrorCode);
            pluginManager.AddProperty("FlightRudder_G", this.GetType(), Rudder_G_last_value);
            pluginManager.AddProperty("FlightRudder_Wind_Force", this.GetType(), Rudder_Wind_Force_last_value);
            for (uint pedali=0; pedali < 3; pedali++)
            {
                Action_currentTime[pedali] = new DateTime();
                Action_currentTime[pedali]=DateTime.Now;
                Action_lastTime[pedali] = new DateTime();
                Action_lastTime[pedali] = DateTime.Now;
            }

            //get version length
            int Version_text_length = Simhub_version.Length;
            //if (Simhub_version[Version_text_length - 2] == 'b')
            if(Simhub_version.Contains("b"))
            {
                Version_Check_Simhub_MSFS = false;
            }
            else
            {
                Version inputVersion = new Version(Simhub_version);
                string MSFS_Version_Above = "9.5.99";
                Version versionThreshold = new Version(MSFS_Version_Above);
                if (inputVersion > versionThreshold)
                {
                    Version_Check_Simhub_MSFS = true;
                }
                else
                {
                    Version_Check_Simhub_MSFS = false;
                }
            }

            
            Version_Check_Simhub_MSFS = true;
            this.AddAction("ChangeSlotA", (a, b) =>
            {
                profile_index = 0;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotA");
                current_profile = "Slot A";
                current_action= "Slot A";
            });

            this.AddAction("ChangeSlotB", (a, b) =>
            {
                
                profile_index = 1;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotB");
                current_profile = "Slot B";
                current_action = "Slot B";
            });

            this.AddAction("ChangeSlotC", (a, b) =>
            {
                profile_index = 2;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotC");
                current_profile = "Slot C";
                current_action = "Slot C";
            });

            this.AddAction("ChangeSlotD", (a, b) =>
            {
                profile_index = 3;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotD");
                current_profile = "Slot D";
                current_action = "Slot D";
            });
            this.AddAction("ChangeSlotE", (a, b) =>
            {
                profile_index = 4;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotE");
                current_profile = "Slot E";
                current_action = "Slot E";
            });
            this.AddAction("ChangeSlotF", (a, b) =>
            {
                profile_index = 5;
                Page_update_flag = true;
                SimHub.Logging.Current.Info("SlotF");
                current_profile = "Slot F";
                current_action = "Slot F";
            });
            this.AddAction("SendConfigToPedal", (a, b) =>
            {
                sendconfig_flag =1;
                SimHub.Logging.Current.Info("SendConfig");
                current_action = "Send Config to Pedal";
            });

            this.AddAction("PreviousProfile", (a, b) =>
            {
                if (profile_index == 0)
                {
                    profile_index=5;
                }
                else
                {
                    profile_index--;
                }
                

                Page_update_flag = true;
                SimHub.Logging.Current.Info("PreviousProfile");
                current_action = "Previous Profile";
            });

            this.AddAction("NextProfile", (a, b) =>
            {
                profile_index++;
                if (profile_index > 5)
                {
                    profile_index = 0;
                }
                Page_update_flag = true;
                SimHub.Logging.Current.Info("NextProfile");
                current_action = "Next Profile";
            });
            this.AddAction("NextPedal", (a, b) =>
            {
                Settings.table_selected++;
                if (Settings.table_selected > 2)
                {
                    Settings.table_selected = 0;
                }
                Page_update_flag = true;
                SimHub.Logging.Current.Info("NextPedal");
                current_action = "Next Pedal";
            });
            this.AddAction("PreviousPedal", (a, b) =>
            {
                
                if (Settings.table_selected == 0)
                {
                    Settings.table_selected = 2;
                }
                else
                {
                    Settings.table_selected--;
                }
                Page_update_flag = true;
                SimHub.Logging.Current.Info("PreviousPedal");
                current_action = "Previous Pedal";
            });
            this.AddAction("ABStoggle", (a, b) =>
            {
                if (Settings.ABS_enable_flag[Settings.table_selected] == 0)
                {
                    Settings.ABS_enable_flag[Settings.table_selected] = 1;
                    SimHub.Logging.Current.Info("ABS on");
                    current_action = "ABS On";
                }
                else
                {
                    Settings.ABS_enable_flag[Settings.table_selected] = 0;
                    SimHub.Logging.Current.Info("ABS off");
                    current_action = "ABS Off";
                }
                Page_update_flag = true;
            });
            this.AddAction("RPMtoggle", (a, b) =>
            {
                if (Settings.RPM_enable_flag[Settings.table_selected] == 0)
                {
                    Settings.RPM_enable_flag[Settings.table_selected] = 1;
                    SimHub.Logging.Current.Info("RPM on");
                    current_action = "RPM On";
                }
                else
                {
                    Settings.RPM_enable_flag[Settings.table_selected] = 0;
                    SimHub.Logging.Current.Info("RPM off");
                    current_action = "RPM Off";
                }
                Page_update_flag = true;
            });
            this.AddAction("Gforce_toggle", (a, b) =>
            {
                if (Settings.table_selected == 1)
                {
                    if (Settings.G_force_enable_flag[Settings.table_selected] == 0)
                    {
                        Settings.G_force_enable_flag[Settings.table_selected] = 1;
                        SimHub.Logging.Current.Info("Gforce on");
                        current_action = "Gforce On";
                    }
                    else
                    {
                        Settings.G_force_enable_flag[Settings.table_selected] = 0;
                        SimHub.Logging.Current.Info("Gforce off");
                        current_action = "Gforce Off";
                    }
                    Page_update_flag = true;
                }

            });
            this.AddAction("WheelSliptoggle", (a, b) =>
            {
                if (Settings.WS_enable_flag[Settings.table_selected] == 0)
                {
                    Settings.WS_enable_flag[Settings.table_selected] = 1;
                    SimHub.Logging.Current.Info("WheelSlip on");
                    current_action = "Wheel Slip On";
                }
                else
                {
                    Settings.WS_enable_flag[Settings.table_selected] = 0;
                    SimHub.Logging.Current.Info("WheelSlip off");
                    current_action = "Wheel Slip Off";
                }
                Page_update_flag = true;
            });

            this.AddAction("RoadImpacttoggle", (a, b) =>
            {
                if (Settings.Road_impact_enable_flag[Settings.table_selected] == 0)
                {
                    Settings.Road_impact_enable_flag[Settings.table_selected] = 1;
                    SimHub.Logging.Current.Info("RoadImpact on");
                    current_action = "Wheel Slip On";
                }
                else
                {
                    Settings.Road_impact_enable_flag[Settings.table_selected] = 0;
                    SimHub.Logging.Current.Info("RoadImpact off");
                    current_action = "RoadImpact Off";
                }
                Page_update_flag = true;
            });

            this.AddAction("OverlayToggle", (a, b) =>
            {
                Page_update_flag = true;
                SimHub.Logging.Current.Info("OverlayToggle");               
                current_action = "OverlayToggle";
                if (overlay_display == 1)
                {
                    overlay_display = 0;
                }
                else
                {
                    overlay_display = 1;
                }
            });
            this.AddAction("Rudder Brake", (a, b) =>
            {
                Rudder_brake_enable_flag = true;
                SimHub.Logging.Current.Info("Rudder Brake");

            });
            
            /*
            this.AddAction("Rudder", (a, b) =>
            {

                Rudder_enable_flag=true;
                SimHub.Logging.Current.Info("Rudder action");

            });
            */

            //Settings.selectedJsonIndexLast[0]
            //SimHub.Logging.Current.Info("Diy active pedas plugin - Test 1");
            //SimHub.Logging.Current.Info("Diy active pedas plugin - COM port: " + Settings.selectedComPortNames[0]);




            // get WPF handler
            //wpfHandler = (SettingsControlDemo)GetWPFSettingsControl(pluginManager);

            //if (wpfHandler.)
            {
                // prepare serial port interfaces
                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {

                    _serialPort[pedalIdx].Handshake = Handshake.None;
                    /*
                    _serialPort[pedalIdx].RtsEnable = false;
                    _serialPort[pedalIdx].DtrEnable = false;
                    */


                    if (_serialPort[pedalIdx].IsOpen)
                    {
                        System.Threading.Thread.Sleep(300);
                    }


                    try
                    {
                        _serialPort[pedalIdx].PortName = Settings.selectedComPortNames[pedalIdx];
                    }
                    catch (Exception caughtEx)
                    {
                    }
                    
                    //try connect back to com port
                    if (Settings.auto_connect_flag[pedalIdx] == 1)
                    {

                        if (Settings.connect_status[pedalIdx] == 1)
                        {
                            //_serialPort[pedalIdx].PortName = Settings.selectedComPortNames[pedalIdx];
                            //SerialPort.GetPortNames
                            if (PortExists(_serialPort[pedalIdx].PortName))
                            {
                                if (_serialPort[pedalIdx].IsOpen == false)
                                {
                                    //if (wpfHandle != null)
                                    //{
                                    //    wpfHandle.openSerialAndAddReadCallback(pedalIdx);
                                    //}

                                    connectSerialPort[pedalIdx] = true;
                                }
                                else
                                {
                                    //if (wpfHandle != null)
                                    //{
                                    //    wpfHandle.closeSerialAndStopReadCallback(pedalIdx);
                                    //}
                                    //ConnectToPedal.IsChecked = false;
                                    //TextBox_debugOutput.Text = "Serialport already open, close it";
                                    //Settings.connect_status[pedalIdx] = 0;
                                    connectSerialPort[pedalIdx] = false;
                                }


                            }
                            else
                            {
                                //Settings.connect_status[pedalIdx] = 0;
                                connectSerialPort[pedalIdx] = false;
                            }
                        }
                        else
                        {
                            //Settings.connect_status[pedalIdx] = 0;
                            connectSerialPort[pedalIdx] = false;
                        }

                    }
                    

                }

            }

            
            
            

         


            //// check if Json config files are present, otherwise create new ones
            //for (uint jsonIndex = 0; jsonIndex < ComboBox_JsonFileSelected.Items.Count; jsonIndex++)
            //{
            //	// which config file is seleced
            //	string currentDirectory = Directory.GetCurrentDirectory();
            //	string dirName = currentDirectory + "\\PluginsData\\Common";
            //	string jsonFileName = ComboBox_JsonFileSelected(ComboBox_JsonFileSelected.Items[jsonIndex]).Text;
            //	string fileName = dirName + "\\" + jsonFileName + ".json";


            //	// Check if file already exists, otherwise create    
            //	if (!File.Exists(fileName))
            //	{
            //		// create default config
            //		// https://stackoverflow.com/questions/3275863/does-net-4-have-a-built-in-json-serializer-deserializer
            //		// https://learn.microsoft.com/en-us/dotnet/framework/wcf/feature-details/how-to-serialize-and-deserialize-json-data?redirectedfrom=MSDN
            //		var stream1 = new MemoryStream();
            //		var ser = new DataContractJsonSerializer(typeof(DAP_config_st));
            //		ser.WriteObject(stream1, dap_config_initial_st);

            //		stream1.Position = 0;
            //		StreamReader sr = new StreamReader(stream1);
            //		string jsonString = sr.ReadToEnd();

            //		System.IO.File.WriteAllText(fileName, jsonString);
            //	}
            //}



            dap_config_initial_st.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_initial_st.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_initial_st.payloadHeader_.storeToEeprom = 0;
            dap_config_initial_st.payloadPedalConfig_.pedalStartPosition = 35;
            dap_config_initial_st.payloadPedalConfig_.pedalEndPosition = 80;
            dap_config_initial_st.payloadPedalConfig_.maxForce = 50;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_initial_st.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_initial_st.payloadPedalConfig_.dampingPress = 0;
            dap_config_initial_st.payloadPedalConfig_.dampingPull = 0;
            dap_config_initial_st.payloadPedalConfig_.absFrequency = 5;
            dap_config_initial_st.payloadPedalConfig_.absAmplitude = 100;
            dap_config_initial_st.payloadPedalConfig_.absPattern = 0;
            dap_config_initial_st.payloadPedalConfig_.absForceOrTarvelBit = 0;

            dap_config_initial_st.payloadPedalConfig_.lengthPedal_a = 205;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_b = 220;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_d = 60;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_c_horizontal = 215;
            dap_config_initial_st.payloadPedalConfig_.lengthPedal_c_vertical = 60;

            dap_config_initial_st.payloadPedalConfig_.Simulate_ABS_trigger = 0;
            dap_config_initial_st.payloadPedalConfig_.Simulate_ABS_value = 50;
            dap_config_initial_st.payloadPedalConfig_.RPM_max_freq = 40;
            dap_config_initial_st.payloadPedalConfig_.RPM_min_freq = 10;
            dap_config_initial_st.payloadPedalConfig_.RPM_AMP = 5;
            dap_config_initial_st.payloadPedalConfig_.BP_trigger_value = 50;
            dap_config_initial_st.payloadPedalConfig_.BP_amp = 1;
            dap_config_initial_st.payloadPedalConfig_.BP_freq = 15;
            dap_config_initial_st.payloadPedalConfig_.BP_trigger = 0;
            dap_config_initial_st.payloadPedalConfig_.G_multi = 50;
            dap_config_initial_st.payloadPedalConfig_.G_window = 60;
            dap_config_initial_st.payloadPedalConfig_.WS_amp = 1;
            dap_config_initial_st.payloadPedalConfig_.WS_freq = 15;

            dap_config_initial_st.payloadPedalConfig_.maxGameOutput = 100;

            dap_config_initial_st.payloadPedalConfig_.kf_modelNoise = 128;
            dap_config_initial_st.payloadPedalConfig_.kf_modelOrder = 0;
            dap_config_initial_st.payloadPedalConfig_.debug_flags_0 = 0;

            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_0 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_1 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_2 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_3 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_a_4 = 0;

            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_0 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_1 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_2 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_3 = 0;
            dap_config_initial_st.payloadPedalConfig_.cubic_spline_param_b_4 = 0;

            dap_config_initial_st.payloadPedalConfig_.PID_p_gain = 0.3f;
            dap_config_initial_st.payloadPedalConfig_.PID_i_gain = 50.0f;
            dap_config_initial_st.payloadPedalConfig_.PID_d_gain = 0.0f;
            dap_config_initial_st.payloadPedalConfig_.PID_velocity_feedforward_gain = 0.0f;

            dap_config_initial_st.payloadPedalConfig_.MPC_0th_order_gain = 10.0f;
            dap_config_initial_st.payloadPedalConfig_.MPC_1st_order_gain = 0.0f;

            dap_config_initial_st.payloadPedalConfig_.control_strategy_b = 2;

            dap_config_initial_st.payloadPedalConfig_.loadcell_rating = 150;

            dap_config_initial_st.payloadPedalConfig_.travelAsJoystickOutput_u8 = 0;

            dap_config_initial_st.payloadPedalConfig_.invertLoadcellReading_u8 = 0;

            dap_config_initial_st.payloadPedalConfig_.invertMotorDirection_u8 = 0;

            dap_config_initial_st.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
            dap_config_initial_st.payloadPedalConfig_.pedal_type = 0;
            //dap_config_initial_st.payloadPedalConfig_.OTA_flag = 0;
            dap_config_initial_st.payloadPedalConfig_.stepLossFunctionFlags_u8 = 0b11;








        }
    }
}