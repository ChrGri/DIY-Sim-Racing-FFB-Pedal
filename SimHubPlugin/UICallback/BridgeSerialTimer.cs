using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO.Ports;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        unsafe public void timerCallback_serial_esphost(object sender, EventArgs e)
        {

            //action here 
            Simhub_action_update();




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
                        receivedLength = sp.BytesToRead;
                    }
                    catch (Exception ex)
                    {
                        TextBox_debugOutput.Text = ex.Message;
                        //ConnectToPedal.IsChecked = false;
                        return;
                    }

                    if (receivedLength > 0)
                    {

                        //TextBox_serialMonitor.Text += "Received:" + receivedLength + "\n";
                        //TextBox_serialMonitor.ScrollToEnd();


                        timeCntr[3] += 1;


                        // determine byte sequence which is defined as message end --> crlf
                        byte[] byteToFind = System.Text.Encoding.GetEncoding(28591).GetBytes(STOPCHAR[0].ToCharArray());
                        int stop_char_length = byteToFind.Length;


                        // calculate current buffer length
                        int currentBufferLength = appendedBufferOffset[3] + receivedLength;


                        // check if buffer is large enough otherwise discard in buffer and set offset to 0
                        if ((bufferSize > currentBufferLength) && (appendedBufferOffset[3] >= 0))
                        {
                            sp.Read(buffer_appended[3], appendedBufferOffset[3], receivedLength);
                        }
                        else
                        {
                            sp.DiscardInBuffer();
                            appendedBufferOffset[3] = 0;
                            return;
                        }
                        // copy to local buffer
                        //byte[] localBuffer = new byte[currentBufferLength];

                        //Buffer.BlockCopy(buffer_appended[pedalSelected], 0, localBuffer, 0, currentBufferLength);


                        // find all occurences of crlf as they indicate message end
                        List<int> indices = FindAllOccurrences(buffer_appended[3], byteToFind, currentBufferLength);
                        // Destination array
                        byte[] destinationArray = new byte[destBufferSize];
                        int srcBufferOffset = 0;
                        // decode every message
                        //foreach (int number in indices)
                        for (int msgId = 0; msgId < indices.Count; msgId++)
                        {
                            // computes the length of bytes to read
                            int destBuffLength = 0; //number - srcBufferOffset;

                            if (msgId == 0)
                            {
                                srcBufferOffset = 0;
                                destBuffLength = indices.ElementAt(msgId);
                            }
                            else
                            {
                                srcBufferOffset = indices.ElementAt(msgId - 1) + stop_char_length;
                                destBuffLength = indices.ElementAt(msgId) - srcBufferOffset;
                            }

                            // check if dest buffer length is within valid length
                            if ((destBuffLength <= 0) | (destBuffLength > destBufferSize))
                            {
                                continue;
                            }
                            // copy bytes to subarray
                            Buffer.BlockCopy(buffer_appended[3], srcBufferOffset, destinationArray, 0, destBuffLength);
                            // check for pedal state struct
                            if ((destBuffLength == sizeof(DAP_state_basic_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_basic_st pedalState_read_st = getStateFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_basic_st* v_state = &pedalState_read_st;
                                byte* p_state = (byte*)v_state;
                                UInt16 pedalSelected = pedalState_read_st.payloadHeader_.PedalTag;
                                // payload type check
                                bool check_payload_state_b = false;
                                if (pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                {
                                    check_payload_state_b = true;
                                }
                                Pedal_version[pedalSelected] = pedalState_read_st.payloadHeader_.version;
                                if (Pedal_version[pedalSelected] != Constants.pedalConfigPayload_version && pedalState_read_st.payloadHeader_.payloadType == Constants.pedalStateBasicPayload_type)
                                {
                                    if (!Version_warning_first_show_b[pedalSelected])
                                    {
                                        Version_warning_first_show_b[pedalSelected] = true;
                                        if (Pedal_version[pedalSelected] > Constants.pedalConfigPayload_version)
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Simhub Plugin.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                        else
                                        {
                                            String MSG_tmp;
                                            MSG_tmp = "Pedal: " + pedalState_read_st.payloadHeader_.PedalTag + " Pedal Dap version: " + Pedal_version[pedalSelected] + ", Plugin DAP version: " + Constants.pedalConfigPayload_version + ". Please update Pedal Firmware.";
                                            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                                        }
                                    }
                                }
                                // CRC check
                                bool check_crc_state_b = false;
                                if (Plugin.checksumCalc(p_state, sizeof(payloadHeader) + sizeof(payloadPedalState_Basic)) == pedalState_read_st.payloadFooter_.checkSum)
                                {
                                    check_crc_state_b = true;
                                }

                                if ((check_payload_state_b) && check_crc_state_b)
                                {

                                    // write vJoy data
                                    Pedal_position_reading[pedalSelected] = pedalState_read_st.payloadPedalBasicState_.joystickOutput_u16;
                                    // GUI update
                                    if (pedalState_read_st.payloadPedalBasicState_.error_code_u8 != 0)
                                    {
                                        Plugin.PedalErrorCode = pedalState_read_st.payloadPedalBasicState_.error_code_u8;
                                        Plugin.PedalErrorIndex = pedalState_read_st.payloadHeader_.PedalTag;
                                        string errorcodetext = "";
                                        //errorcode
                                        switch (Plugin.PedalErrorCode)
                                        {
                                            case 12:
                                                errorcodetext = "Servo Lost connection";
                                                break;
                                            case 101:
                                                errorcodetext = "Config payload type error";
                                                break;
                                            case 102:
                                                errorcodetext = "Config version error";
                                                break;
                                            case 103:
                                                errorcodetext = "Config CRC error";
                                                break;
                                            case 111:
                                                errorcodetext = "Action payload type error";
                                                break;
                                            case 122:
                                                errorcodetext = "Action version error";
                                                break;
                                            case 123:
                                                errorcodetext = "Action CRC error";
                                                break;
                                        }
                                        string temp_str = "Pedal:" + pedalState_read_st.payloadHeader_.PedalTag + " E:" + pedalState_read_st.payloadPedalBasicState_.error_code_u8 + "-" + errorcodetext;
                                        //TextBox2.Text = temp_str;
                                        SimHub.Logging.Current.Info("DIYFFBPedal " + temp_str);
                                        TextBox_serialMonitor_bridge.Text = temp_str;

                                    }
                                    if ((pedalStateHasAlreadyBeenUpdated_b == false) && (indexOfSelectedPedal_u == pedalSelected))
                                    {
                                        pedalStateHasAlreadyBeenUpdated_b = true;
                                        PedalForceTravel_Tab.updatePedalState(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16, pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);
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

                                    }
                                    for (int i = 0; i < 3; i++)
                                    {
                                        PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                        Plugin._calculations.PedalFirmwareVersion[pedalSelected,i]= pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                    }

                                    continue;
                                }

                            }
                            // check for pedal extended state struct
                            if ((destBuffLength == sizeof(DAP_state_extended_st)))
                            {

                                // parse byte array as config struct
                                DAP_state_extended_st pedalState_ext_read_st = getStateExtFromBytes(destinationArray);

                                // check whether receive struct is plausible
                                DAP_state_extended_st* v_state = &pedalState_ext_read_st;
                                byte* p_state = (byte*)v_state;
                                UInt16 pedalSelected = pedalState_ext_read_st.payloadHeader_.PedalTag;
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
                                    //if (indexOfSelectedPedal_u == pedalSelected)
                                    {
                                        if (Plugin._calculations.dumpPedalToResponseFile[indexOfSelectedPedal_u])
                                        {
                                            // Specify the path to the file
                                            string currentDirectory = Directory.GetCurrentDirectory();
                                            string filePath = currentDirectory + "\\PluginsData\\Common" + "\\DiyFfbPedalStateLog_" + pedalSelected + ".txt";

                                            // delete file 
                                            if (true == Plugin._calculations.dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u])
                                            {
                                                Plugin._calculations.dumpPedalToResponseFile_clearFile[indexOfSelectedPedal_u] = false;
                                                File.Delete(filePath);
                                            }

                                            // write header
                                            if (!File.Exists(filePath))
                                            {
                                                using (StreamWriter writer = new StreamWriter(filePath, true))
                                                {
                                                    // Write the content to the file
                                                    writer.Write("cycleCtr, ");
                                                    writer.Write("time_InMs, ");
                                                    writer.Write("forceRaw_InKg, ");
                                                    writer.Write("forceFiltered_InKg, ");
                                                    writer.Write("forceVelocity_InKgPerSec, ");
                                                    writer.Write("servoPos_InSteps, ");
                                                    writer.Write("servoPosEsp_InSteps, ");
                                                    writer.Write("servoCurrent_InPercent, ");
                                                    writer.Write("servoVoltage_InV");
                                                    writer.Write("\n");
                                                }

                                            }
                                            // Use StreamWriter to write to the file
                                            using (StreamWriter writer = new StreamWriter(filePath, true))
                                            {
                                                // Write the content to the file
                                                writeCntr++;
                                                writer.Write(writeCntr);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.timeInMs_u32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_raw_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.pedalForce_filtered_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.forceVel_est_fl32);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPosition_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servoPositionTarget_i16);
                                                writer.Write(", ");
                                                writer.Write(pedalState_ext_read_st.payloadPedalExtendedState_.servo_current_percent_i16);
                                                writer.Write(", ");
                                                writer.Write(((float)pedalState_ext_read_st.payloadPedalExtendedState_.servo_voltage_0p1V_i16) / 10.0);
                                                writer.Write("\n");
                                            }
                                        }
                                    }
                                    continue;
                                }
                            }


                            if ((destBuffLength == sizeof(DAP_bridge_state_st)))
                            {

                                // parse byte array as config struct
                                DAP_bridge_state_st bridge_state = getStateBridgeFromBytes(destinationArray);

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
                                    Bridge_RSSI = bridge_state.payloadBridgeState_.Pedal_RSSI;
                                    Plugin._calculations.RSSI_Value = bridge_state.payloadBridgeState_.Pedal_RSSI;

                                    dap_bridge_state_st.payloadBridgeState_.Pedal_RSSI = bridge_state.payloadBridgeState_.Pedal_RSSI;
                                    string connection_tmp = "";
                                    bool wireless_connection_update = false;
                                    //fill the status into _calculations
                                    Plugin._calculations.PedalAvailability[0] = bridge_state.payloadBridgeState_.Pedal_availability_0 == 1;
                                    Plugin._calculations.PedalAvailability[1] = bridge_state.payloadBridgeState_.Pedal_availability_1 == 1;
                                    Plugin._calculations.PedalAvailability[2] = bridge_state.payloadBridgeState_.Pedal_availability_2 == 1;
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
                                            Plugin.PedalConfigRead_b[0] = false;
                                            Plugin._calculations.PedalAvailability[0] = false;
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
                                            Plugin.PedalConfigRead_b[1] = false;
                                            Plugin._calculations.PedalAvailability[1] = false;
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
                                            Plugin.PedalConfigRead_b[2] = false;
                                            Plugin._calculations.PedalAvailability[2] = false;
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
                                        Plugin._calculations.BridgeFirmwareVersion[i]= bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                    }



                                    //updateTheGuiFromConfig();
                                    continue;
                                }
                            }
                            // decode into config struct

                            if (destBuffLength == sizeof(DAP_config_st))
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
                                UInt16 pedalSelected = pedalConfig_read_st.payloadHeader_.PedalTag;
                                if (waiting_for_pedal_config[pedalSelected])
                                {
                                    if ((check_payload_config_b) && check_crc_config_b)
                                    {
                                        waiting_for_pedal_config[pedalSelected] = false;
                                        dap_config_st[pedalSelected] = pedalConfig_read_st;
                                        updateTheGuiFromConfig();
                                        Plugin.PedalConfigRead_b[pedalSelected] = true;
                                        continue;
                                    }
                                    else
                                    {
                                        TextBox_debugOutput.Text = "Payload config test 1: " + check_payload_config_b;
                                        TextBox_debugOutput.Text += "Payload config test 2: " + check_crc_config_b;
                                    }
                                }
                            }
                            // If non known array datatype was received, assume a text message was received and print it
                            // only print debug messages when debug mode is active as it degrades performance
                            if (Plugin.Settings.advanced_b || _serial_monitor_window != null)
                            {
                                byte[] destinationArray_sub = new byte[destBuffLength];
                                Buffer.BlockCopy(destinationArray, 0, destinationArray_sub, 0, destBuffLength);
                                string resultString = Encoding.GetEncoding(28591).GetString(destinationArray_sub);
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
                        // copy the last not finished buffer element to begining of next cycles buffer
                        // and determine buffer offset
                        if (indices.Count > 0)
                        {
                            // If at least one crlf was detected, check whether it arrieved at the last bytes
                            int lastElement = indices.Last<int>();
                            int remainingMessageLength = currentBufferLength - (lastElement + stop_char_length);
                            if (remainingMessageLength > 0)
                            {
                                appendedBufferOffset[3] = remainingMessageLength;

                                Buffer.BlockCopy(buffer_appended[3], lastElement + stop_char_length, buffer_appended[3], 0, remainingMessageLength);
                            }
                            else
                            {
                                appendedBufferOffset[3] = 0;
                            }
                        }
                        else
                        {
                            appendedBufferOffset[3] += receivedLength;
                        }
                        // Stop the stopwatch
                        stopwatch.Stop();

                        // Get the elapsed time
                        TimeSpan elapsedTime = stopwatch.Elapsed;

                        timeCollector[3] += elapsedTime.TotalMilliseconds;

                        if (timeCntr[3] >= 50)
                        {


                            double avgTime = timeCollector[3] / timeCntr[3];
                            if (Plugin.Settings.advanced_b)
                            {
                                TextBox_debugOutput.Text = "Serial callback time in ms: " + avgTime.ToString();
                            }
                            timeCntr[3] = 0;
                            timeCollector[3] = 0;
                        }
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
