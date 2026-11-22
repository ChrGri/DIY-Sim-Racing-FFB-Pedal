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
                    if (calc.HasRecentServoAlarm(pedal, ErrorHoldTime)) return "Servo alarm";
                    return "Servo not responding";
                default:
                    return connected && status == (byte)enumServoStatus.Idle ? "Servo idle" : "Servo: no connection";
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
