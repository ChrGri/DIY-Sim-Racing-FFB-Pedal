using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Text;

namespace DiyFfbPedal
{
    // The effect library as files: SimHub\PluginsData\Common\DiyFfbPedal\effects
    // One file per effect: <name>.js (JavaScript) or <name>.ncalc (NCalc), with a header
    // of "// @key: value" lines, followed by the script:
    //
    //   // @name: ABS
    //   // @category: Brake
    //   // @pedal: brake            clutch | brake | throttle | any
    //   // @output: angle           angle | force | relative
    //   // @description: ...
    //   // @frequency: ...          optional, script for the phase oscillator (Hz)
    //   // @id: Abs                 optional, translation key part
    //   // @builtin: 3              only in files written by the plugin (see below)
    //
    // The plugin writes its built-in presets there. A built-in file is replaced when the
    // plugin brings a newer version of it (@builtin), so changes to these files can be
    // lost: for an own variant save it under a new file name and remove the @builtin line.
    public static class EffectPresetFiles
    {
        // raise when built-in presets change, so the files are updated
        public const int BuiltInVersion = 1;

        private static readonly string[] CategoryOrder = { "Brake", "Throttle", "Engine", "Road", "Misc", "Test" };

        public static string FolderPath
        {
            get
            {
                return Path.Combine(Directory.GetCurrentDirectory(), "PluginsData", "Common", "DiyFfbPedal", "effects");
            }
        }

        // Writes missing or outdated built-in preset files. Returns an error text or null.
        public static string EnsureBuiltIns()
        {
            try
            {
                Directory.CreateDirectory(FolderPath);
                foreach (EffectPreset preset in EffectLibrary.Presets)
                {
                    string path = Path.Combine(FolderPath, FileName(preset));
                    if (File.Exists(path))
                    {
                        int version = ReadBuiltInVersion(path);
                        if (version < 0 || version >= BuiltInVersion) continue; // user file or up to date
                    }
                    File.WriteAllText(path, ToText(preset), new UTF8Encoding(false));
                }
                string readme = Path.Combine(FolderPath, "README.txt");
                if (!File.Exists(readme)) File.WriteAllText(readme, Readme, new UTF8Encoding(false));
                return null;
            }
            catch (Exception ex)
            {
                return ex.Message;
            }
        }

        // Reads all preset files. errors: files that could not be read.
        public static List<EffectPreset> Load(out List<string> errors)
        {
            errors = new List<string>();
            var presets = new List<EffectPreset>();
            if (!Directory.Exists(FolderPath)) return presets;
            IEnumerable<string> files = Directory.EnumerateFiles(FolderPath, "*.js")
                .Concat(Directory.EnumerateFiles(FolderPath, "*.ncalc"));
            foreach (string file in files)
            {
                try
                {
                    presets.Add(Parse(file, File.ReadAllText(file)));
                }
                catch (Exception ex)
                {
                    errors.Add(Path.GetFileName(file) + ": " + ex.Message);
                }
            }
            return presets
                .OrderBy(p => CategoryRank(p.Category))
                .ThenBy(p => p.Category, StringComparer.OrdinalIgnoreCase)
                .ThenBy(p => BuiltInRank(p))
                .ThenBy(p => p.Name, StringComparer.OrdinalIgnoreCase)
                .ToList();
        }

        public static string FileName(EffectPreset preset)
        {
            string ext = preset.Language == PipelineScriptLanguage.JavaScript ? ".js" : ".ncalc";
            return EffectPreset.DeriveId(preset.Name) + ext;
        }

        private static int CategoryRank(string category)
        {
            int i = Array.FindIndex(CategoryOrder, c => string.Equals(c, category, StringComparison.OrdinalIgnoreCase));
            return i < 0 ? CategoryOrder.Length : i;
        }

        // built-in presets keep the order of the built-in list, user files come after them
        private static int BuiltInRank(EffectPreset preset)
        {
            for (int i = 0; i < EffectLibrary.Presets.Count; i++)
                if (string.Equals(EffectLibrary.Presets[i].Name, preset.Name, StringComparison.OrdinalIgnoreCase)) return i;
            return int.MaxValue;
        }

        private static int ReadBuiltInVersion(string path)
        {
            foreach (string line in File.ReadLines(path).Take(20))
            {
                string key, value;
                if (TryHeader(line, out key, out value) && key == "builtin")
                {
                    // leading number, the rest of the line is a note
                    string digits = new string(value.TakeWhile(char.IsDigit).ToArray());
                    int v;
                    return int.TryParse(digits, NumberStyles.Integer, CultureInfo.InvariantCulture, out v) ? v : 0;
                }
            }
            return -1;
        }

        private static bool TryHeader(string line, out string key, out string value)
        {
            key = value = null;
            string t = line.Trim();
            if (!t.StartsWith("//")) return false;
            t = t.Substring(2).Trim();
            if (!t.StartsWith("@")) return false;
            int colon = t.IndexOf(':');
            if (colon < 0) return false;
            key = t.Substring(1, colon - 1).Trim().ToLowerInvariant();
            value = t.Substring(colon + 1).Trim();
            return true;
        }

