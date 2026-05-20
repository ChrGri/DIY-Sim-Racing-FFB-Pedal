using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
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

        /// <summary>Factory-tuned recommended value (read-only).</summary>
        public int RecommendedValue { get; set; }

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
    /// GeneralSetting_Servo.xaml interaction logic
    /// </summary>
    public partial class GeneralSetting_Servo : UserControl
    {
        // ---------------------------------------------------------------
        // Static register definition table  (address + name only)
        // Recommended values are auto-generated from
        // ESP32/include/isv57_tunedParameters.h via Generated/ServoTunedParameters.tt
        // — edit the firmware header and re-run "Transform All T4 Templates".
        // ---------------------------------------------------------------
        private static readonly (string addr, string name)[] s_registerDefs =
        {
            ("Pr0.00", "Reserved parameters"),
            ("Pr0.01", "Control mode"),
            ("Pr0.02", "Real-time auto-gain tuning mode"),
            ("Pr0.03", "Selection of machine stiffness at real-time auto-gain tuning"),
            ("Pr0.04", "Ratio of inertia"),
            ("Pr0.05", "Command pulse input selection"),
            ("Pr0.06", "Motor rotational direction setup"),
            ("Pr0.07", "Reserved parameters"),
            ("Pr0.08", "Microstep"),
            ("Pr0.09", "1st numerator of electronic gear"),
            ("Pr0.10", "Denominator of electronic gear"),
            ("Pr0.11", "Reserved parameters"),
            ("Pr0.12", "Reserved parameters"),
            ("Pr0.13", "1st torque limit"),
            ("Pr0.14", "Position deviation setup"),
            ("Pr0.15", "Absolute encoder setup"),
            ("Pr0.16", "External regenerative resistor setup"),
            ("Pr0.17", "Regeneration discharge resistance power"),
            ("Pr0.18", "Vibration suppression - N after Stop"),
            ("Pr0.19", "Microseismic inhibition"),
            ("Pr0.20", "Activated pulse edge"),
            ("Pr0.21", "Reserved parameter"),
            ("Pr0.22", "Reserved parameter"),
            ("Pr0.23", "Reserved parameter"),
            ("Pr0.24", "Reserved parameter"),
            ("Pr1.00", "1st position loop gain"),
            ("Pr1.01", "1st velocity loop gain"),
            ("Pr1.02", "1st time constant of velocity loop integration"),
            ("Pr1.03", "1st filter of velocity detection"),
            ("Pr1.04", "1st torque filter"),
            ("Pr1.05", "2nd position loop gain"),
            ("Pr1.06", "2nd velocity loop gain"),
            ("Pr1.07", "2nd time constant of velocity loop"),
            ("Pr1.08", "2nd filter of velocity detection"),
            ("Pr1.09", "2nd torque filter"),
            ("Pr1.10", "Velocity feed forward gain"),
            ("Pr1.11", "Velocity feed forward filter"),
            ("Pr1.12", "Torque feed forward gain"),
            ("Pr1.13", "Torque feed forward filter"),
            ("Pr1.14", "2nd gain setup"),
            ("Pr1.15", "Control switching mode"),
            ("Pr1.16", "Position control switching delay time"),
            ("Pr1.17", "Control switching level"),
            ("Pr1.18", "Control switch hysteresis"),
            ("Pr1.19", "Gain switching time"),
            ("Pr1.20", "Reserved parameter"),
            ("Pr1.21", "Reserved parameter"),
            ("Pr1.22", "Reserved parameter"),
            ("Pr1.23", "Speed regulator-kr"),
            ("Pr1.24", "Speed regulator-km"),
            ("Pr1.25", "Speed regulator-kd"),
            ("Pr1.26", "Filter"),
            ("Pr1.27", "Reserved parameter"),
            ("Pr1.28", "1st position loop integral time"),
            ("Pr1.29", "1st position loop differential time"),
            ("Pr1.30", "2nd position loop integral time"),
            ("Pr1.31", "2nd position loop differential time"),
            ("Pr1.32", "Position loop differential filter"),
            ("Pr1.33", "Speed given filter"),
            ("Pr1.34", "Reserved parameter"),
            ("Pr1.35", "Position command digital filter Settings"),
            ("Pr1.36", "Encoder feedback pulse digital filter Setting"),
            ("Pr1.37", "Special function register"),
            ("Pr1.38", "Reserved parameter"),
            ("Pr1.39", "Reserved parameter"),
            ("Pr2.00", "Adaptive filter mode setup"),
            ("Pr2.01", "1st notch frequency"),
            ("Pr2.02", "1st notch width"),
            ("Pr2.03", "1st notch depth"),
            ("Pr2.04", "2nd notch frequency"),
            ("Pr2.05", "2nd notch width"),
            ("Pr2.06", "2nd notch depth"),
            ("Pr2.07", "3rd notch frequency"),
            ("Pr2.08", "3rd notch width"),
            ("Pr2.09", "3rd notch depth"),
            ("Pr2.10", "4th notch frequency"),
            ("Pr2.11", "4th notch width"),
            ("Pr2.12", "4th notch depth"),
            ("Pr2.13", "Selection of damping filter switching"),
            ("Pr2.14", "1st damping frequency"),
            ("Pr2.15", "1st damping filter"),
            ("Pr2.16", "2nd damping frequency"),
            ("Pr2.17", "2nd damping filter"),
            ("Pr2.18", "3rd damping frequency"),
            ("Pr2.19", "3rd damping filter"),
            ("Pr2.20", "4th damping frequency"),
            ("Pr2.21", "4th damping filter"),
            ("Pr2.22", "Positional command smoothing filter"),
            ("Pr2.23", "Positional command FIR filter"),
            ("Pr2.24", "Reserved parameter"),
            ("Pr2.25", "Reserved parameter"),
            ("Pr2.26", "Reserved parameter"),
            ("Pr2.27", "Reserved parameter"),
            ("Pr2.28", "Reserved parameter"),
            ("Pr2.29", "Reserved parameter"),
            ("Pr3.00", "Velocity setup internal and external switching"),
            ("Pr3.01", "Speed command rotational direction"),
            ("Pr3.02", "Speed command input gain"),
            ("Pr3.03", "Speed command reversal input"),
            ("Pr3.04", "1st speed setup"),
            ("Pr3.05", "2nd speed setup"),
            ("Pr3.06", "3rd speed setup"),
            ("Pr3.07", "4th speed setup"),
            ("Pr3.08", "5th speed setup"),
            ("Pr3.09", "6th speed setup"),
            ("Pr3.10", "7th speed setup"),
            ("Pr3.11", "8th speed setup"),
            ("Pr3.12", "Time setup acceleration"),
            ("Pr3.13", "Time setup deceleration"),
            ("Pr3.14", "Sigmoid acceleration/deceleration time setup"),
            ("Pr3.15", "Speed zero-clamp function selection"),
            ("Pr3.16", "Speed zero-clamp level"),
            ("Pr3.17", "Torque command internal and external switching"),
            ("Pr3.18", "Torque command direction selection"),
            ("Pr3.19", "Torque command input gain"),
            ("Pr3.20", "Torque command input reversal"),
            ("Pr3.21", "Speed limit value 1"),
            ("Pr3.22", "Speed limit value 2"),
            ("Pr3.23", "Reserved parameter"),
            ("Pr3.24", "Maximum speed of motor rotation"),
            ("Pr3.25", "Reserved parameter"),
            ("Pr3.26", "Reserved parameter"),
            ("Pr3.27", "Reserved parameter"),
            ("Pr3.28", "Reserved parameter"),
            ("Pr3.29", "Reserved parameter"),
            ("Pr4.00", "Input selection SI1"),
            ("Pr4.01", "Input selection SI2"),
            ("Pr4.02", "Input selection SI3"),
            ("Pr4.03", "Input selection SI4"),
            ("Pr4.04", "Input selection SI5"),
            ("Pr4.05", "Input selection SI6"),
            ("Pr4.06", "Input selection SI7"),
            ("Pr4.07", "Input selection SI8"),
            ("Pr4.08", "Input selection SI9"),
            ("Pr4.09", "Input selection SI10"),
            ("Pr4.10", "Output selection SO1"),
            ("Pr4.11", "Output selection SO2"),
            ("Pr4.12", "Output selection SO3"),
            ("Pr4.13", "Output selection SO4"),
            ("Pr4.14", "Output selection SO5"),
            ("Pr4.15", "Output selection SO6"),
            ("Pr4.16", "Analog monitor 1 type"),
            ("Pr4.17", "Analog monitor 1 output gain"),
            ("Pr4.18", "Analog monitor 2 type"),
            ("Pr4.19", "Analog monitor 2 output gain"),
            ("Pr4.20", "Type of digital monitor"),
            ("Pr4.21", "Analog monitor output setup"),
            ("Pr4.22", "Analog input 1 (AI1) offset setup"),
            ("Pr4.23", "Analog input 1 (AI1) filter"),
            ("Pr4.24", "Analog input 1 (AI1) overvoltage setup"),
            ("Pr4.25", "Analog input 2 (AI2) offset setup"),
            ("Pr4.26", "Analog input 2 (AI2) filter"),
            ("Pr4.27", "Analog input 2 (AI2) overvoltage setup"),
            ("Pr4.28", "Analog input 3 (AI3) offset setup"),
            ("Pr4.29", "Analog input 3 (AI3) filter"),
            ("Pr4.30", "Analog input 3 (AI3) overvoltage setup"),
            ("Pr4.31", "Positioning complete range"),
            ("Pr4.32", "Positioning complete output setup"),
            ("Pr4.33", "INP hold time"),
            ("Pr4.34", "Zero-speed"),
            ("Pr4.35", "Speed coincidence range"),
            ("Pr4.36", "At-speed"),
            ("Pr4.37", "Mechanical brake action at stalling setup"),
            ("Pr4.38", "Mechanical brake action at running setup"),
            ("Pr4.39", "Brake release speed setup"),
            ("Pr4.40", "Selection of alarm output 1"),
            ("Pr4.41", "Selection of alarm output 2"),
            ("Pr4.42", "2nd positioning complete range"),
            ("Pr4.43", "E-stop function selection"),
            ("Pr4.44", "Input selection SI11"),
            ("Pr4.45", "Input selection SI12"),
            ("Pr4.46", "Input selection SI13"),
            ("Pr4.47", "Input selection SI14"),
            ("Pr4.48", "Reserved parameter"),
            ("Pr4.49", "Reserved parameter"),
            ("Pr5.00", "2nd numerator of electronic gear"),
            ("Pr5.01", "3rd numerator of electronic gear"),
            ("Pr5.02", "4th numerator of electronic gear"),
            ("Pr5.03", "Denominator of pulse output division"),
            ("Pr5.04", "Over-travel inhibit input setup"),
            ("Pr5.05", "Sequence at over-travel inhibit"),
            ("Pr5.06", "Sequence at servo-off"),
            ("Pr5.07", "Main power off sequence"),
            ("Pr5.08", "Main power off LV trip selection"),
            ("Pr5.09", "Main power off detection time"),
            ("Pr5.10", "Sequence at alarm"),
            ("Pr5.11", "Torque setup for emergency stop"),
            ("Pr5.12", "Over-load level setup"),
            ("Pr5.13", "Over-speed level setup"),
            ("Pr5.14", "Motor working range setup"),
            ("Pr5.15", "I/F reading filter"),
            ("Pr5.16", "Alarm clear input setup"),
            ("Pr5.17", "Counter clear input setup"),
            ("Pr5.18", "Command pulse inhibit input invalidation"),
            ("Pr5.19", "Command pulse inhibit input reading setup"),
            ("Pr5.20", "Position setup unit select"),
            ("Pr5.21", "Selection of torque limit"),
            ("Pr5.22", "2nd torque limit"),
            ("Pr5.23", "Torque limit switching setup 1"),
            ("Pr5.24", "Torque limit switching setup 2"),
            ("Pr5.25", "External input positive direction torque limit"),
            ("Pr5.26", "External input negative direction torque limit"),
            ("Pr5.27", "Input gain of analog torque limit"),
            ("Pr5.28", "LED initial status"),
            ("Pr5.29", "RS232 communication baud rate setup"),
            ("Pr5.30", "RS485 communication baud rate setup"),
            ("Pr5.31", "Axis address"),
            ("Pr5.32", "Command pulse input maximum setup"),
            ("Pr5.33", "Pulse regenerative output limit setup"),
            ("Pr5.34", "Reserved parameter"),
            ("Pr5.35", "Front panel lock setup"),
            ("Pr5.36", "Reserved parameter"),
            ("Pr5.37", "Reserved parameter"),
            ("Pr5.38", "Reserved parameter"),
            ("Pr5.39", "Reserved parameter"),
            ("Pr6.00", "Analog torque feed forward conversion gain"),
            ("Pr6.01", "Encoder zero position compensation"),
            ("Pr6.02", "Velocity deviation excess setup"),
            ("Pr6.03", "JOG trial run command torque"),
            ("Pr6.04", "JOG trial run command speed"),
            ("Pr6.05", "Position 3rd gain valid time"),
            ("Pr6.06", "Position 3rd gain scale factor"),
            ("Pr6.07", "Torque command additional value"),
            ("Pr6.08", "Positive direction torque compensation value"),
            ("Pr6.09", "Negative direction torque compensation value"),
            ("Pr6.10", "Function expansion setup"),
            ("Pr6.11", "Current response setup"),
            ("Pr6.12", "Encoder zero correction torque limiter set"),
            ("Pr6.13", "2nd inertia ratio"),
            ("Pr6.14", "Emergency stop time at alarm"),
            ("Pr6.15", "2nd over-speed level setup"),
            ("Pr6.16", "Running mode"),
            ("Pr6.17", "Front panel parameter writing selection"),
            ("Pr6.18", "Power-up wait time"),
            ("Pr6.19", "Encoder Z phase setup"),
            ("Pr6.20", "Trial running distance"),
            ("Pr6.21", "Trial running wait time"),
            ("Pr6.22", "Trial running cycle times"),
            ("Pr6.23", "Disturbance torque compensating gain"),
            ("Pr6.24", "Disturbance observer filter"),
            ("Pr6.25", "Reserved parameter"),
            ("Pr6.26", "Reserved parameter"),
            ("Pr6.27", "Alarm latch time selection"),
            ("Pr6.28", "Reserved parameter"),
            ("Pr6.29", "Reserved parameter"),
            ("Pr6.30", "Reserved parameter"),
            ("Pr6.31", "Real-time auto tuning estimation speed"),
            ("Pr6.32", "Real-time auto tuning custom setup"),
            ("Pr6.33", "Reserved parameter"),
            ("Pr6.34", "Reserved parameter"),
            ("Pr6.35", "Reserved parameter"),
            ("Pr6.36", "Reserved parameter"),
            ("Pr6.37", "Oscillation detection level"),
            ("Pr6.38", "Alarm mask setup"),
            ("Pr6.39", "Reserved parameter"),
            ("Pr7.00", "Current loop gain"),
            ("Pr7.01", "Current loop integral time"),
            ("Pr7.02", "Motor rotor initial position angle compensation"),
            ("Pr7.03", "Reserved parameter"),
            ("Pr7.04", "Motor rated voltage"),
            ("Pr7.05", "Motor pole pairs"),
            ("Pr7.06", "Motor phase resistor"),
            ("Pr7.07", "Motor D/Q inductance"),
            ("Pr7.08", "Motor back EMF coefficient"),
            ("Pr7.09", "Motor torque coefficient"),
            ("Pr7.10", "Motor rated speed"),
            ("Pr7.11", "Motor maximum speed"),
            ("Pr7.12", "Motor rated current"),
            ("Pr7.13", "Motor rotor inertia"),
            ("Pr7.14", "Motor power selection"),
            ("Pr7.15", "Motor model input"),
            ("Pr7.16", "Encoder selection"),
            ("Pr7.17", "Motor maximum current"),
            ("Pr7.18", "Encoder Index Angle compensation"),
            ("Pr7.19", "Reserved parameter"),
            ("Pr7.20", "Drive model input"),
            ("Pr7.21", "Servo model input"),
            ("Pr7.22", "Reserved parameter"),
            ("Pr7.23", "Reserved parameter"),
            ("Pr7.24", "Fan control mode setting"),
            ("Pr7.25", "Fan open temperature setting"),
            ("Pr7.26", "Fan temperature control hysteresis"),
            ("Pr7.27", "Drive over-temperature alarm threshold setting"),
            ("Pr7.28", "Time of Bleeder alarm window"),
            ("Pr7.29", "DC bus voltage detection filter"),
            ("Pr7.30", "Under-voltage point set"),
            ("Pr7.31", "Bleeder control mode setting"),
            ("Pr7.32", "Bleeder open the threshold set"),
            ("Pr7.33", "Bleeder control hysteresis"),
            ("Pr7.34", "Overvoltage point set"),
            ("Pr7.35", "Relay control mode setting"),
            ("Pr7.36", "Threshold setting of relay suction"),
            ("Pr7.37", "Analog quantity AI3 hardware zero drift compensation"),
            ("Pr7.38", "Analog quantity AI3 hardware zero drift compensation"),
            ("Pr7.39", "Analog quantity AI3 hardware zero drift compensation"),
            ("Pr7.40", "RS232 communication mode settings"),
            ("Pr7.41", "RS485 communication baud rate settings"),
            ("Pr7.42", "RS232-ID"),
            ("Pr7.43", "DC bus voltage hardware zero drift compensation"),
            ("Pr7.44", "Temperature measurement hardware zero drift compensation"),
            ("Pr7.45", "DC bus voltage hardware slope coefficient"),
            ("Pr7.46", "Current U phase sampling hardware slope coefficient"),
            ("Pr7.47", "Current V phase sampling hardware slope coefficient"),
            ("Pr7.48", "Reserved parameter"),
            ("Pr7.49", "Reserved parameter"),
        };

        // ---------------------------------------------------------------
        // Observable collection bound to the DataGrid
        // ---------------------------------------------------------------
        public ObservableCollection<ServoRegisterEntry> ServoRegisters { get; }
            = new ObservableCollection<ServoRegisterEntry>();

        // ---------------------------------------------------------------
        // Constructor
        // ---------------------------------------------------------------
        public GeneralSetting_Servo()
        {
            InitializeComponent();
            dap_config_st = new DAP_config_st();

            _readTimeoutTimer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1)
            };
            _readTimeoutTimer.Tick += OnReadTimeout;

            // Write timer: sends one register every 50 ms to avoid flooding the serial port.
            _writeTimer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromMilliseconds(50)
            };
            _writeTimer.Tick += OnWriteTimerTick;

            // Populate the register table.
            // Recommended values come from the auto-generated Isv57TunedParameters class
            // (Generated/ServoTunedParameters.tt ? Generated/ServoTunedParameters.cs),
            // which is parsed from ESP32/include/isv57_tunedParameters.h at build time.
            int idx = 0;
            foreach (var def in s_registerDefs)
            {
                Isv57TunedParameters.Values.TryGetValue(def.addr, out int recommended);
                ServoRegisters.Add(new ServoRegisterEntry
                {
                    Index            = idx++,
                    Address          = def.addr,
                    Name             = def.name,
                    RecommendedValue = recommended,
                    // LiveValue starts null (unknown) – populated by "Load from servo"
                });
            }

            // Apply default sort by Index so registers appear in definition order
            var view = CollectionViewSource.GetDefaultView(ServoRegisters);
            view.SortDescriptions.Clear();
            view.SortDescriptions.Add(new System.ComponentModel.SortDescription(
                nameof(ServoRegisterEntry.Index),
                System.ComponentModel.ListSortDirection.Ascending));
        }

        // ---------------------------------------------------------------
        // DAP_config_st dependency property (unchanged)
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
        /// Raised when the user confirms a change to a register's Current Value cell.
        /// </summary>
        public event EventHandler<ServoRegisterEntry> ServoRegisterValueChanged;

        /// <summary>
        /// Raised to request a single Modbus READ for the given holding-register address.
        /// The parent sends the HID packet; the next request is only fired after the ACK arrives
        /// (or after the per-register timeout expires), ensuring strict sequential ordering.
        /// </summary>
        public event EventHandler<ushort> ServoModbusReadRequested;

        /// <summary>
        /// Raised when the user clicks "Flash to servo".
        /// The parent UI should send the NVM-save command (register 0x019A = 0x5555) to the servo.
        /// </summary>
        public event EventHandler FlashToServoRequested;
        public event EventHandler ResetToFactoryRequested;

        // ---------------------------------------------------------------
        // Sequential load state
        // ---------------------------------------------------------------
        private readonly Queue<ushort> _pendingReadQueue = new Queue<ushort>();
        private readonly System.Windows.Threading.DispatcherTimer _readTimeoutTimer;

        // ---------------------------------------------------------------
        // Sequential write state (used by Btn_ResetToRecommended_Click)
        // ---------------------------------------------------------------
        private readonly Queue<ServoRegisterEntry> _pendingWriteQueue = new Queue<ServoRegisterEntry>();
        private readonly System.Windows.Threading.DispatcherTimer _writeTimer;

        // ---------------------------------------------------------------
        // Button handlers
        // ---------------------------------------------------------------

        /// <summary>
        /// Triggered when the user clicks "Load current from servo".
        /// Resets all live values to unknown, builds the sequential read queue,
        /// and fires the first request. Each subsequent request is fired only after
        /// the ACK (or 1-second timeout) for the previous one is received.
        /// </summary>
        private void Btn_LoadFromServo_Click(object sender, RoutedEventArgs e)
        {
            _readTimeoutTimer.Stop();
            _pendingReadQueue.Clear();

            // Reset all live values to unknown (pending state)
            foreach (var entry in ServoRegisters)
                entry.LiveValue = null;

            // Enqueue addresses in display order
            foreach (var entry in ServoRegisters)
            {
                if (DapAttrHelper.TryParseServoAddress(entry.Address, out ushort addr))
                    _pendingReadQueue.Enqueue(addr);
            }

            SendNextPendingRead();
        }

        /// <summary>
        /// Dequeues the next pending address and raises ServoModbusReadRequested.
        /// </summary>
        private void SendNextPendingRead()
        {
            _readTimeoutTimer.Stop();
            if (_pendingReadQueue.Count == 0) return;

            ushort nextAddr = _pendingReadQueue.Peek(); // leave in queue until ACK arrives
            _readTimeoutTimer.Start();
            ServoModbusReadRequested?.Invoke(this, nextAddr);
        }

        /// <summary>
        /// Called when no ACK arrives within 1 second — skips the current register and continues.
        /// </summary>
        private void OnReadTimeout(object sender, EventArgs e)
        {
            _readTimeoutTimer.Stop();
            if (_pendingReadQueue.Count > 0)
                _pendingReadQueue.Dequeue(); // skip stalled register
            SendNextPendingRead();
        }

        /// <summary>
        /// Called when the ESP32 returns a ServoModbus ACK packet.
        /// Updates the matching LiveValue and advances the sequential read queue.
        /// </summary>
        /// <param name="addr">Absolute Modbus holding-register address.</param>
        /// <param name="value">Register value reported by the servo.</param>
        public void HandleServoModbusAck(ushort addr, short value)
        {
            foreach (var entry in ServoRegisters)
            {
                if (DapAttrHelper.TryParseServoAddress(entry.Address, out ushort entryAddr) && entryAddr == addr)
                {
                    entry.LiveValue = (int)value;
                    break;
                }
            }

            if (_pendingReadQueue.Count > 0 && _pendingReadQueue.Peek() == addr)
                _pendingReadQueue.Dequeue();

            SendNextPendingRead();
        }

        /// <summary>
        /// Resets every CurrentValue back to its RecommendedValue,
        /// then sends all registers to the servo sequentially (one per 50 ms).
        /// FlashToServoRequested is raised only after the last register is sent.
        /// </summary>
        private void Btn_ResetToRecommended_Click(object sender, RoutedEventArgs e)
        {
            // Stop any pending write sequence first
            _writeTimer.Stop();
            _pendingWriteQueue.Clear();

            // Update the table
            foreach (var entry in ServoRegisters)
                entry.LiveValue = entry.RecommendedValue;

            ServoRegisterGrid.Items.Refresh();

            // Enqueue all registers for sequential serial write
            foreach (var entry in ServoRegisters)
                _pendingWriteQueue.Enqueue(entry);

            // Start the write timer – OnWriteTimerTick will drain the queue
            _writeTimer.Start();
        }

        /// <summary>
        /// Sends the next queued register write to the servo.
        /// When the queue is empty, fires FlashToServoRequested to persist to NVM.
        /// </summary>
        private void OnWriteTimerTick(object sender, EventArgs e)
        {
            if (_pendingWriteQueue.Count == 0)
            {
                _writeTimer.Stop();
                // Persist all parameters to NVM
                FlashToServoRequested?.Invoke(this, EventArgs.Empty);
                return;
            }

            var entry = _pendingWriteQueue.Dequeue();
            ServoRegisterValueChanged?.Invoke(this, entry);
        }

        /// <summary>
        /// Sends the iSV57 NVM-save command to the servo:
        /// write 0x5555 to holding register 0x019A.
        /// The servo persists all RAM parameters to flash; a power-cycle is required afterwards.
        /// </summary>
        private void Btn_FlashToServo_Click(object sender, RoutedEventArgs e)
        {
            FlashToServoRequested?.Invoke(this, EventArgs.Empty);
        }

        private void Btn_ResetToFactory_Click(object sender, RoutedEventArgs e)
        {
            ResetToFactoryRequested?.Invoke(this, EventArgs.Empty);
        }

        // ---------------------------------------------------------------
        // DataGrid cell-edit handler
        // ---------------------------------------------------------------
        private void ServoRegisterGrid_CellEditEnding(object sender, DataGridCellEditEndingEventArgs e)
        {
            if (e.EditAction != DataGridEditAction.Commit) return;
            if (!(e.Row.Item is ServoRegisterEntry entry)) return;

            // Raise event so ServoCallbacks sends the new LiveValue to the servo
            ServoRegisterValueChanged?.Invoke(this, entry);
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
