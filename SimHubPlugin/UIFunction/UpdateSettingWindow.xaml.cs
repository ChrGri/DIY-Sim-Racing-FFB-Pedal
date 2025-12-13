using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Http;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// UpdateSettingWindow.xaml 的互動邏輯
    /// </summary>
    public partial class UpdateSettingWindow : Window
    {
        public DIYFFBPedalSettings _settings;
        public CalculationVariables _calculations;
        public static readonly string[] channels = new[] { "main", "dev-build", "daily-build" };
        public static string[] versions = new string[channels.Length];
        public static string[] changelogs = new string[channels.Length];
        public UpdateSettingWindow(DIYFFBPedalSettings settings, CalculationVariables calculations)
        {
            InitializeComponent();
            _settings = settings;
            _calculations = calculations;
            if (_calculations != null && OTAChannel_Sel_1 != null && OTAChannel_Sel_2 != null )
            {
                if (Label_update_channel_notice != null) Label_update_channel_notice.Content = "";
                switch (_settings.updateChannel)
                {
                    case 0:
                        OTAChannel_Sel_1.IsChecked = true;
                        break;
                    case 1:
                        OTAChannel_Sel_2.IsChecked = true;
                        Label_update_channel_notice.Content = "Warning: This is a Dev build intended for development and testing purposes only.\nIt may be unstable and is not recommended for production use.";
                        break;
                    default:
                        OTAChannel_Sel_1.IsChecked=true;
                        break;
                }
                if (_calculations.ForceUpdate_b == true && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = true;
                if (_calculations.ForceUpdate_b == false && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = false;
                if (_calculations.IsTestBuild == true && Checkbox_Force_flash != null) Checkbox_TestBuild.IsChecked = true;
                if (_calculations.IsTestBuild == false && Checkbox_Force_flash != null) Checkbox_TestBuild.IsChecked = false ;
            }
            if (_settings != null)
            {
                if (textbox_SSID != null) textbox_SSID.Text = _settings.SSID_string;
                if (textbox_PASS != null) textbox_PASS.Password = _settings.PASS_string;
            }
            CheckForUpdateAsync();

        }

        private void textbox_PASS_PasswordChanged(object sender, RoutedEventArgs e)
        {
            if (textbox_PASS.Password.Length > 64)
            {
                if (Label_PASS != null) Label_PASS.Content = "Error! Password length >64";
            }
            else
            {
                _settings.PASS_string = textbox_PASS.Password;
                if (Label_PASS != null) Label_PASS.Content = "";
            }
        }

        private void textbox_SSID_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (textbox_SSID.Text.Length > 64)
            {
                if (Label_SSID != null) Label_SSID.Content = "Error! SSID length >64";
            }
            else
            {
                _settings.SSID_string = textbox_SSID.Text;
                if (Label_SSID != null) Label_SSID.Content = "";
            }
        }

        private void OTAChannel_Sel_Checked(object sender, RoutedEventArgs e)
        {
            if (OTAChannel_Sel_1 != null && OTAChannel_Sel_2 != null )
            {
                Label_update_channel_notice.Content = "";
                if ((bool)OTAChannel_Sel_1.IsChecked) _settings.updateChannel = 0;
                if ((bool)OTAChannel_Sel_2.IsChecked)
                {
                    _settings.updateChannel = 1;
                    Label_update_channel_notice.Content = "Warning: This is a Dev build intended for development and testing purposes only.\nIt may be unstable and is not recommended for production use.";
                } 
                 
                textBox_changelog.Text = "Version:" + versions[_settings.updateChannel] + "\n" + changelogs[_settings.updateChannel];

            }
        }

        private void Checkbox_Force_flash_Checked(object sender, RoutedEventArgs e)
        {
            _calculations.ForceUpdate_b = true;
        }

        private void Checkbox_Force_flash_Unchecked(object sender, RoutedEventArgs e)
        {
            _calculations.ForceUpdate_b = false;
        }

        private void Btn_Apply_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void Btn_Leave_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
        public async void CheckForUpdateAsync()
        {
            try
            {
                using (var client = new HttpClient())
                {
                    string json = await client.GetStringAsync(Constants.version_control_url);
                    JObject obj = JObject.Parse(json);
                    var results = new List<string>();

                    for (int i = 0; i < channels.Length; i++)
                    {
                        string channel = channels[i];
                        if (obj.ContainsKey(channel))
                        {
                            versions[i] = (string)obj[channel]["version"];
                            changelogs[i] = (string)obj[channel]["changelog"];
                        }
                        else
                        {
                            versions[i] = "N/A";
                            changelogs[i] = "Channel not found.";
                        }
                    }
                    if (textBox_changelog != null) textBox_changelog.Text = "Version:" + versions[_settings.updateChannel] + "\n" + changelogs[_settings.updateChannel];

                }
                
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error:{ex.Message}");
            }
        }

        private void Checkbox_TestBuild_Checked(object sender, RoutedEventArgs e)
        {
            _calculations.IsTestBuild = true;
        }

        private void Checkbox_TestBuild_Unchecked(object sender, RoutedEventArgs e)
        {
            _calculations.IsTestBuild = false;
        }

        private void Checkbox_platformIo_upload_Checked(object sender, RoutedEventArgs e)
        {
            _calculations.IsOtaUploadFromPlatformIO = true;
        }

        private void Checkbox_platformIo_upload_Unchecked(object sender, RoutedEventArgs e)
        {
            _calculations.IsOtaUploadFromPlatformIO = false;
        }
    }
}
