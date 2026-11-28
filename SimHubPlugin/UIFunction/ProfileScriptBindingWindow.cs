using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace DiyFfbPedal.UIFunction
{
    // Modal dialog for binding an unlimited number of effect scripts (from effects\) to one pedal of a Profile.
    public class ProfileScriptBindingWindow : Window
    {
        private static string[] PedalNames => new[]
        {
            Loc.T("TextPedalIdClutch", "Clutch"),
            Loc.T("TextPedalIdBrake", "Brake"),
            Loc.T("TextPedalIdThrottle", "Throttle"),
        };

        private readonly DAP_system_profile_cls _profile;
        private readonly int _pedal;
        private readonly List<ProfileScriptBinding> _workingBindings;
        private readonly ComboBox _availableCombo;
        private readonly StackPanel _rowsPanel;
        private readonly TextBlock _emptyText;
        private List<EffectPreset> _presets = new List<EffectPreset>();

        public ProfileScriptBindingWindow(DAP_system_profile_cls profile, int pedal)
        {
            _profile = profile;
            _pedal = pedal;
            _workingBindings = new List<ProfileScriptBinding>();
            if (_profile?.ScriptBindings != null)
            {
                foreach (var b in _profile.ScriptBindings)
                {
                    if (b != null && b.Pedal == pedal)
                        _workingBindings.Add(b.Clone());
                }
            }

            string pedalName = (pedal >= 0 && pedal < PedalNames.Length) ? PedalNames[pedal] : "";
            Title = Loc.T("TextProfileScriptsTitle", "Profile Scripts") + " - " + pedalName;
            Width = 560;
            Height = 420;
            WindowStartupLocation = WindowStartupLocation.CenterOwner;
            Background = new SolidColorBrush(Color.FromRgb(0x25, 0x25, 0x25));
            Foreground = Brushes.White;

            var root = new DockPanel { Margin = new Thickness(10) };

            // Top: add script from effects\
            var top = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 0, 8) };
            DockPanel.SetDock(top, Dock.Top);
            _availableCombo = new ComboBox { Width = 320, Height = 24, Margin = new Thickness(0, 0, 6, 0) };
            top.Children.Add(_availableCombo);

            var addBtn = new Button { Content = Loc.T("TextProfileScriptsAdd", "+ Add"), Height = 24, Padding = new Thickness(10, 0, 10, 0), Margin = new Thickness(0, 0, 6, 0) };
            addBtn.Click += (s, e) => AddSelected();
            top.Children.Add(addBtn);

            var reloadBtn = new Button { Content = Loc.T("TextLibraryReload", "Reload"), Height = 24, Padding = new Thickness(10, 0, 10, 0) };
            reloadBtn.Click += (s, e) => LoadAvailablePresets();
            top.Children.Add(reloadBtn);
            root.Children.Add(top);

            // Bottom: OK / Cancel
            var bottom = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right, Margin = new Thickness(0, 8, 0, 0) };
            DockPanel.SetDock(bottom, Dock.Bottom);
            var okBtn = new Button { Content = Loc.T("TextOk", "OK"), Width = 80, Height = 26, Margin = new Thickness(0, 0, 8, 0), IsDefault = true };
            okBtn.Click += (s, e) => SaveAndClose();
            bottom.Children.Add(okBtn);
            var cancelBtn = new Button { Content = Loc.T("TextCancel", "Cancel"), Width = 80, Height = 26, IsCancel = true };
            cancelBtn.Click += (s, e) => { DialogResult = false; Close(); };
            bottom.Children.Add(cancelBtn);
            root.Children.Add(bottom);

            // Center: scrollable list of bound scripts
            _rowsPanel = new StackPanel();
            _emptyText = new TextBlock
            {
                Text = Loc.T("TextProfileScriptsEmpty", "No scripts bound to this pedal yet. Pick one above and click '+ Add'."),
                Foreground = Brushes.Gray,
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(4),
            };
            var scroll = new ScrollViewer
            {
                Content = _rowsPanel,
                VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
                Background = new SolidColorBrush(Color.FromRgb(0x1B, 0x1B, 0x1B)),
                Padding = new Thickness(4),
            };
            root.Children.Add(scroll);

            Content = root;

            LoadAvailablePresets();
            RebuildRows();
        }

        private void LoadAvailablePresets()
        {
            EffectPresetFiles.EnsureBuiltIns();
            List<string> errors;
            _presets = EffectPresetFiles.Load(out errors);
            _availableCombo.Items.Clear();
            foreach (var p in _presets)
            {
                string fileName = string.IsNullOrEmpty(p.FilePath) ? "" : Path.GetFileName(p.FilePath);
                if (string.IsNullOrEmpty(fileName)) continue;
                _availableCombo.Items.Add(new ComboBoxItem
                {
                    Content = p.LocalCategory + " - " + p.LocalName + " (" + fileName + ")",
                    Tag = fileName,
                });
            }
            if (_availableCombo.Items.Count > 0) _availableCombo.SelectedIndex = 0;
        }

        private void AddSelected()
        {
            if (_availableCombo.SelectedItem is ComboBoxItem item && item.Tag is string fileName)
            {
                _workingBindings.Add(new ProfileScriptBinding
                {
                    Pedal = _pedal,
                    ScriptFile = fileName,
                    Enabled = true,
                    Gain = 1.0,
                });
                RebuildRows();
            }
        }

        private void RebuildRows()
        {
            _rowsPanel.Children.Clear();
            if (_workingBindings.Count == 0)
            {
                _rowsPanel.Children.Add(_emptyText);
                return;
            }

            foreach (var binding in _workingBindings.ToList())
            {
                var row = new DockPanel { Margin = new Thickness(2, 3, 2, 3) };

                var removeBtn = new Button
                {
                    Content = Loc.T("TextScriptRemove", "Remove"),
                    Height = 22,
                    Padding = new Thickness(8, 0, 8, 0),
                    Margin = new Thickness(6, 0, 0, 0),
                };
                DockPanel.SetDock(removeBtn, Dock.Right);
                removeBtn.Click += (s, e) =>
                {
                    _workingBindings.Remove(binding);
                    RebuildRows();
                };
                row.Children.Add(removeBtn);

                var gainBox = new TextBox
                {
                    Width = 50,
                    Height = 22,
                    VerticalContentAlignment = VerticalAlignment.Center,
                    Text = binding.Gain.ToString("0.###", CultureInfo.InvariantCulture),
                    Margin = new Thickness(4, 0, 0, 0),
                };
                DockPanel.SetDock(gainBox, Dock.Right);
                gainBox.LostFocus += (s, e) =>
                {
                    double g;
                    if (double.TryParse(gainBox.Text.Replace(',', '.'), NumberStyles.Float, CultureInfo.InvariantCulture, out g))
                        binding.Gain = Math.Max(0.0, Math.Min(10.0, g));
                    gainBox.Text = binding.Gain.ToString("0.###", CultureInfo.InvariantCulture);
                };
                row.Children.Add(gainBox);

                var gainLabel = new TextBlock
                {
                    Text = Loc.T("TextScriptGain", "Gain:"),
                    VerticalAlignment = VerticalAlignment.Center,
                    Foreground = Brushes.LightGray,
                    Margin = new Thickness(8, 0, 0, 0),
                };
                DockPanel.SetDock(gainLabel, Dock.Right);
                row.Children.Add(gainLabel);

                var enableBox = new CheckBox
                {
                    IsChecked = binding.Enabled,
                    VerticalAlignment = VerticalAlignment.Center,
                    Margin = new Thickness(4, 0, 8, 0),
                };
                DockPanel.SetDock(enableBox, Dock.Left);
                enableBox.Checked += (s, e) => binding.Enabled = true;
                enableBox.Unchecked += (s, e) => binding.Enabled = false;
                row.Children.Add(enableBox);

                EffectPreset preset = _presets.FirstOrDefault(p =>
                    !string.IsNullOrEmpty(p.FilePath) &&
                    string.Equals(Path.GetFileName(p.FilePath), binding.ScriptFile, StringComparison.OrdinalIgnoreCase));
                string label = preset != null
                    ? preset.LocalName + " (" + binding.ScriptFile + ")"
                    : binding.ScriptFile + " [missing]";

                row.Children.Add(new TextBlock
                {
                    Text = label,
                    VerticalAlignment = VerticalAlignment.Center,
                    Foreground = preset != null ? Brushes.White : Brushes.OrangeRed,
                    TextTrimming = TextTrimming.CharacterEllipsis,
                });

                _rowsPanel.Children.Add(row);
            }
        }

        private void SaveAndClose()
        {
            if (_profile != null)
            {
                if (_profile.ScriptBindings == null) _profile.ScriptBindings = new List<ProfileScriptBinding>();
                _profile.ScriptBindings.RemoveAll(b => b != null && b.Pedal == _pedal);
                _profile.ScriptBindings.AddRange(_workingBindings);
                _profile.HasScriptBindings = true;
            }
            DialogResult = true;
            Close();
        }
    }
}
