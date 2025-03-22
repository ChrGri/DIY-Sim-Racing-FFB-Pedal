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
    /// SystemSetting_OTA.xaml 的互動邏輯
    /// </summary>
    public partial class SystemSetting_OTA : UserControl
    {
        public SystemSetting_OTA()
        {
            InitializeComponent();
        }


        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(SystemSetting_OTA),
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
            typeof(SystemSetting_OTA),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Cauculation_Property);
            set
            {
                SetValue(Cauculation_Property, value);
                updateUI();
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
                    if(textbox_SSID!=null) textbox_SSID.Text = Settings.SSID_string;
                    if(textbox_PASS!=null) textbox_PASS.Password = Settings.PASS_string;
                }
                if (calculation != null)
                {
                    if (calculation.UpdateChannel == 0 && OTAChannel_Sel_1 != null) OTAChannel_Sel_1.IsChecked = true;
                    if (calculation.UpdateChannel == 1 && OTAChannel_Sel_2 != null) OTAChannel_Sel_2.IsChecked = true;
                    if (calculation.ForceUpdate_b == true && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = true;
                    if (calculation.ForceUpdate_b == false && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = false;
                }
            }
            catch
            {
            }
        }

        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SystemSetting_OTA;
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


        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue)
        {
            SettingsChanged?.Invoke(this, newValue);
        }

        private void textbox_SSID_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (textbox_SSID.Text.Length > 30)
            {
                if (Label_SSID != null) Label_SSID.Content = "Error! SSID length >30";
            }
            else
            {
                Settings.SSID_string = textbox_SSID.Text;
                if(Label_SSID!=null) Label_SSID.Content = "";
                SettingsChangedEvent(Settings);
            }
        }

        private void textbox_PASS_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (textbox_PASS.Password.Length > 30)
            {
                if (Label_PASS != null) Label_PASS.Content = "Error! Password length >30";
            }
            else
            {
                Settings.PASS_string = textbox_PASS.Password;
                if (Label_PASS != null) Label_PASS.Content = "";
                SettingsChangedEvent(Settings);
            }
        }

        private void OTAChannel_Sel_Checked(object sender, RoutedEventArgs e)
        {
            if (OTAChannel_Sel_1 != null && OTAChannel_Sel_2 != null)
            {
                if ((bool)OTAChannel_Sel_1.IsChecked) calculation.UpdateChannel = 0;
                if ((bool)OTAChannel_Sel_2.IsChecked) calculation.UpdateChannel = 1;
                CalculationChangedEvent(calculation);
            }

        }

        private void Checkbox_Force_flash_Checked(object sender, RoutedEventArgs e)
        {
            calculation.ForceUpdate_b = true;
            CalculationChangedEvent(calculation);
        }

        private void Checkbox_Force_flash_Unchecked(object sender, RoutedEventArgs e)
        {
            calculation.ForceUpdate_b = false;
            CalculationChangedEvent(calculation);
        }
    }
}
