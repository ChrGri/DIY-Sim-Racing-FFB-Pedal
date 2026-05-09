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
    public partial class GeneralSetting_Dynamics : UserControl
    {
        public GeneralSetting_Dynamics()
        {
            InitializeComponent();
            dap_config_st = new DAP_config_st();
            //DataContext = this;
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_Dynamics),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double PosSmoothing_value
        {
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
            if (d is GeneralSetting_Dynamics control && e.NewValue is DAP_config_st newData)
            {
                if (control != null)
                {
                    try
                    {
                        

                        if (control.Slider_VirtualPedalMass != null)
                        {
                            control.Slider_VirtualPedalMass.SliderValue = (double)newData.payloadPedalConfig_.virtualPedalMass_u8 * (double)control.Slider_VirtualPedalMass.TickFrequency;
                        }

                        if (control.Slider_CoulombFriction != null)
                        {
                            control.Slider_CoulombFriction.SliderValue = (double)newData.payloadPedalConfig_.coulombFrictionIn0p1N_u8 * (double)control.Slider_CoulombFriction.TickFrequency;
                        }


                        if (control.Slider_VirtualDamping != null)
                        {
                            control.Slider_VirtualDamping.SliderValue = (double)newData.payloadPedalConfig_.virtualPedalDamping_u8 * (double)control.Slider_VirtualDamping.TickFrequency;
                        }

                        if (control.Slider_DampingProgression != null)
                        {
                            control.Slider_DampingProgression.SliderValue = (double)newData.payloadPedalConfig_.dampingProgression_u8 * (double)control.Slider_DampingProgression.TickFrequency;
                        }

                        if (control.Slider_SoftEndstopTravelRange != null)
                        {
                            control.Slider_SoftEndstopTravelRange.SliderValue = (double)newData.payloadPedalConfig_.endstopTravelRange_mm_u8 * (double)control.Slider_SoftEndstopTravelRange.TickFrequency;
                        }

                        if (control.Slider_SoftEndstopStiffness != null)
                        {
                            control.Slider_SoftEndstopStiffness.SliderValue = (double)newData.payloadPedalConfig_.endstopStiffness_kg_mm_u8 * (double)control.Slider_SoftEndstopStiffness.TickFrequency;
                        }
       

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

        private void Slider_VirtualPedalMassChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            double targetValue_fl64 = e.NewValue / (double)Slider_VirtualPedalMass.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.virtualPedalMass_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_CoulombFrictionChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            double targetValue_fl64 = e.NewValue / (double)Slider_CoulombFriction.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.coulombFrictionIn0p1N_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }


        public void Slider_VirtualPedalDampingChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

            double targetValue_fl64 = e.NewValue / (double)Slider_VirtualDamping.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.virtualPedalDamping_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        public void Slider_DampingProgressionChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {

            double targetValue_fl64 = e.NewValue / (double)Slider_DampingProgression.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.dampingProgression_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }


        public void Slider_SoftEndstopTravelRangeChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            double targetValue_fl64 = e.NewValue / (double)Slider_SoftEndstopTravelRange.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.endstopTravelRange_mm_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        public void Slider_SoftEndstopStiffnessChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            double targetValue_fl64 = e.NewValue / (double)Slider_SoftEndstopStiffness.TickFrequency;
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.endstopStiffness_kg_mm_u8 = (Byte)(targetValue_fl64);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_DampingProgression_Loaded(object sender, RoutedEventArgs e)
        {

        }
    }
}
