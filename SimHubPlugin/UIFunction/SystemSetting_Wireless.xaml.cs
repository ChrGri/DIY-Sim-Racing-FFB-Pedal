using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO.Ports;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    public class WirelessNodeRow : INotifyPropertyChanged
    {
        private int _nodeIndex; // 0=Clutch, 1=Brake, 2=Throttle, 3=Bridge
        private string _roleName;
        private string _usbStatusText = "Disconnected";
        private Brush _usbStatusForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
        private Brush _usbStatusBrush = new SolidColorBrush(Color.FromRgb(100, 100, 100));
        private string _macAddress = "--";
        private string _channelDisplay = "--";
        private byte _channelNumber = 0;
        private Brush _channelForeground = new SolidColorBrush(Color.FromRgb(0, 229, 255));
        private Brush _channelBadgeBackground = new SolidColorBrush(Color.FromArgb(26, 0, 229, 255));
        private Brush _channelBadgeBorder = new SolidColorBrush(Color.FromArgb(51, 0, 229, 255));
        private string _rssiDisplay = "--";
        private Brush _rssiForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
        private Brush _rssiBarBrush = new SolidColorBrush(Color.FromRgb(80, 80, 80));
        private string _rssiTooltip = "No signal / Disconnected";
        private Brush _roleBadgeForeground = new SolidColorBrush(Colors.White);
        private Brush _roleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 255, 255));
        private Brush _roleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 255, 255));
        private Brush _rowBackground = Brushes.Transparent;
        private bool _canBeep = false;

        public int NodeIndex { get => _nodeIndex; set { _nodeIndex = value; OnPropertyChanged(); } }
        public string RoleName { get => _roleName; set { _roleName = value; OnPropertyChanged(); } }
        public string UsbStatusText { get => _usbStatusText; set { _usbStatusText = value; OnPropertyChanged(); } }
        public Brush UsbStatusForeground { get => _usbStatusForeground; set { _usbStatusForeground = value; OnPropertyChanged(); } }
        public Brush UsbStatusBrush { get => _usbStatusBrush; set { _usbStatusBrush = value; OnPropertyChanged(); } }
        public string MacAddress { get => _macAddress; set { _macAddress = value; OnPropertyChanged(); } }
        public string ChannelDisplay { get => _channelDisplay; set { _channelDisplay = value; OnPropertyChanged(); } }
        public byte ChannelNumber { get => _channelNumber; set { _channelNumber = value; OnPropertyChanged(); } }
        public Brush ChannelForeground { get => _channelForeground; set { _channelForeground = value; OnPropertyChanged(); } }
        public Brush ChannelBadgeBackground { get => _channelBadgeBackground; set { _channelBadgeBackground = value; OnPropertyChanged(); } }
        public Brush ChannelBadgeBorder { get => _channelBadgeBorder; set { _channelBadgeBorder = value; OnPropertyChanged(); } }
        public string RssiDisplay { get => _rssiDisplay; set { _rssiDisplay = value; OnPropertyChanged(); } }
        public Brush RssiForeground { get => _rssiForeground; set { _rssiForeground = value; OnPropertyChanged(); } }
        public Brush RssiBarBrush { get => _rssiBarBrush; set { _rssiBarBrush = value; OnPropertyChanged(); } }
        public string RssiTooltip { get => _rssiTooltip; set { _rssiTooltip = value; OnPropertyChanged(); } }
        public Brush RoleBadgeForeground { get => _roleBadgeForeground; set { _roleBadgeForeground = value; OnPropertyChanged(); } }
        public Brush RoleBadgeBackground { get => _roleBadgeBackground; set { _roleBadgeBackground = value; OnPropertyChanged(); } }
        public Brush RoleBadgeBorder { get => _roleBadgeBorder; set { _roleBadgeBorder = value; OnPropertyChanged(); } }
        public Brush RowBackground { get => _rowBackground; set { _rowBackground = value; OnPropertyChanged(); } }
        public bool CanBeep { get => _canBeep; set { _canBeep = value; OnPropertyChanged(); } }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged([CallerMemberName] string name = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    public partial class SystemSetting_Wireless : UserControl
    {
        public DIY_FFB_Pedal Plugin { get; set; }
        public DIYFFBPedalControlUI ParentUI { get; set; }

        public ObservableCollection<WirelessNodeRow> NodeRows { get; } = new ObservableCollection<WirelessNodeRow>();

        private DispatcherTimer _liveUpdateTimer;

        public SystemSetting_Wireless()
        {
            InitializeComponent();
            ic_pedal_list.ItemsSource = NodeRows;

            InitDefaultRows();

            _liveUpdateTimer = new DispatcherTimer();
            _liveUpdateTimer.Interval = TimeSpan.FromMilliseconds(250);
            _liveUpdateTimer.Tick += (s, e) =>
            {
                if (this.IsVisible)
                {
                    UpdateLiveStatus();
                }
            };
            _liveUpdateTimer.Start();

            this.Loaded += (s, e) =>
            {
                if (Plugin?.Settings != null && Plugin.Settings.ActiveWifiChannel >= 1 && Plugin.Settings.ActiveWifiChannel <= 14)
                {
                    SetSelectedWifiChannel(Plugin.Settings.ActiveWifiChannel);
                }
                UpdateLiveStatus();

                // If bridge or any pedal is missing MAC, auto-query connected devices in background
                if (NodeRows.Any(r => r.MacAddress == "--" || string.IsNullOrWhiteSpace(r.MacAddress)))
                {
                    _ = QueryAndAutoDetectDevicesAsync(false);
                }
            };
        }

        private void InitDefaultRows()
        {
            NodeRows.Clear();

            // 1. Bridge (Node Index 3) - Gold
            NodeRows.Add(new WirelessNodeRow
            {
                NodeIndex = 3,
                RoleName = "BRIDGE",
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(255, 215, 0)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 215, 0)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 215, 0)),
                CanBeep = false
            });

            // 2. Clutch (Node Index 0) - Red
            NodeRows.Add(new WirelessNodeRow
            {
                NodeIndex = 0,
                RoleName = "CLUTCH",
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(255, 82, 82)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 82, 82)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 82, 82)),
                CanBeep = true
            });

            // 3. Brake (Node Index 1) - Green
            NodeRows.Add(new WirelessNodeRow
            {
                NodeIndex = 1,
                RoleName = "BRAKE",
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 0, 230, 118)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 0, 230, 118)),
                CanBeep = true
            });

            // 4. Throttle (Node Index 2) - Blue
            NodeRows.Add(new WirelessNodeRow
            {
                NodeIndex = 2,
                RoleName = "THROTTLE",
                RoleBadgeForeground = new SolidColorBrush(Color.FromRgb(41, 121, 255)),
                RoleBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 41, 121, 255)),
                RoleBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 41, 121, 255)),
                CanBeep = true
            });
        }

        public void UpdateLiveTable() { UpdateLiveStatus(); }
        public void UpdateLiveStatus()
        {
            if (Plugin == null) return;

            byte selectedCh = GetSelectedWifiChannel();
            byte activeCh = (Plugin.Settings != null && Plugin.Settings.ActiveWifiChannel >= 1 && Plugin.Settings.ActiveWifiChannel <= 14) 
                            ? Plugin.Settings.ActiveWifiChannel 
                            : selectedCh;

            foreach (var row in NodeRows)
            {
                int idx = row.NodeIndex;

                // Sync MAC address from plugin settings if known
                if (Plugin.Settings?.AssignedPedalMac != null && Plugin.Settings.AssignedPedalMac.Length > idx)
                {
                    string savedMac = Plugin.Settings.AssignedPedalMac[idx];
                    if (!string.IsNullOrWhiteSpace(savedMac) && savedMac != "--" && savedMac != "00:00:00:00:00:00")
                    {
                        row.MacAddress = savedMac;
                    }
                }

                bool hasMac = !string.IsNullOrWhiteSpace(row.MacAddress) && row.MacAddress != "--" && row.MacAddress != "00:00:00:00:00:00";
                bool isBridgeOnline = idx == 3 && ((Plugin.BridgeHidService != null && Plugin.BridgeHidService.IsConnected) || (Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen));
                bool isPedalOnline = idx < 3 && Plugin._serialPort != null && Plugin._serialPort.Length > idx && Plugin._serialPort[idx] != null && Plugin._serialPort[idx].IsOpen;

                if (hasMac || isBridgeOnline || isPedalOnline)
                {
                    row.ChannelNumber = activeCh;
                }

                if (idx == 3) // Bridge
                {
                    bool isHid = Plugin.BridgeHidService != null && Plugin.BridgeHidService.IsConnected;
                    bool isSerial = Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen;

                    if (isHid)
                    {
                        row.UsbStatusText = "USB-HID Online";
                        row.UsbStatusForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                        row.UsbStatusBrush = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                    }
                    else if (isSerial)
                    {
                        row.UsbStatusText = $"{Plugin.ESPsync_serialPort.PortName} Online";
                        row.UsbStatusForeground = new SolidColorBrush(Color.FromRgb(0, 229, 255));
                        row.UsbStatusBrush = new SolidColorBrush(Color.FromRgb(0, 229, 255));
                    }
                    else
                    {
                        row.UsbStatusText = "Disconnected";
                        row.UsbStatusForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                        row.UsbStatusBrush = new SolidColorBrush(Color.FromRgb(100, 100, 100));
                    }

                    row.RssiDisplay = "MASTER";
                    row.RssiForeground = new SolidColorBrush(Color.FromRgb(255, 215, 0));
                    row.RssiTooltip = "ESP32 Bridge (ESP-NOW Master Node)";
                    row.CanBeep = false;
                }
                else // Pedals (0=Clutch, 1=Brake, 2=Throttle)
                {
                    bool isCom = Plugin._serialPort != null && Plugin._serialPort.Length > idx &&
                                 Plugin._serialPort[idx] != null && Plugin._serialPort[idx].IsOpen;

                    if (isCom)
                    {
                        row.UsbStatusText = $"{Plugin._serialPort[idx].PortName} Online";
                        row.UsbStatusForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                        row.UsbStatusBrush = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                    }
                    else
                    {
                        row.UsbStatusText = "No USB COM";
                        row.UsbStatusForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                        row.UsbStatusBrush = new SolidColorBrush(Color.FromRgb(100, 100, 100));
                    }

                    // Live wireless status / RSSI
                    int rssi = (Plugin._calculations != null && Plugin._calculations.rssi != null && Plugin._calculations.rssi.Length > idx)
                               ? Plugin._calculations.rssi[idx] : 0;
                    WirelessConnectStateEnum state = (Plugin._calculations != null && Plugin._calculations.pedalWirelessStatus != null && Plugin._calculations.pedalWirelessStatus.Length > idx)
                                                    ? Plugin._calculations.pedalWirelessStatus[idx] : WirelessConnectStateEnum.PEDAL_DISCONNECT;

                    UpdateRowRssi(row, rssi, state);
                }

                // Update Channel badge styling (match vs mismatch)
                if (row.ChannelNumber > 0)
                {
                    row.ChannelDisplay = $"Ch {row.ChannelNumber}";
                    if (row.ChannelNumber == selectedCh)
                    {
                        row.ChannelForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                        row.ChannelBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 0, 230, 118));
                        row.ChannelBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 0, 230, 118));
                    }
                    else
                    {
                        row.ChannelForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38));
                        row.ChannelBadgeBackground = new SolidColorBrush(Color.FromArgb(34, 255, 167, 38));
                        row.ChannelBadgeBorder = new SolidColorBrush(Color.FromArgb(68, 255, 167, 38));
                    }
                }
                else
                {
                    row.ChannelDisplay = "--";
                    row.ChannelForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                    row.ChannelBadgeBackground = new SolidColorBrush(Color.FromArgb(20, 255, 255, 255));
                    row.ChannelBadgeBorder = new SolidColorBrush(Color.FromArgb(40, 255, 255, 255));
                }
            }
        }

        private void UpdateRowRssi(WirelessNodeRow row, int rssi, WirelessConnectStateEnum state)
        {
            if (state == WirelessConnectStateEnum.PEDAL_DISCONNECT || rssi >= 0 || rssi < -120)
            {
                row.RssiDisplay = "Offline";
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(136, 136, 136));
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(80, 80, 80));
                row.RssiTooltip = "Disconnected / No RF signal";
                return;
            }

            row.RssiDisplay = $"{rssi} dBm";

            if (rssi >= -65)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(0, 230, 118)); // Green
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(0, 230, 118));
                row.RssiTooltip = $"Signal: Excellent ({rssi} dBm)";
            }
            else if (rssi >= -75)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(0, 229, 255)); // Cyan
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(0, 229, 255));
                row.RssiTooltip = $"Signal: Good ({rssi} dBm)";
            }
            else if (rssi >= -85)
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(255, 167, 38)); // Orange
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(255, 167, 38));
                row.RssiTooltip = $"Signal: Fair ({rssi} dBm)";
            }
            else
            {
                row.RssiForeground = new SolidColorBrush(Color.FromRgb(255, 82, 82)); // Red
                row.RssiBarBrush = new SolidColorBrush(Color.FromRgb(255, 82, 82));
                row.RssiTooltip = $"Signal: Weak ({rssi} dBm)";
            }
        }

        public byte GetSelectedWifiChannel()
        {
            if (combo_select_channel?.SelectedItem is ComboBoxItem item &&
                byte.TryParse(item.Tag?.ToString(), out byte ch) && ch >= 1 && ch <= 13)
            {
                return ch;
            }
            return 11;
        }

        public void SetSelectedWifiChannel(byte channel)
        {
            if (combo_select_channel == null) return;
            foreach (ComboBoxItem item in combo_select_channel.Items)
            {
                if (item.Tag?.ToString() == channel.ToString())
                {
                    combo_select_channel.SelectedItem = item;
                    break;
                }
            }
        }

        private async void btn_autodetect_usb_Click(object sender, RoutedEventArgs e)
        {
            await QueryAndAutoDetectDevicesAsync(false);
        }

        private async void btn_read_eeprom_Click(object sender, RoutedEventArgs e)
        {
            await QueryAndAutoDetectDevicesAsync(true);
        }

        private async Task QueryAndAutoDetectDevicesAsync(bool readEepromOnly)
        {
            if (Plugin == null) return;
            tb_scan_status.Text = readEepromOnly ? "Reading stored MAC table from EEPROM..." : "Querying connected USB devices for hardware MACs...";
            btn_autodetect_usb.IsEnabled = false;
            btn_read_eeprom.IsEnabled = false;

            int detectedCount = 0;

            try
            {
                // Prepare query packet
                DAP_mac_addresses_st queryPacket = new DAP_mac_addresses_st();
                queryPacket.payloadHeader_.startOfFrame0_u8 = 0xAA;
                queryPacket.payloadHeader_.startOfFrame1_u8 = 0x55;
                queryPacket.payloadHeader_.payloadType = (byte)Constants.macAddressesPayload_type;
                queryPacket.payloadHeader_.version = (byte)Constants.macAddressesPayload_version;
                queryPacket.payloadHeader_.storeToEeprom = 0; // Query mode
                queryPacket.payloadFooter_.enfOfFrame0_u8 = 0xAA;
                queryPacket.payloadFooter_.enfOfFrame1_u8 = 0x56;

                byte[] queryBytes;
                unsafe
                {
                    DAP_mac_addresses_st* pStruct = &queryPacket;
                    byte* pBytes = (byte*)pStruct;
                    queryPacket.payloadFooter_.checkSum = Plugin.checksumCalc(pBytes, sizeof(payloadHeader) + sizeof(payloadMacAddresses));
                    queryBytes = Plugin.getBytes_MacAddresses(queryPacket);
                }

                // 1. Send query to Bridge via USB HID
                if (Plugin.BridgeHidService != null && Plugin.BridgeHidService.IsConnected)
                {
                    try
                    {
                        await Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(queryBytes));
                    }
                    catch { }
                }

                // 2. Send query to Bridge via ESPsync_serialPort if open
                if (Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        Plugin.ESPsync_serialPort.DiscardInBuffer();
                        Plugin.ESPsync_serialPort.Write(queryBytes, 0, queryBytes.Length);
                    }
                    catch { }
                }

                // 3. Send query packet over connected pedal COM ports
                for (int p = 0; p < 3; p++)
                {
                    if (Plugin._serialPort != null && Plugin._serialPort.Length > p &&
                        Plugin._serialPort[p] != null && Plugin._serialPort[p].IsOpen)
                    {
                        try
                        {
                            Plugin._serialPort[p].DiscardInBuffer();
                            Plugin._serialPort[p].Write(queryBytes, 0, queryBytes.Length);
                        }
                        catch { }
                    }
                }

                // Give devices 300ms to reply over HID / Serial
                await Task.Delay(300);

                // 4. Update Pedals (0=Clutch, 1=Brake, 2=Throttle)
                for (int i = 0; i < 3; i++)
                {
                    var row = NodeRows.FirstOrDefault(r => r.NodeIndex == i);
                    if (row != null)
                    {
                        bool found = false;
                        if (Plugin.Settings?.AssignedPedalMac != null &&
                            Plugin.Settings.AssignedPedalMac.Length > i)
                        {
                            string m = Plugin.Settings.AssignedPedalMac[i];
                            if (!string.IsNullOrWhiteSpace(m) && m != "--" && m != "00:00:00:00:00:00")
                            {
                                row.MacAddress = m;
                                detectedCount++;
                                found = true;
                            }
                        }

                        if (!found && Plugin._calculations?.unassignedPedalMacaddress != null &&
                            Plugin._calculations.unassignedPedalMacaddress.Length > i &&
                            Plugin._calculations.unassignedPedalMacaddress[i] != null)
                        {
                            byte[] mac = Plugin._calculations.unassignedPedalMacaddress[i];
                            if (mac.Any(b => b != 0))
                            {
                                row.MacAddress = string.Join(":", mac.Select(b => b.ToString("X2")));
                                detectedCount++;
                            }
                        }
                    }
                }

                // 5. Update Bridge (Node 3)
                var bridgeRow = NodeRows.FirstOrDefault(r => r.NodeIndex == 3);
                if (bridgeRow != null)
                {
                    if (Plugin.Settings?.AssignedPedalMac != null &&
                        Plugin.Settings.AssignedPedalMac.Length > 3)
                    {
                        string bMac = Plugin.Settings.AssignedPedalMac[3];
                        if (!string.IsNullOrWhiteSpace(bMac) && bMac != "--" && bMac != "00:00:00:00:00:00")
                        {
                            bridgeRow.MacAddress = bMac;
                            detectedCount++;
                        }
                    }
                }

                byte activeCh = (Plugin.Settings != null && Plugin.Settings.ActiveWifiChannel >= 1 && Plugin.Settings.ActiveWifiChannel <= 14) 
                                ? Plugin.Settings.ActiveWifiChannel 
                                : GetSelectedWifiChannel();

                foreach (var row in NodeRows)
                {
                    if (!string.IsNullOrWhiteSpace(row.MacAddress) && row.MacAddress != "--" && row.MacAddress != "00:00:00:00:00:00")
                    {
                        if (row.ChannelNumber == 0) row.ChannelNumber = activeCh;
                    }
                }

                UpdateLiveStatus();
                tb_scan_status.Text = $"Device query complete. ({detectedCount} MAC addresses ready)";
            }
            catch (Exception ex)
            {
                tb_scan_status.Text = $"Query error: {ex.Message}";
            }
            finally
            {
                btn_autodetect_usb.IsEnabled = true;
                btn_read_eeprom.IsEnabled = true;
            }
        }

        private void btn_open_docs_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string pluginDir = AppDomain.CurrentDomain.BaseDirectory;
                string docsPath = System.IO.Path.GetFullPath(System.IO.Path.Combine(pluginDir, "..", "..", "docs", "pedal_pairing_and_assignment_guide.md"));
                if (System.IO.File.Exists(docsPath))
                {
                    System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = docsPath,
                        UseShellExecute = true
                    });
                }
                else
                {
                    System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo
                    {
                        FileName = "https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/tree/master/docs",
                        UseShellExecute = true
                    });
                }
            }
            catch (Exception ex)
            {
                System.Windows.MessageBox.Show($"Could not open documentation: {ex.Message}", "Info", MessageBoxButton.OK, MessageBoxImage.Information);
            }
        }

        private async void btn_write_all_usb_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin == null) return;
            byte targetChannel = GetSelectedWifiChannel();
            tb_scan_status.Text = $"Packaging DAP_mac_addresses_st_t (Channel {targetChannel}) and writing to EEPROM...";
            btn_write_all_usb.IsEnabled = false;

            try
            {
                DAP_mac_addresses_st packet = new DAP_mac_addresses_st();
                packet.payloadHeader_.startOfFrame0_u8 = 0xAA;
                packet.payloadHeader_.startOfFrame1_u8 = 0x55;
                packet.payloadHeader_.payloadType = (byte)Constants.macAddressesPayload_type;
                packet.payloadHeader_.version = (byte)Constants.macAddressesPayload_version;
                packet.payloadHeader_.storeToEeprom = 1; // Commit to EEPROM
                packet.payloadHeader_.PedalTag = 0;

                packet.payloadMacAddresses_.Initialize();
                packet.payloadMacAddresses_.wifiChannel_u8 = targetChannel;

                unsafe
                {
                    foreach (var row in NodeRows)
                    {
                        int idx = row.NodeIndex;
                        if (idx >= 0 && idx < 4)
                        {
                            bool valid = packet.payloadMacAddresses_.SetMacAddressFromString(idx, row.MacAddress);
                            packet.payloadMacAddresses_.assignmentState_au8[idx] = (byte)(valid ? 1 : 0);
                            if (valid)
                            {
                                row.ChannelNumber = targetChannel;
                            }
                        }
                    }
                }

                packet.payloadFooter_.enfOfFrame0_u8 = 0xAA;
                packet.payloadFooter_.enfOfFrame1_u8 = 0x56;

                byte[] rawPacket;
                unsafe
                {
                    DAP_mac_addresses_st* pStruct = &packet;
                    byte* pBytes = (byte*)pStruct;
                    packet.payloadFooter_.checkSum = Plugin.checksumCalc(pBytes, sizeof(payloadHeader) + sizeof(payloadMacAddresses));
                    rawPacket = Plugin.getBytes_MacAddresses(packet);
                }

                int successDevices = 0;

                // 1. Send to Bridge via HID
                if (Plugin.BridgeHidService != null && Plugin.BridgeHidService.IsConnected)
                {
                    await Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(rawPacket));
                    successDevices++;
                }
                else if (Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen)
                {
                    Plugin.ESPsync_serialPort.DiscardInBuffer();
                    Plugin.ESPsync_serialPort.Write(rawPacket, 0, rawPacket.Length);
                    successDevices++;
                }

                // 2. Send to Pedals via USB COM ports
                for (int p = 0; p < 3; p++)
                {
                    if (Plugin._serialPort != null && Plugin._serialPort.Length > p &&
                        Plugin._serialPort[p] != null && Plugin._serialPort[p].IsOpen)
                    {
                        try
                        {
                            Plugin._serialPort[p].DiscardInBuffer();
                            Plugin._serialPort[p].Write(rawPacket, 0, rawPacket.Length);
                            successDevices++;
                        }
                        catch (Exception ex)
                        {
                            SimHub.Logging.Current.Error($"Failed sending MAC table to Pedal #{p}: {ex.Message}");
                        }
                    }
                }

                // Save MACs to Plugin settings (0..2=Pedals, 3=Bridge)
                if (Plugin.Settings.AssignedPedalMac == null || Plugin.Settings.AssignedPedalMac.Length < 4)
                {
                    string[] newMacs = new string[4];
                    if (Plugin.Settings.AssignedPedalMac != null)
                    {
                        Array.Copy(Plugin.Settings.AssignedPedalMac, newMacs, Math.Min(Plugin.Settings.AssignedPedalMac.Length, 4));
                    }
                    Plugin.Settings.AssignedPedalMac = newMacs;
                }

                for (int i = 0; i < 4; i++)
                {
                    var row = NodeRows.FirstOrDefault(r => r.NodeIndex == i);
                    if (row != null && row.MacAddress != "--" && !string.IsNullOrWhiteSpace(row.MacAddress))
                    {
                        Plugin.Settings.AssignedPedalMac[i] = row.MacAddress;
                    }
                }

                UpdateLiveStatus();
                tb_scan_status.Text = $"Success! Synchronized MAC table & Channel {targetChannel} to {successDevices} USB device(s) and saved to EEPROM.";
                ParentUI?.ToastNotification("Wireless Sync", $"Configured {successDevices} device(s) on Channel {targetChannel}.");
            }
            catch (Exception ex)
            {
                tb_scan_status.Text = $"Write failed: {ex.Message}";
                ParentUI?.ToastNotification("Sync Error", ex.Message);
            }
            finally
            {
                btn_write_all_usb.IsEnabled = true;
            }
        }

        private async void btn_clear_mac_usb_Click(object sender, RoutedEventArgs e)
        {
            if (Plugin == null) return;

            var confirm = System.Windows.MessageBox.Show(
                "This erases the MAC address table (Bridge + all 3 Pedals) from EEPROM on every USB-connected device, and clears it here. " +
                "Devices will need to be re-paired (Auto-Detect + Sync to All Devices) afterwards.\n\nContinue?",
                "Clear MAC List",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning);
            if (confirm != MessageBoxResult.Yes) return;

            byte keepChannel = GetSelectedWifiChannel();
            tb_scan_status.Text = "Clearing MAC address table on all USB-connected devices...";
            btn_clear_mac_usb.IsEnabled = false;

            try
            {
                DAP_mac_addresses_st packet = new DAP_mac_addresses_st();
                packet.payloadHeader_.startOfFrame0_u8 = 0xAA;
                packet.payloadHeader_.startOfFrame1_u8 = 0x55;
                packet.payloadHeader_.payloadType = (byte)Constants.macAddressesPayload_type;
                packet.payloadHeader_.version = (byte)Constants.macAddressesPayload_version;
                packet.payloadHeader_.storeToEeprom = 1; // Commit - all-zero MACs get stored & applied
                packet.payloadHeader_.PedalTag = 0;

                // All-zero MAC/assignment table; keep the currently selected
                // channel so clearing pairing doesn't also knock devices onto
                // a different Wi-Fi channel.
                packet.payloadMacAddresses_.Initialize();
                packet.payloadMacAddresses_.wifiChannel_u8 = keepChannel;

                byte[] rawPacket;
                unsafe
                {
                    DAP_mac_addresses_st* pStruct = &packet;
                    byte* pBytes = (byte*)pStruct;
                    packet.payloadFooter_.checkSum = Plugin.checksumCalc(pBytes, sizeof(payloadHeader) + sizeof(payloadMacAddresses));
                    packet.payloadFooter_.enfOfFrame0_u8 = 0xAA;
                    packet.payloadFooter_.enfOfFrame1_u8 = 0x56;
                    rawPacket = Plugin.getBytes_MacAddresses(packet);
                }

                int clearedDevices = 0;

                // 1. Bridge via HID
                if (Plugin.BridgeHidService != null && Plugin.BridgeHidService.IsConnected)
                {
                    try
                    {
                        await Task.Run(() => Plugin.BridgeHidService.SendLargeDataAsync(rawPacket));
                        clearedDevices++;
                    }
                    catch { }
                }
                // 2. Bridge via ESPsync_serialPort
                else if (Plugin.ESPsync_serialPort != null && Plugin.ESPsync_serialPort.IsOpen)
                {
                    try
                    {
                        Plugin.ESPsync_serialPort.DiscardInBuffer();
                        Plugin.ESPsync_serialPort.Write(rawPacket, 0, rawPacket.Length);
                        clearedDevices++;
                    }
                    catch { }
                }

                // 3. Pedals via USB COM ports
                for (int p = 0; p < 3; p++)
                {
                    if (Plugin._serialPort != null && Plugin._serialPort.Length > p &&
                        Plugin._serialPort[p] != null && Plugin._serialPort[p].IsOpen)
                    {
                        try
                        {
                            Plugin._serialPort[p].DiscardInBuffer();
                            Plugin._serialPort[p].Write(rawPacket, 0, rawPacket.Length);
                            clearedDevices++;
                        }
                        catch (Exception ex)
                        {
                            SimHub.Logging.Current.Error($"Failed clearing MAC table on Pedal #{p}: {ex.Message}");
                        }
                    }
                }

                // Clear locally: table rows + saved settings
                foreach (var row in NodeRows)
                {
                    row.MacAddress = "--";
                    row.ChannelNumber = 0;
                }
                if (Plugin.Settings.AssignedPedalMac != null)
                {
                    for (int i = 0; i < Plugin.Settings.AssignedPedalMac.Length; i++)
                    {
                        Plugin.Settings.AssignedPedalMac[i] = "--";
                    }
                }

                UpdateLiveStatus();
                tb_scan_status.Text = $"MAC table cleared on {clearedDevices} USB device(s). Re-pair via Auto-Detect + Sync to All Devices.";
                ParentUI?.ToastNotification("Wireless", $"MAC table cleared on {clearedDevices} device(s).");
            }
            catch (Exception ex)
            {
                tb_scan_status.Text = $"Clear failed: {ex.Message}";
                ParentUI?.ToastNotification("Clear Error", ex.Message);
            }
            finally
            {
                btn_clear_mac_usb.IsEnabled = true;
            }
        }

        private void btn_beep_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && btn.Tag is WirelessNodeRow row)
            {
                int nodeIdx = row.NodeIndex;
                if (nodeIdx >= 0 && nodeIdx < 3 && Plugin != null)
                {
                    DAP_action_st action = default;
                    action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                    action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                    action.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.ASSIGNMENT_CHECK_BEEP;
                    Plugin.SendPedalAction(action, (byte)nodeIdx);
                    tb_scan_status.Text = $"Sent identify beep to {row.RoleName}.";
                }
            }
        }
    }
}
