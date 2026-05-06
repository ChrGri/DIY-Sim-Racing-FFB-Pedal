using System;
using System.IO;
using System.IO.Ports;
using System.Windows;
using Microsoft.Win32;

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
            if (CboComPorts.SelectedItem == null || string.IsNullOrWhiteSpace(TxtBinPath.Text))
            {
                MessageBox.Show("Select a COM port and the firmware.bin file.");
                return;
            }

            string firmwarePath = TxtBinPath.Text;
            string directory = Path.GetDirectoryName(firmwarePath);

            // Auto-resolve all required files
            string bootloaderPath = Path.Combine(directory, "bootloader.bin");
            string partitionsPath = Path.Combine(directory, "partitions.bin");
            string bootAppPath = Path.Combine(directory, "boot_app0.bin");

            if (!File.Exists(bootloaderPath) || !File.Exists(partitionsPath) || !File.Exists(bootAppPath))
            {
                MessageBox.Show("Missing files! Ensure 'bootloader.bin', 'partitions.bin', and 'boot_app0.bin' are in the same folder as 'firmware.bin'.",
                                "Files Missing", MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            string port = CboComPorts.SelectedItem.ToString();

            ForceDisconnectSerial(port);

            BtnFlash.IsEnabled = false;
            TxtLog.Clear();

            // Pass all four paths to the flasher
            bool success = await _flasher.FlashFirmwareAsync(port, bootloaderPath, partitionsPath, bootAppPath, firmwarePath);

            if (success)
            {
                MessageBox.Show("Firmware updated successfully!",
                                "Success", MessageBoxButton.OK, MessageBoxImage.Information);
            }
            else
            {
                MessageBox.Show("Flashing Failed. Check the log for details.",
                                "Error", MessageBoxButton.OK, MessageBoxImage.Error);
            }

            BtnFlash.IsEnabled = true;
        }

        private void ForceDisconnectSerial(string targetPort)
        {
            // Iterate through the plugin's serial ports and close them if they match the target
            for (int i = 0; i < 3; i++)
            {
                if (_plugin._serialPort[i].IsOpen && _plugin._serialPort[i].PortName.Equals(targetPort, StringComparison.OrdinalIgnoreCase))
                {
                    try { _plugin.wpfHandle.closeSerialAndStopReadCallback((uint)i); } catch { }
                    try { _plugin._serialPort[i].Close(); } catch { }
                }
            }
            
            // Also check the ESP Now sync port
            if (_plugin.ESPsync_serialPort.IsOpen && _plugin.ESPsync_serialPort.PortName.Equals(targetPort, StringComparison.OrdinalIgnoreCase))
            {
                try { _plugin.ESPsync_serialPort.Close(); } catch { }
            }
        }
    }
}