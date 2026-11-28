using System;
using System.Windows.Media;
using System.Windows.Shapes;

namespace DiyFfbPedal.UIFunction
{
    // Mirror of the two LEDs on the servo drive (green PWR, red ALM), shared by
    // the home dashboard and the pedal info section.
    public static class ServoLedIndicator
    {
        public enum State { Off, Ok, Error }

        public static readonly Brush LedOffBrush = CreateFrozenBrush(42, 48, 56);
        public static readonly Brush LedGreenBrush = CreateFrozenBrush(46, 204, 64);
        public static readonly Brush LedRedBrush = CreateFrozenBrush(231, 46, 46);

        // Error codes are repeated in every state packet while the fault persists;
        // the hold time bridges single lost packets.
        public static readonly TimeSpan ErrorHoldTime = TimeSpan.FromMilliseconds(1500);
        private const long BlinkHalfPeriodMs = 400;

        public static bool IsPedalConnected(CalculationVariables calc, int pedal)
        {
            return calc != null && pedal >= 0 && pedal < 3 &&
                (calc.pedalWirelessStatus[pedal] == WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY ||
                 calc.pedalSerialStatus[pedal] == ConnectStateEnum.PEDAL_IS_READY);
        }

        public static State GetState(CalculationVariables calc, int pedal, bool connected)
        {
            if (!connected || calc == null || pedal < 0 || pedal >= 3) return State.Off;
            byte status = calc.ServoStatus[pedal];
            // servo was powered down on purpose (idle timeout), nothing is talking to it
            if (status == (byte)enumServoStatus.Idle) return State.Off;
            if (status == (byte)enumServoStatus.ForceStop ||
                calc.HasRecentServoError(pedal, ErrorHoldTime) ||
                calc.HasRecentServoAlarm(pedal, ErrorHoldTime)) return State.Error;
            if (status == (byte)enumServoStatus.On) return State.Ok;
            // not connected yet and no error reported (e.g. still starting up)
            return State.Off;
        }

        public static string GetText(CalculationVariables calc, int pedal, bool connected, State state)
        {
            byte status = (calc != null && pedal >= 0 && pedal < 3) ? calc.ServoStatus[pedal] : (byte)0;
            switch (state)
            {
                case State.Ok:
                    return "Servo OK";
                case State.Error:
                    if (status == (byte)enumServoStatus.ForceStop) return "Servo force stopped";
                    if (calc.HasRecentServoAlarm(pedal, ErrorHoldTime))
                    {
                        ushort code = calc.ServoAlarmCode != null ? calc.ServoAlarmCode[pedal] : (ushort)0;
                        // older firmware only sends error code 13 without the alarm code
                        return code != 0 ? "Servo alarm " + FormatAlarmCode(code) + " " + DecodeAlarm(code) : "Servo alarm";
                    }
                    return "Servo not responding";
                default:
                    return connected && status == (byte)enumServoStatus.Idle ? "Servo idle" : "Servo: no connection";
            }
        }

        // Short text for small spaces, e.g. "Alarm Er.0D0".
        public static string GetShortText(CalculationVariables calc, int pedal, bool connected, State state)
        {
            if (state == State.Error && calc != null && pedal >= 0 && pedal < 3 &&
                calc.ServoStatus[pedal] != (byte)enumServoStatus.ForceStop &&
                calc.HasRecentServoAlarm(pedal, ErrorHoldTime))
            {
                ushort code = calc.ServoAlarmCode != null ? calc.ServoAlarmCode[pedal] : (ushort)0;
                return code != 0 ? "Alarm " + FormatAlarmCode(code) : "Alarm";
            }
            if (state == State.Error)
            {
                return calc != null && pedal >= 0 && pedal < 3 && calc.ServoStatus[pedal] == (byte)enumServoStatus.ForceStop
                    ? "Force stop" : "Not responding";
            }
            return null;
        }

        public static string FormatAlarmCode(ushort code)
        {
            return "Er." + (code & 0x0FFF).ToString("X3");
        }

        // Same table as decodeServoAlarmString() in the firmware (isv57communication.cpp).
        public static string DecodeAlarm(ushort code)
        {
            switch (code & 0x0FFF)
            {
                case 0x000: return "Normal";
                case 0x0E0:
                case 0x0E1: return "Overcurrent";
                case 0x100: return "Overload";
                case 0x180: return "Excessive position deviation";
                case 0x1A0: return "Overspeed";
                case 0x1A1: return "Motor out of control";
                case 0x0D0: return "Undervoltage";
                case 0x0C0: return "Overvoltage";
                case 0x171:
                case 0x172: return "Encoder parameter error";
                case 0x190: return "Excessive motor vibration";
                case 0x150: return "Encoder disconnected";
                case 0x151:
                case 0x170: return "Encoder data error";
                case 0x152: return "Encoder HALL signal error";
                case 0x240: return "Parameter saving error";
                case 0x570: return "Emergency stop";
                case 0x120: return "Regenerative energy overload";
                case 0x153: return "Encoder battery error";
                case 0x210:
                case 0x211:
                case 0x212: return "Input configuration error";
                default: return "Unknown alarm";
            }
        }

        // Sets both LEDs; the red one blinks while in error state.
        public static void Apply(Ellipse pwrLed, Ellipse almLed, State state)
        {
            bool blinkOn = (DateTime.UtcNow.Ticks / TimeSpan.TicksPerMillisecond / BlinkHalfPeriodMs) % 2 == 0;
            if (pwrLed != null) pwrLed.Fill = state == State.Ok ? LedGreenBrush : LedOffBrush;
            if (almLed != null) almLed.Fill = state == State.Error && blinkOn ? LedRedBrush : LedOffBrush;
        }

        private static Brush CreateFrozenBrush(byte r, byte g, byte b)
        {
            SolidColorBrush brush = new SolidColorBrush(Color.FromRgb(r, g, b));
            brush.Freeze();
            return brush;
        }
    }
}
