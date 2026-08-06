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

            // Die gespeicherten WLAN-Daten aus den Plugin-Settings in die neuen Textboxen laden
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
                TxtLog.AppendText($"Fehler beim Lesen des Manifests: {ex.Message}\n");
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
                if (stream == null) throw new FileNotFoundException($"Resource {resourceName} nicht gefunden.");
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
                    if (stream == null) throw new FileNotFoundException("espota.exe nicht in den Ressourcen gefunden!");
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
                MessageBox.Show("Bitte wähle eine lokale firmware.bin aus.");
                return;
            }

            if (SSID.Length > 64 || PASS.Length > 64)
            {
                MessageBox.Show("ERROR! SSID oder Passwort dürfen maximal 64 Zeichen lang sein.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            // WLAN-Daten in den Einstellungen speichern
            if (_plugin.Settings != null)
            {
                _plugin.Settings.SSID_string = SSID;
                _plugin.Settings.PASS_string = PASS;
            }

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();

            try
            {
                TxtLog.AppendText("Sende OTA Wake-Up Kommando inkl. WLAN-Daten an das Pedal...\n");

                byte pedalId = (byte)(_plugin.Settings?.table_selected ?? 0);

                // Den speicherkritischen Teil kapseln wir in einen unsafe-Block
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

                    // Header und Footer aus den Projekt-Konstanten setzen
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

                // 2. Dem ESP Zeit geben, Motor zu stoppen, WLAN zu verbinden und ArduinoOTA zu starten
                TxtLog.AppendText("Warte auf WLAN-Verbindung und Initialisierung des OTA-Modus am ESP32 (5 Sekunden)...\n");
                await Task.Delay(5000);

                // 3. Datei auflösen (nur firmware.bin!)
                string firmwarePath;
                if (selectedBoardFolder == "CUSTOM_LOCAL")
                {
                    firmwarePath = TxtBinPath.Text;
                }
                else
                {
                    TxtLog.AppendText($"Entpacke {selectedBoardFolder} firmware.bin...\n");
                    firmwarePath = ExtractFirmwareResource(selectedBoardFolder, "firmware.bin");
                }

                // 4. espota.exe entpacken
                string espotaPath = ExtractEspota();

                // 5. OTA Prozess starten
                TxtLog.AppendText($"Starte Upload via WLAN an {targetHostname}...\n");

                //Hostname vor dem Flashen sicher in eine IP-Adresse auflösen
                string resolvedIp = targetHostname;

                if (targetHostname.EndsWith(".local", StringComparison.OrdinalIgnoreCase))
                {
                    TxtLog.AppendText($"Löse Hostname {targetHostname} über Windows-Ping auf...\n");
                    try
                    {
                        // Wir nutzen den Windows-Ping, da dieser .local besser auflöst als C# nativ
                        Process pingProc = new Process
                        {
                            StartInfo = new ProcessStartInfo
                            {
                                FileName = "ping",
                                Arguments = $"-n 1 -w 2000 -4 {targetHostname}", // 1 Ping, max 2 Sek warten, IPv4 erzwingen
                                UseShellExecute = false,
                                RedirectStandardOutput = true,
                                CreateNoWindow = true
                            }
                        };
                        pingProc.Start();
                        string output = pingProc.StandardOutput.ReadToEnd();
                        pingProc.WaitForExit();

                        // Sucht mit Regex nach einer typischen IPv4-Adresse im Ping-Ergebnis (z.B. "[192.168.178.198]")
                        var match = System.Text.RegularExpressions.Regex.Match(output, @"\b(?:[0-9]{1,3}\.){3}[0-9]{1,3}\b");

                        if (match.Success)
                        {
                            resolvedIp = match.Value;
                            TxtLog.AppendText($"Erfolgreich aufgelöst zu IP: {resolvedIp}\n");
                        }
                        else
                        {
                            TxtLog.AppendText("Warnung: Windows konnte den .local Namen nicht in eine IP auflösen. (Ist Bonjour installiert?)\n");
                        }
                    }
                    catch (Exception ex)
                    {
                        TxtLog.AppendText($"Warnung: Ping-Auflösung fehlgeschlagen ({ex.Message}).\n");
                    }
                }

                // espota.exe bekommt jetzt zwingend die aufgelöste IP-Adresse (resolvedIp) serviert!
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
                    process.ErrorDataReceived += (s, ev) => {
                        if (ev.Data != null) Dispatcher.Invoke(() => { TxtLog.AppendText("ERROR: " + ev.Data + "\n"); TxtLog.ScrollToEnd(); });
                    };

                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await Task.Run(() => process.WaitForExit());

                    if (process.ExitCode == 0)
                    {
                        MessageBox.Show("WLAN-Update erfolgreich!", "Erfolg", MessageBoxButton.OK, MessageBoxImage.Information);
                    }
                    else
                    {
                        MessageBox.Show("OTA Update fehlgeschlagen. Siehe Logs.", "Fehler", MessageBoxButton.OK, MessageBoxImage.Error);
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
    }
}