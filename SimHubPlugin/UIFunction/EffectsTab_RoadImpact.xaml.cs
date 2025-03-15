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
    /// EffectsTab_RoadImpact.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_RoadImpact : UserControl
    {
        public EffectsTab_RoadImpact()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_RoadImpact),
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
            typeof(EffectsTab_RoadImpact),
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



        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    checkbox_enable_impact.IsChecked = (Settings.Road_impact_enable_flag[Settings.table_selected] == 1);
                    textBox_impact_effect_string.Text = Settings.Road_impact_bind;
                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RoadImpact;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                try
                {
                    control.updateUI();
                }
                catch
                {
                }
            }

        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RoadImpact;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {

                    control.Slider_impact_smoothness.SliderValue = control.dap_config_st.payloadPedalConfig_.Impact_window;
                    control.Slider_impact_multi.SliderValue = control.dap_config_st.payloadPedalConfig_.Impact_multi;

                }
                catch
                {
                }

            }
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

        private void checkbox_enable_impact_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Road_impact_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_impact_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Road_impact_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void Bind_Impacteffect_Click(object sender, RoutedEventArgs e)
        {
            Settings.Road_impact_bind = (string)textBox_impact_effect_string.Text;
            Settings.Road_impact_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void Clear_Impacteffect_Click(object sender, RoutedEventArgs e)
        {
            textBox_impact_effect_string.Text = "";
            Settings.Road_impact_bind = "";
            Settings.Road_impact_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void Slider_impact_multi_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.Impact_multi = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_impact_smoothness_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.Impact_window = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
