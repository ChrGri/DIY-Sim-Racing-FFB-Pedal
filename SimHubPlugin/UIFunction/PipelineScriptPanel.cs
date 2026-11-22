using System;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Editor for one effect pipeline script channel of the selected pedal.
    // Built in code; the custom effects tab hosts one panel per channel.
    public class PipelineScriptPanel : UserControl
    {
        public int Channel { get; set; }

        private DIY_FFB_Pedal _plugin;
        private readonly CheckBox _enable;
        private readonly ComboBox _type;
        private readonly TextBox _gain;
        private readonly TextBox _script;
        private readonly TextBlock _result;
        private readonly TextBlock _live;
        private readonly DispatcherTimer _liveTimer;
        private bool _updating;
        private int _shownIndex = -1;
        private DIYFFBPedalSettings _shownSettings;

        private static readonly string HelpText =
            "Variables: t (time in s), dt (0.002 s), prev (last value), pi, and every SimHub property, " +
            "e.g. [DataCorePlugin.GameData.Rpms]. Functions: Sin, Cos, Abs, Min, Max, Pow, Sqrt, if(cond, a, b).\n" +
            "Example (engine, position): 0.02 * [DataCorePlugin.GameData.Throttle] / 100 * Sin(2 * pi * [DataCorePlugin.GameData.Rpms] / 60 * t)\n" +
            "Example (ABS, position): if([DataCorePlugin.GameData.ABSActive] > 0, 0.3 * Sin(2 * pi * 25 * t), 0)";

        public PipelineScriptPanel()
        {
            var root = new StackPanel { Margin = new Thickness(4) };

            var header = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 4) };
            _enable = new CheckBox { VerticalAlignment = VerticalAlignment.Center };
            _enable.Checked += (s, e) => Store();
            _enable.Unchecked += (s, e) => Store();
            header.Children.Add(_enable);
            header.Children.Add(new TextBlock
            {
                Text = "Script effect (offset samples streamed to the pedal)",
                FontWeight = FontWeights.Bold,
                Margin = new Thickness(6, 0, 0, 0),
                VerticalAlignment = VerticalAlignment.Center,
            });
            root.Children.Add(header);

            var options = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 4) };
            options.Children.Add(new TextBlock { Text = "Output:", VerticalAlignment = VerticalAlignment.Center, Width = 50 });
            _type = new ComboBox { Width = 170, Height = 22 };
            _type.Items.Add(new ComboBoxItem { Content = "Position offset (mm)", Tag = EffectPipelineOffsetType.Position_mm });
            _type.Items.Add(new ComboBoxItem { Content = "Force offset (N)", Tag = EffectPipelineOffsetType.Force_N });
            _type.Items.Add(new ComboBoxItem { Content = "Relative force (0.1 = +10 %)", Tag = EffectPipelineOffsetType.ForceRelative });
            _type.SelectionChanged += (s, e) => Store();
            options.Children.Add(_type);
            options.Children.Add(new TextBlock { Text = "Gain:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 4, 0) });
            _gain = new TextBox { Width = 50, Height = 22, VerticalContentAlignment = VerticalAlignment.Center };
            _gain.LostFocus += (s, e) => Store();
            options.Children.Add(_gain);
            root.Children.Add(options);

            _script = new TextBox
            {
                Height = 70,
                AcceptsReturn = true,
                TextWrapping = TextWrapping.Wrap,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 11,
            };
            _script.TextChanged += (s, e) => { Store(); TestScript(); };
            root.Children.Add(_script);

            _result = new TextBlock { FontSize = 10, Margin = new Thickness(0, 2, 0, 0) };
            root.Children.Add(_result);
            _live = new TextBlock { FontSize = 10 };
            root.Children.Add(_live);
            root.Children.Add(new TextBlock
            {
                Text = HelpText,
                FontSize = 9,
                TextWrapping = TextWrapping.Wrap,
                Opacity = 0.7,
                Margin = new Thickness(0, 4, 0, 0),
            });

            Content = new ScrollViewer { Content = root, VerticalScrollBarVisibility = ScrollBarVisibility.Auto };

            _liveTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            _liveTimer.Tick += (s, e) => UpdateLive();
            Loaded += (s, e) => _liveTimer.Start();
            Unloaded += (s, e) => _liveTimer.Stop();
        }

        private int SettingsIndex
        {
            get
            {
                if (_plugin == null || _plugin.Settings == null) return -1;
                return (int)_plugin.Settings.table_selected * Constants.effectPipelineChannels + Channel;
            }
        }

        // Called by the custom effects tab whenever the plugin, settings or the selected pedal change.
        public void Bind(DIY_FFB_Pedal plugin)
        {
            _plugin = plugin;
            int idx = SettingsIndex;
            if (idx < 0 || idx >= _plugin.Settings.Pipeline_scripts.Length) return;
            if (idx == _shownIndex && ReferenceEquals(_shownSettings, _plugin.Settings)) return;
            _shownIndex = idx;
            _shownSettings = _plugin.Settings;

            _updating = true;
            DIYFFBPedalSettings settings = _plugin.Settings;
            _enable.IsChecked = settings.Pipeline_enable_flag[idx];
            _type.SelectedIndex = 0;
            for (int i = 0; i < _type.Items.Count; i++)
            {
                if ((byte)(EffectPipelineOffsetType)((ComboBoxItem)_type.Items[i]).Tag == settings.Pipeline_offset_type[idx])
                    _type.SelectedIndex = i;
            }
            _gain.Text = settings.Pipeline_gain[idx].ToString("0.###", CultureInfo.InvariantCulture);
            _script.Text = settings.Pipeline_scripts[idx] ?? string.Empty;
            _updating = false;
            TestScript();
        }

        private void Store()
        {
            if (_updating) return;
            int idx = SettingsIndex;
            if (idx < 0) return;
            DIYFFBPedalSettings settings = _plugin.Settings;
            settings.Pipeline_enable_flag[idx] = _enable.IsChecked == true;
            if (_type.SelectedItem is ComboBoxItem item)
                settings.Pipeline_offset_type[idx] = (byte)(EffectPipelineOffsetType)item.Tag;
            double gain;
            if (double.TryParse(_gain.Text.Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out gain))
                settings.Pipeline_gain[idx] = Math.Max(0.0, Math.Min(10.0, gain));
            _gain.Text = settings.Pipeline_gain[idx].ToString("0.###", CultureInfo.InvariantCulture);
            settings.Pipeline_scripts[idx] = _script.Text;
        }

        private void TestScript()
        {
            if (_updating || _plugin == null || _plugin.EffectPipelines == null) return;
            if (string.IsNullOrWhiteSpace(_script.Text))
            {
                _result.Text = "Script result at t = 0: -";
                return;
            }
            _result.Text = "Script result at t = 0: " + _plugin.EffectPipelines.TestScript(_script.Text, _plugin.PluginManager);
        }

        private void UpdateLive()
        {
            int idx = SettingsIndex;
            if (idx < 0 || _plugin.EffectPipelines == null) return;
            int pedal = (int)_plugin.Settings.table_selected;
            string error = _plugin.EffectPipelines.GetError(pedal, Channel);
            _live.Text = error != null
                ? "Live: error - " + error
                : "Live output: " + _plugin.EffectPipelines.GetLastOutput(pedal, Channel).ToString("0.###", CultureInfo.InvariantCulture);
        }
    }
}
