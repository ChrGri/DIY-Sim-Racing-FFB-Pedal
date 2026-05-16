using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Interop;

namespace DiyFfbPedal
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        private readonly byte[] _rxBuffer = new byte[4096];
        private int _rxIndex = 0;          
        private int _expectedTotalLen = 0; 
        private bool _isReceiving = false;
        private byte PKT_TYPE_START = 0x01;
        private byte PKT_TYPE_CONT = 0x02;

        // Per-pedal vendor HID reassembly state (for PCB_VERSION=14 direct-HID pedals)
        private readonly byte[][] _pedalHidRxBuf    = { new byte[4096], new byte[4096], new byte[4096] };
        private readonly int[]    _pedalHidRxIndex   = new int[3];
        private readonly int[]    _pedalHidExpected  = new int[3];
        private readonly bool[]   _pedalHidReceiving = new bool[3];
        // Queues are drained from the UI timer thread; items are appended from the HID read thread.
        public readonly ConcurrentQueue<byte[]>[] PedalHidDataQueue =
        {
            new ConcurrentQueue<byte[]>(),
            new ConcurrentQueue<byte[]>(),
            new ConcurrentQueue<byte[]>(),
        };
        public void HidRecieveCallback(byte[] buffer)
        {
            if (buffer == null || buffer.Length < 4) return;
            //PrintHidData(buffer);
            byte type = buffer[1];
            int totalLen = buffer[2]; 
            int chunkLen = buffer[3];
            if (type == PKT_TYPE_START)
            {
                _rxIndex = 0;
                _expectedTotalLen = totalLen;
                _isReceiving = true;
                if (_expectedTotalLen > _rxBuffer.Length)
                {
                    _isReceiving = false;
                    
                    return;
                }
            }

            if (_isReceiving)
            {
                if (_rxIndex + chunkLen > _rxBuffer.Length)
                {
                    _isReceiving = false;
                    return; 
                }

                if (chunkLen > 0)
                {
                    Array.Copy(buffer, 4, _rxBuffer, _rxIndex, chunkLen);
                    _rxIndex += chunkLen;
                }

                if (_rxIndex >= _expectedTotalLen)
                {
                    _isReceiving = false;

                    byte[] finalData = new byte[_expectedTotalLen];
                    Array.Copy(_rxBuffer, 0, finalData, 0, _expectedTotalLen);

                    ProcessFullDataFromESP(finalData);
                }
            }

        }

        void ProcessFullDataFromESP(byte[] data)
        { 
            int length = data.Length;
            string hexString=string.Empty;
            bool pedalStateHasAlreadyBeenUpdated_b = false;
            if (length > 0)
            {
                Dispatcher.Invoke(() =>
                {
                    unsafe 
                    {
                        //state basic update
                        if ((length == sizeof(DAP_state_basic_st)))
                        {

                            // parse byte array as config struct
                            DAP_state_basic_st pedalState_read_st = getStateFromBytes(data);

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
                                if (Plugin._calculations.pedalWirelessStatus[pedalSelected] == WirelessConnectStateEnum.PEDAL_BRIDGE_ENTRY_CONNECT
                                    || Plugin._calculations.pedalWirelessStatus[pedalSelected] == WirelessConnectStateEnum.PEDAL_DISCONNECT)
                                {
                                    Plugin._calculations.pedalWirelessStatus[pedalSelected] = WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW;
                                }
                                Plugin._calculations.pedalWirelessConnetionlastTime[pedalSelected] = DateTime.Now;
                                //if (Plugin.Rudder_enable_flag == false)
                                //{
                                if (Plugin.Settings.vjoy_output_flag == 1)
                                {
                                    switch (pedalSelected)
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
                                if (Plugin._calculations.ServoStatus[pedalSelected] == (byte)enumServoStatus.On && pedalState_read_st.payloadPedalBasicState_.servoStatus == (byte)enumServoStatus.Idle)
                                {
                                    string tmp = "Pedal:" + pedalSelected + " Servo idle reach timeout, power cutoff, please restart pedal to wake it up";
                                    ToastNotification("Wireless Connection", tmp);
                                }
                                // Force stop action
                                if (Plugin._calculations.ServoStatus[pedalSelected] == (byte)enumServoStatus.On && pedalState_read_st.payloadPedalBasicState_.servoStatus == (byte)enumServoStatus.ForceStop)
                                {
                                    string tmp = "Pedal:" + pedalSelected + " force Stopped";
                                    ToastNotification("Wireless Connection", tmp);
                                }

                                //fill servo status

                                Plugin._calculations.ServoStatus[pedalSelected] = pedalState_read_st.payloadPedalBasicState_.servoStatus;

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

                                //update Pedal status
                                double valueMax_u16 = 65535;
                                Plugin.PedalStatusInstance.PedalForceInPercent[pedalSelected] = ((double)pedalState_read_st.payloadPedalBasicState_.pedalForce_u16 / (double)valueMax_u16 * 100.0d);
                                Plugin.PedalStatusInstance.PedalMaxForce[pedalSelected] = (int)dap_config_st[pedalSelected].payloadPedalConfig_.maxForce;
                                Plugin.PedalStatusInstance.PedalMinForce[pedalSelected] = (int)dap_config_st[pedalSelected].payloadPedalConfig_.preloadForce;
                                Plugin.PedalStatusInstance.UpdatePedalStatus();
                                if ((pedalStateHasAlreadyBeenUpdated_b == false) && (indexOfSelectedPedal_u == pedalSelected))
                                {
                                    double control_rect_value_max = 65535;
                                    pedalStateHasAlreadyBeenUpdated_b = true;
                                    PedalForceTravel_Tab.updatePedalState(pedalState_read_st.payloadPedalBasicState_.pedalPosition_u16, pedalState_read_st.payloadPedalBasicState_.pedalForce_u16);
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

                                }
                                for (int i = 0; i < 3; i++)
                                {
                                    //PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                    Plugin._calculations.PedalFirmwareVersion[pedalSelected, i] = pedalState_read_st.payloadPedalBasicState_.pedalFirmwareVersion_u8[i];
                                }
                            }

                        }

                        if ((length == sizeof(DAP_state_extended_st)))
                        {

                            // parse byte array as config struct
                            DAP_state_extended_st pedalState_ext_read_st = getStateExtFromBytes(data);

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
                                        string filePath = Plugin.logFolderPath + "\\DiyFfbPedalStateLog_" + PedalConstStrings.PedalID[pedalSelected] + "_Wireless" + Plugin._calculations.logDateTime + ".txt";


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
                                                writer.Write("WriterIdx");
                                                writer.Write(", servoStateCycleCount_u32");
                                                writer.Write(", servoPositionTarget_i32");
                                                writer.Write(", servoPositionFeedback_i32");
                                                writer.Write(", servoPositionError_i16");
                                                writer.Write(", servoVoltage_fl32");
                                                writer.Write(", servoCurrentPercent_i16");

                                                writer.Write(", timeInUs_u32");
                                                writer.Write(", cycleCount_u32");
                                                writer.Write(", pedalForceRaw_fl32");
                                                writer.Write(", pedalForceFiltered_fl32");
                                                writer.Write(", forceVelEst_fl32");
                                                writer.Write(", targetPosition_i32");
                                                writer.Write(", currentSpeedInHz_i32");
                                                writer.Write(", brakeResistorState_b");
                                                writer.Write(", oscillationMonitorValue_u8");

                                                writer.Write(", admittance_expectedForce_N");
                                                writer.Write(", admittance_isOscillating");
                                                writer.Write(", admittance_admittancePsi_N");
                                                writer.Write(", admittance_virtualMass_kg");
                                                writer.Write(", admittance_virtualDamping_Ns_m");

                                                writer.Write(", admittance_virtualPosition_m");
                                                writer.Write(", admittance_virtualVelocity_mps");
                                                writer.Write(", admittance_virtualAcceleration_mps2");

                                                writer.Write("\n");
                                            }

                                        }

                                        using (StreamWriter writer = new StreamWriter(filePath, true))
                                        {
                                            var state = pedalState_ext_read_st.payloadPedalExtendedState_;
                                            writeCntr++;

                                            // Build the entire string in one line using interpolation
                                            writer.WriteLine(
                                                $"{writeCntr}" +

                                                $",{state.servoStateCycleCount_u32}" +
                                                $",{state.servoPositionTarget_i32}" +
                                                $",{state.servoPositionFeedback_i32}" +
                                                $",{state.servoPositionError_i16}" +
                                                $",{state.servoVoltage0p1V_i16 / 10.0f}" +
                                                $",{state.servoCurrentPercent_i16}" +

                                                $",{state.timeInUs_u32}" +
                                                $",{state.cycleCount_u32}" +
                                                $",{state.pedalForceRaw_fl32}" +
                                                $",{state.pedalForceFiltered_fl32}" +
                                                $",{state.forceVelEst_fl32}" +
                                                $",{state.targetPosition_i32}" +
                                                $",{state.currentSpeedInHz_i32}" +
                                                $",{state.brakeResistorState_b}" +
                                                $",{state.oscillationMonitorValue_u8}" +
                                                $",{state.admittance_expectedForce_N}" +
                                                $",{state.admittance_isOscillating}" +
                                                $",{state.admittance_admittancePsi_N}" +
                                                $",{state.admittance_virtualMass_kg}" +
                                                $",{state.admittance_virtualDamping_Ns_m}" +
                                                $",{state.admittance_virtualPosition_m}" +
                                                $",{state.admittance_virtualVelocity_mps}" +
                                                $",{state.admittance_virtualAcceleration_mps2}"
                                                );
                                        }


                                    }
                                }
                            }
                        }
                        //
                        if ((length == sizeof(DAP_bridge_state_st)))
                        {

                            // parse byte array as config struct
                            DAP_bridge_state_st bridge_state = getStateBridgeFromBytes(data);
                            string buffer_string = BitConverter.ToString(data);
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
                                if (Plugin._calculations.bridgeConnectionStatus == BridgeConnectStateEnum.BRIDGE_ENTRY_CONNECT)
                                {
                                    Plugin._calculations.bridgeConnectionStatus = BridgeConnectStateEnum.BRIDGE_IS_READY;
                                }
                                Plugin._calculations.bridgeConnetionlastTime = DateTime.Now;

                                if (bridge_state.payloadBridgeState_.unassignedPedalCount > 0 && Plugin._calculations.unassignedPedalCount != bridge_state.payloadBridgeState_.unassignedPedalCount)
                                {

                                    string tmp = bridge_state.payloadBridgeState_.unassignedPedalCount + " unassigned pedals founded!";
                                    ToastNotification("New Pedal Detected", tmp);
                                }
                                Plugin._calculations.unassignedPedalCount = bridge_state.payloadBridgeState_.unassignedPedalCount;

                                for (int pedalIDX = 0; pedalIDX < 3; pedalIDX++)
                                {
                                    Plugin._calculations.rssi[pedalIDX] = bridge_state.payloadBridgeState_.Pedal_RSSI_realtime[pedalIDX];
                                    int macInitialAddress = pedalIDX * 6;
                                    for (int macIndex = 0; macIndex < 6; macIndex++)
                                    {
                                        Plugin._calculations.unassignedPedalMacaddress[pedalIDX][macIndex] = bridge_state.payloadBridgeState_.macAddressDetection[macInitialAddress + macIndex];
                                    }
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
                                    // ToastNotification("Wireless Connection", connection_tmp);
                                    updateTheGuiFromConfig();
                                    wireless_connection_update = false;
                                }

                                //fill the version info
                                for (int i = 0; i < 3; i++)
                                {
                                    dap_bridge_state_st.payloadBridgeState_.Bridge_firmware_version_u8[i] = bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                    Plugin._calculations.BridgeFirmwareVersion[i] = bridge_state.payloadBridgeState_.Bridge_firmware_version_u8[i];
                                }

                            }
                        }
                        //
                        if (length == sizeof(DAP_config_st))
                        {

                            // parse byte array as config struct
                            DAP_config_st pedalConfig_read_st = getConfigFromBytes(data);

                            // check whether receive struct is plausible
                            DAP_config_st* v_config = &pedalConfig_read_st;
                            byte* p_config = (byte*)v_config;
                            UInt16 pedalSelected = pedalConfig_read_st.payloadHeader_.PedalTag;
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
                                if (Plugin._calculations.pedalWirelessStatus[pedalSelected] == WirelessConnectStateEnum.PEDAL_GET_BASIC_PACKETS_OVER_ESPNOW)
                                {
                                    Plugin._calculations.pedalWirelessStatus[pedalSelected] = WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY;
                                }
                                waiting_for_pedal_config[pedalSelected] = false;
                                dap_config_st[pedalSelected] = pedalConfig_read_st;
                                Plugin._calculations.configPreviewLock[pedalSelected] = true;
                                Plugin._calculations.configPreviewLockLast[pedalSelected] = DateTime.Now;
                                
                                updateTheGuiFromConfig();
                                string timestamp = DateTime.Now.ToString("HH:mm:ss");
                                TextBox_serialMonitor_bridge.Text += "["+ timestamp+ "] Pedal:" + pedalSelected + " Payload config payload check: " + check_payload_config_b + "\n";
                                TextBox_serialMonitor_bridge.Text += "[" + timestamp + "] Pedal:" + pedalSelected + " Payload config crc check: " + check_crc_config_b + "\n";
                                if (pedalConfig_read_st.payloadPedalConfig_.configHash_u32 == (uint)175245064)
                                {
                                    // if pedal return DefaultConfig, clear the default setting and ask re send a default config in
                                    Plugin.Settings.DefaultConfig[pedalSelected] = "";
                                    Plugin._calculations.ConfigEditing[pedalSelected] = "";
                                    ToastNotification($"No Startup Config found in Pedal{PedalConstStrings.PedalID[pedalSelected]}", $"{PedalConstStrings.PedalID[pedalSelected]}: Please Set a Config as Startup");
                                }
                                else
                                {
                                    Plugin._calculations.ConfigEditing[pedalSelected] = Plugin.ConfigService.ConfigHashMap.GetFileName(pedalConfig_read_st.payloadPedalConfig_.configHash_u32);
                                }

                                Plugin._calculations.IsModifiedConfigNotSave[Plugin.Settings.table_selected] = false;
                                Plugin._calculations.IsApplyingConfig = true;
                                Plugin._calculations.configApplyLockLast = DateTime.Now;
                                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                                
                            }
                            else
                            {
                                string timestamp = DateTime.Now.ToString("HH:mm:ss");
                                TextBox_serialMonitor_bridge.Text += "[" + timestamp + "] Pedal:" + pedalSelected + " Payload config payload check: " + check_payload_config_b + "\n";
                                TextBox_serialMonitor_bridge.Text += "[" + timestamp + "] Pedal:" + pedalSelected + " Payload expected:" + Constants.pedalConfigPayload_type + " Payload get:" + pedalConfig_read_st.payloadHeader_.payloadType + "\n";
                                TextBox_serialMonitor_bridge.Text += "[" + timestamp + "] Pedal:" + pedalSelected + " Payload config crc check: " + check_crc_config_b + "\n";
                                TextBox_serialMonitor_bridge.Text += "[" + timestamp + "] Pedal:" + pedalSelected + " CRC expected" + Plugin.checksumCalc(p_config, sizeof(payloadHeader) + sizeof(payloadPedalConfig)) + " CRC Get:" + pedalConfig_read_st.payloadFooter_.checkSum + "\n";
                            }



                        }
                        //

                        if (length == sizeof(Dap_hidmessage_st))
                        {
                            //PrintHidData(data);
                            Dap_hidmessage_st pedalMessage_read_st = getHidMessageFromBytes(data);
                            bool structChecker = true;
                            if (pedalMessage_read_st.payloadType != Constants.pedalHidMessage_type) structChecker = false;
                            if (pedalMessage_read_st.magicKey1 != Constants.ESPNOW_LOG_MAGIC_KEY) structChecker = false;
                            if (pedalMessage_read_st.magicKey2 != Constants.ESPNOW_LOG_MAGIC_KEY_2) structChecker = false;
                            if (structChecker)
                            {
                                string textContent_orig = System.Text.Encoding.UTF8.GetString(pedalMessage_read_st.text, pedalMessage_read_st.length);
                                string timestamp = DateTime.Now.ToString("HH:mm:ss");
                                string textContent = $"[{timestamp}] {textContent_orig}";
                                TextBox_serialMonitor_bridge.Text += textContent + "\n";
                                if (_serial_monitor_window != null)
                                {
                                    _serial_monitor_window.TextBox_SerialMonitor.Text += textContent + "\n";
                                }
                            }
                        }
                        //
                    }
                    

                });
            }
        }
        /// <summary>
        /// Reassembles chunked HID reports from a direct-connected vendor HID pedal
        /// (PCB_VERSION=14, S3-Zero, UsagePage=0xFF00) and enqueues the complete packet
        /// so that the serial timer can drain it into buffer_appended[].
        /// Called from the HidSharp ReadLoop background thread.
        /// Report layout: [ReportID=0x02][pkt_type][total_len][chunk_len][data...]
        /// </summary>
        public void HidRecieveCallback_DirectPedal(byte[] buffer, int pedalIdx)
        {
            if (buffer == null || buffer.Length < 4 || pedalIdx < 0 || pedalIdx >= 3) return;

            // Detect a DAP attribute reply (marker 0xA1 at buffer[1]).
            // Layout: [ReportID][0xA1][CmdType][ClassId][AttrId_lo][AttrId_hi][Value:8 bytes][...]
            const byte DapAttrMarker = 0xA1;
            if (buffer[1] == DapAttrMarker && buffer.Length >= 14)
            {
                byte cmdType  = buffer[2];
                byte classId  = buffer[3];
                ushort attrId = (ushort)(buffer[4] | (buffer[5] << 8));
                short value   = (short)(BitConverter.ToInt64(buffer, 6) & 0xFFFF);

                if (cmdType == (byte)DapAttrCmdType.Ack && classId == (byte)DapAttrClassId.ServoModbus)
                {
                    Dispatcher.BeginInvoke(new Action(() =>
                    {
                        Servo_Tab?.HandleServoModbusAck(attrId, value);
                    }));
                }
                return; // attr packets are not chunked reassembly
            }

            // buffer[0] = ReportID (0x02), buffer[1] = pkt_type, buffer[2] = totalLen, buffer[3] = chunkLen
            byte type     = buffer[1];
            int  totalLen = buffer[2];
            int  chunkLen = buffer[3];

            if (type == PKT_TYPE_START)
            {
                _pedalHidRxIndex[pedalIdx]   = 0;
                _pedalHidExpected[pedalIdx]  = totalLen;
                _pedalHidReceiving[pedalIdx] = true;
                if (totalLen > _pedalHidRxBuf[pedalIdx].Length)
                {
                    _pedalHidReceiving[pedalIdx] = false;
                    return;
                }
            }

            if (_pedalHidReceiving[pedalIdx] && chunkLen > 0 &&
                _pedalHidRxIndex[pedalIdx] + chunkLen <= _pedalHidRxBuf[pedalIdx].Length)
            {
                Array.Copy(buffer, 4, _pedalHidRxBuf[pedalIdx], _pedalHidRxIndex[pedalIdx], chunkLen);
                _pedalHidRxIndex[pedalIdx] += chunkLen;
            }

            if (_pedalHidReceiving[pedalIdx] && _pedalHidRxIndex[pedalIdx] >= _pedalHidExpected[pedalIdx])
            {
                _pedalHidReceiving[pedalIdx] = false;
                int finalLen = _pedalHidExpected[pedalIdx];
                byte[] packet = new byte[finalLen];
                Array.Copy(_pedalHidRxBuf[pedalIdx], 0, packet, 0, finalLen);
                PedalHidDataQueue[pedalIdx].Enqueue(packet);
            }
        }

        public void PrintHidData(byte[] data)
        {
            int length = data.Length;
            string hexString = string.Empty;
            if (length > 0)
            {
                StringBuilder sb = new StringBuilder();
                foreach (byte b in data)
                {
                    sb.Append($"0x{b:X2}-");
                }

                hexString = sb.ToString().Trim();
                Dispatcher.Invoke(() =>
                {

                    if (TextBox_serialMonitor_bridge.Text.Length > 2000)
                    {
                        TextBox_serialMonitor_bridge.Clear();
                    }

                    TextBox_serialMonitor_bridge.AppendText(hexString + "\n");

                    TextBox_serialMonitor_bridge.ScrollToEnd();
                });
            }
        }
    }
}
