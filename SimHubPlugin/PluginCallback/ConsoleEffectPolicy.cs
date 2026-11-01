using System;

namespace DiyFfbPedal
{
    internal static class ConsoleEffectPolicy
    {
        internal static bool IsGt7(string game)
        {
            return String.Equals(game, "GranTurismo7", StringComparison.OrdinalIgnoreCase) ||
                String.Equals(game, "Gran Turismo 7", StringComparison.OrdinalIgnoreCase);
        }

        // A live game without a local process is using external telemetry.
        // GT7 is identified while waiting for its first packet, too.
        internal static bool IsConsoleSession(string game, bool running, bool processDetected, bool hasData)
        {
            return IsGt7(game) || (running && hasData && !processDetected);
        }

        // GameRunning describes telemetry availability, not a local Windows process.
        // Never infer a live console session from a cached, nonzero RPM value.
        internal static bool IsActive(bool running, bool hasData, bool paused, bool menu, bool replay)
        {
            return running && hasData && !paused && !menu && !replay;
        }

        internal static bool IsDue(DateTime now, DateTime last)
        {
            return now < last || (now - last).TotalMilliseconds >= 50;
        }

        internal static bool ShouldProcess(bool console, bool active, bool changedObject)
        {
            return active && (console || changedObject);
        }

        internal static bool HasRecentTrigger(DateTime now, DateTime last)
        {
            return last != DateTime.MinValue && now >= last && (now - last).TotalMilliseconds <= 100;
        }

        internal static byte ClampPercent(double value)
        {
            if (Double.IsNaN(value) || Double.IsInfinity(value)) return 0;
            return (byte)Math.Max(0, Math.Min(100, value));
        }
    }
}
