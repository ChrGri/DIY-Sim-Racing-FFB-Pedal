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
using DiyFfbPedal.UIElement;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// KFTab.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_GlobalEffects : UserControl
    {
        public GeneralSetting_GlobalEffects()
        {
            InitializeComponent();
            dap_config_st= new DAP_config_st();
            //DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_GlobalEffects),
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
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            
            if (d is GeneralSetting_GlobalEffects control && e.NewValue is DAP_config_st newData)
            {
                //control.UpdateLabelContent();
                if (control != null)
                {
                    try
                    {
                        if (control.Slider_MinForceForEffects != null) control.Slider_MinForceForEffects.SliderValue = newData.payloadPedalConfig_.minForceForEffects;
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
        private void Slider_MinForceForEffects_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.minForceForEffects = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
