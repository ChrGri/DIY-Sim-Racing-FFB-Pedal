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
    /// EffectsTab_GForce.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_GForce : UserControl
    {
        public EffectsTab_GForce()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_GForce),
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
            typeof(EffectsTab_GForce),
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
                if (Settings.table_selected == 1)
                {
                    checkbox_enable_G_force.IsEnabled = true;
                    checkbox_enable_G_force.IsChecked = (Settings.G_force_enable_flag[Settings.table_selected] == 1);
                }
                else
                {
                    checkbox_enable_G_force.IsEnabled = false;
                    checkbox_enable_G_force.IsChecked = false;
                }
            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_GForce;
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
            var control = d as EffectsTab_GForce;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_G_force_smoothness.SliderValue = control.dap_config_st.payloadPedalConfig_.G_window;
                    control.Slider_G_force_multi.SliderValue = control.dap_config_st.payloadPedalConfig_.G_multi;

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

        private void checkbox_enable_G_force_Checked(object sender, RoutedEventArgs e)
        {
            Settings.G_force_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_G_force_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.G_force_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void Slider_G_force_multi_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.G_multi = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_G_force_smoothness_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.G_window = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
