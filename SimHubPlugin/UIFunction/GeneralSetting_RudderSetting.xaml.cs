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
    /// GeneralSetting_RudderSetting.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_RudderSetting : System.Windows.Controls.UserControl
    {
        public GeneralSetting_RudderSetting()
        {
            InitializeComponent();
        }

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_RudderSetting),
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
            typeof(GeneralSetting_RudderSetting),
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
            typeof(GeneralSetting_RudderSetting),
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
            var control = d as GeneralSetting_RudderSetting;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                //slider
                control.updateUI();
            }
            
        }
        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    
                    if (Slider_damping_rudder != null) Slider_damping_rudder.SliderValue = (double)Settings.rudderDamping * (double)Slider_damping_rudder.TickFrequency;
                }
            }
            catch
            {
            }
        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            //UI update here
            var control = d as GeneralSetting_RudderSetting;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                //slider
            }

        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as GeneralSetting_RudderSetting;
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

        private void Slider_damping_rudder_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.rudderDamping= (byte)((double)e.NewValue / (double)Slider_damping_rudder.TickFrequency);
        }
    }
}
