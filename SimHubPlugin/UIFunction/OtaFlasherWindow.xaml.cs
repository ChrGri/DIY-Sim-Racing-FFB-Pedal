using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Windows;
using System.Linq;
using Microsoft.Win32;
using System.Threading.Tasks;
using System.Diagnostics;

namespace DiyFfbPedal.UIFunction
{
    public partial class OtaFlasherWindow : Window
    {
        private readonly DIY_FFB_Pedal _plugin;

        public OtaFlasherWindow(DIY_FFB_Pedal plugin)
        {
            InitializeComponent();
            _plugin = plugin;

            // XAML-Elemente (müssen im XAML definiert sein):
            // CboFirmware (ComboBox)
            // TxtHostname (TextBox, Default: "pedal_ota.local")
            // TxtBinPath (TextBox für CUSTOM_LOCAL)
            // TxtLog (TextBox für den Output)
            // BtnFlash (Button)

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

        // Wird aufgerufen, wenn im Dropdown etwas geändert wird
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
            string resourceName = "DiyFfbPedal.Resources.espota.exe"; // Stelle sicher, dass espota.exe als Embedded Resource im Projekt liegt!
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
            string targetHostname = TxtHostname.Text; // z.B. "pedal_ota.local" oder eine IP-Adresse

            if (selectedBoardFolder == "CUSTOM_LOCAL" && string.IsNullOrWhiteSpace(TxtBinPath.Text))
            {
                MessageBox.Show("Bitte wähle eine lokale firmware.bin aus.");
                return;
            }

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();

            try
            {
                // 1. Wake-Up Signal an das Pedal senden (Versetzt das Pedal in den OTA-Modus)
                TxtLog.AppendText("Sende Wake-Up Kommando an das Pedal...\n");
                DAP_action_st tmp = new DAP_action_st();
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                tmp.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.ENABLE_OTA;
                // Hier deine bestehende Methode aufrufen, um das Paket zu senden:
                // _plugin.SendPedalAction(tmp, _plugin.Settings.table_selected);

                // 2. Dem ESP Zeit geben, Motor zu stoppen und ArduinoOTA zu starten
                TxtLog.AppendText("Warte auf Initialisierung des OTA-Modus am ESP32...\n");
                await Task.Delay(4000);

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
                string args = $"-i {targetHostname} -p 3232 -f \"{firmwarePath}\"";

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