using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;

namespace DiyFfbPedal.UIFunction
{
    // Popup window for creating, editing, and saving effect scripts (stored in effects\*.js / *.ncalc).
    public class ScriptEditorWindow : Window
    {
        private static ScriptEditorWindow _instance;

        private readonly DIY_FFB_Pedal _plugin;
        private readonly int _pedal;
        private readonly Action _onChanged;
        private readonly ComboBox _slotList;
        private readonly Button _duplicate;
        private readonly Button _remove;
        private readonly Button _save;
        private readonly DockPanel _editor;
        private readonly TextBlock _empty;
        private readonly CheckBox _enable;
        private readonly TextBox _name;
        private readonly ComboBox _language;
        private readonly ComboBox _type;
        private readonly TextBox _gain;
        private readonly TextBox _frequency;
        private readonly TextBlock _scriptLabel;
        private readonly TextBox _script;
        private readonly TextBlock _fileLabel;
        private readonly TextBlock _result;
        private readonly TextBlock _help;
        private readonly TextBlock _status;
        private PipelineScriptSlot _slot;
        private bool _updating;

        private const string ExamplesUrl =
            "https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/docs/script_effects_examples.md";

        private static string HelpNCalc => Loc.T("TextScriptHelpNCalc",
            "NCalc. Variables: t (time in s), dt (0.002 s), prev (last value), phase (radians, from the frequency field), pi, " +
            "and every SimHub property, e.g. [DataCorePlugin.GameData.Rpms]. Functions: Sin, Cos, Abs, Min, Max, Pow, Sqrt, if(cond, a, b).\n" +
            "Pedal values (force, travel, ...): SimHub > Available properties, search \"diy_ffb_pedal\".\n" +
            "Example (engine, angle): frequency 10 + 30 * [DataCorePlugin.GameData.Rpms] / [DataCorePlugin.GameData.MaxRpm], script 0.03 * Sin(phase)\n" +
            "Vibrations above ~40 Hz are damped strongly by the pedal.\n" +
            "Example (ABS, angle): if([DataCorePlugin.GameData.ABSActive] > 0, 0.06 * Sin(2 * pi * 25 * t), 0)");

        private static string HelpJs => Loc.T("TextScriptHelpJs",
            "JavaScript (ES5, like SimHub's JS formulas). The script is the body of a function(t, dt, prev, phase): use return. " +
            "Properties: $prop('DataCorePlugin.GameData.Rpms'). root keeps values between calls (root.x = ...). Math.sin, Math.min, ...\n" +
            "Pedal values (force, travel, ...): SimHub > Available properties, search \"diy_ffb_pedal\".\n" +
            "Runs 500 times per second: keep it short, no loops over large data. 'CPU' shows the cost (10 ms/s = 1 % of a core).\n" +
            "Example (lockup steps): var l = $prop('ShakeITMotorsV3Plugin.Export.wheelsliplock.All'); var a = l >= 20 ? 0.8 : l >= 10 ? 0.3 : 0; return 0.06 * a * Math.sin(2 * pi * 30 * t);");

        public static void ShowFor(DIY_FFB_Pedal plugin, Window owner, int pedal, PipelineScriptSlot initialSlot, Action onChanged)
        {
            if (_instance != null)
            {
                _instance.Close();
                _instance = null;
            }
            _instance = new ScriptEditorWindow(plugin, pedal, initialSlot, onChanged);
            if (owner != null) _instance.Owner = owner;
            _instance.Closed += (s, e) => _instance = null;
            _instance.Show();
        }

