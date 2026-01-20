
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

namespace User.PluginSdkDemo
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
            private SynchronizationContext _uiContext;
            public event Action<byte[]> OnDataReceived;
            public event Action OnDeviceDisconnected;
            public bool IsConnected;
            public bool IsDeviceAttached;
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
                bool exists = DeviceList.Local.GetHidDevices(_vid, _pid).Any();
                if (exists)
                {
                    IsDeviceAttached = true;
                    Connect(_vid, _pid, _targetUsagePage);
                }
                else
                {
                    IsConnected = false;
                    Disconnect();
                }
            }

            public bool Connect(int vid, int pid, ushort targetUsagePage)
            {
                //_uiContext = SynchronizationContext.Current;

                var device = GetVendorPageDevice(vid, pid, targetUsagePage);
                if (device == null) return false;

                if (device.TryOpen(out _stream))
                {
                    _device = device;
                    _cancelSource = new CancellationTokenSource();
                    IsConnected = true;
                    
                    Task.Factory.StartNew(
                        ReadLoop,
                        _cancelSource.Token,
                        TaskCreationOptions.LongRunning,
                        TaskScheduler.Default
                    );
                    
                    

                    return true;
                }
                return false;
            }

            private void ReadLoop()
            {
                byte[] buffer = new byte[_device.GetMaxInputReportLength()];

                while (!_cancelSource.IsCancellationRequested && _stream != null)
                {
                    try
                    {
                        int count = _stream.Read(buffer, 0, buffer.Length);
                        if (count > 0)
                        {
                            byte[] actualData = new byte[count];
                            Array.Copy(buffer, actualData, count);
                            OnDataReceived?.Invoke(actualData);
                            //OnDataReceived?.Invoke(actualData);
                        }
                    }
                    catch (Exception)
                    {
                        /*
                        if (_uiContext != null) _uiContext.Post(_ => OnDeviceDisconnected?.Invoke(), null);
                        else OnDeviceDisconnected?.Invoke();
                        break;
                        */
                    }
                }
            }
            public async Task SendLargeDataAsync(byte[] data)
            {
                if (!IsConnected) return;

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
                    await Task.Delay(1);
                }
            }

            public void Write(byte[] data)
            {
                //_stream.WriteTimeout=3;
                if (_stream != null)
                {
                    _stream.WriteTimeout = 3;
                    
                    try {
                        _stream.Write(data);
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
                _cancelSource?.Cancel();
                _stream?.Dispose();
                _stream = null;
                IsConnected = false;
            }

            public void Dispose()
            {
                Disconnect();
            }
        }

        
    }
}
