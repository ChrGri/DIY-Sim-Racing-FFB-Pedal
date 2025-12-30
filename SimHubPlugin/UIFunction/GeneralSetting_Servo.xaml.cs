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
using User.PluginSdkDemo.UIElement;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// KFTab.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_Servo : UserControl
    {
        public GeneralSetting_Servo()
        {
            InitializeComponent();
            dap_config_st= new DAP_config_st();
            //DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_Servo),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double PosSmoothing_value
        {
            /*
            get => (double)GetValue(KF_valueProperty);
            set => SetValue(KF_valueProperty, value);
            */
            get; set;
        }

        public DAP_config_st dap_config_st
        {

            get 
            {
                
                return (DAP_config_st)GetValue(DAP_Config_Property);
            } 
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            
            if (d is GeneralSetting_Servo control && e.NewValue is DAP_config_st newData)
            {
                //control.UpdateLabelContent();
                if (control != null)
                {
                    try
                    {
                        if (control.Slider_ServoRatioOfInertia != null) control.Slider_ServoRatioOfInertia.SliderValue = newData.payloadPedalConfig_.servoRatioOfInertia_u8;
                        if (control.Slider_PositionFilter != null) control.Slider_PositionFilter.SliderValue = newData.payloadPedalConfig_.positionSmoothingFactor_u8;
                        if (control.Slider_ServoEndstopDetecionThreshold != null) control.Slider_ServoEndstopDetecionThreshold.SliderValue = newData.payloadPedalConfig_.endstopDetectionThreshold;
                    }
                    catch
                    {

                    }
                }

            }
        }

        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }
        private void Slider_ServoRatioOfInertia_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.servoRatioOfInertia_u8 = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        public void PosFilterValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            PosSmoothing_value = e.NewValue;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.positionSmoothingFactor_u8 = (byte)PosSmoothing_value;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_ServoEndstopDetecionThreshold_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.endstopDetectionThreshold = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
