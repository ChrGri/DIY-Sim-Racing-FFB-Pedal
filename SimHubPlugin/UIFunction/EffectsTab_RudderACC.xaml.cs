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
    /// EffectsTab_RudderACC.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_RudderACC : UserControl
    {
        public EffectsTab_RudderACC()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_RudderACC),
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
            typeof(EffectsTab_RudderACC),
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
            typeof(EffectsTab_RudderACC),
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

        }
        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    if (checkbox_Rudder_ACC_effect != null)
                    {
                        if (Settings.Rudder_ACC_effect_b) { checkbox_Rudder_ACC_effect.IsChecked = true; }
                        else { checkbox_Rudder_ACC_effect.IsChecked = false; }
                    }

                    if (Checkbox_Rudder_ACC_WindForce != null) 
                    {
                        if (Settings.Rudder_ACC_WindForce) { Checkbox_Rudder_ACC_WindForce.IsChecked = true; }
                        else { Checkbox_Rudder_ACC_WindForce.IsChecked = false; }
                    }
                }
            }
            catch
            {
            }
        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            //UI update here
            var control = d as EffectsTab_RudderACC;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                if (control != null)
                {
                    
                }
            }

        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RPMRudder;
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

        private void checkbox_Rudder_ACC_effect_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Rudder_ACC_effect_b = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_Rudder_ACC_effect_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Rudder_ACC_effect_b = false;
            SettingsChangedEvent(Settings);
        }

        private void Checkbox_Rudder_ACC_WindForce_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Rudder_ACC_WindForce = true;
            SettingsChangedEvent(Settings);
        }

        private void Checkbox_Rudder_ACC_WindForce_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Rudder_ACC_WindForce = false;
            SettingsChangedEvent(Settings);
        }
    }
}
