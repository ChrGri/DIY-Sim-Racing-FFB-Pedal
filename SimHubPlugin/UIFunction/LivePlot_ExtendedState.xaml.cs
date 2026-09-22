using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    public partial class LivePlot_ExtendedState : UserControl
    {
        public class SignalDef
        {
            public string Id { get; set; }
            public string Name { get; set; }
            public string Subsystem { get; set; }
            public string Unit { get; set; }
            public Color DefaultColor { get; set; }
            public Func<payloadPedalState_Extended, double> Getter { get; set; }
            public string Format { get; set; } = "F2";
        }

        private struct TelemetryPoint
        {
            public double TimeSec;
            public payloadPedalState_Extended State;
        }

        // All supported signals from payloadPedalState_Extended
        private static readonly List<SignalDef> AllSignals = new List<SignalDef>
        {
            // Servo Registers
            new SignalDef { Id = "servo_pos_target", Name = "Servo Target Pos", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0xD7, 0x40), Getter = s => s.servoPositionTarget_i32, Format = "F0" },
            new SignalDef { Id = "servo_pos_fb", Name = "Servo Feedback Pos", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0x52, 0x52), Getter = s => s.servoPositionFeedback_i32, Format = "F0" },
            new SignalDef { Id = "servo_pos_err", Name = "Servo Pos Error", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0x40, 0x81), Getter = s => s.servoPositionError_i16, Format = "F0" },
            new SignalDef { Id = "servo_voltage", Name = "Servo Voltage", Subsystem = "Servo Registers", Unit = "V", DefaultColor = Color.FromRgb(0xFF, 0xFF, 0x00), Getter = s => s.servoVoltage0p1V_i16 / 10.0, Format = "F1" },
            new SignalDef { Id = "servo_current", Name = "Servo Current", Subsystem = "Servo Registers", Unit = "%", DefaultColor = Color.FromRgb(0x69, 0xF0, 0xAE), Getter = s => s.servoCurrentPercent_i16, Format = "F0" },
            new SignalDef { Id = "servo_cycle", Name = "Servo Cycle Count", Subsystem = "Servo Registers", Unit = "cts", DefaultColor = Color.FromRgb(0xB0, 0xBE, 0xC5), Getter = s => s.servoStateCycleCount_u32, Format = "F0" },

            // ESP32 & Physical Values
            new SignalDef { Id = "force_filtered", Name = "Filtered Force", Subsystem = "ESP32 & Forces", Unit = "kg", DefaultColor = Color.FromRgb(0x00, 0xE5, 0xFF), Getter = s => s.pedalForceFiltered_fl32, Format = "F2" },
            new SignalDef { Id = "force_raw", Name = "Raw Force", Subsystem = "ESP32 & Forces", Unit = "kg", DefaultColor = Color.FromRgb(0x40, 0xC4, 0xFF), Getter = s => s.pedalForceRaw_fl32, Format = "F2" },
            new SignalDef { Id = "force_vel_est", Name = "Force Velocity Est", Subsystem = "ESP32 & Forces", Unit = "kg/s", DefaultColor = Color.FromRgb(0x76, 0xFF, 0x03), Getter = s => s.forceVelEst_fl32, Format = "F2" },
            new SignalDef { Id = "esp_target_pos", Name = "ESP Target Pos", Subsystem = "ESP32 & Forces", Unit = "cts", DefaultColor = Color.FromRgb(0xFF, 0xAB, 0x00), Getter = s => s.targetPosition_i32, Format = "F0" },
            new SignalDef { Id = "speed_hz", Name = "Current Speed", Subsystem = "ESP32 & Forces", Unit = "Hz", DefaultColor = Color.FromRgb(0xFF, 0x6D, 0x00), Getter = s => s.currentSpeedInHz_i32, Format = "F0" },
            new SignalDef { Id = "brake_resistor", Name = "Brake Resistor", Subsystem = "ESP32 & Forces", Unit = "", DefaultColor = Color.FromRgb(0xEA, 0x80, 0xFC), Getter = s => s.brakeResistorState_b, Format = "F0" },
            new SignalDef { Id = "osc_monitor", Name = "Oscillation Monitor", Subsystem = "ESP32 & Forces", Unit = "", DefaultColor = Color.FromRgb(0xD5, 0x00, 0xF9), Getter = s => s.oscillationMonitorValue_u8, Format = "F0" },
            new SignalDef { Id = "esp_cycle", Name = "ESP Cycle Count", Subsystem = "ESP32 & Forces", Unit = "cts", DefaultColor = Color.FromRgb(0xCF, 0xD8, 0xDC), Getter = s => s.cycleCount_u32, Format = "F0" },

            // Admittance Model
            new SignalDef { Id = "adm_expected_force", Name = "Admittance Exp Force", Subsystem = "Admittance Model", Unit = "N", DefaultColor = Color.FromRgb(0xE0, 0x40, 0xFB), Getter = s => s.admittance_expectedForce_N, Format = "F2" },
            new SignalDef { Id = "adm_is_oscillating", Name = "Is Oscillating", Subsystem = "Admittance Model", Unit = "", DefaultColor = Color.FromRgb(0xFF, 0x17, 0x44), Getter = s => s.admittance_isOscillating, Format = "F0" },
            new SignalDef { Id = "adm_psi", Name = "Admittance Psi", Subsystem = "Admittance Model", Unit = "N", DefaultColor = Color.FromRgb(0xAA, 0x00, 0xFF), Getter = s => s.admittance_admittancePsi_N, Format = "F2" },
            new SignalDef { Id = "adm_mass", Name = "Virtual Mass", Subsystem = "Admittance Model", Unit = "kg", DefaultColor = Color.FromRgb(0x00, 0xB0, 0xFF), Getter = s => s.admittance_virtualMass_kg, Format = "F2" },
            new SignalDef { Id = "adm_damping", Name = "Virtual Damping", Subsystem = "Admittance Model", Unit = "Ns/m", DefaultColor = Color.FromRgb(0x1D, 0xE9, 0xB6), Getter = s => s.admittance_virtualDamping_Ns_m, Format = "F2" },
            new SignalDef { Id = "adm_pos", Name = "Virtual Position", Subsystem = "Admittance Model", Unit = "m", DefaultColor = Color.FromRgb(0xAE, 0xEA, 0x00), Getter = s => s.admittance_virtualPosition_m, Format = "F4" },
            new SignalDef { Id = "adm_vel", Name = "Virtual Velocity", Subsystem = "Admittance Model", Unit = "m/s", DefaultColor = Color.FromRgb(0x00, 0xE6, 0x76), Getter = s => s.admittance_virtualVelocity_mps, Format = "F3" },
            new SignalDef { Id = "adm_acc", Name = "Virtual Accel", Subsystem = "Admittance Model", Unit = "m/s\u00B2", DefaultColor = Color.FromRgb(0x7C, 0x4D, 0xFF), Getter = s => s.admittance_virtualAcceleration_mps2, Format = "F2" }
        };

        // Active signals list & hardware-accelerated Path shapes
        private readonly HashSet<string> _activeSignalIds = new HashSet<string>();
        private readonly Dictionary<string, Path> _signalPaths = new Dictionary<string, Path>();
        private readonly Dictionary<string, (TextBlock Val, TextBlock Range)> _legendBindings = new Dictionary<string, (TextBlock, TextBlock)>();

        // High performance telemetry buffer (contiguous value type array)
        private readonly object _dataLock = new object();
        private readonly List<TelemetryPoint> _points = new List<TelemetryPoint>(15000);
        private const int MAX_BUFFER_POINTS = 12000; // 60s at 200 Hz

        // Monotonic ESP32 hardware time tracking
        private bool _firstPacket = true;
        private uint _lastEspTimeUs = 0;
        private uint _lastSampledEspTimeUs = 0;
        private double _unwrappedTimeSec = 0.0;
        private double _latestTimeSec = 0.0;
        private double _pauseTimeSec = 0.0;

        // Render timer & reference state
        private DispatcherTimer _renderTimer;
        private double _windowSeconds = 5.0;
        private bool _isPaused = false;
        private bool _autoScale = true;
        private int _selectedPedal = 1; // Default to Brake (1)

        // Packet rate calculation
        private int _packetCounter = 0;
        private DateTime _lastRateCheck = DateTime.UtcNow;
        private double _packetRate = 0.0;
        private DateTime _lastPacketTime = DateTime.MinValue;

        // Cached visible range and timestamps for instant O(1) hover lookups
        private double _lastRenderRefTime = 0.0;
        private double _lastRenderViewStart = 0.0;
        private double _lastRenderViewEnd = 0.0;
        private int _cachedStartIdx = 0;
        private int _cachedEndIdx = 0;

        // Reusable point list to avoid any GC heap allocations inside render loop
        private readonly List<Point> _renderPoints = new List<Point>(1000);

        // Pre-allocated gridline visuals (never destroyed/reallocated on tick)
        private bool _gridInitialized = false;
        private readonly Line[] _gridHLines = new Line[3];
        private readonly Line[] _gridVLines = new Line[4];
        private readonly TextBlock[] _gridTimeLabels = new TextBlock[4];

        // Zoom & Pan state
        private double _yViewMin = 0.0;
        private double _yViewMax = 1.0;
        private double _xViewOffsetSec = 0.0;
        private bool _isZoomed = false;
        private bool _isDraggingZoom = false;
        private Point _dragStart = new Point(0, 0);
        private bool _isBoxZoomMode = false;
        private bool _isPanning = false;
        private Point _panStartMouse = new Point(0, 0);
        private double _panStartXOffset = 0.0;
        private double _panStartYMin = 0.0;
        private double _panStartYMax = 1.0;

        // References to SimHub plugin & parent UI
        public DIY_FFB_Pedal Plugin { get; set; }
        public DIYFFBPedalControlUI ParentUI { get; set; }

        public LivePlot_ExtendedState()
        {
            InitializeComponent();
            InitControls();
            InitContextMenu();
            InitRenderTimer();

            // Default active signals: Filtered Force & Feedback Position
            _activeSignalIds.Add("force_filtered");
            _activeSignalIds.Add("servo_pos_fb");
            RebuildPolylinesAndLegends();
        }

        public void SetReferences(DIY_FFB_Pedal plugin, DIYFFBPedalControlUI parentUI)
        {
            Plugin = plugin;
            ParentUI = parentUI;

            if (Plugin != null && Plugin._calculations != null)
            {
                Plugin._calculations.OnExtendedStateReceived -= OnPacketReceived;
                Plugin._calculations.OnExtendedStateReceived += OnPacketReceived;
            }

            UpdatePedalButtonStyles();
        }

        public void OnTabSelected(bool isSelected)
        {
            if (isSelected)
            {
                EnsureStreaming(_selectedPedal, true);
                if (_renderTimer != null && !_renderTimer.IsEnabled) _renderTimer.Start();
            }
            else
            {
                EnsureStreaming(_selectedPedal, false);
                if (_renderTimer != null && _renderTimer.IsEnabled) _renderTimer.Stop();
            }
        }

        private void EnsureStreaming(int pedalIndex, bool enable)
        {
            if (ParentUI == null || ParentUI.Plugin == null) return;
            try
            {
                bool isRudderAssigned = ParentUI.Plugin.Rudder_status &&
                    ParentUI.Plugin.Rudder_Pedal_idx != null &&
                    (ParentUI.Plugin.Rudder_Pedal_idx[0] == pedalIndex || ParentUI.Plugin.Rudder_Pedal_idx[1] == pedalIndex);

                if (isRudderAssigned)
                {
                    // A rudder-assigned pedal's actual running config comes from
                    // dap_config_st_rudder (sent via RudderParameterLiveUpdate),
                    // not the regular per-pedal dap_config_st[] cache used by the
                    // Pedals tab - that cache goes stale the moment a pedal is
                    // handed to rudder duty. Toggling the flag there and sending
                    // it (the old code path below) would silently overwrite the
                    // pedal's real rudder travel/geometry settings with whatever
                    // pre-rudder config happened to still be cached.
                    byte oldFlags = ParentUI.dap_config_st_rudder.payloadPedalConfig_.debug_flags_0;
                    if (enable)
                    {
                        ParentUI.dap_config_st_rudder.payloadPedalConfig_.debug_flags_0 |= 64;
                    }
                    else if (!ParentUI.Plugin._calculations.dumpPedalToResponseFile[pedalIndex])
                    {
                        ParentUI.dap_config_st_rudder.payloadPedalConfig_.debug_flags_0 =
                            (byte)(ParentUI.dap_config_st_rudder.payloadPedalConfig_.debug_flags_0 & ~64);
                    }
                    if (ParentUI.dap_config_st_rudder.payloadPedalConfig_.debug_flags_0 != oldFlags)
                    {
                        // Shared flag on dap_config_st_rudder, so this streams
                        // extended telemetry from both rudder pedals together
                        // rather than just the one currently selected in Live
                        // Plot - a minor bandwidth cost, but correctness (not
                        // clobbering the live rudder config) matters more here.
                        ParentUI.RudderParameterLiveUpdate();
                    }
                    return;
                }

                var config = ParentUI.dap_config_st[pedalIndex];
                byte oldFlagsRegular = config.payloadPedalConfig_.debug_flags_0;
                if (enable)
                {
                    config.payloadPedalConfig_.debug_flags_0 |= 64;
                }
                else
                {
                    if (!ParentUI.Plugin._calculations.dumpPedalToResponseFile[pedalIndex])
                    {
                        config.payloadPedalConfig_.debug_flags_0 = (byte)(config.payloadPedalConfig_.debug_flags_0 & ~64);
                    }
                }
                if (config.payloadPedalConfig_.debug_flags_0 != oldFlagsRegular)
                {
                    ParentUI.dap_config_st[pedalIndex] = config;
                    ParentUI.Plugin.SendConfigWithoutSaveToEEPROM(config, (byte)pedalIndex);
                }
            }
            catch { }
        }

        private void InitControls()
        {
            cb_time_window.Items.Add("3s");
            cb_time_window.Items.Add("5s");
            cb_time_window.Items.Add("10s");
            cb_time_window.Items.Add("20s");
            cb_time_window.Items.Add("30s");
            cb_time_window.SelectedIndex = 1; // 5s default

            UpdatePedalButtonStyles();
        }

        private void InitContextMenu()
        {
            plot_context_menu.Items.Clear();

            var groups = AllSignals.GroupBy(s => s.Subsystem);
            foreach (var group in groups)
            {
                var subMenu = new MenuItem { Header = group.Key };
                foreach (var sig in group)
                {
                    var item = new MenuItem
                    {
                        Header = sig.Name + (!string.IsNullOrEmpty(sig.Unit) ? $" ({sig.Unit})" : ""),
                        Tag = sig.Id,
                        IsCheckable = true,
                        IsChecked = _activeSignalIds.Contains(sig.Id)
                    };
                    item.Click += MenuItem_Click;
                    subMenu.Items.Add(item);
                }
                plot_context_menu.Items.Add(subMenu);
            }

            plot_context_menu.Items.Add(new Separator());

            var itemReset = new MenuItem { Header = "Reset to Defaults (Force + Pos)" };
            itemReset.Click += (s, e) => ResetToDefaultSignals();
            plot_context_menu.Items.Add(itemReset);

            var itemClear = new MenuItem { Header = "Clear All Signals" };
            itemClear.Click += (s, e) => ClearAllSignals();
            plot_context_menu.Items.Add(itemClear);

            plot_context_menu.Items.Add(new Separator());

            var menuZoomXIn = new MenuItem { Header = "Zoom In X  (↔+)" };
            menuZoomXIn.Click += (s, e) => ZoomX(0.75, (canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762) / 2.0);
            plot_context_menu.Items.Add(menuZoomXIn);

            var menuZoomXOut = new MenuItem { Header = "Zoom Out X (↔−)" };
            menuZoomXOut.Click += (s, e) => ZoomX(1.33, (canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762) / 2.0);
            plot_context_menu.Items.Add(menuZoomXOut);

            var menuZoomYIn = new MenuItem { Header = "Zoom In Y  (↕+)" };
            menuZoomYIn.Click += (s, e) => ZoomY(1.33, (canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578) / 2.0);
            plot_context_menu.Items.Add(menuZoomYIn);

            var menuZoomYOut = new MenuItem { Header = "Zoom Out Y (↕−)" };
            menuZoomYOut.Click += (s, e) => ZoomY(0.75, (canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578) / 2.0);
            plot_context_menu.Items.Add(menuZoomYOut);

            var menuResetZoom = new MenuItem { Header = "Reset Zoom / Fit All (⟲)" };
            menuResetZoom.Click += (s, e) => ResetZoom();
            plot_context_menu.Items.Add(menuResetZoom);
        }

        private void Plot_context_menu_Opened(object sender, RoutedEventArgs e)
        {
            foreach (var item in plot_context_menu.Items)
            {
                if (item is MenuItem subMenu && subMenu.HasItems)
                {
                    foreach (var subItem in subMenu.Items)
                    {
                        if (subItem is MenuItem mi && mi.Tag is string sigId)
                        {
                            mi.IsChecked = _activeSignalIds.Contains(sigId);
                        }
                    }
                }
            }
        }

        private void MenuItem_Click(object sender, RoutedEventArgs e)
        {
            if (sender is MenuItem item && item.Tag is string sigId)
            {
                ToggleSignal(sigId);
            }
        }

        private void BtnSignalsMenu_Click(object sender, RoutedEventArgs e)
        {
            plot_context_menu.PlacementTarget = btn_signals_menu;
            plot_context_menu.Placement = System.Windows.Controls.Primitives.PlacementMode.Bottom;
            plot_context_menu.IsOpen = true;
        }

        private void ToggleSignal(string sigId)
        {
            if (_activeSignalIds.Contains(sigId))
            {
                _activeSignalIds.Remove(sigId);
            }
            else
            {
                _activeSignalIds.Add(sigId);
            }
            RebuildPolylinesAndLegends();
        }

        private void ClearAllSignals()
        {
            _activeSignalIds.Clear();
            RebuildPolylinesAndLegends();
        }

        private void ResetToDefaultSignals()
        {
            _activeSignalIds.Clear();
            _activeSignalIds.Add("force_filtered");
            _activeSignalIds.Add("servo_pos_fb");
            RebuildPolylinesAndLegends();
        }

        private void BtnClearAllSignals_Click(object sender, RoutedEventArgs e) => ClearAllSignals();
        private void BtnResetDefaultSignals_Click(object sender, RoutedEventArgs e) => ResetToDefaultSignals();

        private void RebuildPolylinesAndLegends()
        {
            canvas_plot.Children.Clear();
            _signalPaths.Clear();
            _legendBindings.Clear();
            panel_legends.Children.Clear();

            tb_active_count.Text = $"({_activeSignalIds.Count} active)";
            tb_no_signals_hint.Visibility = _activeSignalIds.Count == 0 ? Visibility.Visible : Visibility.Collapsed;

            foreach (var sigId in _activeSignalIds)
            {
                var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                if (sig == null) continue;

                var path = new Path
                {
                    Stroke = new SolidColorBrush(sig.DefaultColor),
                    StrokeThickness = 2.0,
                    StrokeLineJoin = PenLineJoin.Round,
                    SnapsToDevicePixels = true,
                    IsHitTestVisible = false
                };
                _signalPaths[sigId] = path;
                canvas_plot.Children.Add(path);

                var legendBorder = new Border
                {
                    Background = new SolidColorBrush(Color.FromRgb(0x28, 0x28, 0x28)),
                    BorderBrush = new SolidColorBrush(Color.FromArgb(0x80, sig.DefaultColor.R, sig.DefaultColor.G, sig.DefaultColor.B)),
                    BorderThickness = new Thickness(1),
                    CornerRadius = new CornerRadius(3),
                    Padding = new Thickness(6, 3, 6, 3),
                    Margin = new Thickness(0, 0, 6, 4),
                    Tag = sigId
                };

                var sp = new StackPanel { Orientation = Orientation.Horizontal, VerticalAlignment = VerticalAlignment.Center };

                var swatch = new Border
                {
                    Width = 8,
                    Height = 8,
                    CornerRadius = new CornerRadius(4),
                    Background = new SolidColorBrush(sig.DefaultColor),
                    Margin = new Thickness(0, 0, 5, 0),
                    VerticalAlignment = VerticalAlignment.Center
                };
                sp.Children.Add(swatch);

                var tbName = new TextBlock
                {
                    Text = sig.Name + ": ",
                    FontWeight = FontWeights.SemiBold,
                    Foreground = Brushes.White,
                    FontSize = 10,
                    VerticalAlignment = VerticalAlignment.Center
                };
                sp.Children.Add(tbName);

                var tbVal = new TextBlock
                {
                    Name = "val_" + sigId,
                    Text = "-- " + sig.Unit,
                    Foreground = new SolidColorBrush(sig.DefaultColor),
                    FontSize = 10,
                    FontWeight = FontWeights.Bold,
                    VerticalAlignment = VerticalAlignment.Center,
                    Margin = new Thickness(0, 0, 6, 0)
                };
                sp.Children.Add(tbVal);

                var tbRange = new TextBlock
                {
                    Name = "range_" + sigId,
                    Text = "[-- / --]",
                    Foreground = new SolidColorBrush(Color.FromRgb(0x9E, 0x9E, 0x9E)),
                    FontSize = 9,
                    VerticalAlignment = VerticalAlignment.Center,
                    Margin = new Thickness(0, 0, 6, 0)
                };
                sp.Children.Add(tbRange);

                var btnRemove = new Button
                {
                    Content = "×",
                    FontSize = 12,
                    FontWeight = FontWeights.Bold,
                    Foreground = new SolidColorBrush(Color.FromRgb(0xAA, 0xAA, 0xAA)),
                    Background = Brushes.Transparent,
                    BorderThickness = new Thickness(0),
                    Padding = new Thickness(2, 0, 2, 0),
                    Cursor = Cursors.Hand,
                    VerticalAlignment = VerticalAlignment.Center,
                    Tag = sigId
                };
                btnRemove.Click += (s, e) =>
                {
                    if (s is Button b && b.Tag is string id)
                    {
                        ToggleSignal(id);
                    }
                };
                sp.Children.Add(btnRemove);

                legendBorder.Child = sp;
                panel_legends.Children.Add(legendBorder);

                _legendBindings[sigId] = (tbVal, tbRange);
            }
        }

        private void InitRenderTimer()
        {
            _renderTimer = new DispatcherTimer(DispatcherPriority.Render);
            _renderTimer.Interval = TimeSpan.FromMilliseconds(33); // 30 Hz refresh
            _renderTimer.Tick += RenderTimer_Tick;
            _renderTimer.Start();
        }

        public void OnPacketReceived(int pedalIdx, DAP_state_extended_st packet)
        {
            if (pedalIdx != _selectedPedal) return;
            if (_isPaused) return;

            DateTime now = DateTime.UtcNow;
            _lastPacketTime = now;
            _packetCounter++;

            uint espUs = packet.payloadPedalExtendedState_.timeInUs_u32;

            lock (_dataLock)
            {
                if (_firstPacket)
                {
                    _firstPacket = false;
                    _lastEspTimeUs = espUs;
                    _lastSampledEspTimeUs = espUs;
                    _unwrappedTimeSec = 0.0;
                    _latestTimeSec = 0.0;

                    _points.Add(new TelemetryPoint
                    {
                        TimeSec = 0.0,
                        State = packet.payloadPedalExtendedState_
                    });
                    return;
                }

                // Compute elapsed microseconds with automatic 32-bit unsigned rollover handling
                uint diffUs = espUs - _lastEspTimeUs;
                if (diffUs > 5000000) // Discontinuity / reconnect (> 5s gap)
                {
                    diffUs = 5000;
                }
                _unwrappedTimeSec += diffUs / 1000000.0;
                _lastEspTimeUs = espUs;
                _latestTimeSec = _unwrappedTimeSec;

                // Subsample at 200 Hz (every 5000 µs = 5ms) for microsecond accuracy without queue explosion
                uint sampleDiffUs = espUs - _lastSampledEspTimeUs;
                if (sampleDiffUs >= 5000 || sampleDiffUs > 5000000)
                {
                    _lastSampledEspTimeUs = espUs;
                    _points.Add(new TelemetryPoint
                    {
                        TimeSec = _unwrappedTimeSec,
                        State = packet.payloadPedalExtendedState_
                    });

                    // Bulk prune once every ~2.5 seconds (500 samples) instead of shifting memory every frame
                    if (_points.Count > MAX_BUFFER_POINTS + 500)
                    {
                        _points.RemoveRange(0, 500);
                    }
                }
            }
        }

        private void RenderTimer_Tick(object sender, EventArgs e)
        {
            // Update Stream Status & Rate
            TimeSpan rateDiff = DateTime.UtcNow - _lastRateCheck;
            if (rateDiff.TotalSeconds >= 1.0)
            {
                _packetRate = _packetCounter / rateDiff.TotalSeconds;
                _packetCounter = 0;
                _lastRateCheck = DateTime.UtcNow;
            }

            if (!_isPaused && (DateTime.UtcNow - _lastPacketTime).TotalSeconds < 1.5 && _packetRate > 0)
            {
                ellipse_stream_indicator.Fill = new SolidColorBrush(Color.FromRgb(0x00, 0xE6, 0x76));
                tb_stream_status.Text = $"● {_packetRate:F0} Hz";
                tb_stream_status.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xE6, 0x76));
            }
            else if (_isPaused)
            {
                ellipse_stream_indicator.Fill = new SolidColorBrush(Color.FromRgb(0xFF, 0xD7, 0x00));
                tb_stream_status.Text = "❚❚ Paused";
                tb_stream_status.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xD7, 0x00));
            }
            else
            {
                ellipse_stream_indicator.Fill = new SolidColorBrush(Color.FromRgb(0x75, 0x75, 0x75));
                tb_stream_status.Text = "○ Idle";
                tb_stream_status.Foreground = new SolidColorBrush(Color.FromRgb(0x9E, 0x9E, 0x9E));
            }

            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;

            DrawGridlines(width, height);

            if (_activeSignalIds.Count == 0) return;

            lock (_dataLock)
            {
                int ptCount = _points.Count;
                if (ptCount == 0) return;

                double refTime = _isPaused ? _pauseTimeSec : _latestTimeSec;
                double viewEndTime = refTime - _xViewOffsetSec;
                double viewStartTime = viewEndTime - _windowSeconds;

                _lastRenderRefTime = refTime;
                _lastRenderViewStart = viewStartTime;
                _lastRenderViewEnd = viewEndTime;

                // Binary search for visible window range [startIdx, endIdx]
                int startIdx = FindFirstIndexAtOrAfter(viewStartTime);
                int endIdx = FindLastIndexAtOrBefore(viewEndTime);

                _cachedStartIdx = startIdx;
                _cachedEndIdx = endIdx;

                if (startIdx > endIdx || startIdx >= ptCount) return;

                double invWindow = 1.0 / _windowSeconds;
                double ySpan = (_yViewMax > _yViewMin) ? (_yViewMax - _yViewMin) : 1.0;
                double plotH = height - 24.0;
                double plotBaseY = height - 12.0;

                // Render each active signal with GPU-accelerated StreamGeometry
                foreach (var sigId in _activeSignalIds)
                {
                    if (!_signalPaths.TryGetValue(sigId, out var path)) continue;
                    var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                    if (sig == null) continue;

                    double minVal = double.MaxValue;
                    double maxVal = double.MinValue;
                    double lastVal = 0.0;

                    for (int i = startIdx; i <= endIdx; i++)
                    {
                        double v = sig.Getter(_points[i].State);
                        if (v < minVal) minVal = v;
                        if (v > maxVal) maxVal = v;
                        lastVal = v;
                    }

                    if (minVal == double.MaxValue) { minVal = 0; maxVal = 1; }
                    if (Math.Abs(maxVal - minVal) < 1e-6)
                    {
                        maxVal += 1.0;
                        minVal -= 1.0;
                    }

                    double valSpan = maxVal - minVal;

                    // Pixel-decimated point generation into reusable list
                    _renderPoints.Clear();
                    double lastX = -999.0;

                    for (int i = startIdx; i <= endIdx; i++)
                    {
                        var pt = _points[i];
                        double normX = 1.0 - ((viewEndTime - pt.TimeSec) * invWindow);
                        double x = width * normX;

                        if (i == endIdx || Math.Abs(x - lastX) >= 1.5)
                        {
                            lastX = x;
                            double v = sig.Getter(pt.State);
                            double normY_raw = _autoScale ? ((v - minVal) / valSpan) : Math.Max(0.0, Math.Min(1.0, v / 100.0));
                            double normY_zoomed = (normY_raw - _yViewMin) / ySpan;
                            double y = plotBaseY - (normY_zoomed * plotH);
                            _renderPoints.Add(new Point(x, y));
                        }
                    }

                    if (_renderPoints.Count > 0)
                    {
                        var geom = new StreamGeometry();
                        using (var ctx = geom.Open())
                        {
                            ctx.BeginFigure(_renderPoints[0], false, false);
                            for (int p = 1; p < _renderPoints.Count; p++)
                            {
                                ctx.LineTo(_renderPoints[p], true, false);
                            }
                        }
                        geom.Freeze();
                        path.Data = geom;
                    }
                    else
                    {
                        path.Data = null;
                    }

                    // Direct O(1) Legend Update
                    UpdateLegendDisplay(sigId, lastVal, minVal, maxVal, sig);
                }
            }
        }

        private int FindFirstIndexAtOrAfter(double targetTime)
        {
            int low = 0;
            int high = _points.Count - 1;
            int result = _points.Count;

            while (low <= high)
            {
                int mid = (low + high) >> 1;
                if (_points[mid].TimeSec >= targetTime)
                {
                    result = mid;
                    high = mid - 1;
                }
                else
                {
                    low = mid + 1;
                }
            }
            return Math.Max(0, result > 0 ? result - 1 : 0);
        }

        private int FindLastIndexAtOrBefore(double targetTime)
        {
            int low = 0;
            int high = _points.Count - 1;
            int result = -1;

            while (low <= high)
            {
                int mid = (low + high) >> 1;
                if (_points[mid].TimeSec <= targetTime)
                {
                    result = mid;
                    low = mid + 1;
                }
                else
                {
                    high = mid - 1;
                }
            }
            return result == -1 ? _points.Count - 1 : Math.Min(_points.Count - 1, result + 1);
        }

        private void UpdateLegendDisplay(string sigId, double current, double min, double max, SignalDef sig)
        {
            if (_legendBindings.TryGetValue(sigId, out var pair))
            {
                pair.Val.Text = $"{current.ToString(sig.Format)} {sig.Unit}";
                pair.Range.Text = $"[{min.ToString(sig.Format)} / {max.ToString(sig.Format)}]";
            }
        }

        private void DrawGridlines(double width, double height)
        {
            if (!_gridInitialized)
            {
                _gridInitialized = true;
                canvas_grid.Children.Clear();

                // 3 Horizontal lines
                for (int i = 0; i < 3; i++)
                {
                    var line = new Line
                    {
                        X1 = 0,
                        X2 = width,
                        Stroke = new SolidColorBrush(Color.FromRgb(0x28, 0x28, 0x28)),
                        StrokeThickness = 1,
                        StrokeDashArray = new DoubleCollection { 4, 4 }
                    };
                    _gridHLines[i] = line;
                    canvas_grid.Children.Add(line);
                }

                // 4 Vertical lines + 4 Time labels
                for (int i = 0; i < 4; i++)
                {
                    var line = new Line
                    {
                        Y1 = 0,
                        Y2 = height,
                        Stroke = new SolidColorBrush(Color.FromRgb(0x22, 0x22, 0x22)),
                        StrokeThickness = 1
                    };
                    _gridVLines[i] = line;
                    canvas_grid.Children.Add(line);

                    var tb = new TextBlock
                    {
                        Foreground = new SolidColorBrush(Color.FromRgb(0x55, 0x55, 0x55)),
                        FontSize = 9
                    };
                    Canvas.SetBottom(tb, 4);
                    _gridTimeLabels[i] = tb;
                    canvas_grid.Children.Add(tb);
                }
            }

            // Update horizontal line positions
            for (int i = 0; i < 3; i++)
            {
                double y = (height / 4.0) * (i + 1);
                var line = _gridHLines[i];
                line.X2 = width;
                line.Y1 = y;
                line.Y2 = y;
            }

            // Update vertical line and label positions
            int divisions = 5;
            for (int i = 0; i < 4; i++)
            {
                int divIdx = i + 1;
                double x = (width / divisions) * divIdx;

                var line = _gridVLines[i];
                line.X1 = x;
                line.X2 = x;
                line.Y2 = height;

                double sec = _xViewOffsetSec + _windowSeconds * (1.0 - ((double)divIdx / divisions));
                var tb = _gridTimeLabels[i];
                tb.Text = sec < 10.0 ? $"-{sec:F2}s" : $"-{sec:F1}s";
                Canvas.SetLeft(tb, x + 3);
            }
        }

        // ==================== MOUSE INTERACTION, HOVER DATA TIP, ZOOM & PAN ====================

        private void Canvas_plot_MouseDown(object sender, MouseButtonEventArgs e)
        {
            Point pos = e.GetPosition(canvas_plot);

            // Double-click resets Zoom and Pan to fit all
            if (e.ClickCount == 2)
            {
                ResetZoom();
                return;
            }

            bool isCtrl = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            bool isShift = Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift);

            // Pan with Strg (Ctrl) + mouse drag OR Middle Mouse button
            if (e.ChangedButton == MouseButton.Middle || (e.ChangedButton == MouseButton.Left && isCtrl))
            {
                _isPanning = true;
                _isDraggingZoom = false;
                _panStartMouse = pos;
                _panStartXOffset = _xViewOffsetSec;
                _panStartYMin = _yViewMin;
                _panStartYMax = _yViewMax;
                canvas_plot.CaptureMouse();
                canvas_plot.Cursor = Cursors.SizeAll;
                HideDataTip();

                if (!_isPaused)
                {
                    BtnLivePause_Click(null, null);
                }
                return;
            }

            // Zoom with Shift + mouse drag OR when [⊞ Box] mode button is toggled active
            if (e.ChangedButton == MouseButton.Left && (isShift || _isBoxZoomMode))
            {
                _isDraggingZoom = true;
                _isPanning = false;
                _dragStart = pos;
                canvas_plot.CaptureMouse();
                canvas_plot.Cursor = Cursors.Cross;
                HideDataTip();
                return;
            }

            // Normal Left Drag without Shift/Ctrl: also Pan for intuitive direct manipulation
            if (e.ChangedButton == MouseButton.Left)
            {
                _isPanning = true;
                _isDraggingZoom = false;
                _panStartMouse = pos;
                _panStartXOffset = _xViewOffsetSec;
                _panStartYMin = _yViewMin;
                _panStartYMax = _yViewMax;
                canvas_plot.CaptureMouse();
                canvas_plot.Cursor = Cursors.SizeAll;
                HideDataTip();

                if (!_isPaused)
                {
                    BtnLivePause_Click(null, null);
                }
            }
        }

        private void Canvas_plot_MouseMove(object sender, MouseEventArgs e)
        {
            Point pos = e.GetPosition(canvas_plot);
            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;

            // 1. Handle Panning (Strg/Ctrl + Drag or Left Drag)
            if (_isPanning)
            {
                HideDataTip();
                double dx = pos.X - _panStartMouse.X;
                double dy = pos.Y - _panStartMouse.Y;

                // Horizontal Pan: dragging right reveals earlier points from the past
                double dt = (dx / width) * _windowSeconds;
                _xViewOffsetSec = Math.Max(0.0, _panStartXOffset + dt);

                // Vertical Pan: dragging shifts the normalized Y range
                double span = _panStartYMax - _panStartYMin;
                double dNormY = (dy / (height - 24.0)) * span;
                _yViewMin = _panStartYMin + dNormY;
                _yViewMax = _panStartYMax + dNormY;

                _isZoomed = true;
                UpdateResetZoomButton();
                return;
            }

            // 2. Handle Regional Zoom Box (Shift + Drag)
            if (_isDraggingZoom)
            {
                HideDataTip();
                double left = Math.Max(0.0, Math.Min(width, Math.Min(_dragStart.X, pos.X)));
                double top = Math.Max(0.0, Math.Min(height, Math.Min(_dragStart.Y, pos.Y)));
                double right = Math.Max(0.0, Math.Min(width, Math.Max(_dragStart.X, pos.X)));
                double bottom = Math.Max(0.0, Math.Min(height, Math.Max(_dragStart.Y, pos.Y)));

                rect_zoom_selection.Margin = new Thickness(left, top, 0, 0);
                rect_zoom_selection.Width = Math.Max(0.0, right - left);
                rect_zoom_selection.Height = Math.Max(0.0, bottom - top);
                rect_zoom_selection.Visibility = Visibility.Visible;
                return;
            }

            // Update hover cursor based on modifier key
            bool isCtrl = Keyboard.IsKeyDown(Key.LeftCtrl) || Keyboard.IsKeyDown(Key.RightCtrl);
            canvas_plot.Cursor = isCtrl ? Cursors.SizeAll : Cursors.Cross;

            if (_activeSignalIds.Count == 0)
            {
                HideDataTip();
                return;
            }

            if (pos.X < 0 || pos.X > width || pos.Y < 0 || pos.Y > height)
            {
                HideDataTip();
                return;
            }

            // Target time corresponding to cursor X position
            double normX = Math.Max(0.0, Math.Min(1.0, pos.X / width));
            double targetTime = _lastRenderViewStart + normX * _windowSeconds;

            TelemetryPoint closest = default;
            bool found = false;

            lock (_dataLock)
            {
                if (_points.Count > 0 && _cachedStartIdx <= _cachedEndIdx && _cachedStartIdx < _points.Count)
                {
                    int start = Math.Max(0, _cachedStartIdx);
                    int end = Math.Min(_points.Count - 1, _cachedEndIdx);

                    // Binary search for closest point to targetTime
                    int low = start;
                    int high = end;
                    double minDiff = double.MaxValue;

                    while (low <= high)
                    {
                        int mid = (low + high) >> 1;
                        double diff = Math.Abs(_points[mid].TimeSec - targetTime);
                        if (diff < minDiff)
                        {
                            minDiff = diff;
                            closest = _points[mid];
                            found = true;
                        }

                        if (_points[mid].TimeSec < targetTime)
                        {
                            low = mid + 1;
                        }
                        else
                        {
                            high = mid - 1;
                        }
                    }
                }
            }

            if (!found)
            {
                HideDataTip();
                return;
            }

            // Show Crosshair Line
            line_crosshair.X1 = pos.X;
            line_crosshair.X2 = pos.X;
            line_crosshair.Y1 = 0;
            line_crosshair.Y2 = height;
            line_crosshair.Visibility = Visibility.Visible;

            // Display time offset relative to current render reference (fixed and rock-solid when paused)
            double actualAge = _lastRenderRefTime - closest.TimeSec;
            if (actualAge < 0) actualAge = 0;
            tb_datatip_time.Text = actualAge < 10.0 ? $"Time: -{actualAge:F2}s" : $"Time: -{actualAge:F1}s";

            tb_datatip_servo_cycle.Text = $"Servo Cycle: {closest.State.servoStateCycleCount_u32}";
            tb_datatip_esp_cycle.Text = $"ESP Cycle: {closest.State.cycleCount_u32}";

            sp_datatip_items.Children.Clear();
            foreach (var sigId in _activeSignalIds)
            {
                if (sigId == "servo_cycle" || sigId == "esp_cycle") continue;

                var sig = AllSignals.FirstOrDefault(s => s.Id == sigId);
                if (sig == null) continue;

                double val = sig.Getter(closest.State);

                var row = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 1, 0, 1) };
                var dot = new Border
                {
                    Width = 7,
                    Height = 7,
                    CornerRadius = new CornerRadius(3.5),
                    Background = new SolidColorBrush(sig.DefaultColor),
                    Margin = new Thickness(0, 0, 5, 0),
                    VerticalAlignment = VerticalAlignment.Center
                };
                var tbItem = new TextBlock
                {
                    Text = $"{sig.Name}: ",
                    FontSize = 10,
                    Foreground = new SolidColorBrush(Color.FromRgb(0xCC, 0xCC, 0xCC))
                };
                var tbVal = new TextBlock
                {
                    Text = $"{val.ToString(sig.Format)} {sig.Unit}",
                    FontSize = 10,
                    FontWeight = FontWeights.Bold,
                    Foreground = new SolidColorBrush(sig.DefaultColor)
                };

                row.Children.Add(dot);
                row.Children.Add(tbItem);
                row.Children.Add(tbVal);
                sp_datatip_items.Children.Add(row);
            }

            border_datatip_sep.Visibility = sp_datatip_items.Children.Count > 0 ? Visibility.Visible : Visibility.Collapsed;

            // Position Data Tip Card smoothly avoiding edges
            card_datatip.Visibility = Visibility.Visible;
            double tipX = pos.X + 14;
            if (tipX + 175 > width)
            {
                tipX = pos.X - 185;
            }
            double tipY = Math.Max(10, Math.Min(pos.Y - 20, height - 140));

            card_datatip.Margin = new Thickness(tipX, tipY, 0, 0);
        }

        private void Canvas_plot_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (_isPanning)
            {
                _isPanning = false;
                canvas_plot.ReleaseMouseCapture();
                canvas_plot.Cursor = Cursors.Cross;
            }

            if (_isDraggingZoom)
            {
                _isDraggingZoom = false;
                canvas_plot.ReleaseMouseCapture();
                rect_zoom_selection.Visibility = Visibility.Collapsed;

                Point end = e.GetPosition(canvas_plot);
                double w = Math.Abs(end.X - _dragStart.X);
                double h = Math.Abs(end.Y - _dragStart.Y);

                if (w >= 8 && h >= 8)
                {
                    ApplyRegionalZoom(_dragStart, end);
                }
            }
        }

        private void Canvas_plot_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            Point pos = e.GetPosition(canvas_plot);
            if (Keyboard.IsKeyDown(Key.LeftShift) || Keyboard.IsKeyDown(Key.RightShift))
            {
                ZoomY(e.Delta > 0 ? 1.25 : 0.8, pos.Y);
            }
            else
            {
                ZoomX(e.Delta > 0 ? 0.8 : 1.25, pos.X);
            }
            e.Handled = true;
        }

        private void Canvas_plot_MouseLeave(object sender, MouseEventArgs e)
        {
            HideDataTip();
        }

        private void HideDataTip()
        {
            if (line_crosshair != null) line_crosshair.Visibility = Visibility.Collapsed;
            if (card_datatip != null) card_datatip.Visibility = Visibility.Collapsed;
        }

        // ==================== ZOOM CONTROLS ====================

        private void BtnZoomXIn_Click(object sender, RoutedEventArgs e)
        {
            ZoomX(0.75, (canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762) / 2.0);
        }

        private void BtnZoomXOut_Click(object sender, RoutedEventArgs e)
        {
            ZoomX(1.33, (canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762) / 2.0);
        }

        private void BtnZoomYIn_Click(object sender, RoutedEventArgs e)
        {
            ZoomY(1.33, (canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578) / 2.0);
        }

        private void BtnZoomYOut_Click(object sender, RoutedEventArgs e)
        {
            ZoomY(0.75, (canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578) / 2.0);
        }

        private void BtnZoomBox_Click(object sender, RoutedEventArgs e)
        {
            _isBoxZoomMode = !_isBoxZoomMode;
            if (_isBoxZoomMode)
            {
                btn_zoom_box.Background = new SolidColorBrush(Color.FromArgb(0x40, 0x00, 0xE5, 0xFF));
                btn_zoom_box.BorderBrush = new SolidColorBrush(Color.FromRgb(0x00, 0xE5, 0xFF));
                btn_zoom_box.Foreground = Brushes.White;
            }
            else
            {
                btn_zoom_box.Background = new SolidColorBrush(Color.FromRgb(0x2A, 0x2A, 0x2A));
                btn_zoom_box.BorderBrush = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
                btn_zoom_box.Foreground = new SolidColorBrush(Color.FromRgb(0xD0, 0xD0, 0xD0));
            }
        }

        private void BtnResetZoom_Click(object sender, RoutedEventArgs e)
        {
            ResetZoom();
        }

        public void ResetZoom()
        {
            _yViewMin = 0.0;
            _yViewMax = 1.0;
            _xViewOffsetSec = 0.0;
            _isPanning = false;
            _isDraggingZoom = false;
            if (rect_zoom_selection != null) rect_zoom_selection.Visibility = Visibility.Collapsed;
            if (cb_time_window.SelectedItem is string item)
            {
                string s = item.Replace("s", "");
                if (double.TryParse(s, out double sec)) _windowSeconds = sec;
            }
            else
            {
                _windowSeconds = 5.0;
            }
            _isZoomed = false;
            _isBoxZoomMode = false;
            btn_zoom_box.Background = new SolidColorBrush(Color.FromRgb(0x2A, 0x2A, 0x2A));
            btn_zoom_box.BorderBrush = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
            btn_zoom_box.Foreground = new SolidColorBrush(Color.FromRgb(0xD0, 0xD0, 0xD0));
            chk_auto_scale.IsChecked = true;
            _autoScale = true;
            UpdateResetZoomButton();
        }

        private void ZoomX(double factor, double cursorX)
        {
            _isZoomed = true;
            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762;
            double normX = Math.Max(0.0, Math.Min(1.0, cursorX / width));
            double cursorAge = _xViewOffsetSec + (1.0 - normX) * _windowSeconds;

            double newWindow = Math.Max(0.05, Math.Min(120.0, _windowSeconds * factor));
            _windowSeconds = newWindow;
            _xViewOffsetSec = Math.Max(0.0, cursorAge - (1.0 - normX) * _windowSeconds);

            if (_xViewOffsetSec > 0.001 && !_isPaused)
            {
                BtnLivePause_Click(null, null);
            }
            UpdateResetZoomButton();
        }

        private void ZoomY(double factor, double cursorY)
        {
            _isZoomed = true;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;
            double normY = Math.Max(0.0, Math.Min(1.0, ((height - 12.0) - cursorY) / (height - 24.0)));
            double currentCenter = _yViewMin + normY * (_yViewMax - _yViewMin);
            double currentSpan = _yViewMax - _yViewMin;
            double newSpan = Math.Max(0.01, Math.Min(20.0, currentSpan / factor));

            _yViewMin = currentCenter - normY * newSpan;
            _yViewMax = currentCenter + (1.0 - normY) * newSpan;

            UpdateResetZoomButton();
        }

        private void ApplyRegionalZoom(Point p1, Point p2)
        {
            double width = canvas_plot.ActualWidth > 0 ? canvas_plot.ActualWidth : 762;
            double height = canvas_plot.ActualHeight > 0 ? canvas_plot.ActualHeight : 578;

            double x1 = Math.Min(p1.X, p2.X);
            double x2 = Math.Max(p1.X, p2.X);
            double y1 = Math.Min(p1.Y, p2.Y);
            double y2 = Math.Max(p1.Y, p2.Y);

            double normX1 = Math.Max(0.0, Math.Min(1.0, x1 / width));
            double normX2 = Math.Max(0.0, Math.Min(1.0, x2 / width));
            if (normX2 - normX1 < 0.01) return;

            double ageRight = _xViewOffsetSec + (1.0 - normX2) * _windowSeconds;
            double ageLeft = _xViewOffsetSec + (1.0 - normX1) * _windowSeconds;
            double newWindow = Math.Max(0.05, ageLeft - ageRight);

            _xViewOffsetSec = Math.Max(0.0, ageRight);
            _windowSeconds = newWindow;

            double normYBottom = Math.Max(0.0, Math.Min(1.0, ((height - 12.0) - y2) / (height - 24.0)));
            double normYTop = Math.Max(0.0, Math.Min(1.0, ((height - 12.0) - y1) / (height - 24.0)));
            if (normYTop - normYBottom < 0.01) return;

            double currentYSpan = _yViewMax - _yViewMin;
            double newYMin = _yViewMin + normYBottom * currentYSpan;
            double newYMax = _yViewMin + normYTop * currentYSpan;

            _yViewMin = newYMin;
            _yViewMax = newYMax;
            _isZoomed = true;

            if (!_isPaused)
            {
                BtnLivePause_Click(null, null);
            }

            UpdateResetZoomButton();
        }

        private void UpdateResetZoomButton()
        {
            if (btn_reset_zoom == null) return;
            bool active = _isZoomed || Math.Abs(_yViewMin) > 0.001 || Math.Abs(_yViewMax - 1.0) > 0.001 || _xViewOffsetSec > 0.001;
            if (active)
            {
                btn_reset_zoom.BorderBrush = new SolidColorBrush(Color.FromRgb(0x00, 0xE5, 0xFF));
                btn_reset_zoom.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xE5, 0xFF));
                btn_reset_zoom.FontWeight = FontWeights.Bold;
            }
            else
            {
                btn_reset_zoom.BorderBrush = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
                btn_reset_zoom.Foreground = new SolidColorBrush(Color.FromRgb(0xD0, 0xD0, 0xD0));
                btn_reset_zoom.FontWeight = FontWeights.Normal;
            }
        }

        // ==================== PEDAL CONTROLS ====================

        private void BtnPedal_Click(object sender, RoutedEventArgs e)
        {
            if (sender is Button btn && int.TryParse(btn.Tag?.ToString(), out int pIdx))
            {
                if (_selectedPedal != pIdx)
                {
                    EnsureStreaming(_selectedPedal, false);
                    _selectedPedal = pIdx;
                    EnsureStreaming(_selectedPedal, true);

                    lock (_dataLock)
                    {
                        _points.Clear();
                        _firstPacket = true;
                        _unwrappedTimeSec = 0.0;
                        _latestTimeSec = 0.0;
                        _pauseTimeSec = 0.0;
                    }
                    foreach (var path in _signalPaths.Values)
                    {
                        path.Data = null;
                    }
                    UpdatePedalButtonStyles();
                }
            }
        }

        private void UpdatePedalButtonStyles()
        {
            SetPedalButtonStyle(btn_pedal_clutch, _selectedPedal == 0, Color.FromRgb(0xE5, 0x39, 0x35));
            SetPedalButtonStyle(btn_pedal_brake, _selectedPedal == 1, Color.FromRgb(0x43, 0xA0, 0x47));
            SetPedalButtonStyle(btn_pedal_throttle, _selectedPedal == 2, Color.FromRgb(0x1E, 0x88, 0xE5));
        }

        private void SetPedalButtonStyle(Button btn, bool isSelected, Color activeColor)
        {
            if (btn == null) return;
            if (isSelected)
            {
                btn.Background = new SolidColorBrush(Color.FromArgb(0x40, activeColor.R, activeColor.G, activeColor.B));
                btn.BorderBrush = new SolidColorBrush(activeColor);
                btn.Foreground = Brushes.White;
                btn.FontWeight = FontWeights.Bold;
            }
            else
            {
                btn.Background = new SolidColorBrush(Color.FromRgb(0x2A, 0x2A, 0x2A));
                btn.BorderBrush = new SolidColorBrush(Color.FromRgb(0x44, 0x44, 0x44));
                btn.Foreground = new SolidColorBrush(Color.FromRgb(0xBB, 0xBB, 0xBB));
                btn.FontWeight = FontWeights.Normal;
            }
        }

        private void BtnLivePause_Click(object sender, RoutedEventArgs e)
        {
            _isPaused = !_isPaused;
            if (_isPaused)
            {
                _pauseTimeSec = _latestTimeSec;
                btn_live_pause.Content = "▶ Live";
                btn_live_pause.Foreground = new SolidColorBrush(Color.FromRgb(0xFF, 0xD7, 0x00));
            }
            else
            {
                btn_live_pause.Content = "⏸ Pause";
                btn_live_pause.Foreground = Brushes.White;
            }
        }

        private void BtnClear_Click(object sender, RoutedEventArgs e)
        {
            lock (_dataLock)
            {
                _points.Clear();
                _firstPacket = true;
                _unwrappedTimeSec = 0.0;
                _latestTimeSec = 0.0;
                _pauseTimeSec = 0.0;
            }
            foreach (var path in _signalPaths.Values)
            {
                path.Data = null;
            }
            HideDataTip();
        }

        private void CbTimeWindow_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cb_time_window.SelectedItem is string item)
            {
                string s = item.Replace("s", "");
                if (double.TryParse(s, out double sec))
                {
                    _windowSeconds = sec;
                }
            }
        }

        private void ChkAutoScale_Changed(object sender, RoutedEventArgs e)
        {
            _autoScale = chk_auto_scale.IsChecked == true;
        }

        private void Canvas_plot_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            canvas_grid.Width = canvas_plot.ActualWidth;
            canvas_grid.Height = canvas_plot.ActualHeight;
        }
    }
}
