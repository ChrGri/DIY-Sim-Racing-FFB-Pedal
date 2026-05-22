using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Windows;
using System.Linq;
using Microsoft.Win32;

namespace DiyFfbPedal.UIFunction
{
    public partial class FirmwareFlasherWindow : Window
    {
        private readonly DIY_FFB_Pedal _plugin;
        private readonly EspFlasher _flasher;

        public FirmwareFlasherWindow(DIY_FFB_Pedal plugin)
        {
            InitializeComponent();
            _plugin = plugin;
            _flasher = new EspFlasher();

            _flasher.OnOutputReceived += (s, msg) =>
            {
                Dispatcher.Invoke(() =>
                {
                    TxtLog.AppendText(msg + Environment.NewLine);
                    TxtLog.ScrollToEnd();
                });
            };

            RefreshPorts();
            PopulateFirmwareDropdown();
        }

        private void PopulateFirmwareDropdown()
        {
            var firmwareOptions = new Dictionary<string, string>();

            // 1. Add the Custom Local option at the very top of the list
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
                    else
                    {
                        TxtLog.AppendText("Warning: manifest.txt not found in embedded resources.\n");
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Error reading firmware manifest: {ex.Message}\n");
            }

            CboFirmware.ItemsSource = firmwareOptions;

            // Select the first embedded board by default (index 1), unless none exist, then select Custom (index 0)
            if (CboFirmware.Items.Count > 1) CboFirmware.SelectedIndex = 1;
            else if (CboFirmware.Items.Count > 0) CboFirmware.SelectedIndex = 0;
        }

        private void BtnRefreshCom_Click(object sender, RoutedEventArgs e) => RefreshPorts();

        private void RefreshPorts()
        {
            CboComPorts.Items.Clear();

            string[] comPorts = SerialPort.GetPortNames().Distinct().ToArray();
            var sortedPorts = comPorts
                .Select(port => new
                {
                    Name = port,
                    Number = int.TryParse(System.Text.RegularExpressions.Regex.Match(port, @"\d+").Value, out int num) ? num : int.MaxValue
                })
                .OrderBy(p => p.Number)
                .Select(p => p.Name)
                .ToArray();

            foreach (string port in sortedPorts)
            {
                CboComPorts.Items.Add(port);
            }
            if (CboComPorts.Items.Count > 0) CboComPorts.SelectedIndex = 0;
        }

        /// <summary>
        /// Extracts an embedded binary from the DLL to the Windows Temp folder
        /// </summary>
        private string ExtractFirmwareResource(string boardFolder, string fileName)
        {
            // The resource path format: [Namespace].[Folders].[Filename]
            string resourceName = $"DiyFfbPedal.Resources.Firmware.{boardFolder}.{fileName}";

            // Create a specific temp folder for this board to avoid overlaps
            string tempDir = Path.Combine(Path.GetTempPath(), "SimHub_DIY_Pedal_Flasher", boardFolder);
            Directory.CreateDirectory(tempDir);

            string outPath = Path.Combine(tempDir, fileName);

            var assembly = Assembly.GetExecutingAssembly();
            using (Stream stream = assembly.GetManifestResourceStream(resourceName))
            {
                if (stream == null)
                {
                    throw new FileNotFoundException($"Could not find embedded resource: {resourceName}. Please check folder names and Build Action.");
                }

                using (FileStream fs = new FileStream(outPath, FileMode.Create, FileAccess.Write))
                {
                    stream.CopyTo(fs);
                }
            }

            return outPath;
        }

        // Shows or hides the custom file browser based on dropdown selection
        private void CboFirmware_SelectionChanged(object sender, System.Windows.Controls.SelectionChangedEventArgs e)
        {
            if (PnlCustomFile == null) return; // Prevent crash during initialization

            if (CboFirmware.SelectedValue != null && CboFirmware.SelectedValue.ToString() == "CUSTOM_LOCAL")
            {
                PnlCustomFile.Visibility = Visibility.Visible;
            }
            else
            {
                PnlCustomFile.Visibility = Visibility.Collapsed;
            }
        }

        // Opens the file dialog for local firmware
        private void BtnBrowse_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog ofd = new OpenFileDialog
            {
                Filter = "Firmware File (firmware.bin)|firmware.bin|All Binary Files (*.bin)|*.bin",
                Title = "Select firmware.bin"
            };

