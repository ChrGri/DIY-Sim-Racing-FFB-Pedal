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
    /// GeneralSetting_MPC.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_MPC : UserControl
    {
        public GeneralSetting_MPC()
        {
            InitializeComponent();
            DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_MPC),
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
            if (d is GeneralSetting_MPC control && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_MPC_0th_gain.SliderValue=newData.payloadPedalConfig_.MPC_0th_order_gain;
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
        public void MPCGainValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.MPC_0th_order_gain = (float)e.NewValue;
            dap_config_st= tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
