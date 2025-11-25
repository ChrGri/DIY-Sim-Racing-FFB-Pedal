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
    public partial class SettingSection_System : UserControl
    {
        public vJoyInterfaceWrap.vJoy _joystick;
        public bool IsJoystickInitialized = false;
        public bool isVjoyAsigned= false;
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
                if (_joystick != null)
                {
                    if (Settings.vjoy_output_flag == 1)
                    {
                        Checkox_Vjoy.IsChecked = true;
                        if (!IsJoystickInitialized)
                        {
                            //_joystick = new vJoyInterfaceWrap.vJoy();
                            _joystick.AcquireVJD(Settings.vjoy_order);
                            //joystick.Aquire();
                            vjoy_axis_initialize();
                            IsJoystickInitialized = true;
                        }
                    }
                    else
                    {
                        Checkox_Vjoy.IsChecked = false;
                    }
                }


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
                    //if (CheckBox_using_CDC_for_bridge!=null) CheckBox_using_CDC_for_bridge.IsChecked = Settings.Using_CDC_bridge;
                    if (Debug_check != null) Debug_check.IsChecked = Settings.advanced_b;
                    if (CheckBox_ProfileAutoChange != null) CheckBox_ProfileAutoChange.IsChecked = Settings.profileAutoChange;
                    if (Label_vjoy_order!=null) Label_vjoy_order.Content = Settings.vjoy_order;
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

        private void btn_vjoy_plus_Click(object sender, RoutedEventArgs e)
        {
            if (_joystick != null)
            {
                _joystick.RelinquishVJD(Settings.vjoy_order);

                Settings.vjoy_order += 1;
                uint max = 16;
                uint min = 1;
                Settings.vjoy_order = Math.Max(min, Math.Min(Settings.vjoy_order, max));
                Label_vjoy_order.Content = Settings.vjoy_order;
                if (Settings.vjoy_output_flag == 1)
                {
                    //joystick.Release();


                    //VjdStat status;
                    VjdStat status = _joystick.GetVJDStatus(Settings.vjoy_order);
                    //status = joystick.Joystick.GetVJDStatus(Plugin.Settings.vjoy_order);
                    switch (status)
                    {
                        case VjdStat.VJD_STAT_OWN:
                            //TextBox_debugOutput.Text = "vjoy already aquaried";
                            System.Windows.MessageBox.Show("vjoy already aquaried", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            break;
                        case VjdStat.VJD_STAT_FREE:

                            //TextBox_debugOutput.Text = "vjoy aquaried";
                            //joystick = new VirtualJoystick(Plugin.Settings.vjoy_order);
                            //joystick.Aquire();
                            _joystick.AcquireVJD(Settings.vjoy_order);
                            if (Checkox_Vjoy.IsChecked == false)
                            {
                                Checkox_Vjoy.IsChecked = true;
                            }
                            //Console.WriteLine("vJoy Device {0} is free\n", id);
                            break;
                        case VjdStat.VJD_STAT_BUSY:
                            //TextBox_debugOutput.Text = "vjoy was aquaried by other program";
                            System.Windows.MessageBox.Show("vjoy was aquaried by other program", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} is already owned by another feeder\nCannot continue\n", id);
                            return;
                        case VjdStat.VJD_STAT_MISS:
                            //TextBox_debugOutput.Text = "the selected vjoy device not enabled";
                            System.Windows.MessageBox.Show("the selected vjoy device not enabled", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} is not installed or disabled\nCannot continue\n", id);
                            return;
                        default:
                            //TextBox_debugOutput.Text = "vjoy device error";
                            System.Windows.MessageBox.Show("vjoy device error", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} general error\nCannot continue\n", id);
                            return;
                    }
                    ;
                }
            }
        }

        private void btn_vjoy_minus_Click(object sender, RoutedEventArgs e)
        {
            if (_joystick != null)
            {
                _joystick.RelinquishVJD(Settings.vjoy_order);

                Settings.vjoy_order -= 1;
                uint max = 16;
                uint min = 1;
                Settings.vjoy_order = Math.Max(min, Math.Min(Settings.vjoy_order, max));
                Label_vjoy_order.Content = Settings.vjoy_order;
                if (Settings.vjoy_output_flag == 1)
                {
                    //joystick.Release();

                    //VjdStat status;
                    VjdStat status = _joystick.GetVJDStatus(Settings.vjoy_order);
                    //status = joystick.Joystick.GetVJDStatus(Plugin.Settings.vjoy_order);
                    switch (status)
                    {
                        case VjdStat.VJD_STAT_OWN:
                            //TextBox_debugOutput.Text = "vjoy already aquaried";
                            System.Windows.MessageBox.Show("vjoy already aquaried", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            break;
                        case VjdStat.VJD_STAT_FREE:

                            //TextBox_debugOutput.Text = "vjoy aquaried";
                            //joystick = new VirtualJoystick(Plugin.Settings.vjoy_order);
                            //joystick.Aquire();
                            _joystick.AcquireVJD(Settings.vjoy_order);
                            if (Checkox_Vjoy.IsChecked == false)
                            {
                                Checkox_Vjoy.IsChecked = true;
                            }
                            //Console.WriteLine("vJoy Device {0} is free\n", id);
                            break;
                        case VjdStat.VJD_STAT_BUSY:
                            //TextBox_debugOutput.Text = "vjoy was aquaried by other program";
                            System.Windows.MessageBox.Show("vjoy was aquaried by other program", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} is already owned by another feeder\nCannot continue\n", id);
                            return;
                        case VjdStat.VJD_STAT_MISS:
                            //TextBox_debugOutput.Text = "the selected vjoy device not enabled";
                            System.Windows.MessageBox.Show("the selected vjoy device not enabled", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} is not installed or disabled\nCannot continue\n", id);
                            return;
                        default:
                            //TextBox_debugOutput.Text = "vjoy device error";
                            System.Windows.MessageBox.Show("vjoy device error", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                            Settings.vjoy_output_flag = 0;
                            Checkox_Vjoy.IsChecked = false;
                            //Console.WriteLine("vJoy Device {0} general error\nCannot continue\n", id);
                            return;
                    }
                    ;
                }
            }
        }

        private void Checkox_Vjoy_Checked(object sender, RoutedEventArgs e)
        {
            if (_joystick != null)
            {
                Settings.vjoy_output_flag = 1;
                uint vJoystickId = Settings.vjoy_order;
                _joystick.AcquireVJD(vJoystickId);
                vjoy_axis_initialize();
            }

        }

        private void Checkox_Vjoy_Unchecked(object sender, RoutedEventArgs e)
        {
            if (_joystick != null)
            {
                Settings.vjoy_output_flag = 0;
                _joystick.RelinquishVJD(Settings.vjoy_order);
            }

        }

        private void vjoy_axis_initialize()
        {
            //center all axis/hats reader
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_X);
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_Y);
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_Z);
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_RX);
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_RY);
            _joystick.SetAxis(16384, Settings.vjoy_order, HID_USAGES.HID_USAGE_RZ);
            //joystick.SetJoystickHat(0, Hats.Hat);
            //joystick.SetJoystickHat(0, Hats.HatExt1);
            //joystick.SetJoystickHat(0, Hats.HatExt2);
            //joystick.SetJoystickHat(0, Hats.HatExt3);

        }
        public void asignVjoyJoystickPtr(vJoyInterfaceWrap.vJoy _vJoy)
        { 
            _joystick = _vJoy;
            isVjoyAsigned = true;
        }

        private void CheckBox_ProfileAutoChange_Checked(object sender, RoutedEventArgs e)
        {
            Settings.profileAutoChange = true;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_ProfileAutoChange_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.profileAutoChange = false;
            SettingsChangedEvent(Settings);
        }
    }
}
