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

            // Wireless / bridge path: send as a stand-alone attribute report wrapped in
            // the existing large-data helper so it passes through the bridge unchanged.
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
    }
}
