using System.Collections.Generic;
using System.Text.RegularExpressions;

namespace DiyFfbPedal
{
    // Ready-made script effects. They replace the former built-in effects
    // (ABS/TC, engine/speed, G-force, wheel slip, road impact, custom 1-4).
    // All presets are JavaScript with their parameters as variables at the top,
    // so users adapt them right in the script.
    public class EffectPreset
    {
        public string Category;
        public string Name;
        public string Description;
        public int SuggestedPedal = -1; // 0 clutch, 1 brake, 2 throttle, -1 any
        public EffectPipelineOffsetType OffsetType = EffectPipelineOffsetType.Angle_deg;
        public PipelineScriptLanguage Language = PipelineScriptLanguage.JavaScript;
        public string Script;
        public string FreqScript = "";
        // translation key part; null: derived from the name (built-in presets),
        // empty: no translation (user files without @id)
        public string TranslationId;
        // file the preset was loaded from (null for the built-in list)
        public string FilePath;

        // translation key part, from the English name (e.g. "Wheel lock" -> "WheelLock")
        public string Id
        {
            get
            {
                if (TranslationId != null) return TranslationId;
                return DeriveId(Name);
            }
        }

        public static string DeriveId(string name)
        {
            {
                var id = new System.Text.StringBuilder();
                foreach (string word in Regex.Split(name ?? "", "[^A-Za-z0-9]+"))
                {
                    if (word.Length == 0) continue;
                    id.Append(char.ToUpperInvariant(word[0])).Append(word.Substring(1).ToLowerInvariant());
                }
                return id.ToString();
            }
        }

        public string LocalName { get { return string.IsNullOrEmpty(Id) ? Name : Loc.T("TextPreset" + Id + "Name", Name); } }
        public string LocalDescription { get { return string.IsNullOrEmpty(Id) ? Description : Loc.T("TextPreset" + Id + "Description", Description); } }
        public string LocalCategory { get { return Loc.T("TextPresetCategory" + Category, Category); } }

        public PipelineScriptSlot CreateSlot(int pedal)
        {
            return new PipelineScriptSlot
            {
                Pedal = pedal,
                Name = LocalName,
                Enabled = false, // the user switches it on after a look at the parameters
                OffsetType = (byte)OffsetType,
                Gain = 1.0,
                Language = Language,
                Script = Script,
                FreqScript = FreqScript,
            };
        }
    }

