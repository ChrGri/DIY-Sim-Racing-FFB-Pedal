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

        private const int MaxEsptoolAttempts = 3;
        private const int DefaultBaud = 460800;
        private const int FallbackBaud = 115200;

        // esptool output that means "could not talk to the chip" - worth another attempt
        // (port not ready yet, chip not in download mode, USB link dropped mid-transfer).
        private static readonly string[] ConnectionErrorMarkers =
        {
            "Failed to connect",
            "No serial data received",
            "could not open port",
            "Could not open",
            "Lost connection",
            "Write timeout",
            "Invalid head of packet",
            "does not recognize the command",
            "PermissionError",
            "port doesn't exist",
            "port is busy",
            "Timed out waiting for packet",
            "Serial data stream stopped",
        };

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

        private static VidPidResult LookupPort(string comPort)
        {
            SnapshotPresentPorts().TryGetValue(comPort, out VidPidResult info);
            return info;
        }

        // 303A:1001 = USB-Serial/JTAG (ROM download mode or a HW-CDC app),
        // 303A:0002 = ROM download mode over USB-OTG CDC.
        private static bool IsEspBootloaderPort(VidPidResult info)
        {
            return info != null && info.Vid == "303A" && (info.Pid == "1001" || info.Pid == "0002");
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

        // A freshly enumerated port shows up in WMI before the driver accepts an open, and
        // esptool started in that window fails with a fatal "could not open port" /
        // "No serial data received". Wait until the port can be opened twice in a row.
        // Opened at 115200 with DTR/RTS low: neither a 1200-bps touch nor an auto-reset.
        private async Task<bool> WaitForPortReadyAsync(string comPort, int timeoutMs = 5000)
        {
            int consecutiveOk = 0;
            string lastError = null;
            var stopwatch = Stopwatch.StartNew();
            while (stopwatch.ElapsedMilliseconds < timeoutMs)
            {
                bool ok = false;
                if (SnapshotPresentPorts().ContainsKey(comPort))
                {
                    try
                    {
                        using (var port = new SerialPort(comPort, 115200, Parity.None, 8, StopBits.One))
                        {
                            port.Open();
                            port.Close();
                        }
                        ok = true;
                    }
                    catch (Exception ex)
                    {
                        lastError = ex.Message;
                    }
                }
                else
                {
                    lastError = "port not present";
                }

                consecutiveOk = ok ? consecutiveOk + 1 : 0;
                if (consecutiveOk >= 2)
                {
                    await Task.Delay(300);
                    return true;
                }
                await Task.Delay(250);
            }
            OnOutputReceived?.Invoke(this, $"{comPort} not ready after {timeoutMs} ms ({lastError ?? "unknown"}), trying anyway.");
            return false;
        }

        // Picks the upload port and esptool's --before mode for the selected device.
        private async Task<(string Port, string Before)> PrepareUploadPortAsync(string comPort)
        {
            VidPidResult selected = LookupPort(comPort);
            OnOutputReceived?.Invoke(this, $"Selected port: {comPort} [{selected?.Vid ?? "?"}:{selected?.Pid ?? "?"}]");

            if (IsEspBootloaderPort(selected))
            {
                // USB-Serial/JTAG (bridge HW-CDC app or chip already in download mode):
                // esptool's own USB-JTAG reset is reliable, a 1200-bps touch only reboots into the app.
                OnOutputReceived?.Invoke(this, "Native USB-Serial/JTAG port - no 1200-bps touch, esptool resets via USB.");
                await WaitForPortReadyAsync(comPort);
                return (comPort, selected.Pid == "1001" ? "usb-reset" : "no-reset");
            }

            if (selected != null && selected.Vid != null && selected.Vid != "303A")
            {
                // USB-UART chip (CP2102, CH340, ...): the DTR/RTS auto-reset circuit is driven by
                // esptool; a 1200-bps touch would just reset the chip into the app.
                OnOutputReceived?.Invoke(this, "USB-UART port - no 1200-bps touch, esptool resets via DTR/RTS.");
                await WaitForPortReadyAsync(comPort);
                return (comPort, "default-reset");
            }

            // Native TinyUSB CDC of the running app: the 1200-bps touch reboots into the ROM bootloader.
            string uploadPort = EnsurePortExists(await TouchAndResolveBootloaderPortAsync(comPort), comPort);
            await WaitForPortReadyAsync(uploadPort);

            if (IsEspBootloaderPort(LookupPort(uploadPort)))
            {
                // Already in download mode - another reset would make it re-enumerate under esptool's feet.
                OnOutputReceived?.Invoke(this, $"Bootloader active on {uploadPort} - esptool connects without another reset.");
                return (uploadPort, "no-reset");
            }
            return (uploadPort, "default-reset");
        }

        // After a failed attempt the device may have re-enumerated under a different COM number.
        private string ResolveRetryPort(string lastPort, string selectedPort)
        {
            var ports = SnapshotPresentPorts();
            if (ports.ContainsKey(lastPort)) return lastPort;
            if (ports.ContainsKey(selectedPort)) return selectedPort;
            var bootloader = ports.Values.FirstOrDefault(IsEspBootloaderPort);
            if (bootloader != null) return bootloader.ComPortName;
            return lastPort;
        }

        private async Task<int> RunEsptoolAsync(string esptoolPath, string args, List<string> output)
        {
            var psi = new ProcessStartInfo
            {
                FileName = esptoolPath,
                Arguments = args,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using (var process = new Process { StartInfo = psi })
            {
                process.OutputDataReceived += (s, e) =>
                {
                    if (e.Data == null) return;
                    lock (output) output.Add(e.Data);
                    OnOutputReceived?.Invoke(this, e.Data);
                };
                process.ErrorDataReceived += (s, e) =>
                {
                    if (e.Data == null) return;
                    lock (output) output.Add(e.Data);
                    OnOutputReceived?.Invoke(this, "ERROR: " + e.Data);
                };

                process.Start();
                process.BeginOutputReadLine();
                process.BeginErrorReadLine();

                await Task.Run(() => process.WaitForExit());
                return process.ExitCode;
            }
        }

        private static bool IsConnectionError(List<string> output)
        {
            lock (output)
            {
                return output.Any(line => ConnectionErrorMarkers.Any(m => line.IndexOf(m, StringComparison.OrdinalIgnoreCase) >= 0));
            }
        }

        // Prepares the port, runs esptool and retries on connection errors: re-resolves the
        // port, lets esptool pick the reset sequence, and drops to 115200 baud on the last attempt.
        private async Task<bool> RunWithRetriesAsync(string comPort, string description, Func<string, string, int, string> buildArgs)
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

            try
            {
                var (uploadPort, before) = await PrepareUploadPortAsync(comPort);

                for (int attempt = 1; attempt <= MaxEsptoolAttempts; attempt++)
                {
                    int baud = attempt == MaxEsptoolAttempts ? FallbackBaud : DefaultBaud;
                    if (attempt > 1)
                    {
                        await Task.Delay(1500);
                        uploadPort = ResolveRetryPort(uploadPort, comPort);
                        await WaitForPortReadyAsync(uploadPort);
                        before = "default-reset"; // esptool picks the DTR/RTS or USB-JTAG sequence itself
                        OnOutputReceived?.Invoke(this, $"\nRetry {attempt}/{MaxEsptoolAttempts} on {uploadPort} ({baud} baud)...");
                    }

                    OnOutputReceived?.Invoke(this, $"{description} on {uploadPort} (--before {before}, {baud} baud)...");
                    var output = new List<string>();
                    int exitCode = await RunEsptoolAsync(esptoolPath, buildArgs(uploadPort, before, baud), output);
                    if (exitCode == 0) return true;

                    if (!IsConnectionError(output))
                    {
                        OnOutputReceived?.Invoke(this, "esptool failed with a non-connection error - not retrying.");
                        return false;
                    }
                }
                return false;
            }
            catch (Exception ex)
            {
                OnOutputReceived?.Invoke(this, $"Exception: {ex.Message}");
                return false;
            }
        }

        public async Task<bool> FlashFirmwareAsync(string comPort, string bootloaderPath, string partitionsPath, string bootAppPath, string firmwarePath)
        {
            if (!File.Exists(firmwarePath) || !File.Exists(bootloaderPath) || !File.Exists(partitionsPath) || !File.Exists(bootAppPath))
            {
                OnOutputReceived?.Invoke(this, $"Error: One or more required firmware files are missing.");
                return false;
            }

            // Flash all FOUR files to their specific ESP32-S3 memory offsets using updated non-deprecated arguments
            bool success = await RunWithRetriesAsync(comPort, "Starting flash process for 4 files", (port, before, baud) =>
                $"--chip esp32s3 --port {port} --baud {baud} --before {before} --after hard-reset write-flash -z " +
                $"0x0 \"{bootloaderPath}\" " +
                $"0x8000 \"{partitionsPath}\" " +
                $"0xE000 \"{bootAppPath}\" " +
                $"0x10000 \"{firmwarePath}\"");

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

        public async Task<bool> EraseEepromAsync(string comPort)
        {
            // Erase exactly the NVS / EEPROM partition (0x9000, size 0x5000 in every
            // partition table used by pedal and bridge). otadata starts right after it at
            // 0xE000 and holds the boot_app0 record that selects app0 - erasing into it
            // leaves the device without a valid boot selection until it is reflashed.
            bool success = await RunWithRetriesAsync(comPort, "Starting EEPROM / NVS erase (0x9000 - 0xE000)", (port, before, baud) =>
                $"--chip esp32s3 --port {port} --baud {baud} --before {before} --after hard-reset erase-region 0x9000 0x5000");

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
}
