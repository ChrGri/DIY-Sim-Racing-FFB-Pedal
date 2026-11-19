using System;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using DiyFfbPedal.UIFunction;


// -------------------------------------------------------------------------
// Conversion helpers
// -------------------------------------------------------------------------
public static class DapAttrHelper
{
    /// <summary>Bit-exact conversion of a float to the lower 4 bytes of a long.</summary>
    public static long FloatBitsToLong(float f)
    {
        // BitConverter.GetBytes gives us the 4 raw bytes of the float.
        // Interpreting them as an Int32 and sign-extending to Int64 keeps the
        // lower 4 bytes intact, which is what the ESP32 memcpy reads back.
        byte[] bytes = BitConverter.GetBytes(f);
        int i = BitConverter.ToInt32(bytes, 0);
        return i; // implicit widening to long, lower 4 bytes = float bits
    }

    /// <summary>
    /// Converts an iSV57 register address string ("Pr0.00" … "Pr7.49") to
    /// the Modbus holding-register address used by the firmware.
    ///
    /// Register block base addresses (from isv57communication.h):
    ///   pr_0_00 = 0x0000
    ///   pr_1_00 = pr_0_00 + 25  (0x0019)
    ///   pr_2_00 = pr_1_00 + 40  (0x0041)
    ///   pr_3_00 = pr_2_00 + 30  (0x005F)
    ///   pr_4_00 = pr_3_00 + 30  (0x007D)
    ///   pr_5_00 = pr_4_00 + 50  (0x00AF)
    ///   pr_6_00 = pr_5_00 + 40  (0x00D7)
    ///   pr_7_00 = pr_6_00 + 40  (0x00FF)
    /// </summary>
    /// <param name="address">Address string, e.g. "Pr1.03".</param>
    /// <param name="modbusAddr">Resulting Modbus register address.</param>
    /// <returns>True on success, false if the string could not be parsed.</returns>
    public static bool TryParseServoAddress(string address, out ushort modbusAddr)
    {
        modbusAddr = 0;
        if (string.IsNullOrEmpty(address)) return false;

        string s = address.Trim();
        if (s.Length < 2) return false;
        if (!s.StartsWith("Pr", StringComparison.OrdinalIgnoreCase)) return false;

        int dotIdx = s.IndexOf('.');
        if (dotIdx < 0) return false;

        if (!int.TryParse(s.Substring(2, dotIdx - 2), out int block)) return false;
        if (!int.TryParse(s.Substring(dotIdx + 1), out int offset)) return false;

        // Block base addresses (matching #defines in isv57communication.h)
        int[] blockBase = { 0x0000, 0x0019, 0x0041, 0x005F, 0x007D, 0x00AF, 0x00D7, 0x00FF };
        if (block < 0 || block >= blockBase.Length) return false;

        modbusAddr = (ushort)(blockBase[block] + offset);
        return true;
    }
}

