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

namespace DiyFfbPedal.UIFunction
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
            if (_calculations != null)
            {
                if (_calculations.ForceUpdate_b == true && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = true;
                if (_calculations.ForceUpdate_b == false && Checkbox_Force_flash != null) Checkbox_Force_flash.IsChecked = false;
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
                    client.DefaultRequestHeaders.Add("User-Agent", "SimHub-Plugin");
                    string json = await client.GetStringAsync("https://api.github.com/repos/ChrGri/DIY-Sim-Racing-FFB-Pedal/releases/latest");
                    JObject obj = JObject.Parse(json);
                    
                    string tagName = (string)obj["tag_name"];
                    string body = (string)obj["body"];
                    string cleanedVersion = System.Text.RegularExpressions.Regex.Match(tagName ?? "", @"\d+(\.\d+)+").Value;
                    if (string.IsNullOrEmpty(cleanedVersion)) cleanedVersion = "0.0.0.0";

                    for (int i = 0; i < channels.Length; i++)
                    {
                        versions[i] = cleanedVersion;
                        changelogs[i] = body;
                    }
                    if (textBox_changelog != null) textBox_changelog.Text = "Version:" + versions[0] + "\n" + changelogs[0];

                }
                
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error:{ex.Message}");
            }
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