        public static EffectPreset Parse(string path, string text)
        {
            var preset = new EffectPreset
            {
                FilePath = path,
                Language = path.EndsWith(".ncalc", StringComparison.OrdinalIgnoreCase)
                    ? PipelineScriptLanguage.NCalc : PipelineScriptLanguage.JavaScript,
                Name = Path.GetFileNameWithoutExtension(path),
                Category = "Custom",
                Description = "",
                TranslationId = "",
            };

            string[] lines = text.Replace("\r\n", "\n").Split('\n');
            int body = 0;
            for (; body < lines.Length; body++)
            {
                string key, value;
                if (!TryHeader(lines[body], out key, out value))
                {
                    if (lines[body].Trim().Length == 0) continue; // blank lines in the header
                    break;
                }
                switch (key)
                {
                    case "name": preset.Name = value; break;
                    case "category": preset.Category = value; break;
                    case "description": preset.Description = value; break;
                    case "frequency": preset.FreqScript = value; break;
                    case "id": preset.TranslationId = value; break;
                    case "pedal":
                        switch (value.ToLowerInvariant())
                        {
                            case "clutch": preset.SuggestedPedal = 0; break;
                            case "brake": preset.SuggestedPedal = 1; break;
                            case "throttle": preset.SuggestedPedal = 2; break;
                            default: preset.SuggestedPedal = -1; break;
                        }
                        break;
                    case "output":
                        switch (value.ToLowerInvariant())
                        {
                            case "force": preset.OffsetType = EffectPipelineOffsetType.Force_N; break;
                            case "relative": preset.OffsetType = EffectPipelineOffsetType.ForceRelative; break;
                            case "angle": preset.OffsetType = EffectPipelineOffsetType.Angle_deg; break;
                            default: throw new FormatException("unknown @output '" + value + "' (angle, force or relative)");
                        }
                        break;
                }
            }
            preset.Script = string.Join("\r\n", lines.Skip(body)).Trim();
            if (preset.Script.Length == 0) throw new FormatException("no script after the header");
            return preset;
        }

        private static string ToText(EffectPreset preset)
        {
            var sb = new StringBuilder();
            sb.Append("// @name: ").Append(preset.Name).Append("\r\n");
            sb.Append("// @category: ").Append(preset.Category).Append("\r\n");
            sb.Append("// @pedal: ").Append(preset.SuggestedPedal == 0 ? "clutch" : preset.SuggestedPedal == 1 ? "brake" : preset.SuggestedPedal == 2 ? "throttle" : "any").Append("\r\n");
            sb.Append("// @output: ").Append(preset.OffsetType == EffectPipelineOffsetType.Force_N ? "force"
                : preset.OffsetType == EffectPipelineOffsetType.ForceRelative ? "relative" : "angle").Append("\r\n");
            sb.Append("// @description: ").Append((preset.Description ?? "").Replace("\r", " ").Replace("\n", " ")).Append("\r\n");
            if (!string.IsNullOrWhiteSpace(preset.FreqScript)) sb.Append("// @frequency: ").Append(preset.FreqScript).Append("\r\n");
            sb.Append("// @id: ").Append(preset.Id).Append("\r\n");
            sb.Append("// @builtin: ").Append(BuiltInVersion).Append("   (written by the plugin; for own changes save under a new name and remove this line)\r\n");
            sb.Append("\r\n");
            sb.Append((preset.Script ?? "").Replace("\r\n", "\n").Replace("\n", "\r\n")).Append("\r\n");
            return sb.ToString();
        }

        private static string ToUserText(EffectPreset preset)
        {
            var sb = new StringBuilder();
            sb.Append("// @name: ").Append(preset.Name ?? "").Append("\r\n");
            sb.Append("// @category: ").Append(string.IsNullOrWhiteSpace(preset.Category) ? "Custom" : preset.Category).Append("\r\n");
            sb.Append("// @pedal: ").Append(preset.SuggestedPedal == 0 ? "clutch" : preset.SuggestedPedal == 1 ? "brake" : preset.SuggestedPedal == 2 ? "throttle" : "any").Append("\r\n");
            sb.Append("// @output: ").Append(preset.OffsetType == EffectPipelineOffsetType.Force_N ? "force"
                : preset.OffsetType == EffectPipelineOffsetType.ForceRelative ? "relative" : "angle").Append("\r\n");
            if (!string.IsNullOrWhiteSpace(preset.Description))
                sb.Append("// @description: ").Append(preset.Description.Replace("\r", " ").Replace("\n", " ")).Append("\r\n");
            if (!string.IsNullOrWhiteSpace(preset.FreqScript))
                sb.Append("// @frequency: ").Append(preset.FreqScript).Append("\r\n");
            if (!string.IsNullOrWhiteSpace(preset.TranslationId))
                sb.Append("// @id: ").Append(preset.TranslationId).Append("\r\n");
            sb.Append("\r\n");
            sb.Append((preset.Script ?? "").Replace("\r\n", "\n").Replace("\n", "\r\n")).Append("\r\n");
            return sb.ToString();
        }