            if (ofd.ShowDialog() == true)
            {
                TxtBinPath.Text = ofd.FileName;
            }
        }
        private async void BtnFlash_Click(object sender, RoutedEventArgs e)
        {
            if (CboComPorts.SelectedItem == null || CboFirmware.SelectedItem == null)
            {
                MessageBox.Show("Please select a COM port and Hardware version.");
                return;
            }

            string port = CboComPorts.SelectedItem.ToString();
            string selectedBoardFolder = CboFirmware.SelectedValue.ToString();

            // Validation for custom file mode
            if (selectedBoardFolder == "CUSTOM_LOCAL" && string.IsNullOrWhiteSpace(TxtBinPath.Text))
            {
                MessageBox.Show("Please browse and select your custom firmware.bin file.");
                return;
            }

            ForceDisconnectSerial(port);

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();
            TxtLog.AppendText(selectedBoardFolder == "CUSTOM_LOCAL" ? "Preparing to flash local files...\n" : $"Preparing to flash {selectedBoardFolder}...\n");

            try
            {
                string bootloaderPath, partitionsPath, bootAppPath, firmwarePath;

                if (selectedBoardFolder == "CUSTOM_LOCAL")
                {
                    // --- MODE 1: LOCAL FILES ---
                    firmwarePath = TxtBinPath.Text;
                    string directory = Path.GetDirectoryName(firmwarePath);

                    // Auto-resolve sibling files
                    bootloaderPath = Path.Combine(directory, "bootloader.bin");
                    partitionsPath = Path.Combine(directory, "partitions.bin");
                    bootAppPath = Path.Combine(directory, "boot_app0.bin");

                    if (!File.Exists(bootloaderPath) || !File.Exists(partitionsPath) || !File.Exists(bootAppPath))
                    {
                        MessageBox.Show("Missing files! Ensure 'bootloader.bin', 'partitions.bin', and 'boot_app0.bin' are in the same folder as your selected 'firmware.bin'.", "Files Missing", MessageBoxButton.OK, MessageBoxImage.Warning);
                        BtnFlash.IsEnabled = true;
                        return;
                    }
                }
                else
                {
                    // --- MODE 2: EMBEDDED RESOURCES ---
                    bootloaderPath = ExtractFirmwareResource(selectedBoardFolder, "bootloader.bin");
                    partitionsPath = ExtractFirmwareResource(selectedBoardFolder, "partitions.bin");
                    bootAppPath = ExtractFirmwareResource(selectedBoardFolder, "boot_app0.bin");
                    firmwarePath = ExtractFirmwareResource(selectedBoardFolder, "firmware.bin");
                }

                // Pass resolved paths to esptool
                bool success = await _flasher.FlashFirmwareAsync(port, bootloaderPath, partitionsPath, bootAppPath, firmwarePath);

                if (success)
                {
                    MessageBox.Show("Firmware updated successfully!", "Success", MessageBoxButton.OK, MessageBoxImage.Information);
                }
                else
                {
                    MessageBox.Show("Flashing Failed. Check the log for details.", "Error", MessageBoxButton.OK, MessageBoxImage.Error);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Error preparing firmware: {ex.Message}", "File Error", MessageBoxButton.OK, MessageBoxImage.Error);
                TxtLog.AppendText($"\nERROR: {ex.Message}\n");
            }
            finally
            {
                BtnFlash.IsEnabled = true;
            }
        }

        private void ForceDisconnectSerial(string targetPort)
        {
            for (int i = 0; i < 3; i++)
            {
                if (_plugin._serialPort[i].IsOpen && _plugin._serialPort[i].PortName.Equals(targetPort, StringComparison.OrdinalIgnoreCase))
                {
                    try { _plugin.wpfHandle.closeSerialAndStopReadCallback((uint)i); } catch { }
                    try { _plugin._serialPort[i].Close(); } catch { }
                }
            }

            if (_plugin.ESPsync_serialPort.IsOpen && _plugin.ESPsync_serialPort.PortName.Equals(targetPort, StringComparison.OrdinalIgnoreCase))
            {
                try { _plugin.ESPsync_serialPort.Close(); } catch { }
            }
        }
    }
}