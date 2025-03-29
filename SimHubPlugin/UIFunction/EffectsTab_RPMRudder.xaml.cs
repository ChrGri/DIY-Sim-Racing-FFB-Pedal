using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// EffectsTab_RPMRudder.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_RPMRudder : UserControl
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
                    if(control.Rangeslider_RPM_freq_rudder!=null) control.Rangeslider_RPM_freq_rudder.LowerValue = control.dap_config_st.payloadPedalConfig_.RPM_min_freq;
                    if (control.Rangeslider_RPM_freq_rudder != null) control.Rangeslider_RPM_freq_rudder.UpperValue = control.dap_config_st.payloadPedalConfig_.RPM_max_freq;
                    if(control.label_RPM_freq_max_rudder!=null) control.label_RPM_freq_max_rudder.Content = "MAX:" + control.dap_config_st.payloadPedalConfig_.RPM_max_freq + "Hz";
                    if(control.label_RPM_freq_min_rudder!=null) control.label_RPM_freq_min_rudder.Content = "MIN:" + control.dap_config_st.payloadPedalConfig_.RPM_min_freq + "Hz";
                    if(control.Slider_RPM_AMP_rudder!=null) control.Slider_RPM_AMP_rudder.SliderValue = (double)(control.dap_config_st.payloadPedalConfig_.RPM_AMP) / (double)100.0f;
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
            if (checkbox_enable_RPM_rudder != null) Settings.Rudder_RPM_effect_b = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_RPM_rudder_Unchecked(object sender, RoutedEventArgs e)
        {
            if (checkbox_enable_RPM_rudder != null) Settings.Rudder_RPM_effect_b = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_RPM_AMP_rudder_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_AMP = (Byte)(e.NewValue * 100);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Rangeslider_RPM_freq_rudder_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_min_freq = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            if(label_RPM_freq_min_rudder!=null) label_RPM_freq_min_rudder.Content = "MIN:" + dap_config_st.payloadPedalConfig_.RPM_min_freq + "Hz";
        }

        private void Rangeslider_RPM_freq_rudder_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_max_freq = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            if (label_RPM_freq_max_rudder != null) label_RPM_freq_max_rudder.Content = "MAX:" + dap_config_st.payloadPedalConfig_.RPM_max_freq + "Hz";
        }
    }
}
