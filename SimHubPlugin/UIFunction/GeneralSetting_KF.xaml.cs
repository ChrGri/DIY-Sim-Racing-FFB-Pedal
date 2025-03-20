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
    public partial class GeneralSetting_KF : UserControl
    {
        public GeneralSetting_KF()
        {
            InitializeComponent();
            dap_config_st= new DAP_config_st();
            //DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_KF),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get 
            {
                
                return (DAP_config_st)GetValue(DAP_Config_Property);
            } 
            set
            {
                SetValue(DAP_Config_Property, value);
                //KF_selection = value.payloadPedalConfig_.kf_modelOrder;
                //KF_value = value.payloadPedalConfig_.kf_modelNoise;
                try
                {
                    if (dap_config_st.payloadPedalConfig_.kf_Joystick_u8 == 1) Checkbox_joystick_denoise.IsChecked = true;
                    else if(Checkbox_joystick_denoise!=null) Checkbox_joystick_denoise.IsChecked = false;
                }
                catch
                { }

            }
        }
        /*
        public static readonly DependencyProperty KF_selectionProperty =
    DependencyProperty.Register(nameof(KF_selection), typeof(int), typeof(GeneralSetting_KF),
        new FrameworkPropertyMetadata(0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));
        */
        public int KF_selection
        {
            /*
            get => (int)GetValue(KF_selectionProperty);
            set => SetValue(KF_selectionProperty, value);
            */
            get;set;
        }
        /*
        public static readonly DependencyProperty KF_valueProperty =
    DependencyProperty.Register(nameof(KF_value), typeof(double), typeof(GeneralSetting_KF),
        new FrameworkPropertyMetadata(1.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));
        */
        public double KF_value
        {
            /*
            get => (double)GetValue(KF_valueProperty);
            set => SetValue(KF_valueProperty, value);
            */
            get;set;
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            
            if (d is GeneralSetting_KF control && e.NewValue is DAP_config_st newData)
            {
                //control.UpdateLabelContent();
                if (control != null)
                {
                    try
                    {
                        if(control. KF_filter_order!=null) control.KF_filter_order.SelectedIndex = newData.payloadPedalConfig_.kf_modelOrder;
                        if (control.Slider_KF != null) control.Slider_KF.SliderValue = newData.payloadPedalConfig_.kf_modelNoise;
                        if (control.Slider_KF_Joystick != null) control.Slider_KF_Joystick.SliderValue = newData.payloadPedalConfig_.kf_modelNoise_joystick;
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
        
        //public event RoutedPropertyChangedEventHandler<double> KF_ValueChanged;
        public void KFValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            KF_value = e.NewValue;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.kf_modelNoise=(byte)KF_value;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
        //public event RoutedEventHandler KF_SelectionChanged;
        public void KF_filter_order_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            KF_selection = KF_filter_order.SelectedIndex;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.kf_modelOrder = (byte)KF_selection;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);

        }

        private void Checkbox_joystick_denoise_Checked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.kf_Joystick_u8 = (byte)1;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Checkbox_joystick_denoise_Unchecked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.kf_Joystick_u8 = (byte)0;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_KF_Joystick_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.kf_modelNoise_joystick = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
