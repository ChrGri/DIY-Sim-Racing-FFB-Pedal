using Newtonsoft.Json;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Linq.Expressions;
using System.Net.Http;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Threading;
using Windows.UI.Notifications;
using static User.PluginSdkDemo.ComPortHelper;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
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

        private void UpdateSerialPortList_click()
        {

            var SerialPortSelectionArray = new List<SerialPortChoice>();
            
            string[] comPorts = SerialPort.GetPortNames();
            var SerialPortList = new List<string>();
            comPorts = comPorts.Distinct().ToArray(); // unique
            Plugin.comportList.Clear();
            SerialPortList.Clear();
     
            if (comPorts.Length > 0)
            {

                foreach (string portName in comPorts)
                {
                    
                    //SerialPortSelectionArray.Add(new SerialPortChoice(portName, portName));
                    //int index = Plugin.comportList.FindIndex(item => item.ComPortName == portName);
                    var parseResult= ComPortHelper.GetVidPidFromComPort(portName);
                    Plugin.comportList.Add(parseResult);
                    var portDeviceName = portName+" "+parseResult.DeviceName;
                    //SerialPortList.Add((string)Plugin.comportList[index].DeviceName);
                    SerialPortSelectionArray.Add(new SerialPortChoice(portDeviceName, portName));
                    

                }
                
            }
            else
            {
                SerialPortSelectionArray.Add(new SerialPortChoice("NA", "NA"));
            }

            SerialPortSelection.DataContext = SerialPortSelectionArray;
            //SerialPortSelection.DataContext = SerialPortList;
            SerialPortSelection_ESPNow.DataContext = SerialPortSelectionArray;
            

        }



        public void DAP_config_set_default(uint pedalIdx)
        {
            //dumpPedalToResponseFile[pedalIdx] = false;
            //dumpPedalToResponseFile_clearFile[pedalIdx] = false;
            dap_config_st[pedalIdx].payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            dap_config_st[pedalIdx].payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedalStartPosition = 35;
            dap_config_st[pedalIdx].payloadPedalConfig_.pedalEndPosition = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.maxForce = 50;
            dap_config_st[pedalIdx].payloadPedalConfig_.preloadForce = 0;
            /*
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce_p100 = 100;
            */
            dap_config_st[pedalIdx].payloadPedalConfig_.quantityOfControl = 6;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce00 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce01 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce02 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce03 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce04 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce06 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce07 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce08 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce09 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeForce10 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel00 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel01 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel02 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel03 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel04 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel05 = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel06 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel07 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel08 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel09 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.relativeTravel10 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.numOfJoystickMapControl = 6;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped00 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped01 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped02 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped03 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped04 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped05 = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped06 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped07 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped08 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped09 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapMapped10 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig00 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig01 = 20;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig02 = 40;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig03 = 60;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig04 = 80;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig05 = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig06 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig07 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig08 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig09 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.joystickMapOrig10 = 0;
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
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_freq_1 = 10;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_amp_2 = 0;
            dap_config_st[pedalIdx].payloadPedalConfig_.CV_freq_2 = 10;
            dap_config_st[pedalIdx].payloadPedalConfig_.maxGameOutput = 100;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelNoise = 128;
            dap_config_st[pedalIdx].payloadPedalConfig_.kf_modelOrder = 0;
            /*
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
            */

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
            /*
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p000 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p020 = 20;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p040 = 40;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p060 = 60;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p080 = 80;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce_p100 = 100;
            */

            dap_config_st_rudder.payloadPedalConfig_.quantityOfControl = 6;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce00 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce01 = 20;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce02 = 40;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce03 = 60;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce04 = 80;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce06 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce07 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce08 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce09 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce10 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel00 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel01 = 20;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel02 = 40;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel03 = 60;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel04 = 80;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel05 = 100;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel06 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel07 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel08 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel09 = 0;
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel10 = 0;


            dap_config_st_rudder.payloadPedalConfig_.numOfJoystickMapControl = 6;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped00 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped01 = 20;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped02 = 40;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped03 = 60;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped04 = 80;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped05 = 100;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped06 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped07 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped08 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped09 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapMapped10 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig00 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig01 = 20;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig02 = 40;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig03 = 60;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig04 = 80;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig05 = 100;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig06 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig07 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig08 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig09 = 0;
            dap_config_st_rudder.payloadPedalConfig_.joystickMapOrig10 = 0;

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
            dap_config_st_rudder.payloadPedalConfig_.CV_freq_1 = 10;
            dap_config_st_rudder.payloadPedalConfig_.CV_amp_2 = 0;
            dap_config_st_rudder.payloadPedalConfig_.CV_freq_2 = 10;

            dap_config_st_rudder.payloadPedalConfig_.maxGameOutput = 100;
            dap_config_st_rudder.payloadPedalConfig_.kf_modelNoise = 200;
            dap_config_st_rudder.payloadPedalConfig_.kf_modelOrder = 2;
            /*
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
            */

            dap_config_st_rudder.payloadPedalConfig_.PID_p_gain = 0.01f;
            dap_config_st_rudder.payloadPedalConfig_.PID_i_gain = 1.0f;
            dap_config_st_rudder.payloadPedalConfig_.PID_d_gain = 0.0002f;
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
            dap_config_st_rudder.payloadPedalConfig_.servoIdleTimeout = 0;
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
                        diff = ConfigLiveSending_now - PedalTabChange_last;
                        int millseconds_pedaltabchange = (int)diff.TotalMilliseconds;
                        if (millseconds_pedaltabchange > 100)
                        {
                            PedalTabChange = false;
                            PedalTabChange_last = DateTime.Now;

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

            //TextBox_debugOutput.Text = String.Empty;
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
                        TextBox2.Text = errorMessage;
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
                        TextBox2.Text = errorMessage;
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
                    //TextBox_debugOutput.Text = "config was sent to pedal";
                }
            }

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



        public void openSerialAndAddReadCallback(uint pedalIdx)
        {
            try
            {
                /*
                VidPidResult info = ComPortHelper.GetVidPidFromComPort(Plugin._serialPort[pedalIdx].PortName);

                if (info.Found)
                {
                    MessageBox.Show(Plugin._serialPort[pedalIdx].PortName+"\nVID: " + info.Vid + "\nPID: " + info.Pid+ "\n Device Name:"+info.DeviceName);
                }
                else
                {
                    MessageBox.Show("Can't found"+ Plugin._serialPort[pedalIdx].PortName);
                }
                */

                // serial port settings
                //Plugin._serialPort[pedalIdx].BaudRate = 921600;
                var serialInfo = ComPortHelper.GetVidPidFromComPort(Plugin._serialPort[pedalIdx].PortName);
                if (serialInfo.Vid == "1A86" && serialInfo.Pid == "55D3")
                {
                    //target CH343
                    //change baud here
                    Plugin._serialPort[pedalIdx].BaudRate = Constants.BAUD3M;
                    //MessageBox.Show("CH343 connected");
                }
                else
                {
                    Plugin._serialPort[pedalIdx].BaudRate = Constants.DEFAULTBAUD;
                }
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
                if (Plugin.Settings.auto_connect_flag[pedalIdx] == 1 & Plugin.Settings.connect_flag[pedalIdx] == 1)
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
                    Plugin.Settings.autoconnectComPortNames[pedalIdx] = Plugin.Settings.selectedComPortNames[pedalIdx];
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
                        Plugin._calculations.PedalSerialAvailability[pedalIdx] = true;
                    }
                    catch (Exception ex)
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
                    Plugin._calculations.PedalSerialAvailability[pedalIdx] = false;
                }
            }
            catch (Exception ex)
            { }




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
                Profile_change(Plugin._calculations.profile_index);
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
        public void Profile_change(uint profile_index)
        {
            profile_select = profile_index;
            //ProfileTab.SelectedIndex = (int)profile_index;
            //if (Plugin.Settings.file_enable_check[profile_select])
            Parsefile(profile_index);
            string tmp;
            switch (profile_index)
            {
                case 0:
                    tmp = "A:" + Plugin.Settings.Profile_name[profile_index];
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
            Plugin._calculations.current_profile = tmp;
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
                CurveRudderForce_Tab.text_rudder_log.Text += "Read Config from" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[0]] + "\n";
            });

            DelayCall(600, () =>
            {
                Reading_config_auto(Plugin.Rudder_Pedal_idx[1]);//read gas config from pedal
                CurveRudderForce_Tab.text_rudder_log.Text += "Read Config from" + Rudder_Pedal_idx_Name[Plugin.Rudder_Pedal_idx[1]] + "\n";
            });
            //System.Threading.Thread.Sleep(200);
            DelayCall((int)(900), () =>
            {
                readRudderSettingToConfig();
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

        private async Task<DAP_config_st> GetProfileDataAsync(string url)
        {
            using (HttpClient client = new HttpClient())
            {
                string jsonString = await client.GetStringAsync(url);
                //return JsonConvert.DeserializeObject<Profile_Online>(jsonString);
                return JsonConvert.DeserializeObject<DAP_config_st>(jsonString);
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

        public async void CheckForUpdateAsync()
        {
            try
            {
                using (var client = new HttpClient())
                {
                    string json = await client.GetStringAsync(Constants.version_control_url);
                    JObject obj = JObject.Parse(json);
                    var results = new List<string>();

                    for (int i = 0; i < Plugin._calculations.updateChannelString.Length; i++)
                    {
                        string channel = Plugin._calculations.updateChannelString[i];
                        if (obj.ContainsKey(channel))
                        {
                            Plugin._calculations.pluginVersionReading[i] = (string)obj[channel]["version"];
                        }
                        else
                        {
                            Plugin._calculations.pluginVersionReading[i] = "N/A";
                        }
                    }
                    Plugin._calculations.versionCheck_b = true;
                    
                }
                //textBox_VersionUpdate.Text = "Stable:"+ Plugin._calculations.pluginVersionReading[0]+" nightly:"+ Plugin._calculations.pluginVersionReading[1]; ;

            }
            catch (Exception ex)
            {
                //MessageBox.Show($"Error:{ex.Message}");
                Plugin._calculations.versionCheck_b = false;
            }
        }

        public void readRudderSettingToConfig()
        {
            dap_config_st_rudder.payloadPedalConfig_.quantityOfControl=Plugin.Settings.rudderControlQuantity;
            dap_config_st_rudder.payloadPedalConfig_.relativeForce00 = Plugin.Settings.rudderForce[0];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce01 = Plugin.Settings.rudderForce[1];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce02 = Plugin.Settings.rudderForce[2];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce03 = Plugin.Settings.rudderForce[3];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce04 = Plugin.Settings.rudderForce[4];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce05 = Plugin.Settings.rudderForce[5];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce06 = Plugin.Settings.rudderForce[6];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce07 = Plugin.Settings.rudderForce[7];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce08 = Plugin.Settings.rudderForce[8];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce09 = Plugin.Settings.rudderForce[9];
            dap_config_st_rudder.payloadPedalConfig_.relativeForce10 = Plugin.Settings.rudderForce[10];

            dap_config_st_rudder.payloadPedalConfig_.relativeTravel00 = Plugin.Settings.rudderTravel[0];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel01 = Plugin.Settings.rudderTravel[1];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel02 = Plugin.Settings.rudderTravel[2];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel03 = Plugin.Settings.rudderTravel[3];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel04 = Plugin.Settings.rudderTravel[4];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel05 = Plugin.Settings.rudderTravel[5];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel06 = Plugin.Settings.rudderTravel[6];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel07 = Plugin.Settings.rudderTravel[7];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel08 = Plugin.Settings.rudderTravel[8];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel09 = Plugin.Settings.rudderTravel[9];
            dap_config_st_rudder.payloadPedalConfig_.relativeTravel10 = Plugin.Settings.rudderTravel[10];

            dap_config_st_rudder.payloadPedalConfig_.dampingPress = Plugin.Settings.rudderDamping;
            dap_config_st_rudder.payloadPedalConfig_.dampingPull = Plugin.Settings.rudderDamping;
            dap_config_st_rudder.payloadPedalConfig_.maxForce = Plugin.Settings.rudderMaxForce;
            dap_config_st_rudder.payloadPedalConfig_.preloadForce = Plugin.Settings.rudderMinForce;
            dap_config_st_rudder.payloadPedalConfig_.pedalStartPosition = Plugin.Settings.rudderMinTravel;
            dap_config_st_rudder.payloadPedalConfig_.pedalEndPosition = Plugin.Settings.rudderMaxTravel;
            dap_config_st_rudder.payloadPedalConfig_.MPC_0th_order_gain = Plugin.Settings.rudderMPCGain;
            dap_config_st_rudder.payloadPedalConfig_.RPM_max_freq= Plugin.Settings.rudderRPMMaxFrequency;
            dap_config_st_rudder.payloadPedalConfig_.RPM_min_freq = Plugin.Settings.rudderRPMMinFrequency;
            dap_config_st_rudder.payloadPedalConfig_.RPM_AMP = Plugin.Settings.rudderRPMAmp;
        }
        public void writeRudderConfigToSetting()
        {
            Plugin.Settings.rudderControlQuantity = dap_config_st_rudder.payloadPedalConfig_.quantityOfControl;
            Plugin.Settings.rudderForce[0]= dap_config_st_rudder.payloadPedalConfig_.relativeForce00;
            Plugin.Settings.rudderForce[1]= dap_config_st_rudder.payloadPedalConfig_.relativeForce01;
            Plugin.Settings.rudderForce[2] = dap_config_st_rudder.payloadPedalConfig_.relativeForce02;
            Plugin.Settings.rudderForce[3] = dap_config_st_rudder.payloadPedalConfig_.relativeForce03;
            Plugin.Settings.rudderForce[4] = dap_config_st_rudder.payloadPedalConfig_.relativeForce04;
            Plugin.Settings.rudderForce[5] = dap_config_st_rudder.payloadPedalConfig_.relativeForce05;
            Plugin.Settings.rudderForce[6] = dap_config_st_rudder.payloadPedalConfig_.relativeForce06;
            Plugin.Settings.rudderForce[7] = dap_config_st_rudder.payloadPedalConfig_.relativeForce07;
            Plugin.Settings.rudderForce[8] = dap_config_st_rudder.payloadPedalConfig_.relativeForce08;
            Plugin.Settings.rudderForce[9] = dap_config_st_rudder.payloadPedalConfig_.relativeForce09;
            Plugin.Settings.rudderForce[10] = dap_config_st_rudder.payloadPedalConfig_.relativeForce10;

            Plugin.Settings.rudderTravel[0] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel00;
            Plugin.Settings.rudderTravel[1] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel01;
            Plugin.Settings.rudderTravel[2] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel02;
            Plugin.Settings.rudderTravel[3] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel03;
            Plugin.Settings.rudderTravel[4] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel04;
            Plugin.Settings.rudderTravel[5] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel05;
            Plugin.Settings.rudderTravel[6] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel06;
            Plugin.Settings.rudderTravel[7] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel07;
            Plugin.Settings.rudderTravel[8] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel08;
            Plugin.Settings.rudderTravel[9] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel09;
            Plugin.Settings.rudderTravel[10] = dap_config_st_rudder.payloadPedalConfig_.relativeTravel10;

            Plugin.Settings.rudderDamping = dap_config_st_rudder.payloadPedalConfig_.dampingPress;
            Plugin.Settings.rudderMaxForce = dap_config_st_rudder.payloadPedalConfig_.maxForce;
            Plugin.Settings.rudderMinForce = dap_config_st_rudder.payloadPedalConfig_.preloadForce;
            Plugin.Settings.rudderMinTravel = dap_config_st_rudder.payloadPedalConfig_.pedalStartPosition;
            Plugin.Settings.rudderMaxTravel = dap_config_st_rudder.payloadPedalConfig_.pedalEndPosition;
            Plugin.Settings.rudderMPCGain = dap_config_st_rudder.payloadPedalConfig_.MPC_0th_order_gain;
            Plugin.Settings.rudderRPMMaxFrequency = dap_config_st_rudder.payloadPedalConfig_.RPM_max_freq;
            Plugin.Settings.rudderRPMMinFrequency = dap_config_st_rudder.payloadPedalConfig_.RPM_min_freq;
            Plugin.Settings.rudderRPMAmp = dap_config_st_rudder.payloadPedalConfig_.RPM_AMP;
        }
    }
}
