using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Editor for the script effect slots of the selected pedal.
    // Built in code; the custom effects tab hosts it in the SCRIPTS tab.
    public class PipelineScriptPanel : UserControl
    {
        private DIY_FFB_Pedal _plugin;
        private readonly ComboBox _slotList;
        private readonly Button _remove;
        private readonly Button _duplicate;
        private readonly StackPanel _editor;
        private readonly TextBlock _empty;
        private readonly TextBox _name;
        private readonly CheckBox _enable;
        private readonly ComboBox _language;
        private readonly ComboBox _type;
        private readonly TextBox _gain;
        private readonly TextBox _script;
        private readonly TextBox _frequency;
        private readonly TextBlock _scriptLabel;
        private readonly TextBlock _result;
        private readonly TextBlock _live;
        private readonly TextBlock _help;
        private readonly DispatcherTimer _liveTimer;
        private readonly CheckBox _ack;
        private readonly StackPanel _slotRow;
        private bool _updating;
        private int _shownPedal = -1;
        private DIYFFBPedalSettings _shownSettings;
        private PipelineScriptSlot _slot;

        private static readonly string HelpNCalc =
            "NCalc. Variables: t (time in s), dt (0.002 s), prev (last value), phase (radians, from the frequency field), pi, " +
            "and every SimHub property, e.g. [DataCorePlugin.GameData.Rpms]. Functions: Sin, Cos, Abs, Min, Max, Pow, Sqrt, if(cond, a, b).\n" +
            "Example (engine, angle): frequency [DataCorePlugin.GameData.Rpms] / 60, script 0.004 * [DataCorePlugin.GameData.Throttle] / 100 * Sin(phase)\n" +
            "Example (ABS, angle): if([DataCorePlugin.GameData.ABSActive] > 0, 0.06 * Sin(2 * pi * 25 * t), 0)";

        private static readonly string HelpJs =
            "JavaScript (ES5, like SimHub's JS formulas). The script is the body of a function(t, dt, prev, phase): use return. " +
            "Properties: $prop('DataCorePlugin.GameData.Rpms'). root keeps values between calls (root.x = ...). Math.sin, Math.min, ...\n" +
            "Runs 500 times per second: keep it short, no loops over large data. 'CPU' shows the cost (10 ms/s = 1 % of a core).\n" +
            "Example (lockup steps): var l = $prop('ShakeITMotorsV3Plugin.Export.wheelsliplock.All'); var a = l >= 20 ? 0.8 : l >= 10 ? 0.3 : 0; return 0.06 * a * Math.sin(2 * pi * 30 * t);";

        private const string ExamplesUrl =
            "https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/develop/docs/script_effects_examples.md";

        public PipelineScriptPanel()
        {
            var root = new StackPanel { Margin = new Thickness(4) };

            // warning: scripts turn the pedal into an active device
            var warning = new Border
            {
                BorderBrush = new SolidColorBrush(Color.FromRgb(0xFF, 0xC1, 0x07)),
                BorderThickness = new Thickness(1),
                Padding = new Thickness(4),
                Margin = new Thickness(0, 0, 0, 6),
            };
            var warningRow = new DockPanel();
            var robot = new PixelDangerRobot { Margin = new Thickness(0, 0, 8, 0), VerticalAlignment = VerticalAlignment.Top };
            DockPanel.SetDock(robot, Dock.Left);
            warningRow.Children.Add(robot);
            var warningText = new StackPanel();
            warningText.Children.Add(new TextBlock
            {
                Text = "DANGER: robot mode",
                FontWeight = FontWeights.Bold,
                Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xC1, 0x07)),
            });
            warningText.Children.Add(new TextBlock
            {
                Text = "Scripts make the pedal move on its own. It can push your foot and put energy into it instead " +
                       "of only reacting to it. A wrong sign, unit or typo can mean full force or a hard kick. " +
                       "Start with small values, test with your foot on the pedal and gain 1, and disable the script at the first surprise.",
                TextWrapping = TextWrapping.Wrap,
                FontSize = 10,
            });
            _ack = new CheckBox { Content = "I understand, let me script", Margin = new Thickness(0, 4, 0, 0), FontSize = 11 };
            _ack.Checked += (s, e) => StoreAck();
            _ack.Unchecked += (s, e) => StoreAck();
            warningText.Children.Add(_ack);
            warningRow.Children.Add(warningText);
            warning.Child = warningRow;
            root.Children.Add(warning);

            // slot list
            var slotRow = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 4) };
            slotRow.Children.Add(new TextBlock { Text = "Script:", VerticalAlignment = VerticalAlignment.Center, Width = 50 });
            _slotList = new ComboBox { Width = 190, Height = 22 };
            _slotList.SelectionChanged += (s, e) =>
            {
                if (_updating) return;
                ShowSlot(_slotList.SelectedItem is ComboBoxItem item ? item.Tag as PipelineScriptSlot : null);
            };
            slotRow.Children.Add(_slotList);
            var add = SmallButton("+ New");
            add.Click += (s, e) => AddSlot(null);
            slotRow.Children.Add(add);
            _duplicate = SmallButton("Duplicate");
            _duplicate.Click += (s, e) => { if (_slot != null) AddSlot(_slot); };
            slotRow.Children.Add(_duplicate);
            _remove = SmallButton("Remove");
            _remove.Click += (s, e) => RemoveSlot();
            slotRow.Children.Add(_remove);
            var plotButton = SmallButton("Live plot");
            plotButton.Click += (s, e) =>
            {
                if (_plugin != null) EffectPipelinePlotWindow.ShowFor(_plugin, Window.GetWindow(this));
            };
            slotRow.Children.Add(plotButton);
            root.Children.Add(slotRow);
            _slotRow = slotRow;

            _empty = new TextBlock
            {
                Text = "No script effect for this pedal yet. Click \"+ New\". All enabled scripts of a pedal are added up.",
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 6, 0, 0),
            };
            root.Children.Add(_empty);

            _editor = new StackPanel();
            root.Children.Add(_editor);

            var row1 = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 4) };
            _enable = new CheckBox { Content = "Enabled", VerticalAlignment = VerticalAlignment.Center, Width = 75 };
            _enable.Checked += (s, e) => Store();
            _enable.Unchecked += (s, e) => Store();
            row1.Children.Add(_enable);
            row1.Children.Add(new TextBlock { Text = "Name:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 4, 0) });
            _name = new TextBox { Width = 140, Height = 22, VerticalContentAlignment = VerticalAlignment.Center };
            _name.LostFocus += (s, e) => Store();
            row1.Children.Add(_name);
            row1.Children.Add(new TextBlock { Text = "Language:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 4, 0) });
            _language = new ComboBox { Width = 100, Height = 22 };
            _language.Items.Add(new ComboBoxItem { Content = "NCalc", Tag = PipelineScriptLanguage.NCalc });
            _language.Items.Add(new ComboBoxItem { Content = "JavaScript", Tag = PipelineScriptLanguage.JavaScript });
            _language.SelectionChanged += (s, e) => { Store(); UpdateHelp(); TestScript(); };
            row1.Children.Add(_language);
            _editor.Children.Add(row1);

            var row2 = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 4) };
            row2.Children.Add(new TextBlock { Text = "Output:", VerticalAlignment = VerticalAlignment.Center, Width = 50 });
            _type = new ComboBox { Width = 190, Height = 22 };
            _type.Items.Add(new ComboBoxItem { Content = "Pedal angle offset (deg)", Tag = EffectPipelineOffsetType.Angle_deg });
            _type.Items.Add(new ComboBoxItem { Content = "Force offset (N)", Tag = EffectPipelineOffsetType.Force_N });
            _type.Items.Add(new ComboBoxItem { Content = "Relative force (0.1 = +10 %)", Tag = EffectPipelineOffsetType.ForceRelative });
            _type.SelectionChanged += (s, e) => Store();
            row2.Children.Add(_type);
            row2.Children.Add(new TextBlock { Text = "Gain:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 4, 0) });
            _gain = new TextBox { Width = 50, Height = 22, VerticalContentAlignment = VerticalAlignment.Center };
            _gain.LostFocus += (s, e) => Store();
            row2.Children.Add(_gain);
            _editor.Children.Add(row2);

            var freqRow = new DockPanel { Margin = new Thickness(0, 0, 0, 4) };
            var freqLabel = new TextBlock { Text = "Frequency (Hz) for phase:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 6, 0) };
            DockPanel.SetDock(freqLabel, Dock.Left);
            freqRow.Children.Add(freqLabel);
            _frequency = new TextBox { Height = 22, VerticalContentAlignment = VerticalAlignment.Center, FontFamily = new FontFamily("Consolas"), FontSize = 11 };
            _frequency.TextChanged += (s, e) => Store();
            freqRow.Children.Add(_frequency);
            _editor.Children.Add(freqRow);

            _scriptLabel = new TextBlock { FontSize = 10, Opacity = 0.7 };
            _editor.Children.Add(_scriptLabel);
            _script = new TextBox
            {
                Height = 90,
                AcceptsReturn = true,
                AcceptsTab = true,
                TextWrapping = TextWrapping.Wrap,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 11,
            };
            _script.TextChanged += (s, e) => { Store(); TestScript(); };
            _editor.Children.Add(_script);

            _result = new TextBlock { FontSize = 10, Margin = new Thickness(0, 2, 0, 0) };
            _editor.Children.Add(_result);
            _live = new TextBlock { FontSize = 10, TextWrapping = TextWrapping.Wrap };
            _editor.Children.Add(_live);
            _help = new TextBlock
            {
                FontSize = 9,
                TextWrapping = TextWrapping.Wrap,
                Opacity = 0.7,
                Margin = new Thickness(0, 4, 0, 0),
            };
            _editor.Children.Add(_help);

            var link = new Hyperlink(new Run("More examples (GitHub)")) { NavigateUri = new Uri(ExamplesUrl) };
            link.RequestNavigate += (s, e) =>
            {
                try { System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo(e.Uri.AbsoluteUri) { UseShellExecute = true }); }
                catch { }
                e.Handled = true;
            };
            root.Children.Add(new TextBlock(link) { FontSize = 10, Margin = new Thickness(0, 2, 0, 0) });

            Content = new ScrollViewer { Content = root, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };

            _liveTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            _liveTimer.Tick += (s, e) => UpdateLive();
            Loaded += (s, e) => _liveTimer.Start();
            Unloaded += (s, e) => _liveTimer.Stop();

            ShowSlot(null);
            UpdateAckState();
        }

        private static Button SmallButton(string text)
        {
            return new Button { Content = text, Height = 22, Padding = new Thickness(6, 0, 6, 0), Margin = new Thickness(6, 0, 0, 0) };
        }

        private int Pedal
        {
            get { return (_plugin == null || _plugin.Settings == null) ? -1 : (int)_plugin.Settings.table_selected; }
        }

        // Called by the custom effects tab whenever the plugin, settings or the selected pedal change.
        public void Bind(DIY_FFB_Pedal plugin)
        {
            _plugin = plugin;
            if (Pedal < 0 || _plugin.EffectPipelines == null) return;
            if (Pedal == _shownPedal && ReferenceEquals(_shownSettings, _plugin.Settings)) return;
            _shownPedal = Pedal;
            _shownSettings = _plugin.Settings;
            _updating = true;
            _ack.IsChecked = _plugin.Settings.Pipeline_warning_ack;
            _updating = false;
            UpdateAckState();
            RefreshList(null);
        }

        private void StoreAck()
        {
            if (_updating || _plugin == null || _plugin.Settings == null) return;
            _plugin.Settings.Pipeline_warning_ack = _ack.IsChecked == true;
            UpdateAckState();
        }

        // without the accepted warning the scripts can't be edited and don't run
        private void UpdateAckState()
        {
            bool ack = _plugin != null && _plugin.Settings != null && _plugin.Settings.Pipeline_warning_ack;
            _slotRow.IsEnabled = ack;
            _editor.IsEnabled = ack;
        }

        private void RefreshList(PipelineScriptSlot select)
        {
            if (_plugin == null || _plugin.EffectPipelines == null) return;
            List<PipelineScriptSlot> slots = _plugin.EffectPipelines.GetSlots(Pedal);
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

        // updates the entry of the shown slot (name, type, on/off) without rebuilding the list
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
            if (Pedal < 0 || _plugin.EffectPipelines == null) return;
            PipelineScriptSlot slot = _plugin.EffectPipelines.AddSlot(Pedal, template);
            RefreshList(slot);
        }

        private void RemoveSlot()
        {
            if (_slot == null || _plugin.EffectPipelines == null) return;
            string name = string.IsNullOrWhiteSpace(_slot.Name) ? "this script" : "\"" + _slot.Name + "\"";
            if (MessageBox.Show("Remove " + name + "?", "Script effects", MessageBoxButton.YesNo) != MessageBoxResult.Yes) return;
            _plugin.EffectPipelines.RemoveSlot(_slot);
            _slot = null;
            RefreshList(null);
        }

        private void ShowSlot(PipelineScriptSlot slot)
        {
            _slot = slot;
            bool has = slot != null;
            _editor.Visibility = has ? Visibility.Visible : Visibility.Collapsed;
            _empty.Visibility = has ? Visibility.Collapsed : Visibility.Visible;
            _remove.IsEnabled = has;
            _duplicate.IsEnabled = has;
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
            _updating = false;
            UpdateHelp();
            TestScript();
        }

        private void Store()
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
        }

        private void UpdateHelp()
        {
            bool js = _slot != null && _slot.Language == PipelineScriptLanguage.JavaScript;
            _help.Text = js ? HelpJs : HelpNCalc;
            _scriptLabel.Text = js ? "Script (function body, return the value):" : "Script (NCalc formula):";
            if (js)
            {
                string error;
                if (!JintScriptHost.IsAvailable(out error)) _help.Text = error + "\n" + HelpJs;
            }
        }

        private void TestScript()
        {
            if (_updating || _slot == null || _plugin == null || _plugin.EffectPipelines == null) return;
            if (string.IsNullOrWhiteSpace(_script.Text))
            {
                _result.Text = "Script result at t = 0: -";
                return;
            }
            _result.Text = "Script result at t = 0: " + _plugin.EffectPipelines.TestScript(_slot, _plugin.PluginManager);
        }

        private void UpdateLive()
        {
            if (_plugin == null || _plugin.EffectPipelines == null) return;
            // the selected pedal may have changed without a Bind call
            if (Pedal != _shownPedal && Pedal >= 0)
            {
                _shownPedal = Pedal;
                RefreshList(null);
            }
            if (_slot == null) return;
            string error = _plugin.EffectPipelines.GetError(_slot);
            string cpu = "   CPU: " + _plugin.EffectPipelines.GetEvalTime(_slot).ToString("0.0", CultureInfo.InvariantCulture) + " ms/s";
            _live.Text = error != null
                ? "Live: error - " + error
                : "Live output (after gain): " + _plugin.EffectPipelines.GetLastOutput(_slot).ToString("0.###", CultureInfo.InvariantCulture) + cpu;
        }
    }
}
