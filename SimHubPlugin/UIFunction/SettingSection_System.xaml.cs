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
    /// SettingSection_System.xaml 的互動邏輯
    /// </summary>
    public partial class SettingSection_System : UserControl
    {
        public SettingSection_System()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(SettingSection_System),
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
            typeof(SettingSection_System),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);
                //updateUI();

            }
        }

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(SettingSection_System),
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



        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    if (CheckBox_Pedal_ESPNow_autoconnect != null) CheckBox_Pedal_ESPNow_autoconnect.IsChecked = (Settings.Pedal_ESPNow_auto_connect_flag);
                    if (CheckBox_using_CDC_for_bridge!=null) CheckBox_using_CDC_for_bridge.IsChecked = Settings.Using_CDC_bridge;
                    if (Debug_check != null) Debug_check.IsChecked = Settings.advanced_b;
                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SettingSection_System;
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
            var control = d as SettingSection_System;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SettingSection_System;
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
        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
        }

        private void CheckBox_Pedal_ESPNow_autoconnect_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Pedal_ESPNow_auto_connect_flag = true;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_Pedal_ESPNow_autoconnect_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Pedal_ESPNow_auto_connect_flag = false;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_using_CDC_for_bridge_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Using_CDC_bridge = false;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_using_CDC_for_bridge_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Using_CDC_bridge = true;
            SettingsChangedEvent(Settings);
        }

        private void Debug_check_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.advanced_b=false;
            SettingsChangedEvent(Settings);
        }

        private void Debug_check_Checked(object sender, RoutedEventArgs e)
        {
            Settings.advanced_b = true;
            SettingsChangedEvent(Settings);
        }
    }
}
