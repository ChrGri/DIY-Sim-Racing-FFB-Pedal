using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace DiyFfbPedal.UIFunction
{
    // Lists the effect presets (EffectLibrary) and adds a copy of one as a script to a pedal.
    public class EffectLibraryWindow : Window
    {
        private static EffectLibraryWindow _instance;

        private static readonly string[] PedalNames = { "Clutch", "Brake", "Throttle" };

        private readonly DIY_FFB_Pedal _plugin;
        private readonly Action<PipelineScriptSlot> _added;
        private readonly ListBox _list;
        private readonly TextBlock _title;
        private readonly TextBlock _description;
        private readonly TextBlock _details;
        private readonly TextBox _script;
        private readonly Button[] _addButtons = new Button[3];
        private readonly TextBlock _status;

        // Opens the library or brings it to the front. added is called with every new script.
        public static void ShowFor(DIY_FFB_Pedal plugin, Window owner, Action<PipelineScriptSlot> added)
        {
            if (_instance == null)
            {
                _instance = new EffectLibraryWindow(plugin, added);
                if (owner != null) _instance.Owner = owner;
                _instance.Closed += (s, e) => _instance = null;
                _instance.Show();
            }
            else
            {
                _instance.Activate();
            }
        }

        private EffectLibraryWindow(DIY_FFB_Pedal plugin, Action<PipelineScriptSlot> added)
        {
            _plugin = plugin;
            _added = added;
            Title = "Effect library";
            Width = 860;
            Height = 560;
            Background = new SolidColorBrush(Color.FromRgb(0x25, 0x25, 0x25));
            Foreground = Brushes.White;

            var grid = new Grid { Margin = new Thickness(8) };
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(250) });
            grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });

            _list = new ListBox
            {
                Background = new SolidColorBrush(Color.FromRgb(0x1B, 0x1B, 0x1B)),
                Foreground = Brushes.White,
                Margin = new Thickness(0, 0, 8, 0),
            };
            string category = null;
            foreach (EffectPreset preset in EffectLibrary.Presets)
            {
                if (preset.Category != category)
                {
                    category = preset.Category;
                    _list.Items.Add(new ListBoxItem
                    {
                        Content = category.ToUpperInvariant(),
                        IsEnabled = false,
                        FontWeight = FontWeights.Bold,
                        Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xC1, 0x07)),
                        Margin = new Thickness(0, 6, 0, 0),
                    });
                }
                _list.Items.Add(new ListBoxItem { Content = "  " + preset.Name, Tag = preset, Foreground = Brushes.White });
            }
            _list.SelectionChanged += (s, e) => ShowPreset();
            Grid.SetColumn(_list, 0);
            grid.Children.Add(_list);

            var right = new DockPanel();
            var header = new StackPanel();
            DockPanel.SetDock(header, Dock.Top);
            _title = new TextBlock { FontSize = 16, FontWeight = FontWeights.Bold, Foreground = Brushes.White };
            header.Children.Add(_title);
            _description = new TextBlock { TextWrapping = TextWrapping.Wrap, Foreground = Brushes.LightGray, Margin = new Thickness(0, 4, 0, 0) };
            header.Children.Add(_description);
            _details = new TextBlock { Foreground = Brushes.Gray, FontSize = 11, Margin = new Thickness(0, 4, 0, 6) };
            header.Children.Add(_details);
            right.Children.Add(header);

            var footer = new StackPanel { Margin = new Thickness(0, 6, 0, 0) };
            DockPanel.SetDock(footer, Dock.Bottom);
            var buttons = new StackPanel { Orientation = Orientation.Horizontal };
            buttons.Children.Add(new TextBlock { Text = "Add as script to:", VerticalAlignment = VerticalAlignment.Center, Margin = new Thickness(0, 0, 6, 0), Foreground = Brushes.White });
            for (int pedal = 0; pedal < 3; pedal++)
            {
                int p = pedal;
                _addButtons[p] = new Button { Content = PedalNames[p], Height = 24, Padding = new Thickness(10, 0, 10, 0), Margin = new Thickness(0, 0, 6, 0) };
                _addButtons[p].Click += (s, e) => AddTo(p);
                buttons.Children.Add(_addButtons[p]);
            }
            footer.Children.Add(buttons);
            _status = new TextBlock { Foreground = Brushes.LightGreen, FontSize = 11, Margin = new Thickness(0, 4, 0, 0), TextWrapping = TextWrapping.Wrap };
            footer.Children.Add(_status);
            footer.Children.Add(new TextBlock
            {
                Text = "The script is added switched off. Adapt the parameters at the top of the script, then enable it. " +
                       "Start small: the pedal moves on its own.",
                Foreground = Brushes.Gray,
                FontSize = 10,
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(0, 4, 0, 0),
            });
            right.Children.Add(footer);

            _script = new TextBox
            {
                IsReadOnly = true,
                FontFamily = new FontFamily("Consolas"),
                FontSize = 11,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                HorizontalScrollBarVisibility = ScrollBarVisibility.Auto,
                Background = new SolidColorBrush(Color.FromRgb(0x1B, 0x1B, 0x1B)),
                Foreground = Brushes.White,
            };
            right.Children.Add(_script);

            Grid.SetColumn(right, 1);
            grid.Children.Add(right);
            Content = grid;

            // select the first preset
            foreach (object item in _list.Items)
            {
                if (item is ListBoxItem li && li.Tag is EffectPreset)
                {
                    _list.SelectedItem = li;
                    break;
                }
            }
            ShowPreset();
        }

        private EffectPreset Selected
        {
            get { return (_list.SelectedItem as ListBoxItem)?.Tag as EffectPreset; }
        }

        private void ShowPreset()
        {
            EffectPreset preset = Selected;
            bool has = preset != null;
            foreach (Button b in _addButtons) b.IsEnabled = has;
            if (!has)
            {
                _title.Text = "";
                _description.Text = "";
                _details.Text = "";
                _script.Text = "";
                return;
            }
            _title.Text = preset.Name;
            _description.Text = preset.Description;
            string output = preset.OffsetType == EffectPipelineOffsetType.Force_N ? "Force offset (N)"
                          : preset.OffsetType == EffectPipelineOffsetType.ForceRelative ? "Relative force"
                          : "Pedal angle offset (deg)";
            string pedal = preset.SuggestedPedal >= 0 ? PedalNames[preset.SuggestedPedal] : "any pedal";
            _details.Text = "Output: " + output + "   |   " + preset.Language + "   |   for: " + pedal;
            _script.Text = string.IsNullOrEmpty(preset.FreqScript)
                ? preset.Script
                : "// frequency (Hz): " + preset.FreqScript + "\n" + preset.Script;
            for (int p = 0; p < 3; p++)
                _addButtons[p].FontWeight = p == preset.SuggestedPedal ? FontWeights.Bold : FontWeights.Normal;
            _status.Text = "";
        }

        private void AddTo(int pedal)
        {
            EffectPreset preset = Selected;
            if (preset == null || _plugin == null || _plugin.EffectPipelines == null) return;
            PipelineScriptSlot slot = _plugin.EffectPipelines.AddSlot(pedal, preset.CreateSlot(pedal), keepName: true);
            _status.Text = "Added \"" + preset.Name + "\" to the " + PedalNames[pedal].ToLowerInvariant() + " pedal (switched off).";
            _added?.Invoke(slot);
        }
    }
}
