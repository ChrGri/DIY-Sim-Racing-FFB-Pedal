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

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// SettingSection_System.xaml 的互動邏輯
    /// </summary>
    public partial class SettingSection_Rudder : System.Windows.Controls.UserControl
    {
        public vJoyInterfaceWrap.vJoy _joystick;
        public bool IsJoystickInitialized = false;
        public bool isVjoyAsigned= false;
        public SettingSection_Rudder()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(SettingSection_Rudder),
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
            typeof(SettingSection_Rudder),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);

            }
        }

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(SettingSection_Rudder),
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
                    if (CheckBox_RudderType_1 != null && CheckBox_RudderType_2 != null)
                    {
                        CheckBox_RudderType_1.IsChecked = Settings.rudderMode == 0;
                       CheckBox_RudderType_2.IsChecked = Settings.rudderMode == 1;
                    }
                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SettingSection_Rudder;
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
            var control = d as SettingSection_Rudder;
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
            var control = d as SettingSection_Rudder;
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

        private void CheckBox_RudderType_Checked(object sender, RoutedEventArgs e)
        {
            if (CheckBox_RudderType_1 != null && CheckBox_RudderType_2 != null)
            {
                if (CheckBox_RudderType_1.IsChecked == true)
                {
                    Settings.rudderMode = 0;
                }
                if (CheckBox_RudderType_2.IsChecked == true)
                {
                    Settings.rudderMode = 1;
                }
            }

        }
    }
}
