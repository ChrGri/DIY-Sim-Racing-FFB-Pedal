using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Ports;
using System.Linq;
using System.Reflection;
using System.Threading.Tasks;

namespace DiyFfbPedal
{
    public class EspFlasher
    {
        public event EventHandler<string> OnOutputReceived;

        private string ExtractEsptool()
        {
            // Exact namespace based on your AssemblyInfo/Project settings
            string resourceName = "DiyFfbPedal.Resources.esptool.exe";
            string tempFolder = Path.GetTempPath();
            string exePath = Path.Combine(tempFolder, "esptool_simhub_plugin.exe");

            if (!File.Exists(exePath))
            {
                var assembly = Assembly.GetExecutingAssembly();
                using (Stream stream = assembly.GetManifestResourceStream(resourceName))
                {
                    if (stream == null) throw new FileNotFoundException($"Embedded resource '{resourceName}' not found.");
                    using (FileStream fileStream = new FileStream(exePath, FileMode.Create, FileAccess.Write))
                    {
                        stream.CopyTo(fileStream);
                    }
                }
            }
            return exePath;
        }

        private async Task<string> TouchAndResolveBootloaderPortAsync(string comPort)
        {
            var initialPorts = new HashSet<string>(SerialPort.GetPortNames().Distinct(), StringComparer.OrdinalIgnoreCase);

            try
            {
                OnOutputReceived?.Invoke(this, $"Sending 1200-bps touch reset to {comPort}...");
                using (var port = new SerialPort(comPort, 1200, Parity.None, 8, StopBits.One))
                {
                    port.DtrEnable = true;
                    port.RtsEnable = true;
                    port.Open();
                    await Task.Delay(100);
                    port.DtrEnable = false;
                    port.RtsEnable = false;
                    await Task.Delay(100);
                    port.Close();
                }
            }
            catch (Exception ex)
            {
                OnOutputReceived?.Invoke(this, $"Touch note: {ex.Message}");
            }

            OnOutputReceived?.Invoke(this, "Waiting for ESP32-S3 bootloader to enumerate...");

            // Poll for up to 4 seconds (20 x 200ms) to detect if a new bootloader COM port appears
            // (e.g. COM31 switched to COM23) or if the existing port re-appeared
            string targetPort = comPort;
            for (int i = 0; i < 20; i++)
            {
                await Task.Delay(200);
                var currentPorts = SerialPort.GetPortNames().Distinct().ToArray();

                // Check if any new port appeared that wasn't present originally
                var newPort = currentPorts.FirstOrDefault(p => !initialPorts.Contains(p));
                if (!string.IsNullOrEmpty(newPort))
                {
                    OnOutputReceived?.Invoke(this, $"Detected ESP32-S3 bootloader on new port: {newPort}");
                    return newPort;
                }

                // If original port is present in current ports
                if (currentPorts.Any(p => p.Equals(comPort, StringComparison.OrdinalIgnoreCase)))
                {
                    targetPort = comPort;
                }
            }

            OnOutputReceived?.Invoke(this, $"Using port: {targetPort}");
            return targetPort;
        }

        public async Task<bool> FlashFirmwareAsync(string comPort, string bootloaderPath, string partitionsPath, string bootAppPath, string firmwarePath)
        {
            if (!File.Exists(firmwarePath) || !File.Exists(bootloaderPath) || !File.Exists(partitionsPath) || !File.Exists(bootAppPath))
            {
                OnOutputReceived?.Invoke(this, $"Error: One or more required firmware files are missing.");
                return false;
            }

            string esptoolPath;
            try
            {
                esptoolPath = ExtractEsptool();
            }
            catch (Exception ex)
            {
                OnOutputReceived?.Invoke(this, $"Failed to extract flasher: {ex.Message}");
                return false;
            }

            // Perform 1200-bps touch and dynamically resolve the bootloader port (e.g. if COM31 switched to COM23)
            string uploadPort = await TouchAndResolveBootloaderPortAsync(comPort);

            // Flash all FOUR files to their specific ESP32-S3 memory offsets using updated non-deprecated arguments
            string args = $"--chip esp32s3 --port {uploadPort} --baud 460800 --after hard-reset write-flash -z " +
                          $"0x0 \"{bootloaderPath}\" " +
                          $"0x8000 \"{partitionsPath}\" " +
                          $"0xE000 \"{bootAppPath}\" " +
                          $"0x10000 \"{firmwarePath}\"";

            var psi = new ProcessStartInfo
            {
                FileName = esptoolPath,
                Arguments = args,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            try
            {
                using (var process = new Process { StartInfo = psi })
                {
                    process.OutputDataReceived += (s, e) => { if (e.Data != null) OnOutputReceived?.Invoke(this, e.Data); };
                    process.ErrorDataReceived += (s, e) => { if (e.Data != null) OnOutputReceived?.Invoke(this, "ERROR: " + e.Data); };

                    OnOutputReceived?.Invoke(this, "Starting flash process for 4 files...");
                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await Task.Run(() => process.WaitForExit());

                    bool success = process.ExitCode == 0;
                    if (!success)
                    {
                        OnOutputReceived?.Invoke(this, "\n------------------------------------------------------------");
                        OnOutputReceived?.Invoke(this, "TIP: If connection failed ('No serial data received'):");
                        OnOutputReceived?.Invoke(this, "1. Press & hold the 'BOOT' button on the board.");
                        OnOutputReceived?.Invoke(this, "2. Press & release the 'RST' button.");
                        OnOutputReceived?.Invoke(this, "3. Release 'BOOT' and click 'Flash Firmware' again.");
                        OnOutputReceived?.Invoke(this, "------------------------------------------------------------\n");
                    }
                    return success;
                }
            }
            catch (Exception ex)
            {
                OnOutputReceived?.Invoke(this, $"Exception: {ex.Message}");
                return false;
            }
        }
    }
}