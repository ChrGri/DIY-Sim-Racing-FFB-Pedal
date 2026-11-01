using System;
using DiyFfbPedal;
class Program
{
    static void Check(bool condition, string name) { if (!condition) throw new Exception(name); }
    static void Main()
    {
        Check(ConsoleEffectPolicy.IsGt7("GranTurismo7"), "game code");
        Check(ConsoleEffectPolicy.IsGt7("Gran Turismo 7"), "display name");
        Check(!ConsoleEffectPolicy.IsGt7("IRacing"), "other game");
        Check(!ConsoleEffectPolicy.IsGt7(null), "no selected game");
        Check(ConsoleEffectPolicy.IsConsoleSession("F12025", true, false, true), "F1 console telemetry");
        Check(ConsoleEffectPolicy.IsConsoleSession("FutureConsoleGame", true, false, true), "new SimHub console game");
        Check(ConsoleEffectPolicy.IsConsoleSession("GranTurismo7", false, false, false), "GT7 waiting state");
        Check(!ConsoleEffectPolicy.IsConsoleSession("F12025", true, true, true), "PC F1 keeps existing path");
        Check(!ConsoleEffectPolicy.IsConsoleSession("OtherGame", false, false, true), "stale console data");
        Check(!ConsoleEffectPolicy.IsConsoleSession("OtherGame", true, false, false), "no console telemetry");
        Check(ConsoleEffectPolicy.IsActive(true, true, false, false, false), "console telemetry without local process");
        Check(!ConsoleEffectPolicy.IsActive(false, true, false, false, false), "stale cached data");
        Check(!ConsoleEffectPolicy.IsActive(true, false, false, false, false), "missing data");
        Check(!ConsoleEffectPolicy.IsActive(true, true, true, false, false), "paused");
        Check(!ConsoleEffectPolicy.IsActive(true, true, false, true, false), "menu");
        Check(!ConsoleEffectPolicy.IsActive(true, true, false, false, true), "replay");
        Check(ConsoleEffectPolicy.ShouldProcess(true, true, false), "reused telemetry object");
        Check(!ConsoleEffectPolicy.ShouldProcess(false, true, false), "other games retain gate");
        Check(!ConsoleEffectPolicy.ShouldProcess(true, false, true), "inactive GT7");
        DateTime now = new DateTime(2026, 9, 29);
        Check(ConsoleEffectPolicy.HasRecentTrigger(now.AddMilliseconds(50), now), "short ABS pulse survives output interval");
        Check(!ConsoleEffectPolicy.HasRecentTrigger(now.AddMilliseconds(101), now), "ABS pulse expires");
        Check(!ConsoleEffectPolicy.HasRecentTrigger(now, DateTime.MinValue), "no ABS event");
        Check(ConsoleEffectPolicy.IsDue(now, DateTime.MinValue), "first sample");
        Check(!ConsoleEffectPolicy.IsDue(now.AddMilliseconds(49), now), "rate limit");
        Check(ConsoleEffectPolicy.IsDue(now.AddMilliseconds(50), now), "constant RPM refresh");
        Check(ConsoleEffectPolicy.IsDue(now.AddSeconds(-1), now), "clock rollback recovery");
        Check(ConsoleEffectPolicy.ClampPercent(-1) == 0, "negative RPM");
        Check(ConsoleEffectPolicy.ClampPercent(50) == 50, "half RPM");
        Check(ConsoleEffectPolicy.ClampPercent(300) == 100, "over-range RPM");
        Check(ConsoleEffectPolicy.ClampPercent(double.NaN) == 0, "NaN");
        Check(ConsoleEffectPolicy.ClampPercent(double.PositiveInfinity) == 0, "infinity");
        Console.WriteLine("PASS: console detection, PC preservation, state gating, pacing and RPM clamping");
    }
}
