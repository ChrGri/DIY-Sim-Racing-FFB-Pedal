using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using DiyFfbPedal.UIElement;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// One row in the servo register table.
    /// </summary>
    public class ServoRegisterEntry : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private void Notify(string name) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));

        /// <summary>Insertion index – used for natural register order sorting.</summary>
        public int Index { get; set; }
        /// <summary>Register address string, e.g. "Pr0.00"</summary>
        public string Address { get; set; }
        /// <summary>Human-readable register name shown as tooltip and in the Name column.</summary>
        public string Name { get; set; }

        private int _recommendedValue;
        /// <summary>Recommended value of the selected parameter set (silent or smooth).</summary>
        public int RecommendedValue
        {
            get => _recommendedValue;
            set
            {
                _recommendedValue = value;
                Notify(nameof(RecommendedValue));
                Notify(nameof(LiveValueColor));
            }
        }

        // ---- Live read-back from servo ----
        private int? _liveValue;
        /// <summary>
        /// Value shown and edited in the "Live (Servo)" column.
        /// null = not yet read.
        /// </summary>
        public int? LiveValue
        {
            get => _liveValue;
            set
            {
                _liveValue = value;
                Notify(nameof(LiveValue));
                Notify(nameof(LiveValueDisplay));
                Notify(nameof(LiveValueColor));
            }
        }

        /// <summary>Display string for the live value column.</summary>
        public string LiveValueDisplay => _liveValue.HasValue ? _liveValue.Value.ToString() : "-";

        /// <summary>
        /// Green  = matches recommended value
        /// Red    = deviates from recommended value
        /// Neutral = not yet read
        /// </summary>
        public string LiveValueColor
        {
            get
            {
                if (!_liveValue.HasValue) return "#FFDADADA";
                return _liveValue.Value == RecommendedValue ? "#FF4CAF50" : "#FFE53935";
            }
        }
    }

    /// <summary>
    /// Helper class to track a batch of registers sent in a single packet for reading.
    /// </summary>
    public class ServoReadBatch
    {
        public List<ushort> Addresses { get; set; } = new List<ushort>();
        public HashSet<ushort> PendingAddresses { get; set; } = new HashSet<ushort>();
    }

    /// <summary>
    /// Helper class to track a batch of registers sent in a single packet for writing.
    /// </summary>
    public class ServoWriteBatch
    {
        public List<ServoRegisterEntry> Entries { get; set; } = new List<ServoRegisterEntry>();
        public HashSet<ushort> PendingAddresses { get; set; } = new HashSet<ushort>();
    }

    /// <summary>
    /// GeneralSetting_Servo.xaml interaction logic
    /// </summary>
    public partial class GeneralSetting_Servo : UserControl
    {
        public GeneralSetting_Servo()
        {
            InitializeComponent();
            dap_config_st = new DAP_config_st();
        }

        // ---------------------------------------------------------------
        // DAP_config_st dependency property
        // ---------------------------------------------------------------
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_Servo),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double PosSmoothing_value { get; set; }

        public DAP_config_st dap_config_st
        {
            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set => SetValue(DAP_Config_Property, value);
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_Servo control && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    if (control.Slider_ServoEndstopDetecionThreshold != null)
                        control.Slider_ServoEndstopDetecionThreshold.SliderValue = newData.payloadPedalConfig_.endstopDetectionThreshold;
                    if (control.Slider_ServoTimeout != null)
                        control.Slider_ServoTimeout.SliderValue = newData.payloadPedalConfig_.servoIdleTimeout;
                }
                catch { }
            }
        }

        // ---------------------------------------------------------------
        // Events
        // ---------------------------------------------------------------
        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }

        /// <summary>
        /// Raised when user clicks "Open Servo Register Editor" button.
        /// </summary>
        public event EventHandler OpenServoRegisterWindowRequested;

        // Legacy event definitions preserved for backwards-compatibility
        public event EventHandler<ServoRegisterEntry> ServoRegisterValueChanged;
        public event EventHandler<ServoRegisterEntry[]> ServoBatchWriteRequested;
        public event EventHandler<ushort> ServoModbusReadRequested;
        public event EventHandler<List<ushort>> ServoModbusBatchReadRequested;
        public event EventHandler FlashToServoRequested;
        public event EventHandler ResetToFactoryRequested;

        public void HandleServoModbusAck(ushort addr, short value)
        {
            // Modbus ACKs for registers are now handled by the independent ServoRegisterWindow
        }

        private void Btn_OpenServoRegisterWindow_Click(object sender, RoutedEventArgs e)
        {
            OpenServoRegisterWindowRequested?.Invoke(this, EventArgs.Empty);
        }

        // ---------------------------------------------------------------
        // Slider handlers
        // ---------------------------------------------------------------
        private void Slider_ServoEndstopDetecionThreshold_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.endstopDetectionThreshold = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_ServoTimeoutValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.servoIdleTimeout = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
