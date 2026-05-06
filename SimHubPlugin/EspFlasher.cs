using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public class EspFlasher
    {
        public event EventHandler<string> OnOutputReceived;

        private string ExtractEsptool()
        {
            // Exact namespace based on your AssemblyInfo/Project settings
            string resourceName = "User.PluginSdkDemo.Resources.esptool.exe";
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

            // Flash all FOUR files to their specific ESP32-S3 memory offsets
            // Added --after hard_reset
            string args = $"--chip esp32s3 --port {comPort} --baud 460800 --after hard_reset write_flash -z " + $"0x0 \"{bootloaderPath}\" " +
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
                    return process.ExitCode == 0;
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