    public static class EffectLibrary
    {
        // the presets shipped with the plugin; written as files into the effects folder
        // (EffectPresetFiles), the library shows the files
        public static readonly IReadOnlyList<EffectPreset> Presets = new List<EffectPreset>
        {
            // ---------------- brake ----------------
            new EffectPreset
            {
                Category = "Brake",
                Name = "ABS",
                SuggestedPedal = 1,
                Description = "ABS pump: pulse waveform (quick kick, smooth return), signal held for 100 ms against flicker, " +
                              "5/15 ms fade in/out, stronger with brake pressure, faded out at low speed.",
                Script =
@"// ---- parameters ----
var f = 15;              // ABS frequency in Hz
var amp = 0.08;          // degrees at full brake and full speed
var minShare = 0.2;      // share of amp when braking lightly
var vStart = 8;          // km/h, below: no ABS
var vFull = 80;          // km/h, above: full amplitude
// --------------------

if ($prop('DataCorePlugin.GameData.ABSActive')) root.absUntil = t + 0.1;
var active = t < root.absUntil;

var env = root.env || 0;
var target = active ? 1 : 0;
env += (target - env) * Math.min(1, dt / (target > env ? 0.005 : 0.015));
root.env = env;
if (!active && env < 0.001) return 0;

var v = $prop('DataCorePlugin.GameData.SpeedKmh') || 0;
root.v = (root.v === undefined) ? v : root.v + (v - root.v) * dt / 0.1;
if (root.v > vStart + 3) root.moving = true;
if (root.v < vStart) root.moving = false;
var x = root.moving ? Math.min(1, Math.max(0, (root.v - vStart) / (vFull - vStart))) : 0;
var speedFactor = x * x * (3 - 2 * x);

var brake = ($prop('DataCorePlugin.GameData.Brake') || 0) / 100;
var pressureFactor = minShare + (1 - minShare) * Math.min(1, Math.max(0, brake));

var p = (t * f) % 1;
var pulse = p < 0.12 ? Math.sin(0.5 * pi * p / 0.12) : 0.5 * (1 + Math.cos(pi * (p - 0.12) / 0.88));
return amp * env * speedFactor * pressureFactor * pulse;",
            },
            new EffectPreset
            {
                Category = "Brake",
                Name = "ABS simple",
                SuggestedPedal = 1,
                Description = "Plain sine while ABS is active, like the former built-in ABS effect.",
                Script =
@"// ---- parameters ----
var f = 15;       // Hz
var amp = 0.05;   // degrees
// --------------------
return $prop('DataCorePlugin.GameData.ABSActive') ? amp * Math.sin(2 * pi * f * t) : 0;",
            },
            new EffectPreset
            {
                Category = "Brake",
                Name = "Wheel lock",
                SuggestedPedal = 1,
                Description = "Vibration growing with wheel lock. Needs the 'wheelsliplock' effect exported in ShakeIt Motors.",
                Script =
@"// ---- parameters ----
var f = 30;          // Hz
var amp = 0.06;      // degrees at full lock
var threshold = 10;  // % lock below which nothing happens
// --------------------
var lock = $prop('ShakeITMotorsV3Plugin.Export.wheelsliplock.All') || 0;
var x = Math.min(1, Math.max(0, (lock - threshold) / (100 - threshold)));
return amp * x * Math.sin(2 * pi * f * t);",
            },
            new EffectPreset
            {
                Category = "Brake",
                Name = "Wheel lock in steps",
                SuggestedPedal = 1,
                Description = "Lock-up warning in steps with hysteresis: 10 % lock gives 30 %, 20 % lock 80 % intensity.",
                Script =
@"// ---- parameters ----
var f = 30;      // Hz
var amp = 0.08;  // degrees at 100 % intensity
// --------------------
var lock = $prop('ShakeITMotorsV3Plugin.Export.wheelsliplock.All') || 0;
var level = root.level || 0;
if (lock >= 20) level = 0.8;
else if (lock >= 10) level = Math.max(level, 0.3);
if (lock < 17 && level > 0.3) level = 0.3;
if (lock < 7) level = 0;
root.level = level;
return amp * level * Math.sin(2 * pi * f * t);",
            },
            new EffectPreset
            {
                Category = "Brake",
                Name = "Deceleration load (G-force)",
                SuggestedPedal = 1,
                OffsetType = EffectPipelineOffsetType.ForceRelative,
                Description = "Brake gets heavier with deceleration, like the former G-force effect. " +
                              "Check the sign of the acceleration in your game with the live plot.",
                Script =
@"// ---- parameters ----
var perG = 0.15;     // +15 % pedal force per 1 g deceleration
var maxRel = 0.3;    // at most +30 %
var tau = 0.1;       // s, smoothing
// --------------------
var g = $prop('DataCorePlugin.GameData.GlobalAccelerationG') || 0;
var target = Math.min(maxRel, Math.max(0, -g) * perG);
root.f = (root.f || 0) + (target - (root.f || 0)) * Math.min(1, dt / tau);
return root.f;",
            },
            new EffectPreset
            {
                Category = "Brake",
                Name = "Threshold braking trainer",
                SuggestedPedal = 1,
                Description = "Light buzz in the target window of the pedal force, hard knock when pressing too much.",
                Script =
@"// ---- parameters ----
var windowLow = 80;    // % force, target window start
var windowHigh = 90;   // % force, target window end
var over = 95;         // % force, knock above
// --------------------
var f = $prop('DIY_FFB_Pedal.PedalStatus.Brake.PedalForceInPercent') || 0;
if (f > over) root.over = true;
if (f < over - 3) root.over = false;
if (root.over) return 0.08 * ((t * 12) % 1 < 0.15 ? 1 : 0);
if (f > windowLow && f < windowHigh) return 0.02 * Math.sin(2 * pi * 30 * t);
return 0;",
            },

            // ---------------- throttle ----------------
            new EffectPreset
            {
                Category = "Throttle",
                Name = "Traction control",
                SuggestedPedal = 2,
                Description = "Vibration while traction control cuts in, stronger with more throttle.",
                Script =
@"// ---- parameters ----
var f = 20;       // Hz
var amp = 0.05;   // degrees at full throttle
// --------------------
if (!$prop('DataCorePlugin.GameData.TCActive')) return 0;
var thr = ($prop('DataCorePlugin.GameData.Throttle') || 0) / 100;
return amp * (0.3 + 0.7 * thr) * Math.sin(2 * pi * f * t);",
            },
            new EffectPreset
            {
                Category = "Throttle",
                Name = "Wheel spin",
                SuggestedPedal = 2,
                Description = "Vibration growing with wheel spin. Needs the 'wheelsliplock' effect exported in ShakeIt Motors.",
                Script =
@"// ---- parameters ----
var f = 35;          // Hz
var amp = 0.05;      // degrees at full slip
var threshold = 10;  // % slip below which nothing happens
// --------------------
var slip = $prop('ShakeITMotorsV3Plugin.Export.wheelsliplock.All') || 0;
var x = Math.min(1, Math.max(0, (slip - threshold) / (100 - threshold)));
return amp * x * Math.sin(2 * pi * f * t);",
            },

            // ---------------- engine ----------------
            new EffectPreset
            {
                Category = "Engine",
                Name = "Engine vibration",
                Description = "Engine vibration like the former RPM effect: the frequency rises with RPM from fMin to fMax, " +
                              "stronger with RPM and throttle. Keep fMax at about 40 Hz: the pedal damps faster vibrations strongly.",
                Script =
@"// ---- parameters ----
var amp = 0.03;      // degrees at max RPM and full throttle (0.03 deg = about 0.15 mm)
var fMin = 10;       // Hz at idle
var fMax = 40;       // Hz at max RPM (more is damped by the pedal)
var idleShare = 0.4; // amplitude share without throttle
// --------------------
var rpm = $prop('DataCorePlugin.GameData.Rpms') || 0;
var maxRpm = $prop('DataCorePlugin.GameData.MaxRpm') || 8000;
if (rpm < 100) return 0;
var x = Math.min(1, rpm / maxRpm);
var thr = ($prop('DataCorePlugin.GameData.Throttle') || 0) / 100;
var f = fMin + (fMax - fMin) * x;
root.ph = ((root.ph || 0) + 2 * pi * f * dt) % (2 * pi);
return amp * (0.7 + 0.3 * x) * (idleShare + (1 - idleShare) * thr) * Math.sin(root.ph);",
            },
            new EffectPreset
            {
                Category = "Engine",
                Name = "Rev limiter",
                Description = "Hard knock while the rev limiter / red line is reached.",
                Script =
@"// ---- parameters ----
var f = 8;        // Hz
var amp = 0.04;   // degrees
// --------------------
if (!$prop('DataCorePlugin.GameData.CarSettings_RPMRedLineReached')) return 0;
return amp * (Math.sin(2 * pi * f * t) > 0 ? 1 : -1);",
            },
            new EffectPreset
            {
                Category = "Engine",
                Name = "Shift cue",
                Description = "Short buzz when the shift lights are close to the shift point.",
                Script =
@"// ---- parameters ----
var start = 0.9;  // share of the shift light range
var f = 30;       // Hz
var amp = 0.03;   // degrees
// --------------------
var rpm = $prop('DataCorePlugin.GameData.Rpms') || 0;
var shift = $prop('DataCorePlugin.GameData.CarSettings_RPMShiftLight2') || 0;
return (shift > 0 && rpm >= shift * start) ? amp * Math.sin(2 * pi * f * t) : 0;",
            },

            // ---------------- road ----------------
            new EffectPreset
            {
                Category = "Road",
                Name = "Speed rumble",
                Description = "Road rumble whose frequency rises with speed, like the former speed mode of the RPM effect.",
                Script =
@"// ---- parameters ----
var amp = 0.02;       // degrees at vFull
var hzPer100 = 15;    // Hz per 100 km/h
var fMin = 5;         // Hz
var fMax = 40;        // Hz (more is damped by the pedal)
var vFull = 200;      // km/h for full amplitude
// --------------------
var v = $prop('DataCorePlugin.GameData.SpeedKmh') || 0;
if (v < 5) return 0;
var f = Math.min(fMax, Math.max(fMin, v / 100 * hzPer100));
root.ph = ((root.ph || 0) + 2 * pi * f * dt) % (2 * pi);
return amp * Math.min(1, v / vFull) * Math.sin(root.ph);",
            },
            new EffectPreset
            {
                Category = "Road",
                Name = "Bumps (road impact)",
                Description = "Kicks on sudden vertical hits, from the fast part of the heave acceleration, like the former road impact effect. " +
                              "Check the unit of the heave acceleration in your game and adapt gain.",
                Script =
@"// ---- parameters ----
var gain = 0.05;   // degrees per unit of sudden heave change
var max = 0.3;     // degrees
var tau = 0.15;    // s, slower changes are ignored
// --------------------
var h = $prop('DataCorePlugin.GameData.AccelerationHeave') || 0;
root.lp = (root.lp === undefined) ? h : root.lp + (h - root.lp) * Math.min(1, dt / tau);
return Math.max(-max, Math.min(max, (h - root.lp) * gain));",
            },
            new EffectPreset
            {
                Category = "Road",
                Name = "Kerbs (iRacing)",
                Description = "iRacing reports the rumble strip pitch per tyre; the front tyres drive the vibration frequency.",
                Script =
@"// ---- parameters ----
var amp = 0.06;    // degrees
var fMax = 40;     // Hz (more is damped by the pedal)
// --------------------
var p = Math.max($prop('DataCorePlugin.GameRawData.Telemetry.TireLF_RumblePitch') || 0,
                 $prop('DataCorePlugin.GameRawData.Telemetry.TireRF_RumblePitch') || 0);
if (p <= 0) return 0;
root.ph = ((root.ph || 0) + 2 * pi * Math.min(fMax, p) * dt) % (2 * pi);
return amp * Math.sin(root.ph);",
            },

            // ---------------- misc ----------------
            new EffectPreset
            {
                Category = "Misc",
                Name = "Steering force blend (iRacing)",
                SuggestedPedal = 1,
                OffsetType = EffectPipelineOffsetType.Force_N,
                Description = "Part of the steering wheel force on the brake pedal (iRacing).",
                Script =
@"// ---- parameters ----
var maxN = 3;   // N at full wheel torque
// --------------------
return maxN * ($prop('DataCorePlugin.GameRawData.Telemetry.SteeringWheelPctTorque') || 0);",
            },
            new EffectPreset
            {
                Category = "Misc",
                Name = "Pit lane reminder",
                Description = "Short pulse every second while in the pit lane.",
                Script =
@"// ---- parameters ----
var amp = 0.1;   // degrees
// --------------------
return ($prop('DataCorePlugin.GameData.IsInPitLane') && (t % 1) < 0.05) ? amp : 0;",
            },
            new EffectPreset
            {
                Category = "Misc",
                Name = "Custom trigger",
                Description = "Vibrates while any SimHub property is above a threshold, like the former custom effects 1-4.",
                Script =
@"// ---- parameters ----
var property = 'DataCorePlugin.GameData.Brake';  // any SimHub property
var threshold = 90;                              // vibrate above this value
var f = 30;                                      // Hz
var amp = 0.03;                                  // degrees
// --------------------
var v = $prop(property) || 0;
return v > threshold ? amp * Math.sin(2 * pi * f * t) : 0;",
            },
            new EffectPreset
            {
                Category = "Test",
                Name = "Direction check",
                Description = "Constant 0.2 degrees (about 1 mm sled travel): the pedal should move slightly towards you.",
                Script = "return 0.2;",
            },
            new EffectPreset
            {
                Category = "Test",
                Name = "Slow sweep",
                Description = "0.5 Hz sine, easy to follow in the live plot.",
                Script =
@"var amp = 0.3;   // degrees
return amp * Math.sin(2 * pi * 0.5 * t);",
            },
        };
    }
}