namespace DiyFfbPedal
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        // ---------------------------------------------------------------
        // Helper: build a SOF/EOF-framed DAP_servo_config_st byte array
        // that the ESP32 serial parser recognises (DAP_PAYLOAD_TYPE_SERVO_CONFIG_U8).
        // readWriteFlag: 0 = read, 1 = write
        // addresses / values: up to 10 entries; arrays must be the same length.
        // ---------------------------------------------------------------
        private byte[] BuildServoConfigPacket(byte readWriteFlag,
                                              ushort[] addresses,
                                              ushort[] values,
                                              byte pedalIndex = 255)
        {
            var psc = new payloadServoConfig
            {
                readWriteFlag = readWriteFlag,
                numValidFields = (byte)Math.Min(addresses.Length, 10),
                registerAddresses = new ushort[10],
                registerValues = new ushort[10],
            };
            for (int i = 0; i < psc.numValidFields; i++)
            {
                psc.registerAddresses[i] = addresses[i];
                psc.registerValues[i] = (values != null && i < values.Length) ? values[i] : (ushort)0;
            }

            byte targetPedal = pedalIndex != 255 ? pedalIndex : (byte)indexOfSelectedPedal_u;

            var pkt = new DAP_servo_config_st
            {
                payloadHeader_st = new payloadHeader
                {
                    startOfFrame0_u8 = STARTOFFRAME_SERVO_CONFIG[0],
                    startOfFrame1_u8 = STARTOFFRAME_SERVO_CONFIG[1],
                    payloadType = (byte)Constants.servoConfigPayload_type,
                    version = (byte)Constants.pedalConfigPayload_version,
                    storeToEeprom = 0,
                    PedalTag = targetPedal,
                },
                payloadServoConfig_st = psc,
            };

            // Compute CRC over header + payload bytes (matches ESP32 checksumCalculator_u16 call)
            byte[] raw = pkt.toBytes();
            int crcLen = Marshal.SizeOf(typeof(payloadHeader)) + Marshal.SizeOf(typeof(payloadServoConfig));
            ushort crc = Plugin.checksumCalcArray(raw, crcLen);

            pkt.payloadFooter_st = new payloadFooter
            {
                checkSum = crc,
                enfOfFrame0_u8 = ENDOFFRAMCHAR[0],
                enfOfFrame1_u8 = ENDOFFRAMCHAR[1],
            };

            return pkt.toBytes();
        }

        // ---------------------------------------------------------------
        // Helper: send a serial servo config packet on the wired or bridge path
        // ---------------------------------------------------------------
        private void SendServoConfigSerial(byte pedalIdx, byte[] packet)
        {
            if (Plugin.Settings.Pedal_ESPNow_Sync_flag[pedalIdx])
            {
                if (Plugin.BridgeHidService.IsConnected)
                    Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(packet));
                else if (Plugin.ESPsync_serialPort.IsOpen)
                {
                    try { Plugin.ESPsync_serialPort.Write(packet, 0, packet.Length); }
                    catch (Exception ex) { SimHub.Logging.Current.Error("ServoConfig serial error: " + ex.Message); }
                }
            }
            else if (Plugin._serialPort[pedalIdx].IsOpen)
            {
                try { Plugin._serialPort[pedalIdx].Write(packet, 0, packet.Length); }
                catch (Exception ex) { SimHub.Logging.Current.Error("ServoConfig serial error: " + ex.Message); }
            }
        }

        // ---------------------------------------------------------------
        // Public helpers for sending servo config packets (used by ServoRegisterWindow & Servo_Tab)
        // ---------------------------------------------------------------
        public void SendServoRegisterValue(byte pedalIdx, ServoRegisterEntry entry)
        {
            if (Plugin == null || entry == null || !entry.LiveValue.HasValue) return;

            if (!DapAttrHelper.TryParseServoAddress(entry.Address, out ushort modbusAddr))
            {
                SimHub.Logging.Current.Warn(
                    "DIYFFBPedal: Could not parse servo register address '" + entry.Address + "'");
                return;
            }

            byte[] packet = BuildServoConfigPacket(
                readWriteFlag: 1,
                addresses: new ushort[] { modbusAddr },
                values: new ushort[] { (ushort)(entry.LiveValue.Value & 0xFFFF) },
                pedalIndex: pedalIdx);
            SendServoConfigSerial(pedalIdx, packet);
        }

        public void SendServoModbusRead(byte pedalIdx, ushort modbusAddr)
        {
            if (Plugin == null) return;

            byte[] packet = BuildServoConfigPacket(
                readWriteFlag: 0,
                addresses: new ushort[] { modbusAddr },
                values: new ushort[] { 0 },
                pedalIndex: pedalIdx);
            SendServoConfigSerial(pedalIdx, packet);
        }

        public void SendServoModbusBatchRead(byte pedalIdx, System.Collections.Generic.List<ushort> addresses)
        {
            if (Plugin == null || addresses == null || addresses.Count == 0) return;

            ushort[] emptyValues = new ushort[addresses.Count];
            byte[] packet = BuildServoConfigPacket(
                readWriteFlag: 0,
                addresses: addresses.ToArray(),
                values: emptyValues,
                pedalIndex: pedalIdx);
            SendServoConfigSerial(pedalIdx, packet);
        }

        public void SendServoBatchWrite(byte pedalIdx, ServoRegisterEntry[] entries)
        {
            if (Plugin == null || entries == null || entries.Length == 0) return;

            ushort[] addresses = new ushort[entries.Length];
            ushort[] values = new ushort[entries.Length];

            for (int i = 0; i < entries.Length; i++)
            {
                if (DapAttrHelper.TryParseServoAddress(entries[i].Address, out ushort modbusAddr))
                {
                    addresses[i] = modbusAddr;
                    values[i] = entries[i].LiveValue.HasValue ? (ushort)(entries[i].LiveValue.Value & 0xFFFF) : (ushort)0;
                }
            }

            byte[] packet = BuildServoConfigPacket(
                readWriteFlag: 1,
                addresses: addresses,
                values: values,
                pedalIndex: pedalIdx);
            SendServoConfigSerial(pedalIdx, packet);
        }

        public void SendServoFlash(byte pedalIdx)
        {
            if (Plugin == null) return;

            byte[] packet = BuildServoConfigPacket(
                readWriteFlag: 1,
                addresses: new ushort[] { 0x019A },
                values: new ushort[] { 0x5555 },
                pedalIndex: pedalIdx);
            SendServoConfigSerial(pedalIdx, packet);
        }

        unsafe public void SendServoResetToFactory(byte pedalIdx)
        {
            if (Plugin == null) return;

            DAP_config_st cfg = dap_config_st[pedalIdx];
            cfg.payloadHeader_.storeToEeprom = 1;
            cfg.payloadPedalConfig_.debug_flags_0 = (byte)(cfg.payloadPedalConfig_.debug_flags_0 | 32);
            dap_config_st[pedalIdx] = cfg;

            Sendconfig(pedalIdx);
        }

        // ---------------------------------------------------------------
        // Event Handlers for Servo_Tab (backwards-compatibility)
        // ---------------------------------------------------------------
        private void Servo_Tab_ServoRegisterValueChanged(object sender, ServoRegisterEntry entry)
        {
            SendServoRegisterValue((byte)indexOfSelectedPedal_u, entry);
        }

        private void Servo_Tab_ServoModbusReadRequested(object sender, ushort modbusAddr)
        {
            SendServoModbusRead((byte)indexOfSelectedPedal_u, modbusAddr);
        }

        private void Servo_Tab_ServoModbusBatchReadRequested(object sender, System.Collections.Generic.List<ushort> addresses)
        {
            SendServoModbusBatchRead((byte)indexOfSelectedPedal_u, addresses);
        }

        private void Servo_Tab_ServoBatchWriteRequested(object sender, ServoRegisterEntry[] entries)
        {
            SendServoBatchWrite((byte)indexOfSelectedPedal_u, entries);
        }

        private void Servo_Tab_FlashToServoRequested(object sender, EventArgs e)
        {
            SendServoFlash((byte)indexOfSelectedPedal_u);
        }

        unsafe private void Servo_Tab_ResetToFactoryRequested(object sender, EventArgs e)
        {
            SendServoResetToFactory((byte)indexOfSelectedPedal_u);
        }
    }
}
