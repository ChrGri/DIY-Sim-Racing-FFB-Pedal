using log4net.Plugin;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using IPlugin = SimHub.Plugins.IPlugin;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public void SendPedalAction(DAP_action_st action_tmp, Byte PedalID)
        {

            action_tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            action_tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            action_tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            action_tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            action_tmp.payloadHeader_.PedalTag = PedalID;
            action_tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            action_tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;

            byte[] newBuffer;
            unsafe
            {
                DAP_action_st* v = &action_tmp;
                byte* p = (byte*)v;
                action_tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                int length = sizeof(DAP_action_st);
                newBuffer = new byte[length];
                newBuffer = getBytes_Action(action_tmp);
            }
            try
            {
                if (Settings.Pedal_ESPNow_Sync_flag[PedalID])
                {
                    if (BridgeHidService.IsConnected)
                    {
                        Task.Delay(100);
                        Task.Run(() => BridgeHidService.SendLargeDataAsync(newBuffer));
                    }
                    else
                    {
                        if (ESPsync_serialPort.IsOpen)
                        {
                            ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.DiscardOutBuffer();
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                        }
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
                SimHub.Logging.Current.Error("FFB_Pedal_Action_Sending_error:" + errorMessage);
            }

        }
        public void SendPedalActionWireless(DAP_action_st action_tmp, Byte PedalID)
        {

            action_tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            action_tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            action_tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            action_tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            action_tmp.payloadHeader_.PedalTag = PedalID;
            action_tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            action_tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            byte[] newBuffer;
            unsafe
            {
                DAP_action_st* v = &action_tmp;
                byte* p = (byte*)v;
                action_tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                int length = sizeof(DAP_action_st);
                newBuffer = new byte[length];
                newBuffer = getBytes_Action(action_tmp);
            }

            try
            {
                if (BridgeHidService.IsConnected)
                {
                    Task.Delay(100);
                    Task.Run(() => BridgeHidService.SendLargeDataAsync(newBuffer));
                }
                else
                {
                    if (ESPsync_serialPort.IsOpen)
                    {
                        ESPsync_serialPort.DiscardInBuffer();
                        ESPsync_serialPort.DiscardOutBuffer();
                        ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                    }
                }

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                SimHub.Logging.Current.Error("FFB_Pedal_Action_Sending_Wireless_error:" + errorMessage);
            }

        }

        public void SendConfig(DAP_config_st tmp, byte PedalIDX)
        {
            byte[] newBuffer;
            unsafe
            {
                tmp.payloadHeader_.PedalTag = PedalIDX;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                tmp.payloadPedalConfig_.pedal_type = PedalIDX;

                DAP_config_st* v = &tmp;
                byte* p = (byte*)v;
                tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));

                int length = sizeof(DAP_config_st);
                newBuffer = new byte[length];
                newBuffer = getBytesConfig(tmp);
            }
            if (Settings.Pedal_ESPNow_Sync_flag[PedalIDX])
            {
                try
                {
                    if (BridgeHidService.IsConnected)
                    {
                        Task.Delay(100);
                        Task.Run(()=> BridgeHidService.SendLargeDataAsync(newBuffer));
                        
                    }
                    else
                    {
                        if (ESPsync_serialPort.IsOpen)
                        {
                            ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.DiscardOutBuffer();
                            // send data
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                        }

                    }
                    //Plugin._serialPort[indexOfSelectedPedal_u].Write("\n");
                }
                catch (Exception caughtEx)
                {
                    string errorMessage = caughtEx.Message;
                    SimHub.Logging.Current.Error("FFB_Pedal_Config_Sending_error:" + errorMessage);
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
            tmp.payloadHeader_.PedalTag = PedalIDX;
            tmp.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            tmp.payloadPedalConfig_.pedal_type = PedalIDX;
            DAP_config_st* v = &tmp;
            byte* p = (byte*)v;
            tmp.payloadFooter_.checkSum = checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
            bool wirelessUpdate = false;
            bool serialUpdate = false;
            if (_calculations.pedalWirelessStatus[PedalIDX] == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY)
            {
                wirelessUpdate = true;
            }
            if (!wirelessUpdate && _calculations.pedalSerialStatus[PedalIDX] == ConnectStateEnum.PEDAL_IS_READY)
            {
                serialUpdate = true;
            }
            if (serialUpdate || wirelessUpdate) SendConfig(tmp, PedalIDX);
        }
        public void SendBridgeAction(DAP_bridge_state_st tmp)
        {
            int length;
            tmp.payLoadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payLoadHeader_.payloadType = (byte)Constants.bridgeStatePayloadType;
            tmp.payLoadHeader_.PedalTag = (byte)99;
            tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            tmp.payLoadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            tmp.payLoadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            tmp.payloadBridgeState_.unassignedPedalCount = 0;
            tmp.payloadBridgeState_.Pedal_availability_0 = 0;
            tmp.payloadBridgeState_.Pedal_availability_1 = 0;
            tmp.payloadBridgeState_.Pedal_availability_2 = 0;


            byte[] newBuffer_2;
            unsafe
            {
                length = sizeof(DAP_bridge_state_st);
                newBuffer_2 = new byte[length];
                DAP_bridge_state_st* v_2 = &tmp;
                byte* p_2 = (byte*)v_2;
                tmp.payloadFooter_.checkSum = checksumCalc(p_2, sizeof(payloadHeader) + sizeof(payloadBridgeState));
            }
            newBuffer_2 = getBytes_Bridge(tmp);
            try
            {
                if (BridgeHidService.IsConnected)
                {
                    Task.Delay(10);
                    Task.Run(() => BridgeHidService.SendLargeDataAsync(newBuffer_2));
                }
                else
                {
                    if (ESPsync_serialPort.IsOpen)
                    {
                        // clear inbuffer 
                        ESPsync_serialPort.DiscardInBuffer();
                        // send query command
                        ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                    }

                }

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                wpfHandle.TextBox2.Text = errorMessage;
            }

        }

        public void SendOTAActionPedal(DAP_action_ota_st tmp, byte deviceID)
        {
            int length;
            tmp.payloadHeader_.PedalTag = deviceID;
            byte[] newBuffer_2;
            unsafe
            {
                length = sizeof(DAP_action_ota_st);
                newBuffer_2 = new byte[length];
                DAP_action_ota_st* v_2 = &tmp;
                byte* p_2 = (byte*)v_2;
                tmp.payloadFooter_.checkSum = checksumCalc(p_2, sizeof(payloadHeader) + sizeof(payloadOtaInfo));
            }
            newBuffer_2 = getBytes_Action_Ota(tmp);
            try
            {
                if ((BridgeHidService.IsConnected || ESPsync_serialPort.IsOpen) && Settings.Pedal_ESPNow_Sync_flag[deviceID])
                {
                    Task.Delay(10);
                    Task.Run(() => BridgeHidService.SendLargeDataAsync(newBuffer_2));
                }
                else
                {
                    if (_serialPort[deviceID].IsOpen && deviceID != 99)
                    {
                        // clear inbuffer 
                        _serialPort[deviceID].DiscardInBuffer();
                        // send query command
                        _serialPort[deviceID].Write(newBuffer_2, 0, newBuffer_2.Length);
                    }
                }

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                wpfHandle.TextBox2.Text = errorMessage;
            }

        }
        public void SendOTAActionBridge(DAP_action_ota_st tmp)
        {
            int length;
            byte[] newBuffer_2;
            unsafe
            {
                length = sizeof(DAP_action_ota_st);
                newBuffer_2 = new byte[length];
                DAP_action_ota_st* v_2 = &tmp;
                byte* p_2 = (byte*)v_2;
                tmp.payloadFooter_.checkSum = checksumCalc(p_2, sizeof(payloadHeader) + sizeof(payloadOtaInfo));
            }
            newBuffer_2 = getBytes_Action_Ota(tmp);
            try
            {
                if (BridgeHidService.IsConnected)
                {
                    Task.Delay(10);
                    Task.Run(() => BridgeHidService.SendLargeDataAsync(newBuffer_2));
                }
                else
                {
                    if (ESPsync_serialPort.IsOpen)
                    {
                        // clear inbuffer 
                        ESPsync_serialPort.DiscardInBuffer();
                        // send query command
                        ESPsync_serialPort.Write(newBuffer_2, 0, newBuffer_2.Length);
                    }
                }

            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
                wpfHandle.TextBox2.Text = errorMessage;
            }

        }
    }
}
