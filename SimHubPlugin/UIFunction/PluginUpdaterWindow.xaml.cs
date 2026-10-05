using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net.Http;
using System.Threading.Tasks;
using System.Windows;
using Newtonsoft.Json.Linq;

namespace DiyFfbPedal.UIFunction
{
    public partial class PluginUpdaterWindow : Window
    {
        private readonly DIY_FFB_Pedal _plugin;

        public PluginUpdaterWindow(DIY_FFB_Pedal plugin)
        {
            InitializeComponent();
            _plugin = plugin;
        }

        private async void Window_Loaded(object sender, RoutedEventArgs e)
        {
            MessageBox.Show("This feature is for testing only. If you need to report an issue, please update to the latest version before reporting.", "Warning", MessageBoxButton.OK, MessageBoxImage.Warning);
            await LoadReleasesAsync();
        }

        private async Task LoadReleasesAsync()
        {
            TxtLog.AppendText("Fetching releases from GitHub...\n");
            try
            {
                using (HttpClient client = new HttpClient())
                {
                    client.DefaultRequestHeaders.Add("User-Agent", "SimHub-DIY-Pedal-Updater");
                    
                    var response = await client.GetAsync("https://api.github.com/repos/ChrGri/DIY-Sim-Racing-FFB-Pedal/releases");
                    response.EnsureSuccessStatusCode();
                    
                    string json = await response.Content.ReadAsStringAsync();
                    JArray releases = JArray.Parse(json);
                    
                    var availableVersions = new Dictionary<string, string>();
                    Version minVersion = new Version("26.23.06");

                    foreach (var release in releases)
                    {
                        string tagName = release["tag_name"]?.ToString() ?? "";
                        string cleanVersion = tagName.Replace("Build_", "").Replace("V", "").Replace("v", "");
                        
                        if (Version.TryParse(cleanVersion, out Version v))
                        {
                            // Old tags were in format YYYYMMDD.HHMMSS, so v.Major would be 20240603
                            // New tags are YY.WW.DD, so v.Major will be < 100
                            if (v.Major < 100 && v >= minVersion)
                            {
                                // Find the first asset download url (plugin zip)
                                var assets = release["assets"] as JArray;
                                if (assets != null && assets.Count > 0)
                                {
                                    // The plugin is a dll file
                                    var dllAsset = assets.FirstOrDefault(a => a["name"]?.ToString().EndsWith(".dll", StringComparison.OrdinalIgnoreCase) == true);
                                    if (dllAsset != null)
                                    {
                                        string downloadUrl = dllAsset["browser_download_url"]?.ToString();
                                        availableVersions.Add(tagName, downloadUrl);
                                    }
                                }
                            }
                        }
                    }

                    if (availableVersions.Count == 0)
                    {
                        TxtLog.AppendText("No matching versions found (>= 26.23.06).\n");
                    }
                    else
                    {
                        CboVersions.ItemsSource = availableVersions;
                        CboVersions.SelectedIndex = 0;
                        TxtLog.AppendText($"Found {availableVersions.Count} versions.\n");
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Error fetching releases: {ex.Message}\n");
            }
        }

        private async void BtnUpdate_Click(object sender, RoutedEventArgs e)
        {
            if (CboVersions.SelectedItem == null)
            {
                MessageBox.Show("Please select a version to update.");
                return;
            }

            var selected = (KeyValuePair<string, string>)CboVersions.SelectedItem;
            BtnUpdate.IsEnabled = false;
            TxtLog.AppendText($"Downloading {selected.Key}...\n");
            try
            {
                await PluginUpdateHelper.DownloadAsync(selected.Value, null);
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Download failed: {ex.Message}\n");
                BtnUpdate.IsEnabled = true;
                return;
            }

            TxtLog.AppendText("Download finished. SimHub will now restart to install it.\n");
            MessageBox.Show("The update was downloaded. SimHub will now restart to install it.", "Plugin update", MessageBoxButton.OK, MessageBoxImage.Information);
            try
            {
                PluginUpdateHelper.InstallAndRestart();
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Error launching update: {ex.Message}\n");
                BtnUpdate.IsEnabled = true;
            }
        }
    }
}
