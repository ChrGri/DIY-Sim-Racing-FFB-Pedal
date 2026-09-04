using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// EffectsTab_RPMRudder.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_RPMRudder : System.Windows.Controls.UserControl
    {
        public EffectsTab_RPMRudder()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_RPMRudder),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(EffectsTab_RPMRudder),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);
                updateUI();
            }
        }

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(EffectsTab_RPMRudder),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Cauculation_Property);
            set
            {
                SetValue(Cauculation_Property, value);
                //updateUI();
            }
        }

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {

        }
        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    if (checkbox_enable_RPM_rudder != null)
                    {
                        if (Settings.Rudder_RPM_effect_b) { checkbox_enable_RPM_rudder.IsChecked = true; }
                        else { checkbox_enable_RPM_rudder.IsChecked = false; }
                    }
                    if (Rangeslider_RPM_freq_rudder != null) Rangeslider_RPM_freq_rudder.LowerValue = Settings.rudderRPMMinFrequency;
                    if (Rangeslider_RPM_freq_rudder != null) Rangeslider_RPM_freq_rudder.UpperValue = Settings.rudderRPMMaxFrequency;
                    if (label_RPM_freq_max_rudder != null) label_RPM_freq_max_rudder.Content = "MAX:" + Settings.rudderRPMMaxFrequency + "Hz";
                    if (label_RPM_freq_min_rudder != null) label_RPM_freq_min_rudder.Content = "MIN:" + Settings.rudderRPMMinFrequency + "Hz";
                    if (Slider_RPM_AMP_rudder != null) Slider_RPM_AMP_rudder.SliderValue = (double)(Settings.rudderRPMAmp) * 100.0d / 5000.0d;
              
                }
            }
            catch
            {
            }
        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            //UI update here
            var control = d as EffectsTab_RPMRudder;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                if (control != null)
                {

                }
            }

        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RPMRudder;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
                }
                catch
                {
                }
            }
        }

        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
        }
        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue)
        {
            SettingsChanged?.Invoke(this, newValue);
        }

        private void checkbox_enable_RPM_rudder_Checked(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            if (checkbox_enable_RPM_rudder != null) Settings.Rudder_RPM_effect_b = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_RPM_rudder_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            if (checkbox_enable_RPM_rudder != null) Settings.Rudder_RPM_effect_b = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_RPM_AMP_rudder_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderRPMAmp = (Byte)(e.NewValue *5000.0d/100.0d);
            SettingsChangedEvent(Settings);
        }

        private void Rangeslider_RPM_freq_rudder_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderRPMMinFrequency = (byte)e.NewValue;
            if (label_RPM_freq_min_rudder!=null) label_RPM_freq_min_rudder.Content = "MIN:" + Settings.rudderRPMMinFrequency + "Hz";
            SettingsChangedEvent(Settings);
        }

        private void Rangeslider_RPM_freq_rudder_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderRPMMaxFrequency = (byte)e.NewValue;
            if (label_RPM_freq_max_rudder != null) label_RPM_freq_max_rudder.Content = "MAX:" + Settings.rudderRPMMaxFrequency + "Hz";
            SettingsChangedEvent(Settings);
        }

        // Inline Keyboard / Numeric Input Handlers
        private void label_RPM_freq_min_rudder_MouseLeftButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (Settings != null)
                BeginInlineEdit(label_RPM_freq_min_rudder, TextBox_edit_RPM_freq_min, Settings.rudderRPMMinFrequency);
            e.Handled = true;
        }

        private void label_RPM_freq_max_rudder_MouseLeftButtonDown(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            if (Settings != null)
                BeginInlineEdit(label_RPM_freq_max_rudder, TextBox_edit_RPM_freq_max, Settings.rudderRPMMaxFrequency);
            e.Handled = true;
        }

        private void BeginInlineEdit(FrameworkElement displayElem, System.Windows.Controls.TextBox box, double currentValue)
        {
            if (displayElem == null || box == null) return;
            box.Text = currentValue.ToString("0.###", System.Globalization.CultureInfo.InvariantCulture);
            displayElem.Visibility = Visibility.Collapsed;
            box.Visibility = Visibility.Visible;
            box.Focus();
            box.SelectAll();
        }

        private void EditBox_KeyDown(object sender, System.Windows.Input.KeyEventArgs e)
        {
            var box = sender as System.Windows.Controls.TextBox;
            if (box == null) return;
            if (e.Key == System.Windows.Input.Key.Enter)
            {
                CommitInlineEdit(box);
                e.Handled = true;
            }
            else if (e.Key == System.Windows.Input.Key.Escape)
            {
                EndInlineEdit(box);
                e.Handled = true;
            }
        }

        private void EditBox_LostFocus(object sender, RoutedEventArgs e)
        {
            var box = sender as System.Windows.Controls.TextBox;
            if (box != null && box.Visibility == Visibility.Visible)
                CommitInlineEdit(box);
        }

        private void CommitInlineEdit(System.Windows.Controls.TextBox box)
        {
            if (Settings == null || box == null) return;

            string text = (box.Text ?? string.Empty).Trim().Replace(',', '.');
            if (double.TryParse(text, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out double value))
            {
                if (box == TextBox_edit_RPM_freq_min)
                {
                    // Clamp 1 to max frequency
                    value = Math.Max(1, Math.Min(Settings.rudderRPMMaxFrequency, Math.Round(value)));
                    Settings.rudderRPMMinFrequency = (byte)value;
                    if (Rangeslider_RPM_freq_rudder != null) Rangeslider_RPM_freq_rudder.LowerValue = value;
                    if (label_RPM_freq_min_rudder != null) label_RPM_freq_min_rudder.Content = "MIN:" + Settings.rudderRPMMinFrequency + "Hz";
                    SettingsChangedEvent(Settings);
                }
                else if (box == TextBox_edit_RPM_freq_max)
                {
                    // Clamp min frequency to 50
                    value = Math.Max(Settings.rudderRPMMinFrequency, Math.Min(50, Math.Round(value)));
                    Settings.rudderRPMMaxFrequency = (byte)value;
                    if (Rangeslider_RPM_freq_rudder != null) Rangeslider_RPM_freq_rudder.UpperValue = value;
                    if (label_RPM_freq_max_rudder != null) label_RPM_freq_max_rudder.Content = "MAX:" + Settings.rudderRPMMaxFrequency + "Hz";
                    SettingsChangedEvent(Settings);
                }
            }
            EndInlineEdit(box);
        }

        private void EndInlineEdit(System.Windows.Controls.TextBox box)
        {
            if (box == null) return;
            box.Visibility = Visibility.Collapsed;
            FrameworkElement elem = null;
            if (box == TextBox_edit_RPM_freq_min) elem = label_RPM_freq_min_rudder;
            else if (box == TextBox_edit_RPM_freq_max) elem = label_RPM_freq_max_rudder;
            if (elem != null) elem.Visibility = Visibility.Visible;
        }
    }
}
