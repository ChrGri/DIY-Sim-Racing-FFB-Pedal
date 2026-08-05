using System;
using System.Collections.Generic;
using System.IO;
using System.Net.Http;
using System.Net.Http.Headers;
using Newtonsoft.Json;
using System.Threading.Tasks;
using System.Windows;

namespace DiyFfbPedal.UIElement
{
    public partial class LanguageDownloadWindow : Window
    {
        private const string GITHUB_API_URL = "https://api.github.com/repos/tcfshcrw/DIYFFBPedalPluginLocalization/contents/language";
        private const string RESX_DOWNLOAD_URL = "https://raw.githubusercontent.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/develop/SimHubPlugin/Language/DiyFfbPedal.resx";
        
        // Downloads to the "languages" folder inside the SimHub root directory
        private readonly string _languageFolderPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "languages");

        public LanguageDownloadWindow()
        {
            InitializeComponent();
            _ = LoadLanguagesAsync();
        }

        private async void Btn_Refresh_Click(object sender, RoutedEventArgs e)
        {
            Btn_Refresh.IsEnabled = false;
            await LoadLanguagesAsync();
            Btn_Refresh.IsEnabled = true;
        }

        private async Task LoadLanguagesAsync()
        {
            try
            {
                using (var client = new HttpClient())
                {
                    client.DefaultRequestHeaders.UserAgent.Add(new ProductInfoHeaderValue("DIYFFBPedal", "1.0"));
                    var response = await client.GetStringAsync(GITHUB_API_URL);
                    
                    var files = JsonConvert.DeserializeObject<List<GithubFile>>(response);
                    
                    try
                    {
                        var contributorsResponse = await client.GetStringAsync($"https://raw.githubusercontent.com/tcfshcrw/DIYFFBPedalPluginLocalization/main/contributors.json?t={DateTime.Now.Ticks}");
                        var contributors = JsonConvert.DeserializeObject<List<Contributor>>(contributorsResponse);
                        if (files != null && contributors != null)
                        {
                            foreach (var file in files)
                            {
                                var contributor = contributors.Find(c => c.Filename == file.Name);
                                if (contributor != null)
                                {
                                    file.Author = contributor.Author;
                                }
                            }
                        }
                    }
                    catch
                    {
                        // Ignore contributor fetch error
                    }
                    
                    if (files != null)
                    {
                        Listbox_Languages.ItemsSource = files;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to fetch languages: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private async void Btn_Download_Click(object sender, RoutedEventArgs e)
        {
            if (Listbox_Languages.SelectedItem is GithubFile selectedFile)
            {
                Btn_Download.IsEnabled = false;
                try
                {
                    using (var client = new HttpClient())
                    {
                        var fileBytes = await client.GetByteArrayAsync(selectedFile.DownloadUrl);
                        var resxBytes = await client.GetByteArrayAsync(RESX_DOWNLOAD_URL);
                        
                        if (!Directory.Exists(_languageFolderPath))
                        {
                            Directory.CreateDirectory(_languageFolderPath);
                        }
                        
                        var tempPath = Path.Combine(Path.GetTempPath(), selectedFile.Name);
                        File.WriteAllBytes(tempPath, fileBytes);

                        var tempResxPath = Path.Combine(Path.GetTempPath(), "DiyFfbPedal.resx");
                        File.WriteAllBytes(tempResxPath, resxBytes);
                        
                        var result = MessageBox.Show($"Successfully downloaded {selectedFile.Name} and updated base resx file!\nDo you want to restart SimHub now to apply the update?", "Success", MessageBoxButton.YesNo, MessageBoxImage.Information);
                        
                        if (result == MessageBoxResult.Yes)
                        {
                            var finalPath = Path.Combine(_languageFolderPath, selectedFile.Name);
                            var finalResxPath = Path.Combine(_languageFolderPath, "DiyFfbPedal.resx");
                            RestartSimHub(tempPath, finalPath, tempResxPath, finalResxPath);
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show($"Failed to download {selectedFile.Name}: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
                finally
                {
                    Btn_Download.IsEnabled = true;
                }
            }
            else
            {
                MessageBox.Show("Please select a language to download.", "Warning", MessageBoxButton.OK, MessageBoxImage.Warning);
            }
        }

        private void RestartSimHub(string tempFilePath, string finalFilePath, string tempResxPath, string finalResxPath)
        {
            string exePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SimHubWPF.exe");
            
            string psScript = $@"
                $processName = 'SimHubWPF'
                $exePath = '{exePath}'
                $tempFile = '{tempFilePath}'
                $finalFile = '{finalFilePath}'
                $tempResx = '{tempResxPath}'
                $finalResx = '{finalResxPath}'

                Write-Host 'Closing Simhub...'
                $procs = Get-Process -Name $processName -ErrorAction SilentlyContinue
                foreach ($proc in $procs) {{
                    Stop-Process -Id $proc.Id -Force
                    $proc.WaitForExit()
                }}
                Start-Sleep -Seconds 1

                Write-Host 'Applying Language Update...'
                if (Test-Path $tempFile) {{
                    $targetDir = Split-Path -Path $finalFile
                    if (!(Test-Path $targetDir)) {{
                        New-Item -ItemType Directory -Path $targetDir -Force
                    }}
                    Copy-Item -Path $tempFile -Destination $finalFile -Force
                }}

                if (Test-Path $tempResx) {{
                    Copy-Item -Path $tempResx -Destination $finalResx -Force
                }}

                Write-Host 'Restarting Simhub...'
                if (Test-Path $exePath) {{
                    Start-Process -FilePath $exePath
                }}
            ";

            string escapedScript = psScript.Replace("\"", "`\"").Replace("`r", "").Replace("`n", "; ");
            
            var psi = new System.Diagnostics.ProcessStartInfo
            {
                FileName = "powershell.exe",
                Arguments = $"-NoProfile -ExecutionPolicy Bypass -Command \"{escapedScript}\"",
                Verb = "runas", // run as admin to ensure process can be killed
                UseShellExecute = true
            };

            try
            {
                System.Diagnostics.Process.Start(psi);
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Failed to restart SimHub: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void Btn_Close_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }

    public class GithubFile
    {
        [JsonProperty("name")]
        public string Name { get; set; }

        [JsonProperty("download_url")]
        public string DownloadUrl { get; set; }

        public string Author { get; set; }

        public string DisplayText => string.IsNullOrEmpty(Author) ? Name : $"{Name} (by {Author})";
    }

    public class Contributor
    {
        [JsonProperty("language_code")]
        public string LanguageCode { get; set; }
        
        [JsonProperty("filename")]
        public string Filename { get; set; }
        
        [JsonProperty("author")]
        public string Author { get; set; }
    }
}
