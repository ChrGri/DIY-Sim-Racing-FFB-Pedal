using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Ports;
using System.Reflection;
using System.Windows;

namespace User.PluginSdkDemo.UIFunction
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

            // The path to the manifest inside the DLL
            string manifestResourceName = "User.PluginSdkDemo.Resources.Firmware.manifest.txt";
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
                                    // Create a clean display name for the UI. 
                                    // E.g., "esp32s3usbotg_pcbV6" becomes "esp32s3usbotg pcbV6"
                                    string displayName = line.Replace("_", " ");
                                    firmwareOptions.Add(displayName, line);
                                }
                            }
                        }
                    }
                    else
                    {
                        TxtLog.AppendText("Warning: manifest.txt not found in embedded resources. Did you run the batch script?\n");
                    }
                }
            }
            catch (Exception ex)
            {
                TxtLog.AppendText($"Error reading firmware manifest: {ex.Message}\n");
            }

            CboFirmware.ItemsSource = firmwareOptions;
            if (CboFirmware.Items.Count > 0) CboFirmware.SelectedIndex = 0;
        }

        private void BtnRefreshCom_Click(object sender, RoutedEventArgs e) => RefreshPorts();

        private void RefreshPorts()
        {
            CboComPorts.Items.Clear();
            foreach (string port in SerialPort.GetPortNames())
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
            string resourceName = $"User.PluginSdkDemo.Resources.Firmware.{boardFolder}.{fileName}";

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

        private async void BtnFlash_Click(object sender, RoutedEventArgs e)
        {
            if (CboComPorts.SelectedItem == null || CboFirmware.SelectedItem == null)
            {
                MessageBox.Show("Please select a COM port and Hardware version.");
                return;
            }

            string port = CboComPorts.SelectedItem.ToString();
            string selectedBoardFolder = CboFirmware.SelectedValue.ToString();

            ForceDisconnectSerial(port);

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();
            TxtLog.AppendText($"Preparing to flash {selectedBoardFolder}...\n");

            try
            {
                // 1. Extract all 4 files from the embedded resources dynamically
                string bootloaderPath = ExtractFirmwareResource(selectedBoardFolder, "bootloader.bin");
                string partitionsPath = ExtractFirmwareResource(selectedBoardFolder, "partitions.bin");
                string bootAppPath = ExtractFirmwareResource(selectedBoardFolder, "boot_app0.bin");
                string firmwarePath = ExtractFirmwareResource(selectedBoardFolder, "firmware.bin");

                // 2. Pass them to the flasher
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
                MessageBox.Show($"Error extracting firmware: {ex.Message}", "Extraction Error", MessageBoxButton.OK, MessageBoxImage.Error);
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