using FMOD;
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
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Media;
using System.Net.Configuration;
using System.Runtime;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
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
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public CalculationVariables _calculations;
        public PluginManager pluginHandle;// = this;

        public bool sendAbsSignal = false;
		public DAP_config_st dap_config_initial_st;
        public byte[] rpm_last_value = new byte[4] { 0, 0, 0, 0 };
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
        //public string current_profile = "NA" ;
        //public uint profile_index = 0;
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
        //public bool Sync_esp_connection_flag=false;
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
        //public bool[] PedalConfigRead_b = new bool[3] { false, false, false };
        public bool[] isCdcSerial = new bool[3] { false, false, false };
        public List<VidPidResult> comportList = new List<VidPidResult>();
        private const float actionIntervalTolerance = 0.5f;
        public bool flightRpmEffectsStatus_last = false;
        public bool flightGforceEffects_last = false;
        public bool IsGameChanged = true;
        public ProfileService ProfileServicePlugin;
        public ConfigListService ConfigService;
        public HidDeviceController BridgeHidService;
        public string currentGame= null;
        public bool[] IsGetConfigSendRequest = new bool[3] { false, false, false };
        public DAP_config_st[] BufferConfig_st = new DAP_config_st[3] { new DAP_config_st(), new DAP_config_st(), new DAP_config_st() };
        public int ConfigSendInterval_ms = 100;
        public DateTime[] ConfigBufferGet_lastTime = new DateTime[3] { DateTime.Now, DateTime.Now, DateTime.Now };
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




        public byte[] STARTOFFRAMCHAR = { 0xAA, 0x55 };
        public byte[] ENDOFFRAMCHAR = { 0xAA, 0x56 };


        public bool[] connectSerialPort = { false, false, false };


        public DIYFFBPedalSettings Settings { get; set; }



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

        public byte[] getBytes_HidMessage(Dap_hidmessage_st aux)
        {
            int length = Marshal.SizeOf(aux);
            IntPtr ptr = Marshal.AllocHGlobal(length);
            byte[] myBuffer = new byte[length];

            Marshal.StructureToPtr(aux, ptr, true);
            Marshal.Copy(ptr, myBuffer, 0, length);
            Marshal.FreeHGlobal(ptr);

            return myBuffer;
        }

        public byte[] getBytes_Action_Ota(DAP_action_ota_st aux)
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
            string value = "Error";
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

        
        unsafe public void DataUpdate(PluginManager pluginManager, ref GameData data)
        {
			
			bool sendAbsSignal_local_b = false;
            bool sendTcSignal_local_b = false;
            double RPM_value =0;
            double RPM_MAX = 0;
            double _G_force = 128;
            byte WS_value = 0;
            byte Road_impact_value = 0;
            double CV1_value = 0;
            double CV2_value = 0;
            double CV3_value = 0;
            double CV4_value = 0;
            double MSFS_RPM_Value_Simhub = 0;
            double RUDDER_DEFLECTION_Simhub = 0;
            double RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
            double ACCELERATION_BODY_Z_Simhub = 0;
            double ACCELERATION_BODY_Y_Simhub = 0;
            bool Flight_running_simhub = false;
            
            //bool WS_flag = false;

            if (data.GamePaused || (!data.GameRunning))
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
                Current_Game = data.GameName;//(string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame");
                currentGame = data.GameName;
                if (Settings.profileAutoChange)
                {
                    if (PedalConstStrings.AutoProfileSwitchGameList.Contains(Current_Game) && IsGameChanged)
                    {
                        //set default game profile first;
                        ProfileServicePlugin.ApplyProfileAutoForGame(data.GameName);
                        IsGameChanged = false;
                    }
                    //overwrite with car profile
                    ProfileServicePlugin.ApplyProfileAutoForCar(data.NewData.CarId);

                }
                
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
                        if (data.NewData.CarSettings_MaxRPM == 0) RPM_MAX = 10000;
                        else RPM_MAX = data.NewData.CarSettings_MaxRPM;
                        RPM_value = (data.NewData.Rpms / RPM_MAX * 100);
                    }
                    else
                    {
                        if (data.NewData.MaxSpeedKmh == 0) RPM_MAX = 300;
                        else RPM_MAX = data.NewData.MaxSpeedKmh;
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

            if (data.GameRunning && data.NewData!=data.OldData)
            {
                // Send ABS trigger signal via serial
                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    DAP_action_st tmp;
                    tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    tmp.payloadHeader_.PedalTag = (byte)pedalIdx;
                    tmp.payloadPedalAction_.triggerAbs_u8 = 0;
                    tmp.payloadPedalAction_.RPM_u8 = (Byte)rpm_last_value[pedalIdx];

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
                        if (Math.Abs(RPM_value - rpm_last_value[pedalIdx]) > 3 )
                        {
                            //update rpm value every frame when HID bridge is connected
                            tmp.payloadPedalAction_.RPM_u8 = (Byte)RPM_value;
                            update_flag = true;
                            rpm_last_value[pedalIdx] = (Byte)RPM_value;
                        }
                    }
                    else
                    {
                        tmp.payloadPedalAction_.RPM_u8 = 0;
                        if (rpm_last_value[pedalIdx] != 0)
                        {
                            update_flag = true;
                        }
                        rpm_last_value[pedalIdx] = 0;
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
                            if (WS_value >= (Settings.WS_trigger))
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
                        try
                        {
                            //CV1_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV1_bindings[pedalIdx]));
                            string temp_string = Ncalc_reading(Settings.CV1_bindings[pedalIdx]);
                            if (temp_string != "Error")
                            {
                                CV1_value = Convert.ToDouble(temp_string);
                            }
                            else
                            {
                                CV1_value = 0;
                                SimHub.Logging.Current.Error("CV1 Reading error");
                            }
                            if (CV1_value > Byte.MaxValue)
                            {
                                SimHub.Logging.Current.Error("CV1 value exceed limit");
                            }

                            if (CV1_value > (Settings.CV1_trigger[pedalIdx]))
                            {
                                tmp.payloadPedalAction_.Trigger_CV_1 = 1;
                                update_flag = true;
                            }
                        }
                        catch (Exception caughtEx)
                        {
                            CV1_value = 0;
                            SimHub.Logging.Current.Error("CV1 Reading error:" + caughtEx);
                        }

                    }
                    if (Settings.CV2_enable_flag[pedalIdx] == true)
                    {

                        //CV2_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV2_bindings[pedalIdx]));
                        try
                        {
                            string temp_string = Ncalc_reading(Settings.CV2_bindings[pedalIdx]);
                            if (temp_string != "Error")
                            {
                                CV2_value = Convert.ToDouble(temp_string);
                            }
                            else
                            {
                                CV2_value = 0;
                                SimHub.Logging.Current.Error("CV2 Reading error");
                            }
                            if (CV2_value > Byte.MaxValue)
                            {
                                SimHub.Logging.Current.Error("CV2 value exceed limit");
                            }
                            if (CV2_value > (Settings.CV2_trigger[pedalIdx]))
                            {
                                tmp.payloadPedalAction_.Trigger_CV_2 = 1;
                                update_flag = true;
                            }
                        }
                        catch (Exception caughtEx)
                        {
                            CV2_value = 0;
                            SimHub.Logging.Current.Error("CV2 Reading error:"+ caughtEx);
                        }
                    }
                    if (Settings.CV3_enable_flag[pedalIdx] == true)
                    {

                        //CV2_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV2_bindings[pedalIdx]));
                        try
                        {
                            string temp_string = Ncalc_reading(Settings.CV3_bindings[pedalIdx]);
                            if (temp_string != "Error")
                            {
                                CV3_value = Convert.ToDouble(temp_string);
                            }
                            else
                            {
                                CV3_value = 0;
                                SimHub.Logging.Current.Error("CV3 Reading error");
                            }
                            if (CV3_value > Byte.MaxValue)
                            {
                                SimHub.Logging.Current.Error("CV3 value exceed limit");
                            }
                            if (CV3_value > (Settings.CV2_trigger[pedalIdx]))
                            {
                                tmp.payloadPedalAction_.Trigger_CV_3 = 1;
                                update_flag = true;
                            }
                        }
                        catch (Exception caughtEx)
                        {
                            CV3_value = 0;
                            SimHub.Logging.Current.Error("CV3 Reading error:" + caughtEx);
                        }
                    }
                    if (Settings.CV4_enable_flag[pedalIdx] == true)
                    {

                        //CV4_value = Convert.ToByte(pluginManager.GetPropertyValue(Settings.CV2_bindings[pedalIdx]));
                        try
                        {
                            string temp_string = Ncalc_reading(Settings.CV4_bindings[pedalIdx]);
                            if (temp_string != "Error")
                            {
                                CV4_value = Convert.ToDouble(temp_string);
                            }
                            else
                            {
                                CV4_value = 0;
                                SimHub.Logging.Current.Error("CV4 Reading error");
                            }
                            if (CV4_value > Byte.MaxValue)
                            {
                                SimHub.Logging.Current.Error("CV4 value exceed limit");
                            }
                            if (CV4_value > (Settings.CV2_trigger[pedalIdx]))
                            {
                                tmp.payloadPedalAction_.Trigger_CV_3 = 1;
                                update_flag = true;
                            }
                        }
                        catch (Exception caughtEx)
                        {
                            CV4_value = 0;
                            SimHub.Logging.Current.Error("CV4 Reading error:" + caughtEx);
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
                    // if connect with HID bridge, ignore the fps limit, update all the time.
                    if (update_flag && !BridgeHidService.IsConnected)
                    {
                        Action_currentTime[pedalIdx] = DateTime.Now;
                        TimeSpan diff_action = Action_currentTime[pedalIdx] - Action_lastTime[pedalIdx];
                        int millisceonds_action = (int)diff_action.TotalMilliseconds;
                        float time_interval= (1000.0f / Settings.Pedal_action_fps[pedalIdx])-actionIntervalTolerance;
                        if (millisceonds_action <= time_interval )
                        {
                            update_flag = false;
                        }
                        else
                        {
                            Action_lastTime[pedalIdx] = DateTime.Now;
                            
                        }

                        
                    }


                    if (update_flag && !Rudder_status)
                    {

                        DAP_action_st* v = &tmp;
                        tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                        tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                        tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                        tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];

                        byte* p = (byte*)v;
                        tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                        SendPedalAction(tmp, (byte)pedalIdx);
                    }
                    
                }

                if (((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2020" || ((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2024"|| ((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "DCS")
                {
                    if (((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2020" || ((string)pluginManager.GetPropertyValue("DataCorePlugin.CurrentGame")) == "FlightSimulator2024")
                    {
                        MSFS_RPM_Value_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameData.CarSettings_CurrentDisplayedRPMPercent"));
                        //RUDDER_DEFLECTION_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.RUDDER_DEFLECTION")); 
                        RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AircraftWindZ"));
                        ACCELERATION_BODY_Z_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AccelerationBodyZ"));
                        ACCELERATION_BODY_Y_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.AccelerationBodyY"));
                    }
                    else
                    {
                        MSFS_RPM_Value_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.EngineInfo.Rpm.Left"));
                        //RUDDER_DEFLECTION_Simhub = Convert.ToDouble(pluginManager.GetPropertyValue("DataCorePlugin.GameRawData.FSStatus.RUDDER_DEFLECTION")); 
                        RELATIVE_WIND_VELOCITY_BODY_Z_Simhub = 0;
                        ACCELERATION_BODY_Z_Simhub = 0;
                        ACCELERATION_BODY_Y_Simhub = 0;
                    }

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
                    ProfileServicePlugin.ClearAutoSwitchStatus();//clear auto profile switch status
                }
            }



            // Send ABS test signal if requested
            if (sendAbsSignal || _calculations.SendAbsSignal)
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
                    bool update_b = false;
                    TimeSpan diff_action =  DateTime.Now - Action_lastTime[PIDX];
                    int millisceonds_action = (int)diff_action.TotalMilliseconds;
                    float time_interval = (1000.0f / Settings.Pedal_action_fps[PIDX]) - actionIntervalTolerance;
                    if ( millisceonds_action> 30)
                    {
                        Action_lastTime[PIDX] = DateTime.Now;
                        update_b = true;
                    }

                    if (update_b)
                    {
                        tmp.payloadHeader_.PedalTag = (byte)PIDX;
                        tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                        tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                        tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                        tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];

                        DAP_action_st* v = &tmp;
                        byte* p = (byte*)v;
                        tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                        SendPedalAction(tmp, (byte)PIDX);
                        Task.Delay(2);
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
                if (_calculations.Rudder_status == false)
                {
                    _calculations.Rudder_status = true;
                }
                else
                {
                    _calculations.Rudder_status = false;
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
                if (Settings.rudderMode == 0)
                {
                    if (Rudder_Pedal_idx[0] == 0)
                    {
                        tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableRudderThreePedals;
                    }
                    else
                    {
                        tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableRudderTwoPedals;
                    }
                }
                if (Settings.rudderMode == 1)
                {
                    if (Rudder_Pedal_idx[0] == 0)
                    {
                        tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableHeliRudderThreePedals;
                    }
                    else
                    {
                        tmp.payloadPedalAction_.Rudder_action = (byte)RudderAction.EnableHeliRudderTwoPedals;
                    }
                }

                
                tmp.payloadPedalAction_.Rudder_brake_action = 0;

                for (uint i = 0; i < 2; i++)
                {
                    uint PIDX = Rudder_Pedal_idx[i];
                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                    DAP_action_st* v = &tmp;
                    tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                    tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                    tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                    tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    SendPedalAction(tmp, (byte)PIDX);
                }
                Rudder_enable_flag = false;
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
                                flightRpmEffectsStatus_last = true;
                                byte Rudder_RPM_value = 0;
                                if (MSFS_Plugin_Status)
                                {
                                    Rudder_RPM_value = Convert.ToByte(pluginManager.GetPropertyValue("FlightPlugin.FlightData.GENERAL_ENG_PCT_MAX_RPM_1"));
                                }
                                else
                                {
                                    if (Flight_running_simhub)
                                    {
                                        Rudder_RPM_value = (byte)MSFS_RPM_Value_Simhub;
                                    }
                                }




                                if (Math.Abs(Rudder_RPM_value - Rudder_RPM_Effect_last_value) > 3)
                                {
                                    tmp.payloadPedalAction_.RPM_u8 = Rudder_RPM_value;
                                    Rudder_Effect_update_b = true;
                                    Rudder_Action_lastTime = DateTime.Now;
                                    Rudder_RPM_Effect_last_value = Rudder_RPM_value;
                                }
                            }
                            else
                            {
                                if (Rudder_RPM_Effect_last_value!=0)
                                {
                                    Rudder_RPM_Effect_last_value = 0;
                                    Rudder_Effect_update_b = true;
                                    tmp.payloadPedalAction_.RPM_u8 = Rudder_RPM_Effect_last_value;
                                }
                                Rudder_RPM_Effect_last_value = 0;
                            }

                            if (Settings.Rudder_ACC_effect_b)
                            {
                                flightGforceEffects_last = true;
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
                                if (Flight_running_simhub && !MSFS_Plugin_Status)
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
                                Rudder_G_percent = Rudder_G_constrain / max_G * 100.0d;
                                double Rudder_G_Wind_combined = Math.Min(Rudder_G_percent + Rudder_Wind_Froce_Ratio, max_G);

                                if (Math.Abs(Rudder_G_last_value - Rudder_G_percent) > 2)
                                {
                                    tmp.payloadPedalAction_.impact_value = (Byte)Rudder_G_Wind_combined;
                                    Rudder_Effect_update_b = true;
                                    Rudder_Action_lastTime = DateTime.Now;
                                    Rudder_G_last_value = (Byte)Rudder_G_Wind_combined;
                                }
                            }
                            else
                            {
                                if (Rudder_G_last_value!=0)
                                {
                                    Rudder_G_last_value = 0;
                                    tmp.payloadPedalAction_.impact_value = Rudder_G_last_value;
                                    Rudder_Effect_update_b = true;
                                }
                                Rudder_G_last_value = 0;

                            }




                            //Write to Pedal
                            if (Rudder_Effect_update_b)
                            {
                                for (uint i = 0; i < 2; i++)
                                {
                                    uint PIDX = Rudder_Pedal_idx[i];
                                    tmp.payloadHeader_.PedalTag = (byte)PIDX;
                                    tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                                    tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                                    tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                                    tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                                    DAP_action_st* v = &tmp;
                                    byte* p = (byte*)v;
                                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                                    SendPedalAction(tmp, (byte)PIDX);
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
                    tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                    tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                    tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                    tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    SendPedalAction(tmp, (byte)PIDX);
                    Rudder_brake_enable_flag = false;
                    
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
                
                Road_impact_last = 0;
                debug_value = 0;

                for (uint pedalIdx = 0; pedalIdx < 3; pedalIdx++)
                {
                    rpm_last_value[pedalIdx] = 0;

                    tmp.payloadHeader_.PedalTag = (byte)pedalIdx;
                    DAP_action_st* v = &tmp;
                    tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                    tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                    tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                    tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                    SendPedalAction(tmp, (byte)pedalIdx);
                }
                clear_action = false;
            }

            //config sending queue function
            if (data.GameRunning) ConfigSendInterval_ms = 100;
            else ConfigSendInterval_ms = 30;
            ConfigSendingQueue();
            
            this.AttachDelegate("CurrentProfile", () => _calculations.ProfileSelected);
            pluginManager.SetPropertyValue("SelectedPedal", this.GetType(), current_pedal);
            pluginManager.SetPropertyValue("Action", this.GetType(), current_action);
            pluginManager.SetPropertyValue("ABS_effect_status", this.GetType(), Settings.ABS_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("RPM_effect_status", this.GetType(), Settings.RPM_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("Gforce_effect_status", this.GetType(), Settings.G_force_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("WheelSlip_effect_status", this.GetType(), Settings.WS_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("RoadImpact_effect_status", this.GetType(), Settings.Road_impact_enable_flag[Settings.table_selected]);
            pluginManager.SetPropertyValue("Overlay_display", this.GetType(), overlay_display);
            pluginManager.SetPropertyValue("Theme_color", this.GetType(), simhub_theme_color);
            pluginManager.SetPropertyValue("ProfileIndex", this.GetType(), _calculations.ProfileIndex);
            pluginManager.SetPropertyValue("debugvalue", this.GetType(), debug_value);
            pluginManager.SetPropertyValue("rudder_status", this.GetType(), Rudder_status);
            pluginManager.SetPropertyValue("rudder_brake_status", this.GetType(), Rudder_brake_status);
            pluginManager.SetPropertyValue("pedal_position", this.GetType(), pedal_state_in_ratio);
            pluginManager.SetPropertyValue("PedalErrorIndex", this.GetType(), PedalErrorIndex);
            pluginManager.SetPropertyValue("PedalErrorCode", this.GetType(), PedalErrorCode);
            pluginManager.SetPropertyValue("FlightRudder_G", this.GetType(), Rudder_G_last_value);
            pluginManager.SetPropertyValue("FlightRudder_Wind_Force", this.GetType(), Rudder_Wind_Force_last_value);
            this.AttachDelegate("CustomEffectTirggerReading1", () => CV1_value);
            this.AttachDelegate("CustomEffectTirggerReading2", () => CV2_value);
            this.AttachDelegate("WheelSlipEffectReading", () => WS_value);
            this.AttachDelegate("RoadImpactEffectReading", () => Road_impact_value);
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
            //disable rudder once rudder is in aciton
            if (Rudder_status)
            {
                if (wpfHandle != null)
                {
                    try
                    {
                        PedalRestartAction();
                    }
                    catch (Exception caughtEx)
                    {
                    }
                }

            }
            // Save settings
            this.SaveCommonSettings("GeneralSettings", Settings);
            BridgeHidService.Dispose();
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
            /*
            if (ToastNotificationManager.History.GetHistory("Pedal_notification").Count != 0)
            {
                ToastNotificationManager.History.Remove("Pedal_notification");
            }
            */
            

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
            _calculations = new CalculationVariables();
            SimHub.Logging.Current.Info("Starting DIY active pedal plugin");

            // Load settings
            Settings = this.ReadCommonSettings<DIYFFBPedalSettings>("GeneralSettings", () => new DIYFFBPedalSettings());
            Simhub_version = (String)pluginManager.GetPropertyValue("DataCorePlugin.SimHubVersion");
            // Declare a property available in the property list, this gets evaluated "on demand" (when shown or used in formulas)
            //this.AttachDelegate("CurrentDateTime", () => DateTime.Now);
            pluginManager.AddProperty("ProfileIndex", this.GetType(), _calculations.ProfileIndex);
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
            ProfileServicePlugin = new ProfileService(this);
            ConfigService = new ConfigListService(this);
            BridgeHidService = new HidDeviceController(Constants.VendorId, Constants.BridgePid, Constants.TargetUsagePage);
            EnsureFolderExistsAndProcess();
            DefaultConfigInitializing();
            DefaultProfile = new DAP_system_profile_cls();

            for (uint pedali = 0; pedali < 3; pedali++)
            {
                Action_currentTime[pedali] = new DateTime();
                Action_currentTime[pedali] = DateTime.Now;
                Action_lastTime[pedali] = new DateTime();
                Action_lastTime[pedali] = DateTime.Now;
            }

            //get version length
            int Version_text_length = Simhub_version.Length;
            //if (Simhub_version[Version_text_length - 2] == 'b')
            if (Simhub_version.Contains("b"))
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

            this.AddAction("ApplyProfile", (a, b) =>
            {
                var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileSelected);
                if (foundItem != null) ProfileServicePlugin.ApplyProfile(foundItem.FullPath);
                _calculations.ProfileEditing = _calculations.ProfileSelected;
                wpfHandle.SystemProfile_TabNew.ApplyProfileOnUiWithPath(foundItem.FullPath);
                SimHub.Logging.Current.Info("Apply Profile");
                current_action = "Apply Profile";
            });
            for (int i = 0; i < 6; i++)
            {
                int index = i;
                if (Settings.ProfileShortcutName[index] != string.Empty)
                {
                    this.AddAction(Settings.ProfileShortcutName[index], (a, b) =>
                    {
                        var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == Settings.ProfileShortcut[index]);
                        if (foundItem != null)
                        {
                            _calculations.ProfileIndex = this.ProfileServicePlugin.ProfileList.IndexOf(foundItem);
                            ProfileServicePlugin.ApplyProfile(foundItem.FullPath);
                            _calculations.ProfileEditing = foundItem.FileName;
                            _calculations.ProfileSelected = _calculations.ProfileEditing;
                            wpfHandle.SystemProfile_TabNew.ApplyProfileOnUiWithPath(foundItem.FullPath);
                            wpfHandle.ToastNotification("Profile:"+ foundItem.FileName,"Applied");
                        }

                        SimHub.Logging.Current.Info("Apply" + Settings.ProfileShortcutName[index]);
                        current_action = "Apply" + Settings.ProfileShortcutName[index];
                    });
                }
            }


            this.AddAction("PreviousProfile", (a, b) =>
            {
                if(ProfileServicePlugin.ProfileList.Count() > 0)
                {
                    if (_calculations.ProfileIndex == -1)
                    {
                        if (_calculations.ProfileEditing != string.Empty)
                        {
                            var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileEditing);
                            _calculations.ProfileIndex = this.ProfileServicePlugin.ProfileList.IndexOf(foundItem);

                        }
                        else
                        {
                            _calculations.ProfileIndex = 0;

                        }
                    }
                    else
                    {
                        var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileEditing);
                        _calculations.ProfileIndex = this.ProfileServicePlugin.ProfileList.IndexOf(foundItem);
                        if (_calculations.ProfileIndex == 0)
                        {
                            _calculations.ProfileIndex = ProfileServicePlugin.ProfileList.Count()-1;
                        }
                        else
                        {
                            _calculations.ProfileIndex--;
                        }
                    }
                    //ApplyProfile(ProfileList[_calculations.ProfileIndex].FullPath);
                    _calculations.ProfileEditing = ProfileServicePlugin.ProfileList[_calculations.ProfileIndex].FileName;
                    _calculations.ProfileSelected = _calculations.ProfileEditing;
                }
                SimHub.Logging.Current.Info("PreviousProfile");
                current_action = "Previous Profile";
            });

            this.AddAction("NextProfile", (a, b) =>
            {
                if (ProfileServicePlugin.ProfileList.Count() > 0)
                {
                    if (_calculations.ProfileIndex == -1)
                    {
                        if (_calculations.ProfileEditing != string.Empty)
                        {
                            var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileEditing);
                            _calculations.ProfileIndex = this.ProfileServicePlugin.ProfileList.IndexOf(foundItem);

                        }
                        else
                        {
                            _calculations.ProfileIndex = 0;

                        }
                    }
                    else
                    {
                        var foundItem = this.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileEditing);
                        _calculations.ProfileIndex = this.ProfileServicePlugin.ProfileList.IndexOf(foundItem);
                        _calculations.ProfileIndex++;
                        if (_calculations.ProfileIndex > ProfileServicePlugin.ProfileList.Count - 1) _calculations.ProfileIndex = 0;
                    }
                    //ApplyProfile(ProfileList[_calculations.ProfileIndex].FullPath);
                    _calculations.ProfileEditing = ProfileServicePlugin.ProfileList[_calculations.ProfileIndex].FileName;
                    _calculations.ProfileSelected = _calculations.ProfileEditing;
                }
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
            this.AddAction("GforceToggle", (a, b) =>
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
            this.AddAction("Log Pedal State", (a, b) =>
            {
                if (!_calculations.dumpPedalToResponseFile[Settings.table_selected])
                {
                    _calculations.dumpPedalToResponseFile[Settings.table_selected] = true;
                }
                else
                {
                    _calculations.dumpPedalToResponseFile[Settings.table_selected] = false;
                }
                wpfHandle.ToastNotification("Log Pedal State:"+ _calculations.dumpPedalToResponseFile[Settings.table_selected], "Pedal:" + Settings.table_selected);
                SimHub.Logging.Current.Info("Log pedal state for pedal: " + Settings.table_selected);

            });




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



            








        }
    }
}