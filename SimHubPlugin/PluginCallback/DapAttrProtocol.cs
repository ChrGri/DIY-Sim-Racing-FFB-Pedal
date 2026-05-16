// ---------------------------------------------------------------------------
// DapAttrProtocol.cs
//
// C# mirror of DapAttributeProtocol.h.
// Attribute-based HID protocol for PC→ESP32 configuration updates.
// ---------------------------------------------------------------------------

using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    public enum DapAttrCmdType : byte
    {
        Write = 0x00,
        Read  = 0x01,
        Ack   = 0x0A,
        Err   = 0x07,
    }

    public enum DapAttrClassId : byte
    {
        Config     = 0x00,
        Action     = 0x01,
        ServoModbus = 0x02,   // iSV57 Modbus register read/write
    }

    public enum DapConfigAttrId : ushort
    {
        // Pedal travel limits
        PedalStartPos                   = 0x0000,
        PedalEndPos                     = 0x0001,

        // Forces
        MaxForce                        = 0x0002,
        PreloadForce                    = 0x0003,

        // Force-vs-travel curve (11 control points each)
        QuantityOfControl               = 0x0010,
        RelativeForce00                 = 0x0011,
        RelativeForce01                 = 0x0012,
        RelativeForce02                 = 0x0013,
        RelativeForce03                 = 0x0014,
        RelativeForce04                 = 0x0015,
        RelativeForce05                 = 0x0016,
        RelativeForce06                 = 0x0017,
        RelativeForce07                 = 0x0018,
        RelativeForce08                 = 0x0019,
        RelativeForce09                 = 0x001A,
        RelativeForce10                 = 0x001B,
        RelativeTravel00                = 0x001C,
        RelativeTravel01                = 0x001D,
        RelativeTravel02                = 0x001E,
        RelativeTravel03                = 0x001F,
        RelativeTravel04                = 0x0020,
        RelativeTravel05                = 0x0021,
        RelativeTravel06                = 0x0022,
        RelativeTravel07                = 0x0023,
        RelativeTravel08                = 0x0024,
        RelativeTravel09                = 0x0025,
        RelativeTravel10                = 0x0026,

        // Joystick mapping (11 orig + 11 mapped)
        NumOfJoystickMapControl         = 0x0030,
        JoystickMapOrig00               = 0x0031,
        JoystickMapOrig01               = 0x0032,
        JoystickMapOrig02               = 0x0033,
        JoystickMapOrig03               = 0x0034,
        JoystickMapOrig04               = 0x0035,
        JoystickMapOrig05               = 0x0036,
        JoystickMapOrig06               = 0x0037,
        JoystickMapOrig07               = 0x0038,
        JoystickMapOrig08               = 0x0039,
        JoystickMapOrig09               = 0x003A,
        JoystickMapOrig10               = 0x003B,
        JoystickMapMapped00             = 0x003C,
        JoystickMapMapped01             = 0x003D,
        JoystickMapMapped02             = 0x003E,
        JoystickMapMapped03             = 0x003F,
        JoystickMapMapped04             = 0x0040,
        JoystickMapMapped05             = 0x0041,
        JoystickMapMapped06             = 0x0042,
        JoystickMapMapped07             = 0x0043,
        JoystickMapMapped08             = 0x0044,
        JoystickMapMapped09             = 0x0045,
        JoystickMapMapped10             = 0x0046,

        // ABS effect
        AbsFrequency                    = 0x0050,
        AbsAmplitude                    = 0x0051,
        AbsPattern                      = 0x0052,
        AbsForceOrTravelBit             = 0x0053,

        // Pedal geometry (mm, int16)
        LengthPedalA                    = 0x0060,
        LengthPedalB                    = 0x0061,
        LengthPedalD                    = 0x0062,
        LengthPedalCHorizontal          = 0x0063,
        LengthPedalCVertical            = 0x0064,
        LengthPedalTravel               = 0x0065,

        // ABS simulation
        SimulateAbsTrigger              = 0x0070,
        SimulateAbsValue                = 0x0071,

        // RPM effect
        RpmMaxFreq                      = 0x0080,
        RpmMinFreq                      = 0x0081,
        RpmAmp                          = 0x0082,

        // Bite point effect
        BpTriggerValue                  = 0x0090,
        BpAmp                           = 0x0091,
        BpFreq                          = 0x0092,
        BpTrigger                       = 0x0093,

        // G-force effect
        GMulti                          = 0x00A0,
        GWindow                         = 0x00A1,

        // Wheel slip effect
        WsAmp                           = 0x00B0,
        WsFreq                          = 0x00B1,

        // Road / impact effect
        RoadMulti                       = 0x00C0,
        RoadWindow                      = 0x00C1,

        // Custom vibrations 1-4
        CvAmp1                          = 0x00D0,
        CvFreq1                         = 0x00D1,
        CvAmp2                          = 0x00D2,
        CvFreq2                         = 0x00D3,
        CvAmp3                          = 0x00D4,
        CvFreq3                         = 0x00D5,
        CvAmp4                          = 0x00D6,
        CvFreq4                         = 0x00D7,

        // Controller / output
        MaxGameOutput                   = 0x00E0,

        // Kalman filter
        KfModelNoise                    = 0x00F0,
        KfModelOrder                    = 0x00F1,

        // Debug & hardware config
        DebugFlags0                     = 0x0100,
        LoadcellRating                  = 0x0101,
        TravelAsJoystickOutput          = 0x0102,
        InvertLoadcellReading           = 0x0103,
        InvertMotorDirection            = 0x0104,
        SpindlePitchMmPerRev            = 0x0105,
        PedalType                       = 0x0106,
        StepLossFunctionFlags           = 0x0107,
        KfJoystick                      = 0x0108,
        KfModelNoiseJoystick            = 0x0109,
        ServoIdleTimeout                = 0x010A,
        MinForceForEffects              = 0x010B,
        ConfigHash                      = 0x010C,
        EndstopDetectionThreshold       = 0x010D,

        // Virtual pedal dynamics
        VirtualPedalMassInPercent       = 0x0110,
        VirtualPedalDampingInPercent    = 0x0111,

        // Endstop parameters
        EndstopStiffnessKgMm            = 0x0120,
        EndstopTravelRangeMm            = 0x0121,

        // Elastomere / spring behavior
        DampingProgression              = 0x0130,

        // Friction
        CoulombFrictionIn0p1N           = 0x0140,

        // Special control attributes
        StoreToEeprom                   = 0xFFFE,
        Commit                          = 0xFFFF,
    }

    public enum DapActionAttrId : ushort
    {
        SystemAction                    = 0x0000,
        TriggerAbs                      = 0x0001,
        Rpm                             = 0x0002,
        GValue                          = 0x0003,
        WheelSlip                       = 0x0004,
        ImpactValue                     = 0x0005,
        StartSystemIdentification       = 0x0006,
        TriggerCv1                      = 0x0007,
        TriggerCv2                      = 0x0008,
        TriggerCv3                      = 0x0009,
        TriggerCv4                      = 0x000A,
        ReturnPedalConfig               = 0x000B,
        RudderAction                    = 0x000C,
        RudderBrakeAction               = 0x000D,
        Execute                         = 0xFFFF,
    }

    // -------------------------------------------------------------------------
    // Wire packet (12 bytes, packed) mirroring DapAttrPacket_t
    // -------------------------------------------------------------------------
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DapAttrPacket
    {
        public byte   CmdType;   // DapAttrCmdType
        public byte   ClassId;   // DapAttrClassId
        public ushort AttrId;    // DapConfigAttrId or DapActionAttrId
        public long   Value;     // float fields bit-exact via FloatBitsToLong

        /// <summary>
        /// Packs this 12-byte attribute packet into a 64-byte HID OUT report.
        ///
        /// Wire layout (matches s_vendor_set_report on ESP32):
        ///   byte[0]  = ReportId_OUTPUT (0x03)
        ///   byte[1]  = DAP_ATTR_HID_MARKER (0xA1)
        ///   byte[2..13] = DapAttrPacket (12 bytes, little-endian)
        ///   byte[14..63] = zeros
        /// </summary>
        public byte[] ToHidReport()
        {
            const byte ReportIdOutput   = 0x03;
            const byte DapAttrHidMarker = 0xA1;

            byte[] report = new byte[64];
            report[0] = ReportIdOutput;
            report[1] = DapAttrHidMarker;

            // Manually serialise the 12-byte packed struct in little-endian order
            report[2] = CmdType;
            report[3] = ClassId;
            report[4] = (byte)(AttrId & 0xFF);
            report[5] = (byte)(AttrId >> 8);
            byte[] valueBytes = BitConverter.GetBytes(Value);
            Array.Copy(valueBytes, 0, report, 6, 8);

            return report;
        }
    }

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
}
