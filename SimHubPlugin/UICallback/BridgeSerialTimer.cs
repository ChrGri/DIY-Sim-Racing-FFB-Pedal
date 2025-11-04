using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Ports;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Runtime.CompilerServices;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        
        unsafe public void timerCallback_serial_esphost(object sender, EventArgs e)
        {

            //action here 
            Simhub_action_update();

            int bridgeBufferIndex = 3; // bridge is buffer 3




            //int pedalSelected = Int32.Parse((sender as System.Windows.Forms.Timer).Tag.ToString());
            //int pedalSelected = (int)(sender as System.Windows.Forms.Timer).Tag;

            bool pedalStateHasAlreadyBeenUpdated_b = false;
            if (Plugin.Settings.Serial_auto_clean_bridge)
            {
                if (TextBox_serialMonitor_bridge.LineCount > 100)
                {
                    TextBox_serialMonitor_bridge.Clear();
                }
                /*
                if (TextBox_serialMonitor.LineCount > 100)
                {
                    TextBox_serialMonitor.Clear();
                }*/
                if (_serial_monitor_window != null && _serial_monitor_window.TextBox_SerialMonitor.LineCount > 100)
                {
                    _serial_monitor_window.TextBox_SerialMonitor.Clear();
                }
            }
            try
            {
                // Create a Stopwatch instance
                Stopwatch stopwatch = new Stopwatch();
                // Start the stopwatch
                stopwatch.Start();
                SerialPort sp = Plugin.ESPsync_serialPort;
                // https://stackoverflow.com/questions/9732709/the-calling-thread-cannot-access-this-object-because-a-different-thread-owns-it
                if (sp.IsOpen)
                {
                    int receivedLength = 0;
                    try
                    {
                        //receivedLength = sp.BytesToRead;
                        receivedLength = Math.Min(sp.BytesToRead, bufferSize);
                    }
                    catch (Exception ex)
                    {
                        //TextBox_debugOutput.Text = ex.Message;
                        //ConnectToPedal.IsChecked = false;
                        return;
                    }





                    if ((receivedLength > 0) && (receivedLength < bufferSize))
                    {

                        //TextBox_serialMonitor.Text += "Received:" + receivedLength + "\n";
                        //TextBox_serialMonitor.ScrollToEnd();

                        
						
						
                        timeCntr[bridgeBufferIndex] += 1;


                        // determine byte sequence which is defined as message end --> crlf
                        byte[] byteToFind = System.Text.Encoding.GetEncoding(28591).GetBytes(STOPCHAR[0].ToCharArray());
                        int stop_char_length = byteToFind.Length;

                        // check if buffer is large enough otherwise discard in buffer and set offset to 0
                        //if ((bufferSize > currentBufferLength) && (appendedBufferOffset[pedalSelected] >= 0))
                        // Copy all bytes
                        Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], 0, buffer_appended_clone[bridgeBufferIndex], 0, bufferSize);

                        int prevOffset = appendedBufferOffset[bridgeBufferIndex];

                        if (appendedBufferOffset[bridgeBufferIndex] > 0)
                        {
                            int tmp = 5;
                        }

                        bool inBufferDicarded = false;
                        int currentBufferLength = 0;
                        if (bufferSize > currentBufferLength)
                        {
                            sp.Read(buffer_appended[bridgeBufferIndex], appendedBufferOffset[bridgeBufferIndex], receivedLength);

                            // calculate current buffer length
                            appendedBufferOffset[bridgeBufferIndex] += receivedLength;
                            currentBufferLength = appendedBufferOffset[bridgeBufferIndex];

                            Array.Clear(buffer_appended[bridgeBufferIndex], currentBufferLength, bufferSize - currentBufferLength);
                        }
                        else
                        {
                            inBufferDicarded = true;
                            sp.DiscardInBuffer();
                            appendedBufferOffset[bridgeBufferIndex] = 0;
                            return;
                        }


                        if (!((buffer_appended[bridgeBufferIndex][0] == 170) && (buffer_appended[3][1] == 85)))
                        {
                            int tmp = 5;
                        }





                        // copy to local buffer
                        //byte[] localBuffer = new byte[currentBufferLength];

                        //Buffer.BlockCopy(buffer_appended[pedalSelected], 0, localBuffer, 0, currentBufferLength);


                        // find all occurences of crlf as they indicate message end
                        List<int> indices = FindAllOccurrences(buffer_appended[bridgeBufferIndex], byteToFind, currentBufferLength);


                        List<int> indices_sof = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAMCHAR, currentBufferLength);
                        List<int> indices_sof_extended_struct = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAME_EXTENDED_STRUCT, currentBufferLength);
                        List<int> indices_sof_basic_struct = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAME_BASIC_STRUCT, currentBufferLength);
                        List<int> indices_sof_bridge_basic_struct = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAME_BRIDGE_BASIC_STRUCT, currentBufferLength);
                        List<int> indices_sof_config = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAME_CONFIG, currentBufferLength);
                        List<int> indices_eof = FindAllOccurrences(buffer_appended[bridgeBufferIndex], ENDOFFRAMCHAR, currentBufferLength);

                        var validPairsExtendedStruct = new List<Tuple<int, int>>();
                        var validPairsBasicStruct = new List<Tuple<int, int>>();
                        var validPairsConfig = new List<Tuple<int, int>>();
                        var validPairsBridgeState = new List<Tuple<int, int>>();

                        bool sofHasBeenReceivedEofNotYet = false;
                        byte[] bufferByteAssignedToStruct_class = new byte[bufferSize];
                        bool[] bufferByteAssignedToStruct = new bool[bufferSize];

                        // Search for the basic struct
                        FindValidMessagePairs(
                            indices_sof_basic_struct,
                            indices_eof,
                            sizeof(DAP_state_basic_st),
                            validPairsBasicStruct,
                            ref sofHasBeenReceivedEofNotYet,
                            bufferByteAssignedToStruct_class,
                            1);

                        // Search for the extended struct
                        FindValidMessagePairs(
                            indices_sof_extended_struct,
                            indices_eof,
                            sizeof(DAP_state_extended_st),
                            validPairsExtendedStruct,
                            ref sofHasBeenReceivedEofNotYet,
                            bufferByteAssignedToStruct_class,
                            2);

                        // Search for the config struct
                        FindValidMessagePairs(
                            indices_sof_config,
                            indices_eof,
                            sizeof(DAP_config_st),
                            validPairsConfig,
                            ref sofHasBeenReceivedEofNotYet,
                            bufferByteAssignedToStruct_class,
                            3);
                        // Search for the bridge state struct
                        FindValidMessagePairs(
                            indices_sof_bridge_basic_struct,
                            indices_eof,
                            sizeof(DAP_bridge_state_st),
                            validPairsBridgeState,
                            ref sofHasBeenReceivedEofNotYet,
                            bufferByteAssignedToStruct_class,
                            4);
                        // check if at least SOF1 byte was received, but EOF was not for last packet
                        List<int> indices_sof1 = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAMCHAR_SOF_byte0, currentBufferLength);
                        List<int> indices_sof1_and_sof2 = FindAllOccurrences(buffer_appended[bridgeBufferIndex], STARTOFFRAMCHAR, currentBufferLength);
                        // when last element is SOF1

                        try
                        {
                            if ((currentBufferLength - 1) == indices_sof1.Last<int>())
                            {
                                bufferByteAssignedToStruct.AsSpan((currentBufferLength - 1), 1).Fill(true);
                                sofHasBeenReceivedEofNotYet = true;
                            }

                            // when last element is SOF2 and seconmd to last is SOF1
                            if ((currentBufferLength - 2) == indices_sof1_and_sof2.Last<int>())
                            {
                                bufferByteAssignedToStruct.AsSpan((currentBufferLength - 2), 2).Fill(true);
                                sofHasBeenReceivedEofNotYet = true;
                            }
                        }
                        catch
                        {

                        }


                        // Todo: 
                        // Make "bufferByteAssignedToStruct" to hold states
                        // 0: not assigned
                        // 1: basic struct
                        // 2: extended struct
                        // 3: config struct
                        // 4: not assigned struct

                        // CRC check inside of "FindValidMessagePairs(...)"

                        // provide "bufferByteAssignedToStruct" to "FindValidMessagePairs(...)" to label the data.


                        // Todo 12.08:
                        // when sofHasBeenReceivedEofNotYet, only not serial print the last chunk









                        // Destination array
                        byte[] destinationArray = new byte[destBufferSize];
                        
                        
                        int lastTrueElementIndex = 0;




                        if (true)
                        {
                            // extended struct
                            for (int pairId = 0; pairId < validPairsExtendedStruct.Count; pairId++)
                            {
                                int srcBufferOffset_0 = validPairsExtendedStruct[pairId].Item1;
                                int srcBufferOffset_1 = validPairsExtendedStruct[pairId].Item2;

                                // copy bytes to subarray
                                Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], srcBufferOffset_0, destinationArray, 0, sizeof(DAP_state_extended_st));

                                int destBuffLength = srcBufferOffset_1 - srcBufferOffset_0;

                                // check for pedal extended state struct
                                if ((destBuffLength == sizeof(DAP_state_extended_st)))
                                {

                                    // parse byte array as config struct
                                    DAP_state_extended_st pedalState_ext_read_st = getStateExtFromBytes(destinationArray);

                                    // check whether receive struct is plausible
                                    DAP_state_extended_st* v_state = &pedalState_ext_read_st;
                                    byte* p_state = (byte*)v_state;

                                    // payload type check
                                    bool check_payload_state_b = false;
                                    if (pedalState_ext_read_st.payloadHeader_.payloadType == Constants.pedalStateExtendedPayload_type)
                                    {
                                        check_payload_state_b = true;
                                    }

                                    // CRC check
                                    bool check_crc_state_b = false;
                                    if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Extended)) == pedalState_ext_read_st.payloadFooter_.checkSum)
                                    {
                                        check_crc_state_b = true;
                                    }

                                    if ((check_payload_state_b) && check_crc_state_b)
                                    {

                                        bufferByteAssignedToStruct.AsSpan(srcBufferOffset_0, sizeof(DAP_state_extended_st)).Fill(true);
                                        lastTrueElementIndex = Math.Max(lastTrueElementIndex, srcBufferOffset_0 + sizeof(DAP_state_extended_st));

										UInt16 pedalSelectedFromPacket_u16 = pedalState_ext_read_st.payloadHeader_.PedalTag;
										
                                        if (indexOfSelectedPedal_u == pedalSelectedFromPacket_u16)
                                        {

                                            if (Plugin._calculations.dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                            //if (dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                            {
                                                // Specify the path to the file
                                                string currentDirectory = Directory.GetCurrentDirectory();
                                                string filePath = currentDirectory + "\\PluginsData\\Common" + "\\DiyFfbPedalStateLog_" + indexOfSelectedPedal_u.ToString() + ".txt";


                                                // delete file 
                                                if (true == Plugin._calculations.dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u])
                                                {
                                                    Plugin._calculations.dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u] = false;
                                                    File.Delete(filePath);

                                                    // write header
                                                    if (!File.Exists(filePath))
                                                    {
                                                        using (StreamWriter writer = new StreamWriter(filePath, true))
                                                        {
                                                            // Write the content to the file
                                                            writer.Write("cycleCtr");
                                                            writer.Write(", time_InUs");
                                                            writer.Write(", cycleCount_u32");
                                                            writer.Write(", forceRaw_InKg");
                                                            writer.Write(", forceFiltered_InKg");
                                                            writer.Write(", forceVelocity_InKgPerSec");
                                                            writer.Write(", servoPos_InSteps");
                                                            writer.Write(", servoPosEsp_InSteps");
                                                            writer.Write(", servoPosError_InSteps");
                                                            writer.Write(", servoCurrent_InPercent");
                                                            writer.Write(", servoVoltage_InV");
                                                            writer.Write(", angleSensorOutput");
                                                            writer.Write(", brakeResistorState_b");
                                                            writer.Write(", servoPosEstimated_InSteps");
                                                            writer.Write(", targetPosition_InSteps");
                                                            writer.Write(", currentSpeedInMilliHz_i32");
                                                            //writer.Write(", servoPositionEstimated_stepperPos_i16");
                                                            writer.Write("\n");
                                                        }

                                                    }
                                                }





                                                using (StreamWriter writer = new StreamWriter(filePath, true))
                                                {
                                                    var state = pedalState_ext_read_st.payloadPedalExtendedState_;
                                                    writeCntr++;

                                                    // Build the entire string in one line using interpolation
                                                    writer.WriteLine($"{writeCntr},{state.timeInUs_u32},{state.cycleCount_u32},{state.pedalForce_raw_fl32},{state.pedalForce_filtered_fl32},{state.forceVel_est_fl32},{state.servoPosition_i32},{state.servoPositionTarget_i32},{state.servo_position_error_i16},{state.servo_current_percent_i16},{state.servo_voltage_0p1V_i16 / 10.0f},{state.angleSensorOutput_ui16},{state.brakeResistorState_b},{state.servoPositionEstimated_i16},{state.targetPosition_i32},{state.currentSpeedInMilliHz_i32}");
                                                }



                                            }
                                        }
                                    }
                                    else
                                    {
                                        bufferByteAssignedToStruct_class.AsSpan(srcBufferOffset_0, sizeof(DAP_state_extended_st)).Fill(0);
                                    }
                                }
                            }

                            // basic struct
                            for (int pairId = 0; pairId < validPairsBasicStruct.Count; pairId++)
                            {
                                int srcBufferOffset_0 = validPairsBasicStruct[pairId].Item1;
                                int srcBufferOffset_1 = validPairsBasicStruct[pairId].Item2;
                                int destBuffLength = srcBufferOffset_1 - srcBufferOffset_0;

                                // check for pedal extended state struct
                                if ((destBuffLength == sizeof(DAP_state_basic_st)))
                                {

                                    // copy bytes to subarray
                                    Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], srcBufferOffset_0, destinationArray, 0, sizeof(DAP_state_basic_st));

                                    // parse byte array as config struct
                                    DAP_state_basic_st pedalState_read_st = getStateFromBytes(destinationArray);

                                    // check whether receive struct is plausible
                                    DAP_state_basic_st* v_state = &pedalState_read_st;
                                    byte* p_state = (byte*)v_state;

                                    // payload type check
                                    bool check_payload_state_b = false;
                                    if (pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                    {
                                        check_payload_state_b = true;
                                    }

                                    //Pedal version and Plugin DAP version check
                                    int pedalSelected = pedalState_read_st.payloadHeader_.PedalTag;
                                    Pedal_version[pedalSelected] = pedalState_read_st.payloadHeader_.version;
                                    

                                    // CRC check
                                    bool check_crc_state_b = false;
                                    if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Basic)) == pedalState_read_st.payloadFooter_.checkSum)
                                    {
                                        check_crc_state_b = true;
                                    }

                                    if ((check_payload_state_b) && check_crc_state_b)
                                    {

                                        bufferByteAssignedToStruct.AsSpan(srcBufferOffset_0, sizeof(DAP_state_basic_st)).Fill(true);
                                        lastTrueElementIndex = Math.Max(lastTrueElementIndex, srcBufferOffset_0 + sizeof(DAP_state_basic_st));
										
										UInt16 pedalSelectedFromPacket_u16 = pedalState_read_st.payloadHeader_.PedalTag;
										
                                        // write vJoy data
                                        Pedal_position_reading[pedalSelectedFromPacket_u16] = pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16;
                                        //if (Plugin.Rudder_enable_flag == false)
                                        //{
                                        if (Plugin.Settings.vjoy_output_flag == 1)
                                        {
                                            switch (pedalSelectedFromPacket_u16)
                                            {

                                                case 0:
                                                    //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RX);  // Center X axis
                                                    Plugin._calculations._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16 / 2, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RX);   // HID_USAGES Enums
                                                    break;
                                                case 1:
                                                    //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RY);  // Center X axis
                                                    Plugin._calculations._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16 / 2, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RY);   // HID_USAGES Enums
                                                    break;
                                                case 2:
                                                    //joystick.SetJoystickAxis(pedalState_read_st.payloadPedalState_.joystickOutput_u16, Axis.HID_USAGE_RZ);  // Center X axis
                                                    Plugin._calculations._joystick.SetAxis(pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16 / 2, Plugin.Settings.vjoy_order, HID_USAGES.HID_USAGE_RZ);   // HID_USAGES Enums
                                                    break;
                                                default:
                                                    break;
                                            }

                                        }

                                        //check servo status change
                                        if (Plugin._calculations.ServoStatus[pedalSelectedFromPacket_u16] == (byte)enumServoStatus.On && pedalState_read_st.payloadPedalBasicState_.servoStatus == (byte)enumServoStatus.Idle)
                                        {
                                            string tmp = "Pedal:" + pedalSelected + " Servo idle reach timeout, power cutoff, please restart pedal to wake it up";
                                            ToastNotification("Wireless Connection", tmp);
                                        }
                                        // Force stop action
                                        if (Plugin._calculations.ServoStatus[pedalSelectedFromPacket_u16] == (byte)enumServoStatus.On && pedalState_read_st.payloadPedalBasicState_.servoStatus == (byte)enumServoStatus.ForceStop)
                                        {
                                            string tmp = "Pedal:" + pedalSelected + " force Stopped";
                                            ToastNotification("Wireless Connection", tmp);
                                        }

                                        //fill servo status

                                        Plugin._calculations.ServoStatus[pedalSelectedFromPacket_u16] = pedalState_read_st.payloadPedalBasicState_.servoStatus;



                                        // GUI update
                                        if ((pedalStateHasAlreadyBeenUpdated_b == false) && (indexOfSelectedPedal_u == pedalSelected))
                                        {
                                            //TextBox_debugOutput.Text = "Pedal pos: " + pedalState_read_st.payloadPedalState_.pedalPosition_u16;
                                            //TextBox_debugOutput.Text += "Pedal force: " + pedalState_read_st.payloadPedalState_.pedalForce_u16;
                                            //TextBox_debugOutput.Text += ",  Servo pos targe: " + pedalState_read_st.payloadPedalState_.servoPosition_i32;
                                            //TextBox_debugOutput.Text += ",  Servo pos: " + pedalState_read_st.payloadPedalState_.servoPosition_i32;

                                            PedalForceTravel_Tab.updatePedalState(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16, pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);

                                            pedalStateHasAlreadyBeenUpdated_b = true;

                                            double control_rect_value_max = 65535;


                                            if (Plugin.Settings.advanced_b)
                                            {
                                                int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                                int x_showed = round_x + 1;

                                                current_pedal_travel_state = x_showed;
                                                Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;

                                            }
                                            else
                                            {
                                                int round_x = (int)(100 * pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16 / control_rect_value_max) - 1;
                                                int x_showed = round_x + 1;
                                                round_x = Math.Max(0, Math.Min(round_x, 99));
                                                current_pedal_travel_state = x_showed;
                                                Plugin.pedal_state_in_ratio = (byte)current_pedal_travel_state;
                                            }
                                            if (dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.travelAsJoystickOutput_u8 == 1)
                                            {
                                                PedalJoystick_Tab.JoystickStateUpdate(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16);
                                            }
                                            else
                                            {
                                                PedalJoystick_Tab.JoystickStateUpdate(pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);
                                            }
                                            for (int i = 0; i < 3; i++)
                                            {
                                                //PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                                Plugin._calculations.PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                            }
                                        }
                                    }
                                    else
                                    {
                                        bufferByteAssignedToStruct_class.AsSpan(srcBufferOffset_0, sizeof(DAP_state_extended_st)).Fill(0);
                                    }
                                }
                            }

                            // config struct
                            for (int pairId = 0; pairId < validPairsConfig.Count; pairId++)
                            {
                                int srcBufferOffset_0 = validPairsConfig[pairId].Item1;
                                int srcBufferOffset_1 = validPairsConfig[pairId].Item2;

                                // copy bytes to subarray
                                Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], srcBufferOffset_0, destinationArray, 0, sizeof(DAP_config_st));

                                int destBuffLength = srcBufferOffset_1 - srcBufferOffset_0;

                                // decode into config struct
                                if ((destBuffLength == sizeof(DAP_config_st)))
                                {

                                    // parse byte array as config struct
                                    DAP_config_st pedalConfig_read_st = getConfigFromBytes(destinationArray);

                                    // check whether receive struct is plausible
                                    DAP_config_st* v_config = &pedalConfig_read_st;
                                    byte* p_config = (byte*)v_config;
									
									

                                    // payload type check
                                    bool check_payload_config_b = false;
                                    if (pedalConfig_read_st.payloadHeader_.payloadType == Constants.pedalConfigPayload_type)
                                    {
                                        check_payload_config_b = true;
                                    }

                                    // CRC check
                                    bool check_crc_config_b = false;
                                    if (Plugin.checksumCalc(p_config, sizeof(payloadHeader) + sizeof(payloadPedalConfig)) == pedalConfig_read_st.payloadFooter_.checkSum)
                                    {
                                        check_crc_config_b = true;
                                    }

                                    if ((check_payload_config_b) && check_crc_config_b)
                                    {
                                        UInt16 pedalSelectedFromPacket_u16 = pedalConfig_read_st.payloadHeader_.PedalTag;
                                        if (waiting_for_pedal_config[pedalSelectedFromPacket_u16])
                                        {
                                            waiting_for_pedal_config[pedalSelectedFromPacket_u16] = false;
                                            dap_config_st[pedalSelectedFromPacket_u16] = pedalConfig_read_st;
                                            updateTheGuiFromConfig();
                                        }

                                            
                                        bufferByteAssignedToStruct.AsSpan(srcBufferOffset_0, sizeof(DAP_config_st)).Fill(true);
                                        lastTrueElementIndex = Math.Max(lastTrueElementIndex, srcBufferOffset_0 + sizeof(DAP_config_st));



                                        continue;
                                    }
                                    else
                                    {

                                        bufferByteAssignedToStruct_class.AsSpan(srcBufferOffset_0, sizeof(DAP_state_extended_st)).Fill(0);

                                        TextBox2.Text = "Payload config test 1: " + check_payload_config_b;
                                        TextBox2.Text += "Payload config test 2: " + check_crc_config_b;
                                    }

                                }

                            }


                            //bridge states here
                            for (int pairId = 0; pairId < validPairsBridgeState.Count; pairId++)
                            {
                                int srcBufferOffset_0 = validPairsBridgeState[pairId].Item1;
                                int srcBufferOffset_1 = validPairsBridgeState[pairId].Item2;

                                int destBuffLength = srcBufferOffset_1 - srcBufferOffset_0;

                                if ((destBuffLength == sizeof(DAP_bridge_state_st)))
                                {
                                    // copy bytes to subarray
                                    Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], srcBufferOffset_0, destinationArray, 0, sizeof(DAP_bridge_state_st));
                                    // parse byte array as config struct
                                    DAP_bridge_state_st bridge_state = getStateBridgeFromBytes(destinationArray);
                                    string buffer_string = BitConverter.ToString(destinationArray);
                                    // check whether receive struct is plausible
                                    DAP_bridge_state_st* v_state = &bridge_state;
                                    byte* p_state = (byte*)v_state;

                                    // payload type check
                                    bool check_payload_state_b = false;
                                    if (bridge_state.payLoadHeader_.payloadType == Constants.bridgeStatePayloadType)
                                    {
                                        check_payload_state_b = true;
                                    }

                                    if (bridge_state.payLoadHeader_.version != Constants.pedalConfigPayload_version && bridge_state.payLoadHeader_.payloadType == Constants.bridgeStatePayloadType)
                                    {
                                        if (!Version_warning_first_show_b_bridge)
                                        {
                                            Version_warning_first_show_b_bridge = true;
                                            if (bridge_state.payLoadHeader_.version > Constants.pedalConfigPayload_version)
                                            {
                                                String MSG_tmp;
                                                MSG_tmp = "Bridge Dap version: " + bridge_state.payLoadHeader_.version + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Simhub Plugin.";
                                                System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                            }
                                            else
                                            {
                                                String MSG_tmp;
                                                MSG_tmp = "Bridge Dap version: " + bridge_state.payLoadHeader_.version + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Bridge Firmware.";
                                                System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                            }
                                        }
                                    }
                                    // CRC check
                                    bool check_crc_state_b = false;
                                    if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadBridgeState)) == bridge_state.payloadFooter_.checkSum)
                                    {
                                        check_crc_state_b = true;
                                    }

                                    if ((check_payload_state_b) && check_crc_state_b)
                                    {
                                        //Bridge_RSSI = bridge_state.payloadBridgeState_.Pedal_RSSI;
                                        for (int pedalIDX = 0; pedalIDX < 3; pedalIDX++)
                                        {
                                            Plugin._calculations.rssi[pedalIDX] = bridge_state.payloadBridgeState_.Pedal_RSSI_realtime[pedalIDX];
                                        }

                                        string connection_tmp = "";
                                        bool wireless_connection_update = false;
                                        //fill the status into _calculations
                                        //Plugin._calculations.PedalAvailability[0] = bridge_state.payloadBridgeState_.Pedal_availability_0 == 1;
                                        //Plugin._calculations.PedalAvailability[1] = bridge_state.payloadBridgeState_.Pedal_availability_1 == 1;
                                        //Plugin._calculations.PedalAvailability[2] = bridge_state.payloadBridgeState_.Pedal_availability_2 == 1;
                                        //check wireless pedal connection, if status change make toast notification
                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 != bridge_state.payloadBridgeState_.Pedal_availability_0)
                                        {

                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 0)
                                            {
                                                //ToastNotification("Wireless Clutch", "Connected");
                                                connection_tmp += "Clutch Connected";
                                                wireless_connection_update = true;
                                                Pedal_wireless_connection_update_b[0] = true;

                                            }
                                            else
                                            {
                                                ///ToastNotification("Wireless Clutch", "Disconnected");
                                                connection_tmp += "Clutch Disconnected";
                                                wireless_connection_update = true;
                                                //Plugin._calculations.PedalAvailability[0] = false;
                                            }
                                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 = bridge_state.payloadBridgeState_.Pedal_availability_0;
                                            //updateTheGuiFromConfig();
                                        }


                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 != bridge_state.payloadBridgeState_.Pedal_availability_1)
                                        {

                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 0)
                                            {
                                                //ToastNotification("Wireless Brake", "Connected");
                                                connection_tmp += " Brake Connected";
                                                wireless_connection_update = true;
                                                Pedal_wireless_connection_update_b[1] = true;


                                            }
                                            else
                                            {
                                                //ToastNotification("Wireless Brake", "Disconnected");
                                                connection_tmp += " Brake Disconnected";
                                                wireless_connection_update = true;
                                                //Plugin._calculations.PedalAvailability[1] = false;
                                            }
                                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 = bridge_state.payloadBridgeState_.Pedal_availability_1;

                                            //updateTheGuiFromConfig();
                                        }

                                        if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 != bridge_state.payloadBridgeState_.Pedal_availability_2)
                                        {

                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 0)
                                            {
                                                //ToastNotification("Wireless Throttle", "Connected");
                                                connection_tmp += " Throttle Connected";
                                                wireless_connection_update = true;
                                                Pedal_wireless_connection_update_b[2] = true;

                                            }
                                            else
                                            {
                                                //ToastNotification("Wireless Throttle", "Disconnected");
                                                connection_tmp += " Throttle Disconnected";
                                                wireless_connection_update = true;
                                                //Plugin._calculations.PedalAvailability[2] = false;
                                            }
                                            dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 = bridge_state.payloadBridgeState_.Pedal_availability_2;

                                        }

                                        //Pedal availability status update
                                        int PedalAvailabilityCheck = dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 + dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 + dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2;
                                        if (PedalAvailabilityCheck == 3)
                                        {
                                            Pedal_connect_status = (byte)PedalAvailability.ThreePedalConnect;
                                        }

                                        if (PedalAvailabilityCheck == 2)
                                        {
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectClutchBrake;
                                            }
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectClutchThrottle;
                                            }
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1 && dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.TwoPedalConnectBrakeThrottle;
                                            }
                                        }

                                        if (PedalAvailabilityCheck == 1)
                                        {
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_0 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.SinglePedalClutch;
                                            }
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_1 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.SinglePedalBrake;
                                            }
                                            if (dap_bridge_state_st.payloadBridgeState_.Pedal_availability_2 == 1)
                                            {
                                                Pedal_connect_status = (byte)PedalAvailability.SinglePedalThrottle;
                                            }
                                        }

                                        if (wireless_connection_update)
                                        {
                                            ToastNotification("Wireless Connection", connection_tmp);
                                            updateTheGuiFromConfig();
                                            wireless_connection_update = false;
                                        }
                                        //fill the version info
                                        for (int i = 0; i < 3; i++)
                                        {
                                            dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[i] = bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                            Plugin._calculations.BridgeFirmwareVersion[i] = bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                        }



                                        //updateTheGuiFromConfig();
                                        continue;
                                    }
                                }
                            }




                                // print all non identified structs to serial monitor
                                // If non known array datatype was received, assume a text message was received and print it
                                // only print debug messages when debug mode is active as it degrades performance
                            if (_serial_monitor_window != null)
                            {
                                // Create a list to hold filtered elements
                                List<byte> filteredList = new List<byte>();

                                // Todo: dont print any bytes, where SOF has started, but no EOF was received yet
                                if (!sofHasBeenReceivedEofNotYet)
                                {
                                    for (int i = 0; i < currentBufferLength; i++)
                                    {
                                        //if (!bufferByteAssignedToStruct[i]) // copy only if not true
                                        //{
                                        //    filteredList.Add(buffer_appended[pedalSelected][i]);
                                        //}

                                        if (bufferByteAssignedToStruct_class[i] == 0)  // copy only if not true
                                        {
                                            filteredList.Add(buffer_appended[3][i]);
                                        }
                                    }

                                    // observation:
                                    // 1) last packet not finished yet. Its printed in the monitor though.
                                    // 2) first two bytes are 170 and 86 --> EOF from last frame?
                                    // 3) FIXED on 11.08.2025: Some corrupted packets in the middle of the buffer, although SOF and EOF are visible. Exp datalength = 44, measured data length = 307 - 264 + 1 = 44



                                    // Convert to array
                                    byte[] newArray = filteredList.ToArray();
                                    int size = newArray.Length;
                                    string resultString = Encoding.GetEncoding(28591).GetString(newArray);

                                    if (resultString.Length > 3)
                                    {
                                        string str_chk = resultString.Substring(0, 3);
                                        if (String.Equals(str_chk, "[L]"))
                                        {
                                            string temp = resultString.Substring(3, resultString.Length - 3);
                                            //TextBox_serialMonitor.Text += str_chk + "\n";
                                            TextBox_serialMonitor_bridge.Text += temp + "\n";
                                            //TextBox_serialMonitor.Text += temp + "\n";
                                            if (_serial_monitor_window != null)
                                            {
                                                _serial_monitor_window.TextBox_SerialMonitor.Text += temp + "\n";
                                            }
                                            TextBox_serialMonitor_bridge.ScrollToEnd();

                                            SimHub.Logging.Current.Info(temp);
                                        }
                                        if (String.Equals(str_chk, "E ("))
                                        {
                                            TextBox_serialMonitor_bridge.Text += resultString + "\n";
                                            //TextBox_serialMonitor.Text += resultString + "\n";
                                            SimHub.Logging.Current.Info(resultString);
                                            if (_serial_monitor_window != null)
                                            {
                                                _serial_monitor_window.TextBox_SerialMonitor.Text += resultString + "\n";
                                            }
                                            TextBox_serialMonitor_bridge.ScrollToEnd();
                                        }
                                    }

                                }
                            }


                            // remove elements from buffer
                            //lastTrueElementIndex
                            int lastTrueIndex = -1; // -1 means "not found"
                            for (int i = currentBufferLength - 1; i >= 0; i--)
                            {
                                //if (bufferByteAssignedToStruct[i])
                                //{
                                if ( (bufferByteAssignedToStruct_class[i] != 0) && (bufferByteAssignedToStruct_class[i] !=  255) )
                                {
                                        lastTrueIndex = i;
                                        break;
                                }
                            }

                            if (lastTrueIndex > (-1))
                            {
                                int remainingMessageLength = currentBufferLength - (lastTrueIndex + 1);
                                if (remainingMessageLength > 0)
                                {
                                    appendedBufferOffset[bridgeBufferIndex] = remainingMessageLength;

                                    Buffer.BlockCopy(buffer_appended[bridgeBufferIndex], lastTrueIndex + 1, buffer_appended[bridgeBufferIndex], 0, remainingMessageLength);
                                    Array.Clear(buffer_appended[bridgeBufferIndex], appendedBufferOffset[bridgeBufferIndex], bufferSize - appendedBufferOffset[bridgeBufferIndex]); // 120 - 20 + 1 = 101 elements



                                    if (!((buffer_appended[bridgeBufferIndex][0] == 170) && (buffer_appended[bridgeBufferIndex][1] == 85)))
                                    {
                                        int tmp = 5;
                                    }
                                }
                                else
                                {
                                    appendedBufferOffset[bridgeBufferIndex] = 0;
                                }
                            }
                            else
                            {
                                //appendedBufferOffset[pedalSelected] += receivedLength;
                                appendedBufferOffset[bridgeBufferIndex] = 0;
                            }




                        }
                        









                        // Stop the stopwatch
                        stopwatch.Stop();

                        // Get the elapsed time
                        /*
                        TimeSpan elapsedTime = stopwatch.Elapsed;

                        timeCollector[pedalSelected] += elapsedTime.TotalMilliseconds;

                        if (timeCntr[pedalSelected] >= 50)
                        {


                            double avgTime = timeCollector[pedalSelected] / timeCntr[pedalSelected];
                            if (Plugin.Settings.advanced_b)
                            {
                                TextBox_debugOutput.Text = "Serial callback time in ms: " + avgTime.ToString();
                            }
                            timeCntr[pedalSelected] = 0;
                            timeCollector[pedalSelected] = 0;
                        }
                        */
                    }
                    else
                    {
                        sp.DiscardInBuffer();
                    }

                }
            }
            catch (Exception caughtEx)
            {

                string errorMessage = caughtEx.Message;

                SimHub.Logging.Current.Error(errorMessage);
            }

        }


    }
}
