using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Linq;
using Microsoft.Win32;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Text;

namespace DiyFfbPedal.UIFunction
{
    public partial class OtaFlasherWindow : Window
    {
        private readonly DIY_FFB_Pedal _plugin;

        public OtaFlasherWindow(DIY_FFB_Pedal plugin)
        {
            InitializeComponent();
            _plugin = plugin;

            // Load the saved WiFi data from the plugin settings into the new text boxes
            if (_plugin.Settings != null)
            {
                TxtSSID.Text = _plugin.Settings.SSID_string;
                TxtPASS.Text = _plugin.Settings.PASS_string;
            }

            PopulateFirmwareDropdown();
        }

        private void PopulateFirmwareDropdown()
        {
            var firmwareOptions = new Dictionary<string, string>();
            firmwareOptions.Add("Custom Local Firmware...", "CUSTOM_LOCAL");

            string manifestResourceName = "DiyFfbPedal.Resources.Firmware.manifest.txt";
            var assembly = Assembly.GetExecutingAssembly();

            try
            {
                using (Stream stream = assembly.GetManifestResourceStream(manifestResourceName))
                {
                    if (stream != null)
                    {
                        using (StreamReader reader = new StreamReader(stream))
                        {
                            string line;
                            while ((line = reader.ReadLine()) != null)
                            {
                                line = line.Trim();
                                if (!string.IsNullOrEmpty(line))
                                {
                                    string displayName = line.Replace("_", " ");
                                    firmwareOptions.Add(displayName, line);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Error reading manifest: {ex.Message}\n");
            }

            CboFirmware.ItemsSource = firmwareOptions;
            if (CboFirmware.Items.Count > 1) CboFirmware.SelectedIndex = 1;
            else if (CboFirmware.Items.Count > 0) CboFirmware.SelectedIndex = 0;
        }

        private void CboFirmware_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (PnlCustomFile == null) return;
            PnlCustomFile.Visibility = (CboFirmware.SelectedValue != null && CboFirmware.SelectedValue.ToString() == "CUSTOM_LOCAL")
                                       ? Visibility.Visible : Visibility.Collapsed; 
        }

        private void BtnBrowse_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog
            {
                Filter = "Firmware File (firmware.bin)|firmware.bin|All Binary Files (*.bin)|*.bin",
                Title = "Select firmware.bin for OTA"
            };

            if (ofd.ShowDialog() == true) TxtBinPath.Text = ofd.FileName;
        }

        private string ExtractFirmwareResource(string boardFolder, string fileName)
        {
            string resourceName = $"DiyFfbPedal.Resources.Firmware.{boardFolder}.{fileName}";
            string tempDir = Path.Combine(Path.GetTempPath(), "SimHub_DIY_Pedal_OTA_Flasher", boardFolder);
            Directory.CreateDirectory(tempDir);
            string outPath = Path.Combine(tempDir, fileName);

            var assembly = Assembly.GetExecutingAssembly();
            using (Stream stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream == null) throw new FileNotFoundException($"Resource {resourceName} not found.");
                using (FileStream fs = new FileStream(outPath, FileMode.Create, FileAccess.Write))
                {
                    stream.CopyTo(fs);
                }
            }
            return outPath;
        }

        private string ExtractEspota()
        {
            string resourceName = "DiyFfbPedal.Resources.espota.exe";
            string outPath = Path.Combine(Path.GetTempPath(), "espota_simhub.exe");

            if (!File.Exists(outPath))
            {
                var assembly = Assembly.GetExecutingAssembly();
                using (Stream stream = assembly.GetManifestResourceStream(resourceName))
                {
                    if (stream == null) throw new FileNotFoundException("espota.exe not found in resources!");
                    using (FileStream fs = new FileStream(outPath, FileMode.Create, FileAccess.Write))
                    {
                        stream.CopyTo(fs);
                    }
                }
            }
            return outPath;
        }

        private async void BtnFlash_Click(object sender, RoutedEventArgs e)
        {
            if (CboFirmware.SelectedItem == null) return;

            string selectedBoardFolder = CboFirmware.SelectedValue.ToString();
            string targetHostname = TxtHostname.Text;
            string SSID = TxtSSID.Text;
            string PASS = TxtPASS.Text;

            if (selectedBoardFolder == "CUSTOM_LOCAL" && string.IsNullOrWhiteSpace(TxtBinPath.Text))
            {
                MessageBox.Show("Please select a local firmware.bin.");
                return;
            }

            if (SSID.Length > 64 || PASS.Length > 64)
            {
                MessageBox.Show("ERROR! SSID or password must be a maximum of 64 characters long.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // Save WiFi data in settings
            if (_plugin.Settings != null)
            {
                _plugin.Settings.SSID_string = SSID;
                _plugin.Settings.PASS_string = PASS;
            }

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();

            try
            {
                TxtLog.AppendText("Sending OTA Wake-Up command incl. WiFi data to the pedal...\n");

                byte pedalId = (byte)(_plugin.Settings?.table_selected ?? 0);

                // We encapsulate the memory-critical part in an unsafe block
                unsafe
                {
                    DAP_action_ota_st tmp_2 = default;

                    tmp_2.payloadOtaInfo_.ota_action = (byte)otaAction.OTA_ACTION_UPLOAD_FROM_PLATFORMIO;

                    if (_plugin._calculations != null)
                    {
                        if (_plugin._calculations.ForceUpdate_b)
                            tmp_2.payloadOtaInfo_.ota_action = (byte)2; // OTA_ACTION_FORCE_UPDATE
                        if (_plugin._calculations.IsOtaUploadFromPlatformIO)
                            tmp_2.payloadOtaInfo_.ota_action = (byte)3; // OTA_ACTION_UPLOAD_FROM_PLATFORMIO
                    }

                    tmp_2.payloadOtaInfo_.mode_select = 1;
                    tmp_2.payloadOtaInfo_.SSID_Length = (byte)SSID.Length;
                    tmp_2.payloadOtaInfo_.PASS_Length = (byte)PASS.Length;
                    tmp_2.payloadOtaInfo_.device_ID = pedalId;

                    // Set header and footer from project constants
                    tmp_2.payloadHeader_.payloadType = (byte)Constants.OtaPayloadType;
                    tmp_2.payloadHeader_.startOfFrame0_u8 = _plugin.STARTOFFRAMCHAR[0];
                    tmp_2.payloadHeader_.startOfFrame1_u8 = _plugin.STARTOFFRAMCHAR[1];
                    tmp_2.payloadFooter_.enfOfFrame0_u8 = _plugin.ENDOFFRAMCHAR[0];
                    tmp_2.payloadFooter_.enfOfFrame1_u8 = _plugin.ENDOFFRAMCHAR[1];

                    byte[] array_ssid = Encoding.ASCII.GetBytes(SSID);
                    for (int i = 0; i < SSID.Length; i++)
                    {
                        tmp_2.payloadOtaInfo_.WIFI_SSID[i] = array_ssid[i];
                    }

                    byte[] array_pass = Encoding.ASCII.GetBytes(PASS);
                    for (int i = 0; i < PASS.Length; i++)
                    {
                        tmp_2.payloadOtaInfo_.WIFI_PASS[i] = array_pass[i];
                    }

                    _plugin.SendOTAActionPedal(tmp_2, pedalId);
                }

                // 2. Give the ESP time to stop the motor, connect to WiFi and start ArduinoOTA
                TxtLog.AppendText("Waiting for WiFi connection and initialization of OTA mode on ESP32 (5 seconds)...\n");
                await Task.Delay(5000);

                // 3. Resolve file (firmware.bin only!)
                string firmwarePath;
                if (selectedBoardFolder == "CUSTOM_LOCAL")
                {
                    firmwarePath = TxtBinPath.Text;
                }
                else
                {
                    TxtLog.AppendText($"Extracting {selectedBoardFolder} firmware.bin...\n");
                    firmwarePath = ExtractFirmwareResource(selectedBoardFolder, "firmware.bin");
                }

                // 4. Extract espota.exe
                string espotaPath = ExtractEspota();

                // 5. Start OTA process
                TxtLog.AppendText($"Starting upload via WiFi to {targetHostname}...\n");

                // Safely resolve hostname to an IP address before flashing
                string resolvedIp = targetHostname;

                if (targetHostname.EndsWith(".local", StringComparison.OrdinalIgnoreCase))
                {
                    try
                    {
                        // Native mDNS resolution via UDP Multicast (C# Implementation)
                        TxtLog.AppendText($"Sending mDNS request to 224.0.0.251:5353 for {targetHostname}...\n");
                        string foundIp = await ResolveMdnsIpv4Async(targetHostname);

                        if (!string.IsNullOrEmpty(foundIp))
                        {
                            resolvedIp = foundIp;
                            TxtLog.AppendText($"Successfully resolved to IP: {resolvedIp}\n");
                        }
                        else
                        {
                            TxtLog.AppendText("Warning: Could not resolve .local name. ESP32 not found on network.\n");
                        }
                    }
                    catch (Exception ex)
                    {
                        TxtLog.AppendText($"Warning: mDNS resolution failed ({ex.Message}).\n");
                    }
                }

                // espota.exe now obligatorily gets the resolved IP address (resolvedIp) served!
                string args = $"-i {resolvedIp} -p 3232 -f \"{firmwarePath}\"";

                ProcessStartInfo psi = new ProcessStartInfo
                {
                    FileName = espotaPath,
                    Arguments = args,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true
                };

                using (var process = new Process { StartInfo = psi })
                {
                    process.OutputDataReceived += (s, ev) => {
                        if (ev.Data != null) Dispatcher.Invoke(() => { TxtLog.AppendText(ev.Data + "\n"); TxtLog.ScrollToEnd(); });
                    };
                    // espota.exe writes status messages and progress (Uploading...) to StandardError (stderr).
                    // Therefore we must NOT blindly prepend "ERROR: " here.
                    process.ErrorDataReceived += (s, ev) => {
                        if (ev.Data != null) Dispatcher.Invoke(() => { TxtLog.AppendText(ev.Data + "\n"); TxtLog.ScrollToEnd(); });
                    };

                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await Task.Run(() => process.WaitForExit());

                    if (process.ExitCode == 0)
                    {
                        MessageBox.Show("WiFi Update successful!", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
                    }
                    else
                    {
                        MessageBox.Show("OTA Update failed. See logs.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"\nEXCEPTION: {ex.Message}\n");
            }
            finally
            {
                BtnFlash.IsEnabled = true;
            }
        }

        private async Task<string> ResolveMdnsIpv4Async(string hostname)
        {
            try
            {
                // 1. Try native DNS resolution first
                try {
                    var hostEntry = await System.Net.Dns.GetHostEntryAsync(hostname);
                    var ip = hostEntry.AddressList.FirstOrDefault(a => a.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork);
                    if (ip != null) return ip.ToString();
                } catch { }

                // 2. Fallback: Robust mDNS query across all network interfaces
                List<byte> query = new List<byte>();
                query.AddRange(new byte[] { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });
                foreach (string part in hostname.Split('.'))
                {
                    query.Add((byte)part.Length);
                    query.AddRange(Encoding.ASCII.GetBytes(part));
                }
                query.Add(0x00);
                query.AddRange(new byte[] { 0x00, 0x01, 0x00, 0x01 });
                byte[] req = query.ToArray();
                var mcastEp = new System.Net.IPEndPoint(System.Net.IPAddress.Parse("224.0.0.251"), 5353);

                var udpClients = new List<System.Net.Sockets.UdpClient>();
                var receiveTasks = new List<Task<System.Net.Sockets.UdpReceiveResult>>();

                // Bind to all available IPv4 interfaces to avoid VirtualBox/VPN adapter issues
                var interfaces = System.Net.NetworkInformation.NetworkInterface.GetAllNetworkInterfaces()
                    .Where(n => n.OperationalStatus == System.Net.NetworkInformation.OperationalStatus.Up && 
                                n.NetworkInterfaceType != System.Net.NetworkInformation.NetworkInterfaceType.Loopback);

                foreach (var iface in interfaces)
                {
                    var props = iface.GetIPProperties();
                    var ipv4Props = props.GetIPv4Properties();
                    if (ipv4Props == null) continue;

                    foreach (var ip in props.UnicastAddresses)
                    {
                        if (ip.Address.AddressFamily == System.Net.Sockets.AddressFamily.InterNetwork)
                        {
                            try {
                                var udp = new System.Net.Sockets.UdpClient();
                                udp.Client.Bind(new System.Net.IPEndPoint(ip.Address, 0));
                                udp.JoinMulticastGroup(System.Net.IPAddress.Parse("224.0.0.251"), ip.Address);
                                udpClients.Add(udp);
                            } catch { }
                        }
                    }
                }

                if (udpClients.Count == 0)
                {
                    // Fallback in case interfaces could not be read
                    var udp = new System.Net.Sockets.UdpClient();
                    udp.Client.Bind(new System.Net.IPEndPoint(System.Net.IPAddress.Any, 0));
                    udpClients.Add(udp);
                }

                DateTime start = DateTime.Now;
                // Wait up to 15 seconds for the ESP32 (often takes longer to connect to WiFi)
                while ((DateTime.Now - start).TotalMilliseconds < 15000)
                {
                    receiveTasks.Clear();
                    foreach (var udp in udpClients)
                    {
                        try {
                            udp.Send(req, req.Length, mcastEp);
                            receiveTasks.Add(udp.ReceiveAsync());
                        } catch { }
                    }

                    if (receiveTasks.Count > 0)
                    {
                        var completedTask = await Task.WhenAny(Task.WhenAny(receiveTasks), Task.Delay(2000));
                        if (completedTask is Task<Task<System.Net.Sockets.UdpReceiveResult>> wrappedTask)
                        {
                            var result = await wrappedTask.Result;
                            byte[] res = result.Buffer;
                            for (int i = 0; i < res.Length - 13; i++)
                            {
                                if (res[i] == 0x00 && res[i + 1] == 0x01 && 
                                    (res[i + 2] == 0x00 || res[i + 2] == 0x80) && res[i + 3] == 0x01 &&
                                    res[i + 8] == 0x00 && res[i + 9] == 0x04)
                                {
                                    foreach (var c in udpClients) { try { c.Close(); } catch { } }
                                    return $"{res[i + 10]}.{res[i + 11]}.{res[i + 12]}.{res[i + 13]}";
                                }
                            }
                        }
                    }
                }
                foreach (var c in udpClients) { try { c.Close(); } catch { } }
            }
            catch { }
            return null;
        }
    }
}