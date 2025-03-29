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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// GeneralSetting_ControlStrategy.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_ControlStrategy : System.Windows.Controls.UserControl
    {
        public GeneralSetting_ControlStrategy()
        {
            InitializeComponent();
            DataContext = this;
            //ControlStrategy = 0;
        }
        /*
        public static readonly DependencyProperty ControlStrategyProperty =
DependencyProperty.Register(nameof(ControlStrategy), typeof(int), typeof(GeneralSetting_ControlStrategy),
new FrameworkPropertyMetadata(0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));
        public int ControlStrategy
        {
            get => (int)GetValue(ControlStrategyProperty);
            set {
                SetValue(ControlStrategyProperty, value);
                ControlStrategy_Sel_1.IsChecked = (value == 0);
                ControlStrategy_Sel_2.IsChecked = (value == 1);
                ControlStrategy_Sel_3.IsChecked = (value == 2);
                //ControlStrategyChanged?.Invoke(control, newValue);
            }  
        }
        */

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_ControlStrategy),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }
        /*
        public event EventHandler<int> ControlStrategyChanged;
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_ControlStrategy control)
            {
                
                int newValue = (int)e.NewValue;
                control.ControlStrategy_Sel_1.IsChecked = (newValue == 0);
                control.ControlStrategy_Sel_2.IsChecked = (newValue == 1);
                control.ControlStrategy_Sel_3.IsChecked = (newValue == 2);
                control.ControlStrategyChanged?.Invoke(control, newValue);
                
                control.ControlStrategyChangedEvent(control.ControlStrategy);
            }
        }

        */
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_ControlStrategy control && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.ControlStrategy_Sel_1.IsChecked = (newData.payloadPedalConfig_.control_strategy_b == 0);
                    control.ControlStrategy_Sel_2.IsChecked = (newData.payloadPedalConfig_.control_strategy_b == 1);
                    control.ControlStrategy_Sel_3.IsChecked = (newData.payloadPedalConfig_.control_strategy_b == 2);
                    //control.ConfigChangedEvent?.Invoke(e.NewValue);
                }
                catch
                { 
                    
                }
            }
        }
        private void updateUI()
        { 
            
        }
        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }
        private void ControlStrategy_Sel_Checked(object sender, RoutedEventArgs e)
        {
            if (sender == ControlStrategy_Sel_1)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.control_strategy_b = 0;
                dap_config_st = tmp;

            }
            if (sender == ControlStrategy_Sel_2)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.control_strategy_b = 1;
                dap_config_st = tmp;
            }
            if (sender == ControlStrategy_Sel_3)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.control_strategy_b = 2;
                dap_config_st = tmp;
            }

            ConfigChangedEvent(dap_config_st);
        }


    }
}