        private ScriptEditorWindow(DIY_FFB_Pedal plugin, int pedal, PipelineScriptSlot initialSlot, Action onChanged)
        {
            _plugin = plugin;
            _pedal = pedal;
            _onChanged = onChanged;

            string[] pedalNames = { Loc.T("TextPedalIdClutch", "Clutch"), Loc.T("TextPedalIdBrake", "Brake"), Loc.T("TextPedalIdThrottle", "Throttle") };
            string pedalLabel = (pedal >= 0 && pedal < pedalNames.Length) ? pedalNames[pedal] : "";
            Title = Loc.T("TextScriptEditorTitle", "Script Editor") + (string.IsNullOrEmpty(pedalLabel) ? "" : " - " + pedalLabel);
            Width = 720;
            Height = 580;
            MinWidth = 620;
            MinHeight = 460;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            Background = new SolidColorBrush(Color.FromRgb(0x25, 0x25, 0x25));
            Foreground = Brushes.White;

            var root = new DockPanel { Margin = new Thickness(10) };

            // Top bar: slot selector + Library / + New / Duplicate / Remove / Save
            var topRow = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 8) };
            DockPanel.SetDock(topRow, Dock.Top);
            topRow.Children.Add(new TextBlock { Text = Loc.T("TextScriptSlot", "Script:"), VerticalAlignment = VerticalAlignment.Center, Width = 50 });
            _slotList = new ComboBox { Width = 180, Height = 24 };
            _slotList.SelectionChanged += (s, e) =>
            {
                if (_updating) return;
                ShowSlot(_slotList.SelectedItem is ComboBoxItem item ? item.Tag as PipelineScriptSlot : null);
            };
            topRow.Children.Add(_slotList);

            var library = SmallButton(Loc.T("TextScriptLibrary", "Library"));
            library.ToolTip = Loc.T("TextScriptLibraryTooltip", "Ready-made effects: ABS, engine, wheel slip, bumps, ...");
            library.Click += (s, e) =>
            {
                EffectLibraryWindow.ShowFor(_plugin, this, slot =>
                {
                    if (slot != null && slot.Pedal == _pedal)
                    {
                        RefreshList(slot);
                        _onChanged?.Invoke();
                    }
                });
            };
            topRow.Children.Add(library);

            var add = SmallButton(Loc.T("TextScriptNew", "+ New"));
            add.Click += (s, e) => AddSlot(null);
            topRow.Children.Add(add);

            _duplicate = SmallButton(Loc.T("TextScriptDuplicate", "Duplicate"));
            _duplicate.Click += (s, e) => { if (_slot != null) AddSlot(_slot); };
            topRow.Children.Add(_duplicate);

            _remove = SmallButton(Loc.T("TextScriptRemove", "Remove"));
            _remove.Click += (s, e) => RemoveSlot();
            topRow.Children.Add(_remove);

            _save = SmallButton(Loc.T("TextScriptSave", "Save"));
            _save.FontWeight = FontWeights.Bold;
            _save.Click += (s, e) => SaveCurrentSlotToFile();
            topRow.Children.Add(_save);

            root.Children.Add(topRow);

