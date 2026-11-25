using System;
using System.Collections.Generic;

namespace DiyFfbPedal
{
    // Test scenarios for the script effects: short loops of simulated SimHub
    // property values, played in test mode instead of a game.
    public class EffectTestScenario
    {
        public string Id;          // translation key part
        public string Name;
        public double Duration_s;
        // time within the loop -> property values
        public Action<double, Dictionary<string, object>> Values;

        public string LocalName { get { return Loc.T("TextTestScenario" + Id, Name); } }
    }

    public static class EffectTestScenarios
    {
        private const string GD = "DataCorePlugin.GameData.";
        private const string IR = "DataCorePlugin.GameRawData.Telemetry.";
        private const string SlipLock = "ShakeITMotorsV3Plugin.Export.wheelsliplock.All";

        private static double Clamp01(double x) { return Math.Max(0.0, Math.Min(1.0, x)); }
        private static double Lerp(double a, double b, double x) { return a + (b - a) * Clamp01(x); }
        // smooth bump of width w (s) around t0
        private static double Bump(double t, double t0, double w) { double x = (t - t0) / w; return Math.Exp(-x * x); }

        public static readonly IReadOnlyList<EffectTestScenario> All = new List<EffectTestScenario>
        {
            new EffectTestScenario
            {
                Id = "Braking",
                Name = "Full braking with ABS",
                Duration_s = 6.0,
                Values = (t, v) =>
                {
                    bool braking = t >= 1.0 && t < 5.0;
                    double speed = t < 1.0 ? 200.0 : t < 5.0 ? Lerp(200.0, 20.0, (t - 1.0) / 4.0) : 20.0;
                    v[GD + "SpeedKmh"] = speed;
                    v[GD + "Brake"] = braking ? 100.0 : 0.0;
                    v[GD + "Throttle"] = t < 1.0 ? 100.0 : 0.0;
                    // ABS cycles while braking above 30 km/h
                    v[GD + "ABSActive"] = braking && speed > 30.0 && ((t - 1.0) % 0.45) < 0.3;
                    v[IR + "BrakeABSactive"] = v[GD + "ABSActive"];
                    v[SlipLock] = braking ? 40.0 * Clamp01((t - 1.0) / 0.5) * (speed > 30.0 ? 1.0 : 0.2) : 0.0;
                    v[GD + "GlobalAccelerationG"] = braking ? -1.2 : (t < 1.0 ? 0.3 : 0.0);
                    v[GD + "AccelerationSurge"] = braking ? -12.0 : (t < 1.0 ? 3.0 : 0.0);
                    v[GD + "Rpms"] = Lerp(3000.0, 7000.0, speed / 200.0);
                    v[GD + "MaxRpm"] = 8000.0;
                },
            },
            new EffectTestScenario
            {
                Id = "RevUp",
                Name = "Rev up through the gears",
                Duration_s = 9.0,
                Values = (t, v) =>
                {
                    double gearTime = 1.5;
                    int gear = Math.Min(6, (int)(t / gearTime) + 1);
                    double inGear = (t % gearTime) / gearTime;
                    double rpm = t < 6 * gearTime ? Lerp(3500.0, 8000.0, inGear) : 8000.0; // last part: on the limiter
                    v[GD + "Rpms"] = rpm;
                    v[GD + "MaxRpm"] = 8000.0;
                    v[GD + "Throttle"] = 100.0;
                    v[GD + "Brake"] = 0.0;
                    v[GD + "Gear"] = gear.ToString();
                    v[GD + "SpeedKmh"] = 30.0 * gear * rpm / 8000.0;
                    v[GD + "CarSettings_RPMShiftLight2"] = 7600.0;
                    v[GD + "CarSettings_RPMRedLineReached"] = rpm >= 7800.0;
                    v[IR + "ShiftIndicatorPct"] = Clamp01((rpm - 6000.0) / 1800.0);
                    v[GD + "TCActive"] = false;
                },
            },
            new EffectTestScenario
            {
                Id = "Kerbs",
                Name = "Kerbs and bumps",
                Duration_s = 4.0,
                Values = (t, v) =>
                {
                    bool kerb = (t >= 1.0 && t < 2.0) || (t >= 3.0 && t < 3.5);
                    v[GD + "SpeedKmh"] = 150.0;
                    v[GD + "Throttle"] = 60.0;
                    v[GD + "Brake"] = 0.0;
                    v[IR + "TireLF_RumblePitch"] = kerb ? 40.0 : 0.0;
                    v[IR + "TireRF_RumblePitch"] = kerb ? 38.0 : 0.0;
                    double heave = 20.0 * Bump(t, 0.5, 0.03) - 15.0 * Bump(t, 2.5, 0.04);
                    v[GD + "AccelerationHeave"] = heave;
                    v[IR + "LFshockVel"] = heave / 40.0;
                },
            },
            new EffectTestScenario
            {
                Id = "WheelSpin",
                Name = "Wheel spin and TC",
                Duration_s = 5.0,
                Values = (t, v) =>
                {
                    bool spin = t >= 0.5 && t < 4.0;
                    v[GD + "Throttle"] = t < 4.0 ? 100.0 : 0.0;
                    v[GD + "Brake"] = 0.0;
                    v[GD + "SpeedKmh"] = Lerp(20.0, 120.0, t / 4.0);
                    v[GD + "Rpms"] = 6500.0;
                    v[GD + "MaxRpm"] = 8000.0;
                    v[SlipLock] = spin ? 30.0 + 25.0 * Math.Sin(2.0 * Math.PI * 1.5 * t) : 0.0;
                    v[GD + "TCActive"] = spin && ((t % 0.4) < 0.25);
                },
            },
        };
    }
}
