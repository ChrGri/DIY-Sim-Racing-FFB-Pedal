using System;
using System.Threading.Tasks;
using DiyFfbPedal.UIFunction;

namespace DiyFfbPedal
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        /// <summary>
        /// Called when the user commits a new CurrentValue for a servo register in the DataGrid.
        /// Converts the register address string (e.g. "Pr1.03") to a Modbus holding-register
        /// address and writes it to the selected pedal via the DAP attribute protocol.
        /// </summary>
        private void Servo_Tab_ServoRegisterValueChanged(object sender, ServoRegisterEntry entry)
        {
            if (Plugin == null || entry == null) return;

            // Parse the Modbus address from the human-readable register name
            if (!DapAttrHelper.TryParseServoAddress(entry.Address, out ushort modbusAddr))
            {
                SimHub.Logging.Current.Warn(
                    $"DIYFFBPedal: Could not parse servo register address '{entry.Address}'");
                return;
            }

            byte pedalIdx = (byte)indexOfSelectedPedal_u;

            var pkt = new DapAttrPacket
            {
                CmdType = (byte)DapAttrCmdType.Write,
                ClassId = (byte)DapAttrClassId.ServoModbus,
                AttrId  = modbusAddr,
                Value   = entry.CurrentValue,
            };

            // Direct HID pedal (PCB_VERSION=14)
            if (Plugin.PedalHidService[pedalIdx]?.IsConnected == true)
            {
                Plugin.PedalHidService[pedalIdx].SendAttrPacket(pkt);
                return;
            }

            // Wireless / bridge path
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
            {
                byte[] report = pkt.ToHidReport();
                if (Plugin.BridgeHidService.IsConnected)
                {
                    Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(report));
                }
                else if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        Plugin.ESPsync_serialPort.Write(report, 0, report.Length);
                    }
                    catch (Exception ex)
                    {
                        SimHub.Logging.Current.Error("ServoRegisterWrite serial error: " + ex.Message);
                    }
                }
                return;
            }

            // Wired serial pedal
            if (Plugin._serialPort[pedalIdx].IsOpen)
            {
                byte[] report = pkt.ToHidReport();
                try
                {
                    Plugin._serialPort[pedalIdx].Write(report, 0, report.Length);
                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error("ServoRegisterWrite serial error: " + ex.Message);
                }
            }
        }

        /// <summary>
        /// Called when the user clicks "Load current from servo".
        /// Sends a READ attr packet for every servo register to the selected pedal.
        /// Responses arrive asynchronously and are dispatched to Servo_Tab.HandleServoModbusAck().
        /// </summary>
        private async void Servo_Tab_LoadFromServoRequested(object sender, EventArgs e)
        {
            if (Plugin == null) return;

            byte pedalIdx = (byte)indexOfSelectedPedal_u;

            foreach (var entry in Servo_Tab.ServoRegisters)
            {
                if (!DapAttrHelper.TryParseServoAddress(entry.Address, out ushort modbusAddr))
                    continue;

                var pkt = new DapAttrPacket
                {
                    CmdType = (byte)DapAttrCmdType.Read,
                    ClassId = (byte)DapAttrClassId.ServoModbus,
                    AttrId  = modbusAddr,
                    Value   = 0,
                };

                if (Plugin.PedalHidService[pedalIdx]?.IsConnected == true)
                {
                    Plugin.PedalHidService[pedalIdx].SendAttrPacket(pkt);
                }
                else if (Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
                {
                    byte[] report = pkt.ToHidReport();
                    if (Plugin.BridgeHidService.IsConnected)
                        await Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(report));
                    else if (Plugin.ESPsync_serialPort.IsOpen)
                    {
                        try { Plugin.ESPsync_serialPort.Write(report, 0, report.Length); }
                        catch (Exception ex) { SimHub.Logging.Current.Error("ServoRegisterRead serial error: " + ex.Message); }
                    }
                }
                else if (Plugin._serialPort[pedalIdx].IsOpen)
                {
                    byte[] report = pkt.ToHidReport();
                    try { Plugin._serialPort[pedalIdx].Write(report, 0, report.Length); }
                    catch (Exception ex) { SimHub.Logging.Current.Error("ServoRegisterRead serial error: " + ex.Message); }
                }

                // Small inter-packet delay to avoid flooding the ESP32
                await Task.Delay(5);
            }
        }
        /// <summary>
        /// Called when the user clicks "Flash to servo".
        /// Sends the iSV57 NVM-save command: write 0x5555 to holding register 0x019A.
        /// The servo persists all RAM parameters to flash; a power-cycle is required afterwards.
        /// </summary>
        private void Servo_Tab_FlashToServoRequested(object sender, EventArgs e)
        {
            if (Plugin == null) return;

            byte pedalIdx = (byte)indexOfSelectedPedal_u;

            // iSV57 NVM save command: register 0x019A = 0x5555
            // (identified via logic analyzer, see isv57communication.cpp)
            var pkt = new DapAttrPacket
            {
                CmdType = (byte)DapAttrCmdType.Write,
                ClassId = (byte)DapAttrClassId.ServoModbus,
                AttrId  = 0x019A,
                Value   = 0x5555,
            };

            if (Plugin.PedalHidService[pedalIdx]?.IsConnected == true)
            {
                Plugin.PedalHidService[pedalIdx].SendAttrPacket(pkt);
                return;
            }

            byte[] report = pkt.ToHidReport();
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
            {
                if (Plugin.BridgeHidService.IsConnected)
                    System.Threading.Tasks.Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(report));
                else if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try { Plugin.ESPsync_serialPort.Write(report, 0, report.Length); }
                    catch (Exception ex) { SimHub.Logging.Current.Error("FlashToServo serial error: " + ex.Message); }
                }
            }
            else if (Plugin._serialPort[pedalIdx].IsOpen)
            {
                try { Plugin._serialPort[pedalIdx].Write(report, 0, report.Length); }
                catch (Exception ex) { SimHub.Logging.Current.Error("FlashToServo serial error: " + ex.Message); }
            }
        }
    }
}
