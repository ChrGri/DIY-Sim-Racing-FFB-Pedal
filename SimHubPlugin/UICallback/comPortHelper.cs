using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Management;


namespace User.PluginSdkDemo
{
    public class VidPidResult
    {
        public bool Found { get; set; }
        public string Vid { get; set; }
        public string Pid { get; set; }
        public string DeviceName { get; set; }
        public string ComPortName { get; set; }
    }
    public static class ComPortHelper
    {

        // Cache the results here: Key = ComPortName (e.g., "COM3"), Value = VidPidResult
        private static Dictionary<string, VidPidResult> _cachedPorts;
        private static DateTime _lastCacheTime;

        // Refresh the cache only if it's older than, say, 5 seconds
        private static readonly TimeSpan CacheDuration = TimeSpan.FromSeconds(5);

        private static void RefreshCache()
        {
            _cachedPorts = new Dictionary<string, VidPidResult>();

            try
            {
                // 1. You could add a filter here to exclude Bluetooth devices entirely,
                // but filtering the resulting collection in the loop is often simpler.
                //var searcher = new ManagementObjectSearcher("SELECT Name, DeviceID FROM Win32_PnPEntity WHERE Name LIKE '%(COM%)'");
                var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE Name LIKE '%(COM%'");
                foreach (var device in searcher.Get())
                {
                    string name = device["Name"]?.ToString() ?? "";
                    string deviceID = device["DeviceID"]?.ToString() ?? "";

                    //// 🌟 ADD THIS FILTER to exclude Bluetooth-related ports 🌟
                    //if (name.IndexOf("bluetooth", StringComparison.OrdinalIgnoreCase) >= 0 ||
                    //    name.IndexOf("seriell-über-bluetooth", StringComparison.OrdinalIgnoreCase) >= 0)
                    //{
                    //    // Skip this port, it's a Bluetooth device
                    //    continue;
                    //}
                    // -------------------------------------------------------------

                    // Extract the COM port name itself from the full name (e.g., "COM3" from "USB-SERIAL CH340 (COM3)")
                    var comPortMatch = Regex.Match(name, @"(COM\d+)");
                    if (!comPortMatch.Success) continue;

                    string comPortName = comPortMatch.Groups[1].Value;

                    var result = new VidPidResult
                    {
                        DeviceName = name,
                        ComPortName = comPortName,
                        Found = true
                    };

                    Match vidMatch = Regex.Match(deviceID, "VID_([0-9a-fA-F]{4})");
                    Match pidMatch = Regex.Match(deviceID, "PID_([0-9a-fA-F]{4})");

                    if (vidMatch.Success) result.Vid = vidMatch.Groups[1].Value.ToUpper();
                    if (pidMatch.Success) result.Pid = pidMatch.Groups[1].Value.ToUpper();

                    // Add or update the entry in the dictionary
                    _cachedPorts[comPortName] = result;
                }
            }
            catch (Exception ex)
            {
                // Handle potential exceptions from WMI query
                Console.WriteLine("Error refreshing COM Port cache: " + ex.Message);
            }

            _lastCacheTime = DateTime.Now;
        }


        public static VidPidResult GetVidPidFromComPort(string targetPort)
        {
            // Refresh the cache if it's empty or stale
            if (_cachedPorts == null || DateTime.Now - _lastCacheTime > CacheDuration)
            {
                RefreshCache();
            }

            // Fast lookup from the dictionary
            if (_cachedPorts.ContainsKey(targetPort))
            {
                return _cachedPorts[targetPort];
            }

            // Return a default "not found" result
            return new VidPidResult { Found = false, ComPortName = targetPort };
        }

        public static VidPidResult GetVidPidFromComPort_orig(string targetPort)
        {
            var result = new VidPidResult { Found = false };
            result.Pid = "na";
            result.Vid = "na";
            result.DeviceName = "na";
            result.ComPortName = targetPort;
            var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE Name LIKE '%(COM%'");

            foreach (ManagementObject device in searcher.Get())
            {
                string name = device["Name"] != null ? device["Name"].ToString() : "";
                string deviceID = device["DeviceID"] != null ? device["DeviceID"].ToString() : "";

                // check if the port in the list
                if (!name.Contains("(" + targetPort.ToUpper() + ")"))
                    continue;

                // Match VID/PID
                Match vidMatch = Regex.Match(deviceID, "VID_([0-9A-Fa-f]{4})");
                Match pidMatch = Regex.Match(deviceID, "PID_([0-9A-Fa-f]{4})");

                if (vidMatch.Success && pidMatch.Success)
                {
                    result.Found = true;
                    result.Vid = vidMatch.Groups[1].Value.ToUpper();
                    result.Pid = pidMatch.Groups[1].Value.ToUpper();
                    result.DeviceName = name;
                    //result.ComPortName = targetPort;
                    return result;
                }
            }

            return result;
        }
    
    }
}
