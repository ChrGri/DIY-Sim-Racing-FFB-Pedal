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

        // Live snapshot of present COM devices (WMI). SerialPort.GetPortNames() reads the
        // registry, which keeps entries of unplugged devices - e.g. the bootloader's COM number
        // from a previous flash, or the pedal's app port after it already left - and must not be
        // used to detect the re-enumeration.
        private static Dictionary<string, VidPidResult> SnapshotPresentPorts()
        {
            return ComPortHelper.GetPresentPorts(forceRefresh: true)
                .GroupBy(p => p.ComPortName, StringComparer.OrdinalIgnoreCase)
                .ToDictionary(g => g.Key, g => g.First(), StringComparer.OrdinalIgnoreCase);
        }

        private static string DescribeSnapshot(Dictionary<string, VidPidResult> ports)
        {
            return ports.Count == 0 ? "none" : string.Join(", ", ports.Values.Select(p => $"{p.ComPortName} [{p.Vid ?? "?"}:{p.Pid ?? "?"}]"));
        }

        private async Task<string> TouchAndResolveBootloaderPortAsync(string comPort)
        {
            var initialPorts = SnapshotPresentPorts();
            OnOutputReceived?.Invoke(this, $"Ports before reset: {DescribeSnapshot(initialPorts)}");

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

            // The bootloader may come up under a different COM number (e.g. COM31 -> COM23).
            // Other devices (a bridge, a hub re-enumerating, ghost registry entries) can make
            // unrelated ports appear in the same window, so a new port is only accepted if
            //  - it appeared after the selected port went away (the pedal left its app),
            //  - it is an Espressif device (VID 303A), and
            //  - it is still present on the next poll (not a transient entry).
            // Otherwise the selected port is used if it came back.
            bool originalGone = false;
            string pendingCandidate = null;
            var reportedIgnored = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
            string lastSnapshotText = null;

            for (int i = 0; i < 30; i++)
            {
                await Task.Delay(200);
                var currentPorts = SnapshotPresentPorts();
                string snapshotText = DescribeSnapshot(currentPorts);
                if (snapshotText != lastSnapshotText)
                {
                    OnOutputReceived?.Invoke(this, $"Ports now: {snapshotText}");
                    lastSnapshotText = snapshotText;
                }

                bool originalPresent = currentPorts.ContainsKey(comPort);
                if (!originalPresent) originalGone = true;

                string candidate = null;
                if (originalGone)
                {
                    foreach (var p in currentPorts.Values.Where(p => !initialPorts.ContainsKey(p.ComPortName)))
                    {
                        if (p.Vid == "303A")
                        {
                            candidate = p.ComPortName;
                            break;
                        }
                        if (reportedIgnored.Add(p.ComPortName))
                        {
                            OnOutputReceived?.Invoke(this, $"Ignoring new port {p.ComPortName} (VID {p.Vid ?? "?"}) - not an ESP32 bootloader.");
                        }
                    }
                }

                if (candidate != null)
                {
                    if (string.Equals(candidate, pendingCandidate, StringComparison.OrdinalIgnoreCase))
                    {
                        OnOutputReceived?.Invoke(this, $"Detected ESP32-S3 bootloader on new port: {candidate}");
                        return candidate;
                    }
                    pendingCandidate = candidate; // confirm on the next poll
                    continue;
                }
                pendingCandidate = null;

                // Bootloader re-used the original COM number
                if (originalGone && originalPresent)
                {
                    OnOutputReceived?.Invoke(this, $"Bootloader re-appeared on {comPort}.");
                    return comPort;
                }
            }

            OnOutputReceived?.Invoke(this, $"No new bootloader port detected, using port: {comPort}");
            return comPort;
        }

        // Final guard right before esptool starts: never hand over a port that vanished again.
        private string EnsurePortExists(string uploadPort, string fallbackPort)
        {
            bool exists = SnapshotPresentPorts().ContainsKey(uploadPort);
            if (exists || string.Equals(uploadPort, fallbackPort, StringComparison.OrdinalIgnoreCase))
            {
                return uploadPort;
            }
            OnOutputReceived?.Invoke(this, $"{uploadPort} disappeared again, falling back to {fallbackPort}.");
            return fallbackPort;
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
            string uploadPort = EnsurePortExists(await TouchAndResolveBootloaderPortAsync(comPort), comPort);

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
        public async Task<bool> EraseEepromAsync(string comPort)
        {
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

            // Perform 1200-bps touch and dynamically resolve the bootloader port (e.g. if COM35 switched to COM34)
            string uploadPort = EnsurePortExists(await TouchAndResolveBootloaderPortAsync(comPort), comPort);

            // Erase exactly the NVS / EEPROM partition (0x9000, size 0x5000 in every
            // partition table used by pedal and bridge). otadata starts right after it at
            // 0xE000 and holds the boot_app0 record that selects app0 - erasing into it
            // leaves the device without a valid boot selection until it is reflashed.
            string args = $"--chip esp32s3 --port {uploadPort} --baud 460800 --after hard-reset erase-region 0x9000 0x5000";

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

                    OnOutputReceived?.Invoke(this, $"Starting EEPROM / NVS erase on {uploadPort} (0x9000 - 0xE000)...");
                    process.Start();
                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    await Task.Run(() => process.WaitForExit());

                    bool success = process.ExitCode == 0;
                    if (success)
                    {
                        OnOutputReceived?.Invoke(this, "\n------------------------------------------------------------");
                        OnOutputReceived?.Invoke(this, "SUCCESS: EEPROM / NVS erased successfully!");
                        OnOutputReceived?.Invoke(this, "Device restarted to factory default state.");
                        OnOutputReceived?.Invoke(this, "------------------------------------------------------------\n");
                    }
                    else
                    {
                        OnOutputReceived?.Invoke(this, "\n------------------------------------------------------------");
                        OnOutputReceived?.Invoke(this, "TIP: If connection failed ('No serial data received'):");
                        OnOutputReceived?.Invoke(this, "1. Press and hold the 'BOOT' button on the board.");
                        OnOutputReceived?.Invoke(this, "2. Press and release the 'RST' button.");
                        OnOutputReceived?.Invoke(this, "3. Release 'BOOT' and click 'Reset EEPROM' again.");
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