            var link = new Hyperlink(new Run(Loc.T("TextScriptMoreExamples", "More examples (GitHub)"))) { NavigateUri = new Uri(ExamplesUrl) };
            link.RequestNavigate += (s, e) =>
            {
                try { System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true }); }
                catch { }
                e.Handled = true;
            };
            var linkBlock = new TextBlock(link) { FontSize = 11, Margin = new Thickness(0, 6, 0, 0) };
            DockPanel.SetDock(linkBlock, Dock.Bottom);
            root.Children.Add(linkBlock);

            _empty = new TextBlock
            {
                Text = Loc.T("TextScriptEmpty", "No effect for this pedal yet. Pick one from the \"Library\" or write your own with \"+ New\". All enabled scripts of a pedal are added up."),
                TextWrapping = TextWrapping.Wrap,
                Foreground = Brushes.LightGray,
                Margin = new Thickness(0, 8, 0, 0),
            };
            DockPanel.SetDock(_empty, Dock.Top);
            root.Children.Add(_empty);

            _editor = new DockPanel();
            root.Children.Add(_editor);

            var editorTop = new StackPanel();
            DockPanel.SetDock(editorTop, Dock.Top);

            var row1 = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 6) };
            _enable = new CheckBox { Content = Loc.T("TextScriptEnabled", "Enabled"), VerticalAlignment = VerticalAlignment.Center, Width = 80, Foreground = Brushes.White };
            _enable.Checked += (s, e) => ApplyToMemory();
            _enable.Unchecked += (s, e) => ApplyToMemory();
            row1.Children.Add(_enable);
            row1.Children.Add(new TextBlock { Text = Loc.T("TextScriptName", "Name:"), VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 4, 0) });
            _name = new TextBox { Width = 170, Height = 24, VerticalContentAlignment = VerticalAlignment.Center };
            _name.LostFocus += (s, e) => ApplyToMemory();
            row1.Children.Add(_name);
            row1.Children.Add(new TextBlock { Text = Loc.T("TextScriptLanguage", "Language:"), VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 4, 0) });
            _language = new ComboBox { Width = 110, Height = 24 };
            _language.Items.Add(new ComboBoxItem { Content = "NCalc", Tag = PipelineScriptLanguage.NCalc });
            _language.Items.Add(new ComboBoxItem { Content = "JavaScript", Tag = PipelineScriptLanguage.JavaScript });
            _language.SelectionChanged += (s, e) => { ApplyToMemory(); UpdateHelp(); TestScript(); };
            row1.Children.Add(_language);
            editorTop.Children.Add(row1);

            var row2 = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 6) };
            row2.Children.Add(new TextBlock { Text = Loc.T("TextScriptOutput", "Output:"), VerticalAlignment = VerticalAlignment.Center, Width = 55 });
            _type = new ComboBox { Width = 200, Height = 24 };
            _type.Items.Add(new ComboBoxItem { Content = Loc.T("TextScriptOutputAngle", "Pedal angle offset (deg)"), Tag = EffectPipelineOffsetType.Angle_deg });
            _type.Items.Add(new ComboBoxItem { Content = Loc.T("TextScriptOutputForce", "Force offset (N)"), Tag = EffectPipelineOffsetType.Force_N });
            _type.Items.Add(new ComboBoxItem { Content = Loc.T("TextScriptOutputRelative", "Relative force (0.1 = +10 %)"), Tag = EffectPipelineOffsetType.ForceRelative });
            _type.SelectionChanged += (s, e) => ApplyToMemory();
            row2.Children.Add(_type);
            row2.Children.Add(new TextBlock { Text = Loc.T("TextScriptGain", "Gain:"), VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 4, 0) });
            _gain = new TextBox { Width = 55, Height = 24, VerticalContentAlignment = VerticalAlignment.Center };
            _gain.LostFocus += (s, e) => ApplyToMemory();
            row2.Children.Add(_gain);
            _fileLabel = new TextBlock { VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 0, 0), Foreground = Brushes.LightGray, FontSize = 11 };
            row2.Children.Add(_fileLabel);
            editorTop.Children.Add(row2);

            var freqRow = new DockPanel { Margin = new Thickness(0, 0, 0, 6) };
            var freqLabel = new TextBlock { Text = Loc.T("TextScriptFrequency", "Frequency (Hz) for phase:"), VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 6, 0) };
            DockPanel.SetDock(freqLabel, Dock.Left);
            freqRow.Children.Add(freqLabel);
            _frequency = new TextBox { Height = 24, VerticalContentAlignment = VerticalAlignment.Center, FontFamily = new FontFamily("Consolas"), FontSize = 11 };
            _frequency.TextChanged += (s, e) => ApplyToMemory();
            freqRow.Children.Add(_frequency);
            editorTop.Children.Add(freqRow);

            _scriptLabel = new TextBlock { FontSize = 11, Foreground = Brushes.LightGray, Margin = new Thickness(0, 0, 0, 2) };
            editorTop.Children.Add(_scriptLabel);
            _editor.Children.Add(editorTop);

            var editorBottom = new StackPanel();
            DockPanel.SetDock(editorBottom, Dock.Bottom);
            _result = new TextBlock { FontSize = 11, Margin = new Thickness(0, 4, 0, 0), Foreground = Brushes.LightGreen };
            editorBottom.Children.Add(_result);
            _status = new TextBlock { FontSize = 11, Margin = new Thickness(0, 2, 0, 0), Foreground = Brushes.LightSkyBlue };
            editorBottom.Children.Add(_status);
            _help = new TextBlock
            {
                FontSize = 10,
                TextWrapping = TextWrapping.Wrap,
                Foreground = Brushes.LightGray,
                Margin = new Thickness(0, 6, 0, 0),
            };
            editorBottom.Children.Add(_help);
            _editor.Children.Add(editorBottom);

            _script = new TextBox
            {
                MinHeight = 120,
                AcceptsReturn = true,
                AcceptsTab = true,
                TextWrapping = TextWrapping.Wrap,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                VerticalAlignment = VerticalAlignment.Stretch,
                VerticalContentAlignment = VerticalAlignment.Top,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 12,
                Background = new SolidColorBrush(Color.FromRgb(0x1B, 0x1B, 0x1B)),
                Foreground = Brushes.White,
                CaretBrush = Brushes.White,
            };
            _script.TextChanged += (s, e) => { ApplyToMemory(); TestScript(); };
            _editor.Children.Add(_script);

            Content = root;
            RefreshList(initialSlot);
        }

        private static Button SmallButton(string text)
        {
            return new Button { Content = text, Height = 24, Padding = new Thickness(8, 0, 8, 0), Margin = new Thickness(6, 0, 0, 0) };
        }

        private void RefreshList(PipelineScriptSlot select)
        {
            if (_plugin?.EffectPipelines == null) return;
            List<PipelineScriptSlot> slots = _plugin.EffectPipelines.GetSlots(_pedal);
            if (select == null || !slots.Contains(select))
                select = (_slot != null && slots.Contains(_slot)) ? _slot : (slots.Count > 0 ? slots[0] : null);

            _updating = true;
            _slotList.Items.Clear();
            for (int i = 0; i < slots.Count; i++)
            {
                var item = new ComboBoxItem { Content = slots[i].DisplayName(i + 1), Tag = slots[i] };
                _slotList.Items.Add(item);
                if (slots[i] == select) _slotList.SelectedItem = item;
            }
            _updating = false;
            ShowSlot(select);
        }

        private void RefreshListEntry()
        {
            for (int i = 0; i < _slotList.Items.Count; i++)
            {
                if (_slotList.Items[i] is ComboBoxItem item && item.Tag == _slot)
                    item.Content = _slot.DisplayName(i + 1);
            }
        }

        private void AddSlot(PipelineScriptSlot template)
        {
            if (_plugin?.EffectPipelines == null) return;
            PipelineScriptSlot slot = _plugin.EffectPipelines.AddSlot(_pedal, template);
            if (template == null) slot.Language = PipelineScriptLanguage.JavaScript;
            RefreshList(slot);
            _onChanged?.Invoke();
        }

        private void RemoveSlot()
        {
            if (_slot == null || _plugin?.EffectPipelines == null) return;
            string name = string.IsNullOrWhiteSpace(_slot.Name) ? Loc.T("TextScriptThisScript", "this script") : "\"" + _slot.Name + "\"";
            if (MessageBox.Show(string.Format(Loc.T("TextScriptRemoveConfirm", "Remove {0}?"), name), Loc.T("TextScriptEffects", "Script effects"), MessageBoxButton.YesNo) != MessageBoxResult.Yes) return;
            _plugin.EffectPipelines.RemoveSlot(_slot);
            _slot = null;
            RefreshList(null);
            _onChanged?.Invoke();
        }

        private void ShowSlot(PipelineScriptSlot slot)
        {
            _slot = slot;
            bool has = slot != null;
            _editor.Visibility = has ? Visibility.Visible : Visibility.Collapsed;
            _empty.Visibility = has ? Visibility.Collapsed : Visibility.Visible;
            _remove.IsEnabled = has;
            _duplicate.IsEnabled = has;
            _save.IsEnabled = has;
            if (!has) return;

            _updating = true;
            _enable.IsChecked = slot.Enabled;
            _name.Text = slot.Name ?? "";
            _language.SelectedIndex = slot.Language == PipelineScriptLanguage.JavaScript ? 1 : 0;
            _type.SelectedIndex = 0;
            for (int i = 0; i < _type.Items.Count; i++)
            {
                if ((byte)(EffectPipelineOffsetType)((ComboBoxItem)_type.Items[i]).Tag == slot.OffsetType)
                    _type.SelectedIndex = i;
            }
            _gain.Text = slot.Gain.ToString("0.###", CultureInfo.InvariantCulture);
            _script.Text = slot.Script ?? "";
            _frequency.Text = slot.FreqScript ?? "";
            _fileLabel.Text = string.IsNullOrEmpty(slot.ScriptFile) ? "(unsaved)" : slot.ScriptFile;
            _status.Text = "";
            _updating = false;
            UpdateHelp();
            TestScript();
        }

        private void ApplyToMemory()
        {
            if (_updating || _slot == null) return;
            _slot.Enabled = _enable.IsChecked == true;
            _slot.Name = _name.Text;
            if (_language.SelectedItem is ComboBoxItem lang)
                _slot.Language = (PipelineScriptLanguage)lang.Tag;
            if (_type.SelectedItem is ComboBoxItem item)
                _slot.OffsetType = (byte)(EffectPipelineOffsetType)item.Tag;
            double gain;
            if (double.TryParse(_gain.Text.Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out gain))
                _slot.Gain = Math.Max(0.0, Math.Min(10.0, gain));
            _gain.Text = _slot.Gain.ToString("0.###", CultureInfo.InvariantCulture);
            _slot.Script = _script.Text;
            _slot.FreqScript = _frequency.Text;
            RefreshListEntry();
            _onChanged?.Invoke();
        }

        private void SaveCurrentSlotToFile()
        {
            if (_slot == null) return;
            ApplyToMemory();
            var preset = new EffectPreset
            {
                Category = "Custom",
                Name = string.IsNullOrWhiteSpace(_slot.Name) ? "CustomScript" : _slot.Name.Trim(),
                SuggestedPedal = _pedal,
                OffsetType = (EffectPipelineOffsetType)_slot.OffsetType,
                Language = _slot.Language,
                Script = _slot.Script ?? "",
                FreqScript = _slot.FreqScript ?? "",
            };
            string savedName = EffectPresetFiles.SaveUserPreset(preset, string.IsNullOrWhiteSpace(_slot.ScriptFile) ? null : _slot.ScriptFile);
            _slot.ScriptFile = savedName;
            _fileLabel.Text = savedName;
            _plugin?.EffectPipelines?.ReloadFromPresetFile(savedName, preset);
            _status.Text = string.Format(Loc.T("TextScriptSavedTo", "Saved to effects\\{0}"), savedName);
            RefreshListEntry();
            _onChanged?.Invoke();
        }

        private void UpdateHelp()
        {
            bool js = _slot != null && _slot.Language == PipelineScriptLanguage.JavaScript;
            _help.Text = js ? HelpJs : HelpNCalc;
            _scriptLabel.Text = js ? Loc.T("TextScriptLabelJs", "Script (function body, return the value):") : Loc.T("TextScriptLabelNCalc", "Script (NCalc formula):");
            if (js)
            {
                string error;
                if (!JintScriptHost.IsAvailable(out error)) _help.Text = error + "\n" + HelpJs;
            }
        }

        private void TestScript()
        {
            if (_updating || _slot == null || _plugin?.EffectPipelines == null) return;
            if (string.IsNullOrWhiteSpace(_script.Text))
            {
                _result.Text = Loc.T("TextScriptResult", "Script result at t = 0:") + " -";
                return;
            }
            _result.Text = Loc.T("TextScriptResult", "Script result at t = 0:") + " " + _plugin.EffectPipelines.TestScript(_slot, _plugin.PluginManager);
        }
    }
}
