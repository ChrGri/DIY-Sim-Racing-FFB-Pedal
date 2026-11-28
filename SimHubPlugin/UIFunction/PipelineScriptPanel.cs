using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Compact Scripts tab panel: dropdown, mounted scripts summary, enable toggle, Test, Live plot, and Edit popup.
    // Built in code; the custom effects tab hosts it in the SCRIPTS tab.
    public class PipelineScriptPanel : UserControl
    {
        private DIY_FFB_Pedal _plugin;
        private readonly ComboBox _slotList;
        private readonly CheckBox _enable;
        private readonly TextBlock _empty;
        private readonly TextBlock _mountedSummary;
        private readonly TextBlock _live;
        private readonly DispatcherTimer _liveTimer;
        private readonly CheckBox _ackFull;
        private readonly CheckBox _ackCompact;
        private readonly DockPanel _warningRow;
        private readonly StackPanel _slotRow;
        private bool _updating;
        private int _shownPedal = -1;
        private DIYFFBPedalSettings _shownSettings;
        private PipelineScriptSlot _slot;

        public PipelineScriptPanel()
        {
            var root = new StackPanel { Margin = new Thickness(4) };

            // Warning: scripts turn the pedal into an active device (full robot bubble when unchecked).
            var yellow = new SolidColorBrush(Color.FromRgb(0xFF, 0xC1, 0x07));
            var bubbleBackground = new SolidColorBrush(Color.FromRgb(0x2E, 0x2A, 0x1E));
            _warningRow = new DockPanel { Margin = new Thickness(0, 0, 0, 6) };
            var robot = new PixelDangerRobot { Margin = new Thickness(0, 4, 0, 0), VerticalAlignment = VerticalAlignment.Top };
            DockPanel.SetDock(robot, Dock.Left);
            _warningRow.Children.Add(robot);

            var bubble = new Border
            {
                BorderBrush = yellow,
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(8),
                Background = bubbleBackground,
                Padding = new Thickness(8, 4, 8, 4),
            };
            var warningText = new StackPanel();
            var titleRow = new StackPanel { Orientation = Orientation.Horizontal };
            titleRow.Children.Add(new PixelDangerSign { Margin = new Thickness(0, 0, 6, 0), VerticalAlignment = VerticalAlignment.Center });
            titleRow.Children.Add(new TextBlock
            {
                Text = Loc.T("TextScriptDangerTitle", "DANGER: robot mode!"),
                FontWeight = FontWeights.Bold,
                Foreground = yellow,
                VerticalAlignment = VerticalAlignment.Center,
            });
            warningText.Children.Add(titleRow);
            warningText.Children.Add(new TextBlock
            {
                Text = Loc.T("TextScriptDangerText", "With scripts I move the pedal on my own. I can push your foot and put energy into it instead " +
                       "of only reacting to it. A wrong sign, unit or typo can mean full force or a hard kick. " +
                       "Start with small values and gain 1, keep your foot on the pedal when testing, and disable the script at the first surprise. " +
                       "Any injury or damage is solely your own responsibility."),
                TextWrapping = TextWrapping.Wrap,
                FontSize = 10,
                Margin = new Thickness(0, 2, 0, 0),
            });
            _ackFull = new CheckBox
            {
                Content = Loc.T("TextScriptDangerAck", "I understand, let me script"),
                Margin = new Thickness(0, 4, 0, 0),
                FontSize = 11,
            };
            _ackFull.Checked += (s, e) => StoreAck(true);
            _ackFull.Unchecked += (s, e) => StoreAck(false);
            warningText.Children.Add(_ackFull);
            bubble.Child = warningText;

            var bubbleHost = new Grid { Margin = new Thickness(12, 0, 0, 0) };
            bubbleHost.Children.Add(bubble);
            var tail = new Canvas
            {
                Width = 14,
                Height = 18,
                HorizontalAlignment = HorizontalAlignment.Left,
                VerticalAlignment = VerticalAlignment.Top,
                Margin = new Thickness(-12, 20, 0, 0),
            };
            tail.Children.Add(new System.Windows.Shapes.Polygon
            {
                Points = new PointCollection { new Point(0, 9), new Point(14, 1), new Point(14, 17) },
                Fill = bubbleBackground,
            });
            tail.Children.Add(new System.Windows.Shapes.Polyline
            {
                Points = new PointCollection { new Point(12.5, 1), new Point(0.5, 9), new Point(12.5, 17) },
                Stroke = yellow,
                StrokeThickness = 1,
            });
            bubbleHost.Children.Add(tail);
            _warningRow.Children.Add(bubbleHost);
            root.Children.Add(_warningRow);

            // Simplified single-line acknowledgment shown when already checked.
            _ackCompact = new CheckBox
            {
                Content = Loc.T("TextScriptDangerAckShort", "I understand the safety risks and take full responsibility for any injury or damage."),
                Margin = new Thickness(0, 0, 0, 6),
                FontSize = 10,
                Foreground = yellow,
                Visibility = Visibility.Collapsed,
            };
            _ackCompact.Checked += (s, e) => StoreAck(true);
            _ackCompact.Unchecked += (s, e) => StoreAck(false);
            root.Children.Add(_ackCompact);

            // Control row: dropdown + Enabled + Script Library and Editor + Test + Live plot
            _slotRow = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 6) };
            _slotRow.Children.Add(new TextBlock { Text = Loc.T("TextScriptSlot", "Script:"), VerticalAlignment = VerticalAlignment.Center, Width = 45 });
            _slotList = new ComboBox { Width = 140, Height = 22 };
            _slotList.SelectionChanged += (s, e) =>
            {
                if (_updating) return;
                ShowSlot(_slotList.SelectedItem is ComboBoxItem item ? item.Tag as PipelineScriptSlot : null);
            };
            _slotRow.Children.Add(_slotList);

            _enable = new CheckBox { Content = Loc.T("TextScriptEnabled", "Enabled"), VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(8, 0, 0, 0) };
            _enable.Checked += (s, e) => ToggleEnabled(true);
            _enable.Unchecked += (s, e) => ToggleEnabled(false);
            _slotRow.Children.Add(_enable);

            var editButton = SmallButton(Loc.T("TextScriptLibraryAndEditor", "Script Library and Editor"));
            editButton.MinWidth = 155;
            editButton.Padding = new Thickness(10, 0, 10, 0);
            editButton.Click += (s, e) =>
            {
                if (_plugin == null)
                {
                    MessageBox.Show("Plugin is not initialized yet.", "Script Library and Editor", MessageBoxButton.OK, MessageBoxImage.Warning);
                    return;
                }
                int pedal = Pedal >= 0 ? Pedal : 1;
                try
                {
                    ScriptEditorWindow.ShowFor(_plugin, Window.GetWindow(this), pedal, _slot, () => RefreshList(_slot));
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString(), "Script Library and Editor", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            };
            _slotRow.Children.Add(editButton);

            var testButton = SmallButton(Loc.T("TextScriptTest", "Test"));
            testButton.ToolTip = Loc.T("TextScriptTestTooltip", "Try the effects without a game: simulated values and scenarios");
            testButton.Click += (s, e) =>
            {
                if (_plugin != null) EffectTestWindow.ShowFor(_plugin, Window.GetWindow(this), () => _slot);
            };
            _slotRow.Children.Add(testButton);

            var plotButton = SmallButton(Loc.T("TextScriptLivePlot", "Live plot"));
            plotButton.Click += (s, e) =>
            {
                if (_plugin != null) EffectPipelinePlotWindow.ShowFor(_plugin, Window.GetWindow(this));
            };
            _slotRow.Children.Add(plotButton);
            root.Children.Add(_slotRow);

            _empty = new TextBlock
            {
                Text = Loc.T("TextScriptEmpty", "No effect for this pedal yet. Pick one from the \"Library\" or write your own with \"+ New\". All enabled scripts of a pedal are added up."),
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 4, 0, 4),
            };
            root.Children.Add(_empty);

            _mountedSummary = new TextBlock
            {
                FontSize = 11,
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 2, 0, 4),
            };
            root.Children.Add(_mountedSummary);

            _live = new TextBlock { FontSize = 10, TextWrapping = TextWrapping.Wrap, Opacity = 0.85 };
            root.Children.Add(_live);

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
            bool ack = _plugin.Settings.Pipeline_warning_ack;
            _ackFull.IsChecked = ack;
            _ackCompact.IsChecked = ack;
            _updating = false;
            UpdateAckState();
            RefreshList(null);
        }

        public void RefreshFromExternal()
        {
            if (_plugin == null || Pedal < 0) return;
            RefreshList(_slot);
        }

        private void StoreAck(bool ack)
        {
            if (_updating || _plugin == null || _plugin.Settings == null) return;
            _plugin.Settings.Pipeline_warning_ack = ack;
            _updating = true;
            _ackFull.IsChecked = ack;
            _ackCompact.IsChecked = ack;
            _updating = false;
            UpdateAckState();
        }

        private void UpdateAckState()
        {
            bool ack = _plugin != null && _plugin.Settings != null && _plugin.Settings.Pipeline_warning_ack;
            _warningRow.Visibility = ack ? Visibility.Collapsed : Visibility.Visible;
            _ackCompact.Visibility = ack ? Visibility.Visible : Visibility.Collapsed;
            _slotRow.IsEnabled = ack;
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
            UpdateMountedSummary(slots);
            ShowSlot(select);
        }

        private void UpdateMountedSummary(List<PipelineScriptSlot> slots)
        {
            if (slots == null || slots.Count == 0)
            {
                _mountedSummary.Visibility = Visibility.Collapsed;
                return;
            }
            var parts = new List<string>();
            for (int i = 0; i < slots.Count; i++)
            {
                string name = string.IsNullOrWhiteSpace(slots[i].Name)
                    ? Loc.T("TextScriptDefaultName", "Script") + " " + (i + 1)
                    : slots[i].Name.Trim();
                parts.Add(name + (slots[i].Enabled ? " (ON)" : " (off)"));
            }
            _mountedSummary.Text = Loc.T("TextScriptMountedList", "Mounted:") + " " + string.Join(", ", parts);
            _mountedSummary.Visibility = Visibility.Visible;
        }

        private void ShowSlot(PipelineScriptSlot slot)
        {
            _slot = slot;
            bool has = slot != null;
            _enable.IsEnabled = has;
            _empty.Visibility = has ? Visibility.Collapsed : Visibility.Visible;
            _live.Visibility = has ? Visibility.Visible : Visibility.Collapsed;
            if (!has)
            {
                _updating = true;
                _enable.IsChecked = false;
                _updating = false;
                return;
            }

            _updating = true;
            _enable.IsChecked = slot.Enabled;
            _updating = false;
        }

        private void ToggleEnabled(bool enabled)
        {
            if (_updating || _slot == null || _plugin?.EffectPipelines == null) return;
            _slot.Enabled = enabled;
            for (int i = 0; i < _slotList.Items.Count; i++)
            {
                if (_slotList.Items[i] is ComboBoxItem item && item.Tag == _slot)
                    item.Content = _slot.DisplayName(i + 1);
            }
            UpdateMountedSummary(_plugin.EffectPipelines.GetSlots(Pedal));
        }

        private void UpdateLive()
        {
            if (_plugin == null || _plugin.EffectPipelines == null) return;
            if (Pedal != _shownPedal && Pedal >= 0)
            {
                _shownPedal = Pedal;
                RefreshList(null);
            }
            if (_slot == null) return;
            string error = _plugin.EffectPipelines.GetError(_slot);
            string cpu = "   CPU: " + _plugin.EffectPipelines.GetEvalTime(_slot).ToString("0.0", CultureInfo.InvariantCulture) + " ms/s";
            _live.Text = error != null
                ? Loc.T("TextScriptLiveError", "Live: error -") + " " + error
                : Loc.T("TextScriptLiveOutput", "Live output (after gain):") + " " + _plugin.EffectPipelines.GetLastOutput(_slot).ToString("0.###", CultureInfo.InvariantCulture) + cpu;
        }
    }
}