        public static string SanitizeFileName(string name, PipelineScriptLanguage lang)
        {
            string ext = lang == PipelineScriptLanguage.JavaScript ? ".js" : ".ncalc";
            string raw = (name ?? "Script").Trim();
            foreach (char c in Path.GetInvalidFileNameChars())
                raw = raw.Replace(c, '_');
            if (string.IsNullOrWhiteSpace(raw)) raw = "Script";
            if (!raw.EndsWith(".js", StringComparison.OrdinalIgnoreCase) && !raw.EndsWith(".ncalc", StringComparison.OrdinalIgnoreCase))
                raw += ext;
            else
                raw = Path.GetFileNameWithoutExtension(raw) + ext;
            return raw;
        }

        // Saves a user effect file (without @builtin) into FolderPath and returns its file name.
        public static string SaveUserPreset(EffectPreset preset, string fileName = null)
        {
            Directory.CreateDirectory(FolderPath);
            string targetName = string.IsNullOrWhiteSpace(fileName)
                ? SanitizeFileName(preset.Name, preset.Language)
                : SanitizeFileName(fileName, preset.Language);
            string path = Path.Combine(FolderPath, targetName);
            File.WriteAllText(path, ToUserText(preset), new UTF8Encoding(false));
            preset.FilePath = path;
            return targetName;
        }

        public static bool DeleteUserPreset(string fileName)
        {
            if (string.IsNullOrWhiteSpace(fileName)) return false;
            string path = Path.Combine(FolderPath, Path.GetFileName(fileName));
            if (!File.Exists(path)) return false;
            File.Delete(path);
            return true;
        }

        public static EffectPreset FindByFileName(string fileName)
        {
            if (string.IsNullOrWhiteSpace(fileName)) return null;
            string path = Path.Combine(FolderPath, Path.GetFileName(fileName));
            if (!File.Exists(path)) return null;
            try
            {
                return Parse(path, File.ReadAllText(path));
            }
            catch
            {
                return null;
            }
        }

        // Ensures built-in files exist, migrates any legacy inline slots without ScriptFile
        // to user files in effects\, and refreshes slot definitions from their linked files.
        public static void SyncSlotsFromFiles(DIYFFBPedalSettings settings)
        {
            if (settings == null || settings.Pipeline_slots == null) return;
            EnsureBuiltIns();
            foreach (PipelineScriptSlot slot in settings.Pipeline_slots)
            {
                if (string.IsNullOrWhiteSpace(slot.ScriptFile))
                {
                    if (string.IsNullOrWhiteSpace(slot.Script)) continue;
                    string baseName = SanitizeFileName(string.IsNullOrWhiteSpace(slot.Name) ? "CustomScript" : slot.Name, slot.Language);
                    string candidate = baseName;
                    int suffix = 1;
                    while (File.Exists(Path.Combine(FolderPath, candidate)))
                    {
                        candidate = Path.GetFileNameWithoutExtension(baseName) + "_" + suffix + Path.GetExtension(baseName);
                        suffix++;
                    }
                    var preset = new EffectPreset
                    {
                        Category = "Custom",
                        Name = string.IsNullOrWhiteSpace(slot.Name) ? Path.GetFileNameWithoutExtension(candidate) : slot.Name,
                        SuggestedPedal = slot.Pedal,
                        OffsetType = (EffectPipelineOffsetType)slot.OffsetType,
                        Language = slot.Language,
                        Script = slot.Script ?? "",
                        FreqScript = slot.FreqScript ?? "",
                    };
                    slot.ScriptFile = SaveUserPreset(preset, candidate);
                }
                else
                {
                    EffectPreset preset = FindByFileName(slot.ScriptFile);
                    if (preset != null) slot.ApplyFromPreset(preset);
                }
            }
        }

        private const string Readme =
@"DIY FFB pedal - effect library

Every file here is one effect in the library of the plugin (Effects > Scripts > Library).
  *.js     JavaScript
  *.ncalc  NCalc

Header lines at the top of the file, then the script:
  // @name: My effect
  // @category: Brake              shown as group in the library
  // @pedal: brake                 clutch | brake | throttle | any
  // @output: angle                angle (deg) | force (N) | relative (0.1 = +10 %)
  // @description: what it does
  // @frequency: 20                optional: frequency script (Hz) for the phase variable

Files with an '@builtin' line are written by the plugin and are replaced when the plugin
brings a newer version. For own changes save the file under a new name and remove the
'@builtin' line. Click 'Reload' in the library after changing files.

Examples and all variables: docs/script_effects_examples.md in the GitHub repository.
";
    }
}
