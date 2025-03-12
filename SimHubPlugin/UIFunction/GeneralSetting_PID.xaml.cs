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
    /// GeneralSetting_PID.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_PID : UserControl
    {
        public GeneralSetting_PID()
        {
            InitializeComponent();
            DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_PID),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_PID control && e.NewValue is DAP_config_st newData)
            {
                //control.UpdateLabelContent();
                try
                {
                    control.Slider_Pgain.SliderValue = newData.payloadPedalConfig_.PID_p_gain;
                    control.Slider_Igain.SliderValue = newData.payloadPedalConfig_.PID_i_gain;
                    control.Slider_Dgain.SliderValue = newData.payloadPedalConfig_.PID_d_gain;
                    control.Slider_VFgain.SliderValue = newData.payloadPedalConfig_.PID_velocity_feedforward_gain;
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

        private void Slider_Pgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.PID_p_gain = (float)e.NewValue;
            dap_config_st= tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_Igain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.PID_i_gain = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_Dgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.PID_d_gain = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_VFgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.PID_velocity_feedforward_gain = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
