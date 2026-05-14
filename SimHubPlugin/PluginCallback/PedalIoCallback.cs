using log4net.Plugin;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using IPlugin = SimHub.Plugins.IPlugin;

namespace DiyFfbPedal
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
                            //ESPsync_serialPort.DiscardInBuffer();
                            ESPsync_serialPort.DiscardOutBuffer();
                            ESPsync_serialPort.Write(newBuffer, 0, newBuffer.Length);
                        }
                    }
                }
                else
                {
                    if (PedalHidService[PedalID]?.IsConnected == true)
                    {
                        Task.Run(() => PedalHidService[PedalID].SendLargeDataAsync(newBuffer));
                    }
                    else if (_serialPort[PedalID].IsOpen)
                    {

                        // clear inbuffer 
                        //_serialPort[PedalID].DiscardInBuffer();
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
                            //ESPsync_serialPort.DiscardInBuffer();
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
                if (PedalHidService[PedalIDX]?.IsConnected == true)
                {
                    _ = SendConfigViaAttributesAsync(tmp, PedalIDX);
                }
                else if (_serialPort[PedalIDX].IsOpen)
                {
                    // clear inbuffer 
                    //_serialPort[PedalIDX].DiscardInBuffer();
                    _serialPort[PedalIDX].DiscardOutBuffer();
                    // send data
                    _serialPort[PedalIDX].Write(newBuffer, 0, newBuffer.Length);
                }
            }
        }

        /// <summary>
        /// Sends every field of <paramref name="config"/> to the ESP32 via individual HID
        /// attribute packets (DAP attribute protocol), then commits the shadow config.
        /// A 1 ms delay between packets prevents queue overflow on the ESP32 side.
        /// </summary>
        private async Task SendConfigViaAttributesAsync(DAP_config_st config, byte pedalIdx)
        {
            var hid = PedalHidService[pedalIdx];
            if (hid == null || !hid.IsConnected) return;

            var c = config.payloadPedalConfig_;
            bool storeToEeprom = config.payloadHeader_.storeToEeprom != 0;

            DapAttrPacket MakePkt(DapConfigAttrId id, long value) =>
                new DapAttrPacket
                {
                    CmdType = (byte)DapAttrCmdType.Write,
                    ClassId = (byte)DapAttrClassId.Config,
                    AttrId  = (ushort)id,
                    Value   = value,
                };

            // Pedal travel limits
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.PedalStartPos,               c.pedalStartPosition));          await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.PedalEndPos,                 c.pedalEndPosition));            await Task.Delay(1);

            // Forces (float → bit-exact long)
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.MaxForce,                    DapAttrHelper.FloatBitsToLong(c.maxForce)));     await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.PreloadForce,                DapAttrHelper.FloatBitsToLong(c.preloadForce))); await Task.Delay(1);

            // Force-vs-travel curve
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.QuantityOfControl,           c.quantityOfControl));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce00,             c.relativeForce00));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce01,             c.relativeForce01));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce02,             c.relativeForce02));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce03,             c.relativeForce03));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce04,             c.relativeForce04));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce05,             c.relativeForce05));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce06,             c.relativeForce06));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce07,             c.relativeForce07));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce08,             c.relativeForce08));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce09,             c.relativeForce09));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeForce10,             c.relativeForce10));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel00,            c.relativeTravel00));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel01,            c.relativeTravel01));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel02,            c.relativeTravel02));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel03,            c.relativeTravel03));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel04,            c.relativeTravel04));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel05,            c.relativeTravel05));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel06,            c.relativeTravel06));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel07,            c.relativeTravel07));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel08,            c.relativeTravel08));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel09,            c.relativeTravel09));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RelativeTravel10,            c.relativeTravel10));            await Task.Delay(1);

            // Joystick mapping
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.NumOfJoystickMapControl,     c.numOfJoystickMapControl));     await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig00,           c.joystickMapOrig00));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig01,           c.joystickMapOrig01));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig02,           c.joystickMapOrig02));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig03,           c.joystickMapOrig03));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig04,           c.joystickMapOrig04));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig05,           c.joystickMapOrig05));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig06,           c.joystickMapOrig06));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig07,           c.joystickMapOrig07));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig08,           c.joystickMapOrig08));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig09,           c.joystickMapOrig09));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapOrig10,           c.joystickMapOrig10));           await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped00,         c.joystickMapMapped00));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped01,         c.joystickMapMapped01));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped02,         c.joystickMapMapped02));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped03,         c.joystickMapMapped03));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped04,         c.joystickMapMapped04));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped05,         c.joystickMapMapped05));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped06,         c.joystickMapMapped06));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped07,         c.joystickMapMapped07));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped08,         c.joystickMapMapped08));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped09,         c.joystickMapMapped09));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.JoystickMapMapped10,         c.joystickMapMapped10));         await Task.Delay(1);

            // ABS effect
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.AbsFrequency,               c.absFrequency));                await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.AbsAmplitude,               c.absAmplitude));                await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.AbsPattern,                 c.absPattern));                  await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.AbsForceOrTravelBit,        c.absForceOrTarvelBit));         await Task.Delay(1);

            // Pedal geometry (int16 fields)
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalA,               c.lengthPedal_a));               await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalB,               c.lengthPedal_b));               await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalD,               c.lengthPedal_d));               await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalCHorizontal,     c.lengthPedal_c_horizontal));    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalCVertical,       c.lengthPedal_c_vertical));      await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LengthPedalTravel,          c.lengthPedal_travel));          await Task.Delay(1);

            // ABS simulation
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.SimulateAbsTrigger,         c.Simulate_ABS_trigger));        await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.SimulateAbsValue,           c.Simulate_ABS_value));          await Task.Delay(1);

            // RPM effect
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RpmMaxFreq,                 c.RPM_max_freq));                await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RpmMinFreq,                 c.RPM_min_freq));                await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RpmAmp,                     c.RPM_AMP));                     await Task.Delay(1);

            // Bite point effect
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.BpTriggerValue,             c.BP_trigger_value));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.BpAmp,                      c.BP_amp));                      await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.BpFreq,                     c.BP_freq));                     await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.BpTrigger,                  c.BP_trigger));                  await Task.Delay(1);

            // G-force effect
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.GMulti,                     c.G_multi));                     await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.GWindow,                    c.G_window));                    await Task.Delay(1);

            // Wheel slip effect
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.WsAmp,                      c.WS_amp));                      await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.WsFreq,                     c.WS_freq));                     await Task.Delay(1);

            // Road / impact effect (C# fields Impact_multi / Impact_window)
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RoadMulti,                  c.Impact_multi));                await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.RoadWindow,                 c.Impact_window));               await Task.Delay(1);

            // Custom vibrations 1-4
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvAmp1,                     c.CV_amp_1));                    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvFreq1,                    c.CV_freq_1));                   await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvAmp2,                     c.CV_amp_2));                    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvFreq2,                    c.CV_freq_2));                   await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvAmp3,                     c.CV_amp_3));                    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvFreq3,                    c.CV_freq_3));                   await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvAmp4,                     c.CV_amp_4));                    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CvFreq4,                    c.CV_freq_4));                   await Task.Delay(1);

            // Controller / output
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.MaxGameOutput,              c.maxGameOutput));               await Task.Delay(1);

            // Kalman filter
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.KfModelNoise,               c.kf_modelNoise));               await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.KfModelOrder,               c.kf_modelOrder));               await Task.Delay(1);

            // Debug & hardware config
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.DebugFlags0,                c.debug_flags_0));               await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.LoadcellRating,             c.loadcell_rating));             await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.TravelAsJoystickOutput,     c.travelAsJoystickOutput_u8));   await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.InvertLoadcellReading,      c.invertLoadcellReading_u8));    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.InvertMotorDirection,       c.invertMotorDirection_u8));     await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.SpindlePitchMmPerRev,       c.spindlePitch_mmPerRev_u8));    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.PedalType,                  c.pedal_type));                  await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.StepLossFunctionFlags,      c.stepLossFunctionFlags_u8));    await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.KfJoystick,                 c.kf_Joystick_u8));              await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.KfModelNoiseJoystick,       c.kf_modelNoise_joystick));      await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.ServoIdleTimeout,           c.servoIdleTimeout));            await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.MinForceForEffects,         c.minForceForEffects));          await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.ConfigHash,                 c.configHash_u32));              await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.EndstopDetectionThreshold,  c.endstopDetectionThreshold));   await Task.Delay(1);

            // Virtual pedal dynamics
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.VirtualPedalMassInPercent,  c.virtualPedalMass_u8));         await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.VirtualPedalDampingInPercent, c.virtualPedalDamping_u8));    await Task.Delay(1);

            // Endstop parameters
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.EndstopStiffnessKgMm,       c.endstopStiffness_kg_mm_u8));   await Task.Delay(1);
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.EndstopTravelRangeMm,       c.endstopTravelRange_mm_u8));    await Task.Delay(1);

            // Elastomere / spring behavior
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.DampingProgression,         c.dampingProgression_u8));       await Task.Delay(1);

            // Friction
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.CoulombFrictionIn0p1N,      c.coulombFrictionIn0p1N_u8));    await Task.Delay(1);

            // Optional: mark for EEPROM save before commit
            if (storeToEeprom)
            {
                hid.SendAttrPacket(MakePkt(DapConfigAttrId.StoreToEeprom, 1));     await Task.Delay(1);
            }

            // Commit — pushes the shadow config to the active config queue on ESP32
            hid.SendAttrPacket(MakePkt(DapConfigAttrId.Commit, 0));
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
            if (!wirelessUpdate && (_calculations.pedalSerialStatus[PedalIDX] == ConnectStateEnum.PEDAL_IS_READY
                                    || _calculations.pedalSerialStatus[PedalIDX] == ConnectStateEnum.PEDAL_GET_BASIC_PACKETS))
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
                        //ESPsync_serialPort.DiscardInBuffer();
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
