using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Test mode for the script effects: simulated SimHub property values instead of a game.
    // Shows a slider / checkbox for every property the scripts of the selected pedal read,
    // plus ready-made scenarios. Closing the window ends the test mode.
    public class EffectTestWindow : Window
    {
        private static EffectTestWindow _instance;

        public static void ShowFor(DIY_FFB_Pedal plugin, Window owner, Func<PipelineScriptSlot> currentSlot)
        {
            if (_instance == null)
            {
                _instance = new EffectTestWindow(plugin, currentSlot);
                if (owner != null) _instance.Owner = owner;
                _instance.Closed += (s, e) => _instance = null;
                _instance.Show();
            }
            else
            {
                _instance.Activate();
            }
        }

        private class Row
        {
            public string Name;
            public bool IsBool;
            public Slider Slider;
            public CheckBox Check;
            public TextBox Value;
        }

        private readonly DIY_FFB_Pedal _plugin;
        private readonly Func<PipelineScriptSlot> _currentSlot;
        private readonly Button _startStop;
        private readonly TextBlock _status;
        private readonly StackPanel _rows;
        private readonly TextBlock _noRows;
        private readonly List<Row> _rowList = new List<Row>();
        private readonly DispatcherTimer _timer;
        private string _shownSet = null;
        private bool _updating;

        private static readonly SolidColorBrush Yellow = new SolidColorBrush(Color.FromRgb(0xFF, 0xC1, 0x07));
        private static readonly SolidColorBrush Red = new SolidColorBrush(Color.FromRgb(0xE5, 0x39, 0x35));

        private EffectPipelineEngine Engine { get { return _plugin != null ? _plugin.EffectPipelines : null; } }

        private EffectTestWindow(DIY_FFB_Pedal plugin, Func<PipelineScriptSlot> currentSlot)
        {
            _plugin = plugin;
            _currentSlot = currentSlot;
            Title = Loc.T("TextTestTitle", "Script effects - test values");
            Width = 640;
            Height = 560;
            Background = new SolidColorBrush(Color.FromRgb(0x25, 0x25, 0x25));
            Foreground = Brushes.White;

            var root = new DockPanel { Margin = new Thickness(10) };

            var top = new StackPanel();
            DockPanel.SetDock(top, Dock.Top);
            var startRow = new StackPanel { Orientation = Orientation.Horizontal };
            _startStop = new Button { Height = 28, Padding = new Thickness(14, 0, 14, 0), FontWeight = FontWeights.Bold };
            _startStop.Click += (s, e) =>
            {
                if (Engine == null) return;
                if (Engine.TestActive) Engine.StopTest();
                else Engine.StartTest();
                UpdateStatus();
            };
            startRow.Children.Add(_startStop);
            _status = new TextBlock { VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(12, 0, 0, 0), Foreground = Brushes.LightGray };
            startRow.Children.Add(_status);
            top.Children.Add(startRow);

            top.Children.Add(new TextBlock
            {
                Text = Loc.T("TextTestNote",
                    "The scripts run without a game and see these values instead of the real ones (only the scripts, " +
                    "SimHub itself is not changed). Pedal force and travel stay real. The test mode switches off after " +
                    "60 s without input, when this window is closed, and as soon as a game sends data."),
                TextWrapping = TextWrapping.Wrap,
                FontSize = 11,
                Foreground = Brushes.Gray,
                Margin = new Thickness(0, 6, 0, 8),
            });

            top.Children.Add(new TextBlock { Text = Loc.T("TextTestScenarios", "Scenarios (played in a loop):"), FontWeight = FontWeights.Bold });
            var scenarios = new WrapPanel { Margin = new Thickness(0, 4, 0, 8) };
            foreach (EffectTestScenario scenario in EffectTestScenarios.All)
            {
                EffectTestScenario sc = scenario;
                var b = new Button { Content = sc.LocalName, Height = 24, Padding = new Thickness(8, 0, 8, 0), Margin = new Thickness(0, 0, 6, 4) };
                b.Click += (s, e) => { if (Engine != null) { Engine.StartScenario(sc); UpdateStatus(); } };
                scenarios.Children.Add(b);
            }
            var stopScenario = new Button { Content = Loc.T("TextTestStopScenario", "Stop scenario"), Height = 24, Padding = new Thickness(8, 0, 8, 0), Margin = new Thickness(0, 0, 6, 4) };
            stopScenario.Click += (s, e) => { if (Engine != null) { Engine.StopScenario(); UpdateStatus(); } };
            scenarios.Children.Add(stopScenario);
            var plot = new Button { Content = Loc.T("TextScriptLivePlot", "Live plot"), Height = 24, Padding = new Thickness(8, 0, 8, 0), Margin = new Thickness(0, 0, 6, 4) };
            plot.Click += (s, e) => EffectPipelinePlotWindow.ShowFor(_plugin, this);
            scenarios.Children.Add(plot);
            top.Children.Add(scenarios);

            top.Children.Add(new TextBlock
            {
                Text = Loc.T("TextTestValues", "Values read by the scripts of the selected pedal:"),
                FontWeight = FontWeights.Bold,
            });
            root.Children.Add(top);

            _rows = new StackPanel { Margin = new Thickness(0, 4, 0, 0) };
            _noRows = new TextBlock
            {
                Text = Loc.T("TextTestNoValues", "The enabled scripts of this pedal and the selected script read no SimHub property."),
                Foreground = Brushes.Gray,
                TextWrapping = TextWrapping.Wrap,
            };
            var scroll = new ScrollViewer { VerticalScrollBarVisibility = ScrollBarVisibility.Auto, Content = _rows };
            root.Children.Add(scroll);
            Content = root;

            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(200) };
            _timer.Tick += (s, e) => Refresh();
            Loaded += (s, e) => { Refresh(); _timer.Start(); };
            Closed += (s, e) =>
            {
                _timer.Stop();
                if (Engine != null) Engine.StopTest();
            };
        }

        private int Pedal
        {
            get { return (_plugin == null || _plugin.Settings == null) ? 1 : (int)_plugin.Settings.table_selected; }
        }

        private void Refresh()
        {
            UpdateStatus();
            if (Engine == null) return;

            // properties of the enabled scripts of the selected pedal and of the script in the editor
            var names = new List<string>();
            var slots = Engine.GetSlots(Pedal).Where(sl => sl.Enabled).ToList();
            PipelineScriptSlot current = _currentSlot != null ? _currentSlot() : null;
            if (current != null && !slots.Contains(current)) slots.Add(current);
            foreach (PipelineScriptSlot slot in slots)
                foreach (string name in EffectPipelineEngine.FindProperties(slot))
                    if (!names.Contains(name)) names.Add(name);
            names.Sort(StringComparer.OrdinalIgnoreCase);

            string set = string.Join("|", names);
            if (set != _shownSet)
            {
                _shownSet = set;
                BuildRows(names);
            }

            // show values set by a running scenario
            _updating = true;
            foreach (Row row in _rowList)
            {
                object value = Engine.GetOverride(row.Name);
                if (value == null) continue;
                if (row.IsBool)
                {
                    row.Check.IsChecked = ToBool(value);
                }
                else
                {
                    double d = ToDouble(value);
                    if (!row.Value.IsKeyboardFocused) row.Value.Text = d.ToString("0.###", CultureInfo.InvariantCulture);
                    if (!row.Slider.IsMouseCaptureWithin)
                    {
                        if (d > row.Slider.Maximum) row.Slider.Maximum = d;
                        if (d < row.Slider.Minimum) row.Slider.Minimum = d;
                        row.Slider.Value = d;
                    }
                }
            }
            _updating = false;
        }

        private void UpdateStatus()
        {
            bool active = Engine != null && Engine.TestActive;
            _startStop.Content = active ? Loc.T("TextTestStop", "Stop test") : Loc.T("TextTestStart", "Start test");
            _startStop.Background = active ? Red : Yellow;
            _startStop.Foreground = Brushes.Black;
            if (!active)
            {
                _status.Text = Loc.T("TextTestOff", "Test mode off");
                return;
            }
            string scenario = Engine.ScenarioName;
            _status.Text = string.Format(Loc.T("TextTestOn", "Test mode on - switches off in {0} s"), Math.Ceiling(Engine.TestRemaining_s)) +
                           (scenario != null ? "   |   " + scenario : "");
        }

        private void BuildRows(List<string> names)
        {
            _rows.Children.Clear();
            _rowList.Clear();
            if (names.Count == 0)
            {
                _rows.Children.Add(_noRows);
                return;
            }
            foreach (string name in names)
            {
                var row = new Row { Name = name, IsBool = LooksBool(name) };
                var panel = new DockPanel { Margin = new Thickness(0, 2, 0, 2) };
                string shortName = name.Substring(name.LastIndexOf('.') + 1);
                var label = new TextBlock { Text = shortName, Width = 190, VerticalAlignment = VerticalAlignment.Center, ToolTip = name, Foreground = Brushes.White };
                DockPanel.SetDock(label, Dock.Left);
                panel.Children.Add(label);

                object current = Engine.GetOverride(name);
                if (row.IsBool)
                {
                    row.Check = new CheckBox { VerticalAlignment = VerticalAlignment.Center, IsChecked = current != null && ToBool(current) };
                    row.Check.Checked += (s, e) => Set(row, true);
                    row.Check.Unchecked += (s, e) => Set(row, false);
                    panel.Children.Add(row.Check);
                }
                else
                {
                    double min, max, def;
                    GuessRange(name, out min, out max, out def);
                    double value = current != null ? ToDouble(current) : def;
                    row.Value = new TextBox { Width = 70, Height = 22, VerticalContentAlignment = VerticalAlignment.Center, Text = value.ToString("0.###", CultureInfo.InvariantCulture) };
                    DockPanel.SetDock(row.Value, Dock.Right);
                    row.Value.LostFocus += (s, e) => SetFromText(row);
                    row.Value.KeyDown += (s, e) => { if (e.Key == System.Windows.Input.Key.Enter) SetFromText(row); };
                    panel.Children.Add(row.Value);
                    row.Slider = new Slider
                    {
                        Minimum = Math.Min(min, value),
                        Maximum = Math.Max(max, value),
                        Value = value,
                        Margin = new Thickness(0, 0, 8, 0),
                        VerticalAlignment = VerticalAlignment.Center,
                    };
                    row.Slider.ValueChanged += (s, e) =>
                    {
                        if (_updating) return;
                        row.Value.Text = row.Slider.Value.ToString("0.###", CultureInfo.InvariantCulture);
                        Set(row, row.Slider.Value);
                    };
                    panel.Children.Add(row.Slider);
                    // start with a defined value, so the script sees it as soon as the test starts
                    if (current == null) Engine.SetOverride(name, value);
                }
                if (row.IsBool && current == null) Engine.SetOverride(name, false);
                _rowList.Add(row);
                _rows.Children.Add(panel);
            }
        }

        private void Set(Row row, object value)
        {
            if (_updating || Engine == null) return;
            // moving a value by hand ends a running scenario and starts the test mode
            Engine.StopScenario();
            Engine.SetOverride(row.Name, value);
            Engine.StartTest();
            UpdateStatus();
        }

        private void SetFromText(Row row)
        {
            double d;
            if (!double.TryParse(row.Value.Text.Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out d)) return;
            _updating = true;
            if (d > row.Slider.Maximum) row.Slider.Maximum = d;
            if (d < row.Slider.Minimum) row.Slider.Minimum = d;
            row.Slider.Value = d;
            _updating = false;
            Set(row, d);
        }

        private static bool LooksBool(string name)
        {
            string n = name.ToLowerInvariant();
            return n.EndsWith("active") || n.Contains(".is") || n.EndsWith("reached") || n.EndsWith("abson");
        }

        // slider range and start value from the property name
        private static void GuessRange(string name, out double min, out double max, out double def)
        {
            string n = name.ToLowerInvariant();
            min = 0; max = 100; def = 0;
            if (n.EndsWith("maxrpm")) { min = 1000; max = 20000; def = 8000; }
            else if (n.Contains("rpm")) { max = 10000; def = 3000; }
            else if (n.Contains("speed")) { max = 350; def = 100; }
            else if (n.Contains("pcttorque")) { min = -1; max = 1; }
            else if (n.Contains("pct") || n.Contains("percent")) { max = n.Contains("indicator") ? 1 : 100; }
            else if (n.Contains("accelerationg") || n.EndsWith("accel") || n.Contains("lataccel") || n.Contains("longaccel")) { min = -5; max = 5; }
            else if (n.Contains("acceleration")) { min = -50; max = 50; }
            else if (n.Contains("shockvel")) { min = -1; max = 1; }
            else if (n.Contains("temperature")) { max = 1200; def = 300; }
            else if (n.Contains("torque")) { min = -50; max = 50; }
        }

        private static bool ToBool(object value)
        {
            if (value is bool b) return b;
            try { return Convert.ToDouble(value, CultureInfo.InvariantCulture) != 0.0; } catch { return false; }
        }

        private static double ToDouble(object value)
        {
            if (value is bool b) return b ? 1.0 : 0.0;
            try { return Convert.ToDouble(value, CultureInfo.InvariantCulture); } catch { return 0.0; }
        }
    }
}
