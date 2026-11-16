using System;

namespace DiyFfbPedal
{
    /// <summary>
    /// Scales the ABS amplitude with vehicle speed: no ABS below the start speed,
    /// full ABS from the full speed, smoothstep transition in between.
    /// The speed is low-pass filtered and the start speed has a small hysteresis
    /// so the effect does not flicker around standstill.
    /// </summary>
    internal sealed class AbsSpeedScaling
    {
        internal const double HysteresisKmh = 3.0;
        internal const double FilterTimeConstantS = 0.1;

        private double filteredSpeedKmh = 0.0;
        private bool active = false;
        private DateTime lastUpdate = DateTime.MinValue;

        /// <summary>Smoothstep ramp in [0, 1] between startKmh and fullKmh.</summary>
        internal static double Smoothstep(double speedKmh, double startKmh, double fullKmh)
        {
            if (fullKmh <= startKmh)
            {
                return speedKmh >= startKmh ? 1.0 : 0.0;
            }
            double t = (speedKmh - startKmh) / (fullKmh - startKmh);
            t = Math.Max(0.0, Math.Min(1.0, t));
            return t * t * (3.0 - 2.0 * t);
        }

        /// <summary>Updates the filter and returns the ABS scale in [0, 1].</summary>
        internal double Update(double speedKmh, double startKmh, double fullKmh, DateTime now)
        {
            if (double.IsNaN(speedKmh) || double.IsInfinity(speedKmh)) speedKmh = 0.0;
            speedKmh = Math.Abs(speedKmh);

            double dt = (lastUpdate == DateTime.MinValue) ? -1.0 : (now - lastUpdate).TotalSeconds;
            if (dt < 0.0 || dt > 1.0)
            {
                // first sample or long gap (pause, game change): no filtering
                filteredSpeedKmh = speedKmh;
            }
            else
            {
                double alpha = dt / (FilterTimeConstantS + dt);
                filteredSpeedKmh += alpha * (speedKmh - filteredSpeedKmh);
            }
            lastUpdate = now;

            // hysteresis around the start speed
            if (active)
            {
                if (filteredSpeedKmh < startKmh) active = false;
            }
            else
            {
                if (filteredSpeedKmh >= startKmh + HysteresisKmh) active = true;
            }

            return active ? Smoothstep(filteredSpeedKmh, startKmh, fullKmh) : 0.0;
        }

        /// <summary>Converts a scale in [0, 1] into the payload attenuation byte (0 = full, 255 = muted).</summary>
        internal static byte ToAttenuation(double scale)
        {
            if (double.IsNaN(scale)) scale = 1.0;
            scale = Math.Max(0.0, Math.Min(1.0, scale));
            return (byte)Math.Round((1.0 - scale) * 255.0);
        }
    }
}
