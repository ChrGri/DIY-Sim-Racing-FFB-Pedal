
using HidSharp;
using HidSharp.Reports;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using static System.Windows.Forms.VisualStyles.VisualStyleElement.TrackBar;
//using HidLibrary;

namespace DiyFfbPedal
{

    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {

        public class HidDeviceController : IDisposable
        {

            private const int ReportLength = 64;
            private const byte ReportId_INPUT = 0x02;
            private const byte ReportId_OUTPUT = 0x03;
            private const byte PKT_TYPE_START = 0x01;
            private const byte PKT_TYPE_CONT = 0x02;
            private const byte PKT_TYPE_END = 0x03;
            private const int HeaderOffset = 4;
            private const int PayloadSize = ReportLength - HeaderOffset; // 64 - 3 = 60 bytes data
            private HidDevice _device;
            private HidStream _stream;
            private CancellationTokenSource _cancelSource;
            private int _vid;
            private int _pid;
            private ushort _targetUsagePage;
            public event Action<byte[]> OnDataReceived;
            #pragma warning disable CS0067
            public event Action OnDeviceDisconnected;
#pragma warning restore CS0067

            public volatile bool IsConnected;
            public volatile bool IsDeviceAttached;
            // Serializes Connect/Disconnect: DeviceList.Changed fires on a HidSharp thread for
            // every USB change on the PC, concurrently with the UI calling in.
            private readonly object _connectionLock = new object();
            private const int ReadErrorBackoffMs = 50;
            private const int MaxConsecutiveReadErrors = 20;
            public HidDeviceController(int VID, int PID, ushort targetUsagePage)
            {
                _vid = VID;
                _pid = PID;
                _targetUsagePage = targetUsagePage;
                //_uiContext = SynchronizationContext.Current;
                DeviceList.Local.Changed += OnDeviceListChanged;
                _targetUsagePage = targetUsagePage;
                Connect(_vid, _pid, _targetUsagePage);
            }
            public static HidDevice GetVendorPageDevice(int vid, int pid, ushort targetUsagePage)
            {
                var candidates = DeviceList.Local.GetHidDevices(vid, pid);
                return candidates.FirstOrDefault(device =>
                {
                    try
                    {
                        ReportDescriptor desc = device.GetReportDescriptor();
                        foreach (var item in desc.DeviceItems)
                        {
                            foreach (uint usage in item.Usages.GetAllValues())
                            {
                                uint page = (usage >> 16) & 0xFFFF;

                                if (page == targetUsagePage)
                                {
                                    return true;
                                }
                            }
                        }
                    }
                    catch
                    {
                    }

                    return false;
                });
            }
            private void OnDeviceListChanged(object sender, DeviceListChangedEventArgs e)
            {
                // Fires for any USB change on the PC, not just the bridge. Only (re)connect when the
                // bridge's vendor interface is present; Connect() is a no-op if already connected.
                bool exists = GetVendorPageDevice(_vid, _pid, _targetUsagePage) != null;
                IsDeviceAttached = exists;
                if (exists)
                {
                    Connect(_vid, _pid, _targetUsagePage);
                }
                else
                {
                    Disconnect();
                }
            }

            public bool Connect(int vid, int pid, ushort targetUsagePage)
            {
                lock (_connectionLock)
                {
                    // Previously every DeviceList.Changed event opened another stream and started
                    // another ReadLoop without stopping the old one. The piled-up loops contended on
                    // the same stream and burned several CPU cores.
                    if (IsConnected && _stream != null) return true;

                    CloseStream();

                    var device = GetVendorPageDevice(vid, pid, targetUsagePage);
                    if (device == null) return false;

                    if (!device.TryOpen(out HidStream stream)) return false;

                    var cancelSource = new CancellationTokenSource();
                    _device = device;
                    _stream = stream;
                    _cancelSource = cancelSource;
                    IsConnected = true;

                    int maxInputReportLength = device.GetMaxInputReportLength();
                    Task.Factory.StartNew(
                        () => ReadLoop(stream, maxInputReportLength, cancelSource.Token),
                        cancelSource.Token,
                        TaskCreationOptions.LongRunning,
                        TaskScheduler.Default
                    );

                    return true;
                }
            }

            // Each loop owns its stream and token, so a replaced or disposed connection always ends
            // its loop instead of leaving it running against whatever _stream currently holds.
            private void ReadLoop(HidStream stream, int maxInputReportLength, CancellationToken token)
            {
                byte[] buffer = new byte[maxInputReportLength];
                int consecutiveErrors = 0;

                while (!token.IsCancellationRequested)
                {
                    int count;
                    try
                    {
                        count = stream.Read(buffer, 0, buffer.Length);
                        consecutiveErrors = 0;
                    }
                    catch (TimeoutException)
                    {
                        // No report within ReadTimeout (idle bridge) - normal, keep waiting.
                        continue;
                    }
                    catch (ObjectDisposedException)
                    {
                        break;
                    }
                    catch (Exception)
                    {
                        if (token.IsCancellationRequested) break;

                        // Back off instead of spinning; give up on a persistently broken handle so
                        // the next device-list change can open a fresh one.
                        if (++consecutiveErrors >= MaxConsecutiveReadErrors)
                        {
                            lock (_connectionLock)
                            {
                                if (ReferenceEquals(_stream, stream)) CloseStream();
                            }
                            break;
                        }
                        Thread.Sleep(ReadErrorBackoffMs);
                        continue;
                    }

                    if (count > 0)
                    {
                        byte[] actualData = new byte[count];
                        Array.Copy(buffer, actualData, count);
                        try
                        {
                            OnDataReceived?.Invoke(actualData);
                        }
                        catch (Exception ex)
                        {
                            SimHub.Logging.Current.Error($"HID receive handler error: {ex.Message}");
                        }
                    }
                }
            }
            private static readonly System.Threading.SemaphoreSlim _sendLock = new System.Threading.SemaphoreSlim(1, 1);
            public async Task SendLargeDataAsync(byte[] data)
            {
                if (!IsConnected) return;

                await _sendLock.WaitAsync();
                try
                {
                    int totalLen = data.Length;
                    int offset = 0;
                    while (offset < totalLen)
                    {
                        byte[] buffer = new byte[ReportLength];
                        int chunkLen = Math.Min(PayloadSize, totalLen - offset);

                        byte type;
                        
                        if (offset == 0)
                            type = PKT_TYPE_START;
                        else
                            type = PKT_TYPE_CONT; 

                        buffer[0] = ReportId_OUTPUT;
                        buffer[1] = type;
                        buffer[2] = (byte)totalLen; 
                        buffer[3] = (byte)chunkLen;

                        Array.Copy(data, offset, buffer, HeaderOffset, chunkLen);
                        Write(buffer);

                        offset += chunkLen;
                        // pace multi-chunk transfers only; a trailing delay just stalls the next send
                        if (offset < totalLen) await Task.Delay(2);
                    }
                }
                finally
                {
                    _sendLock.Release();
                }
            }

            public void Write(byte[] data)
            {
                //_stream.WriteTimeout=3;
                HidStream stream = _stream;
                if (stream != null)
                {
                    try {
                        stream.WriteTimeout = 3;
                        stream.Write(data);
                    }
                    catch (Exception ex)
                    {
                        SimHub.Logging.Current.Error($"HID error: {ex.Message}");
                        //throw;
                    }
                    
                }
            }

            public void Disconnect()
            {
                lock (_connectionLock)
                {
                    CloseStream();
                }
            }

            // Caller must hold _connectionLock.
            private void CloseStream()
            {
                IsConnected = false;
                _cancelSource?.Cancel();
                _cancelSource = null;
                try
                {
                    _stream?.Dispose();
                }
                catch (Exception)
                {
                }
                _stream = null;
            }

            public void Dispose()
            {
                DeviceList.Local.Changed -= OnDeviceListChanged;
                Disconnect();
            }
        }

        
    }
}
