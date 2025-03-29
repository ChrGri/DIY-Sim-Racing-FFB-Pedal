using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
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
    /// SettingSection_Pedal.xaml 的互動邏輯
    /// </summary>
    public partial class SettingSection_Pedal : UserControl
    {
        
        public vJoyInterfaceWrap.vJoy _joystick;
        private bool IsJoystickInitialized = false;
        public SettingSection_Pedal()
        {
            InitializeComponent();
            _joystick = new vJoyInterfaceWrap.vJoy();
            
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(SettingSection_Pedal),
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
            typeof(SettingSection_Pedal),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);
                if (Settings.vjoy_output_flag == 1)
                {
                    Vjoy_out_check.IsChecked = true;
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
                    Vjoy_out_check.IsChecked = false;
                }


                updateUI();

            }
        }

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(SettingSection_Pedal),
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
                    Label_Pedal_interval_trigger.Content = "Action Interval: " + Settings.Pedal_action_interval[Settings.table_selected] + "ms";
                    Slider_Pedal_interval_trigger.Value = Settings.Pedal_action_interval[Settings.table_selected];
                    if (Settings.reading_config == 1)
                    {
                        checkbox_pedal_read.IsChecked = true;
                        CheckBox_LivePreview.IsEnabled = true;
                        if (Settings.LivePreview[Settings.table_selected])
                        {
                            CheckBox_LivePreview.IsChecked = true;
                        }
                        else
                        {
                            CheckBox_LivePreview.IsChecked = false;
                        }



                    }
                    else
                    {
                        checkbox_pedal_read.IsChecked = false;
                        for (int i = 0; i < 3; i++)
                        {
                            Settings.LivePreview[i] = false;
                        }
                        CheckBox_LivePreview.IsChecked = false;
                        CheckBox_LivePreview.IsEnabled = false;
                    }

                    if (Settings.Pedal_ESPNow_Sync_flag[Settings.table_selected])
                    {
                        CheckBox_Pedal_ESPNow_SyncFlag.IsChecked = true;
                    }
                    else
                    {
                        CheckBox_Pedal_ESPNow_SyncFlag.IsChecked = false;
                    }

                    if (Settings.USING_ESP32S3[Settings.table_selected] == true)
                    {
                        CheckBox_USINGESP32S3.IsChecked = true;
                    }
                    else
                    {
                        CheckBox_USINGESP32S3.IsChecked = false;
                    }
                    if (Settings.auto_connect_flag[Settings.table_selected] == 1)
                    {
                        checkbox_auto_connect.IsChecked = true;
                    }
                    else
                    {
                        checkbox_auto_connect.IsChecked = false;
                    }
                    Label_vjoy_order.Content = Settings.vjoy_order;
                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SettingSection_Pedal;
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
            var control = d as SettingSection_Pedal;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.textBox_debug_Flag_0.Text=newData.payloadPedalConfig_.debug_flags_0.ToString();

                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SettingSection_Pedal;
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

        private void Slider_Pedal_interval_trigger_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.Pedal_action_interval[Settings.table_selected] = (byte)e.NewValue;
            Label_Pedal_interval_trigger.Content = "Action Interval: " + Settings.Pedal_action_interval[Settings.table_selected] + "ms";
            SettingsChangedEvent(Settings);
        }

        private void textBox_debug_Flag_0_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (int.TryParse(textBox_debug_Flag_0.Text, out int result))
            {
                if ((result >= 0) && (result <= 255))
                {
                    var tmp = dap_config_st;
                    tmp.payloadPedalConfig_.debug_flags_0 = (byte)result;
                    dap_config_st = tmp;
                    ConfigChangedEvent(tmp);
                }
            }
            
        }

        private void NumericTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            // Use a regular expression to allow only numeric input
            Regex regex = new Regex("^[.][0-9]+$|^[0-9]*[.]{0,4}[0-9]*$");

            System.Windows.Controls.TextBox textBox = (System.Windows.Controls.TextBox)sender;

            e.Handled = !regex.IsMatch(textBox.Text + e.Text);

        }

        private void checkbox_auto_connect_Checked(object sender, RoutedEventArgs e)
        {
            Settings.auto_connect_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_auto_connect_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.auto_connect_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_pedal_read_Checked(object sender, RoutedEventArgs e)
        {
            Settings.reading_config = 1;
            CheckBox_LivePreview.IsEnabled = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_pedal_read_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.reading_config = 0;
            for (int i = 0; i < 3; i++)
            {
                Settings.LivePreview[i] = false;
            }
            CheckBox_LivePreview.IsChecked = false;
            CheckBox_LivePreview.IsEnabled = false;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_LivePreview_Checked(object sender, RoutedEventArgs e)
        {
            Settings.LivePreview[Settings.table_selected] = true;
            /*
            btn_SendConfig.Content = "Save Config in Pedal";
            btn_SendConfig.ToolTip = "Push Config into Pedal storage";
            */
            calculation.btn_SendConfig_Content= "Save Config in Pedal";
            calculation.btn_SendConfig_tooltip= "Push Config into Pedal storage";
            CalculationChangedEvent(calculation);
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_LivePreview_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.LivePreview[Settings.table_selected] = false;
            /*
            btn_SendConfig.Content = "Save Config in Pedal";
            btn_SendConfig.ToolTip = "Push Config into Pedal storage";
            */
            calculation.btn_SendConfig_Content = "Send Config to Pedal";
            calculation.btn_SendConfig_tooltip = "Send Config to Pedal and save in storage";
            //calculation.IsUIRefreshNeeded = true;
            CalculationChangedEvent(calculation);
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_Pedal_ESPNow_SyncFlag_Checked(object sender, RoutedEventArgs e)
        {
            Settings.Pedal_ESPNow_Sync_flag[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_Pedal_ESPNow_SyncFlag_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.Pedal_ESPNow_Sync_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_USINGESP32S3_Checked(object sender, RoutedEventArgs e)
        {
            Settings.USING_ESP32S3[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void CheckBox_USINGESP32S3_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.USING_ESP32S3[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void dump_pedal_response_to_file_Checked(object sender, RoutedEventArgs e)
        {
            calculation.dumpPedalToResponseFile_clearFile[Settings.table_selected] = true;
            calculation.dumpPedalToResponseFile[Settings.table_selected] = true;
            CalculationChangedEvent(calculation);
        }

        private void dump_pedal_response_to_file_Unchecked(object sender, RoutedEventArgs e)
        {
            
            calculation.dumpPedalToResponseFile[Settings.table_selected] = false;
            CalculationChangedEvent(calculation);
        }

        private void Vjoy_out_check_Checked(object sender, RoutedEventArgs e)
        {
            Settings.vjoy_output_flag = 1;
            uint vJoystickId = Settings.vjoy_order;
            _joystick.AcquireVJD(vJoystickId);            
            vjoy_axis_initialize();

        }

        private void Vjoy_out_check_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.vjoy_output_flag = 0;
            _joystick.RelinquishVJD(Settings.vjoy_order);
        }

        private void btn_plus_Click(object sender, RoutedEventArgs e)
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
                        Vjoy_out_check.IsChecked = false;
                        break;
                    case VjdStat.VJD_STAT_FREE:

                        //TextBox_debugOutput.Text = "vjoy aquaried";
                        //joystick = new VirtualJoystick(Plugin.Settings.vjoy_order);
                        //joystick.Aquire();
                        _joystick.AcquireVJD(Settings.vjoy_order);
                        if (Vjoy_out_check.IsChecked == false)
                        {
                            Vjoy_out_check.IsChecked = true;
                        }
                        //Console.WriteLine("vJoy Device {0} is free\n", id);
                        break;
                    case VjdStat.VJD_STAT_BUSY:
                        //TextBox_debugOutput.Text = "vjoy was aquaried by other program";
                        System.Windows.MessageBox.Show("vjoy was aquaried by other program", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} is already owned by another feeder\nCannot continue\n", id);
                        return;
                    case VjdStat.VJD_STAT_MISS:
                        //TextBox_debugOutput.Text = "the selected vjoy device not enabled";
                        System.Windows.MessageBox.Show("the selected vjoy device not enabled", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} is not installed or disabled\nCannot continue\n", id);
                        return;
                    default:
                        //TextBox_debugOutput.Text = "vjoy device error";
                        System.Windows.MessageBox.Show("vjoy device error", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} general error\nCannot continue\n", id);
                        return;
                };
            }
        }

        private void btn_minus_Click(object sender, RoutedEventArgs e)
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
                        Vjoy_out_check.IsChecked = false;
                        break;
                    case VjdStat.VJD_STAT_FREE:

                        //TextBox_debugOutput.Text = "vjoy aquaried";
                        //joystick = new VirtualJoystick(Plugin.Settings.vjoy_order);
                        //joystick.Aquire();
                        _joystick.AcquireVJD(Settings.vjoy_order);
                        if (Vjoy_out_check.IsChecked == false)
                        {
                            Vjoy_out_check.IsChecked = true;
                        }
                        //Console.WriteLine("vJoy Device {0} is free\n", id);
                        break;
                    case VjdStat.VJD_STAT_BUSY:
                        //TextBox_debugOutput.Text = "vjoy was aquaried by other program";
                        System.Windows.MessageBox.Show("vjoy was aquaried by other program", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} is already owned by another feeder\nCannot continue\n", id);
                        return;
                    case VjdStat.VJD_STAT_MISS:
                        //TextBox_debugOutput.Text = "the selected vjoy device not enabled";
                        System.Windows.MessageBox.Show("the selected vjoy device not enabled", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} is not installed or disabled\nCannot continue\n", id);
                        return;
                    default:
                        //TextBox_debugOutput.Text = "vjoy device error";
                        System.Windows.MessageBox.Show("vjoy device error", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                        Settings.vjoy_output_flag = 0;
                        Vjoy_out_check.IsChecked = false;
                        //Console.WriteLine("vJoy Device {0} general error\nCannot continue\n", id);
                        return;
                };
            }
        }
    }
}
