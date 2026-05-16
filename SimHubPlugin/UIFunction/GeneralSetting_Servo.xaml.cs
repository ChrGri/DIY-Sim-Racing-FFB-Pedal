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
        /// <summary>Current value – editable by the user.</summary>
        public int CurrentValue { get; set; }
        /// <summary>Factory-tuned recommended value (read-only).</summary>
        public int RecommendedValue { get; set; }

        // ---- Live read-back from servo ----
        private int? _liveValue;
        /// <summary>
        /// Live register value received from the servo.
        /// null = not yet read (unknown state).
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

        // Foreground color as a string so WPF's binding engine compares by value, not by reference.
        // This avoids the optimization where WPF skips an update because the Brush object reference
        // didn't change (all static frozen brushes are the same object each time).
        public string LiveValueColor
        {
            get
            {
                if (!_liveValue.HasValue) return "#FFDADADA"; // neutral
                return _liveValue.Value == RecommendedValue ? "#FF4CAF50" : "#FFE53935"; // green / red
            }
        }
    }

    /// <summary>
    /// GeneralSetting_Servo.xaml interaction logic
    /// </summary>
    public partial class GeneralSetting_Servo : UserControl
    {
        // ---------------------------------------------------------------
        // Static register definition table
        // (address, name, recommended value)
        // ---------------------------------------------------------------
        private static readonly (string addr, string name, int recommended)[] s_registerDefs =
        {
            ("Pr0.00", "Reserved parameters", 1),
            ("Pr0.01", "Control mode", 0),
            ("Pr0.02", "Real-time auto-gain tuning mode", 0),
            ("Pr0.03", "Selection of machine stiffness at real-time auto-gain tuning", 9),
            ("Pr0.04", "Ratio of inertia", 110),
            ("Pr0.05", "Command pulse input selection", 0),
            ("Pr0.06", "Motor rotational direction setup", 0),
            ("Pr0.07", "Reserved parameters", 3),
            ("Pr0.08", "Microstep", 3780),
            ("Pr0.09", "1st numerator of electronic gear", 1),
            ("Pr0.10", "Denominator of electronic gear", 1),
            ("Pr0.11", "Reserved parameters", 4000),
            ("Pr0.12", "Reserved parameters", 0),
            ("Pr0.13", "1st torque limit", 500),
            ("Pr0.14", "Position deviation setup", 500),
            ("Pr0.15", "Absolute encoder setup", 0),
            ("Pr0.16", "External regenerative resistor setup", 50),
            ("Pr0.17", "Regeneration discharge resistance power", 50),
            ("Pr0.18", "Vibration suppression - N after Stop", 0),
            ("Pr0.19", "Microseismic inhibition", 0),
            ("Pr0.20", "Activated pulse edge", 0),
            ("Pr0.21", "Reserved parameter", -1137),
            ("Pr0.22", "Reserved parameter", -729),
            ("Pr0.23", "Reserved parameter", 0),
            ("Pr0.24", "Reserved parameter", 0),
            ("Pr1.00", "1st position loop gain", 600),
            ("Pr1.01", "1st velocity loop gain", 500),
            ("Pr1.02", "1st time constant of velocity loop integration", 500),
            ("Pr1.03", "1st filter of velocity detection", 27),
            ("Pr1.04", "1st torque filter", 100),
            ("Pr1.05", "2nd position loop gain", 175),
            ("Pr1.06", "2nd velocity loop gain", 110),
            ("Pr1.07", "2nd time constant of velocity loop", 10000),
            ("Pr1.08", "2nd filter of velocity detection", 9),
            ("Pr1.09", "2nd torque filter", 50),
            ("Pr1.10", "Velocity feed forward gain", 500),
            ("Pr1.11", "Velocity feed forward filter", 1000),
            ("Pr1.12", "Torque feed forward gain", 0),
            ("Pr1.13", "Torque feed forward filter", 1000),
            ("Pr1.14", "2nd gain setup", 1),
            ("Pr1.15", "Control switching mode", 0),
            ("Pr1.16", "Position control switching delay time", 50),
            ("Pr1.17", "Control switching level", 50),
            ("Pr1.18", "Control switch hysteresis", 33),
            ("Pr1.19", "Gain switching time", 33),
            ("Pr1.20", "Reserved parameter", 100),
            ("Pr1.21", "Reserved parameter", 100),
            ("Pr1.22", "Reserved parameter", 0),
            ("Pr1.23", "Speed regulator-kr", 100),
            ("Pr1.24", "Speed regulator-km", 0),
            ("Pr1.25", "Speed regulator-kd", 0),
            ("Pr1.26", "Filter", 10),
            ("Pr1.27", "Reserved parameter", 0),
            ("Pr1.28", "1st position loop integral time", 10000),
            ("Pr1.29", "1st position loop differential time", 0),
            ("Pr1.30", "2nd position loop integral time", 10000),
            ("Pr1.31", "2nd position loop differential time", 0),
            ("Pr1.32", "Position loop differential filter", 10),
            ("Pr1.33", "Speed given filter", 0),
            ("Pr1.34", "Reserved parameter", 0),
            ("Pr1.35", "Position command digital filter Settings", 0),
            ("Pr1.36", "Encoder feedback pulse digital filter Setting", 0),
            ("Pr1.37", "Special function register", 1052),
            ("Pr1.38", "Reserved parameter", 0),
            ("Pr1.39", "Reserved parameter", 0),
            ("Pr2.00", "Adaptive filter mode setup", 2),
            ("Pr2.01", "1st notch frequency", 50),
            ("Pr2.02", "1st notch width", 20),
            ("Pr2.03", "1st notch depth", 99),
            ("Pr2.04", "2nd notch frequency", 90),
            ("Pr2.05", "2nd notch width", 20),
            ("Pr2.06", "2nd notch depth", 99),
            ("Pr2.07", "3rd notch frequency", 2000),
            ("Pr2.08", "3rd notch width", 0),
            ("Pr2.09", "3rd notch depth", 0),
            ("Pr2.10", "4th notch frequency", 2000),
            ("Pr2.11", "4th notch width", 0),
            ("Pr2.12", "4th notch depth", 0),
            ("Pr2.13", "Selection of damping filter switching", 0),
            ("Pr2.14", "1st damping frequency", 0),
            ("Pr2.15", "1st damping filter", 1),
            ("Pr2.16", "2nd damping frequency", 0),
            ("Pr2.17", "2nd damping filter", 1),
            ("Pr2.18", "3rd damping frequency", 0),
            ("Pr2.19", "3rd damping filter", 0),
            ("Pr2.20", "4th damping frequency", 0),
            ("Pr2.21", "4th damping filter", 0),
            ("Pr2.22", "Positional command smoothing filter", 0),
            ("Pr2.23", "Positional command FIR filter", 0),
            ("Pr2.24", "Reserved parameter", 0),
            ("Pr2.25", "Reserved parameter", 0),
            ("Pr2.26", "Reserved parameter", 0),
            ("Pr2.27", "Reserved parameter", 0),
            ("Pr2.28", "Reserved parameter", 0),
            ("Pr2.29", "Reserved parameter", 0),
            ("Pr3.00", "Velocity setup internal and external switching", 0),
            ("Pr3.01", "Speed command rotational direction", 1),
            ("Pr3.02", "Speed command input gain", 500),
            ("Pr3.03", "Speed command reversal input", 0),
            ("Pr3.04", "1st speed setup", 0),
            ("Pr3.05", "2nd speed setup", 0),
            ("Pr3.06", "3rd speed setup", 0),
            ("Pr3.07", "4th speed setup", 0),
            ("Pr3.08", "5th speed setup", 0),
            ("Pr3.09", "6th speed setup", 0),
            ("Pr3.10", "7th speed setup", 0),
            ("Pr3.11", "8th speed setup", 0),
            ("Pr3.12", "Time setup acceleration", 0),
            ("Pr3.13", "Time setup deceleration", 0),
            ("Pr3.14", "Sigmoid acceleration/deceleration time setup", 0),
            ("Pr3.15", "Speed zero-clamp function selection", 0),
            ("Pr3.16", "Speed zero-clamp level", 30),
            ("Pr3.17", "Torque command internal and external switching", 0),
            ("Pr3.18", "Torque command direction selection", 0),
            ("Pr3.19", "Torque command input gain", 30),
            ("Pr3.20", "Torque command input reversal", 0),
            ("Pr3.21", "Speed limit value 1", 0),
            ("Pr3.22", "Speed limit value 2", 0),
            ("Pr3.23", "Reserved parameter", 0),
            ("Pr3.24", "Maximum speed of motor rotation", 5000),
            ("Pr3.25", "Reserved parameter", 0),
            ("Pr3.26", "Reserved parameter", 0),
            ("Pr3.27", "Reserved parameter", 0),
            ("Pr3.28", "Reserved parameter", 0),
            ("Pr3.29", "Reserved parameter", 0),
            ("Pr4.00", "Input selection SI1", 3084),
            ("Pr4.01", "Input selection SI2", 3341),
            ("Pr4.02", "Input selection SI3", 5654),
            ("Pr4.03", "Input selection SI4", 5911),
            ("Pr4.04", "Input selection SI5", 6168),
            ("Pr4.05", "Input selection SI6", 18),
            ("Pr4.06", "Input selection SI7", 4608),
            ("Pr4.07", "Input selection SI8", 3584),
            ("Pr4.08", "Input selection SI9", 0x0303),
            ("Pr4.09", "Input selection SI10", 0),
            ("Pr4.10", "Output selection SO1", 0x101),
            ("Pr4.11", "Output selection SO2", 0),
            ("Pr4.12", "Output selection SO3", 0),
            ("Pr4.13", "Output selection SO4", 0),
            ("Pr4.14", "Output selection SO5", 0),
            ("Pr4.15", "Output selection SO6", 9),
            ("Pr4.16", "Analog monitor 1 type", 10),
            ("Pr4.17", "Analog monitor 1 output gain", 11),
            ("Pr4.18", "Analog monitor 2 type", 12),
            ("Pr4.19", "Analog monitor 2 output gain", 13),
            ("Pr4.20", "Type of digital monitor", 14),
            ("Pr4.21", "Analog monitor output setup", 15),
            ("Pr4.22", "Analog input 1 (AI1) offset setup", 0),
            ("Pr4.23", "Analog input 1 (AI1) filter", 0),
            ("Pr4.24", "Analog input 1 (AI1) overvoltage setup", 0),
            ("Pr4.25", "Analog input 2 (AI2) offset setup", 0),
            ("Pr4.26", "Analog input 2 (AI2) filter", 0),
            ("Pr4.27", "Analog input 2 (AI2) overvoltage setup", 0),
            ("Pr4.28", "Analog input 3 (AI3) offset setup", 0),
            ("Pr4.29", "Analog input 3 (AI3) filter", 0),
            ("Pr4.30", "Analog input 3 (AI3) overvoltage setup", 0),
            ("Pr4.31", "Positioning complete range", 10),
            ("Pr4.32", "Positioning complete output setup", 0),
            ("Pr4.33", "INP hold time", 0),
            ("Pr4.34", "Zero-speed", 50),
            ("Pr4.35", "Speed coincidence range", 50),
            ("Pr4.36", "At-speed", 1000),
            ("Pr4.37", "Mechanical brake action at stalling setup", 0),
            ("Pr4.38", "Mechanical brake action at running setup", 0),
            ("Pr4.39", "Brake release speed setup", 30),
            ("Pr4.40", "Selection of alarm output 1", 0),
            ("Pr4.41", "Selection of alarm output 2", 0),
            ("Pr4.42", "2nd positioning complete range", 10),
            ("Pr4.43", "E-stop function selection", 1),
            ("Pr4.44", "Input selection SI11", 0),
            ("Pr4.45", "Input selection SI12", 0),
            ("Pr4.46", "Input selection SI13", 0),
            ("Pr4.47", "Input selection SI14", 0),
            ("Pr4.48", "Reserved parameter", 50),
            ("Pr4.49", "Reserved parameter", 500),
            ("Pr5.00", "2nd numerator of electronic gear", 1),
            ("Pr5.01", "3rd numerator of electronic gear", 1),
            ("Pr5.02", "4th numerator of electronic gear", 1),
            ("Pr5.03", "Denominator of pulse output division", 2500),
            ("Pr5.04", "Over-travel inhibit input setup", 0),
            ("Pr5.05", "Sequence at over-travel inhibit", 0),
            ("Pr5.06", "Sequence at servo-off", 0),
            ("Pr5.07", "Main power off sequence", 0),
            ("Pr5.08", "Main power off LV trip selection", 1),
            ("Pr5.09", "Main power off detection time", 70),
            ("Pr5.10", "Sequence at alarm", 0),
            ("Pr5.11", "Torque setup for emergency stop", 0),
            ("Pr5.12", "Over-load level setup", 0),
            ("Pr5.13", "Over-speed level setup", 5000),
            ("Pr5.14", "Motor working range setup", -2241),
            ("Pr5.15", "I/F reading filter", -4117),
            ("Pr5.16", "Alarm clear input setup", 0),
            ("Pr5.17", "Counter clear input setup", 3),
            ("Pr5.18", "Command pulse inhibit input invalidation", 0),
            ("Pr5.19", "Command pulse inhibit input reading setup", 0),
            ("Pr5.20", "Position setup unit select", 1),
            ("Pr5.21", "Selection of torque limit", 0),
            ("Pr5.22", "2nd torque limit", 300),
            ("Pr5.23", "Torque limit switching setup 1", 0),
            ("Pr5.24", "Torque limit switching setup 2", 0),
            ("Pr5.25", "External input positive direction torque limit", 0),
            ("Pr5.26", "External input negative direction torque limit", 0),
            ("Pr5.27", "Input gain of analog torque limit", 30),
            ("Pr5.28", "LED initial status", 1),
            ("Pr5.29", "RS232 communication baud rate setup", 21),
            ("Pr5.30", "RS485 communication baud rate setup", 2),
            ("Pr5.31", "Axis address", 1),
            ("Pr5.32", "Command pulse input maximum setup", 0),
            ("Pr5.33", "Pulse regenerative output limit setup", 0),
            ("Pr5.34", "Reserved parameter", 0),
            ("Pr5.35", "Front panel lock setup", 1),
            ("Pr5.36", "Reserved parameter", 0),
            ("Pr5.37", "Reserved parameter", 322),
            ("Pr5.38", "Reserved parameter", 5000),
            ("Pr5.39", "Reserved parameter", 0),
            ("Pr6.00", "Analog torque feed forward conversion gain", 0),
            ("Pr6.01", "Encoder zero position compensation", 0),
            ("Pr6.02", "Velocity deviation excess setup", 0),
            ("Pr6.03", "JOG trial run command torque", 0),
            ("Pr6.04", "JOG trial run command speed", 300),
            ("Pr6.05", "Position 3rd gain valid time", 0),
            ("Pr6.06", "Position 3rd gain scale factor", 100),
            ("Pr6.07", "Torque command additional value", 0),
            ("Pr6.08", "Positive direction torque compensation value", 0),
            ("Pr6.09", "Negative direction torque compensation value", 0),
            ("Pr6.10", "Function expansion setup", 0),
            ("Pr6.11", "Current response setup", 50),
            ("Pr6.12", "Encoder zero correction torque limiter set", 25),
            ("Pr6.13", "2nd inertia ratio", 0),
            ("Pr6.14", "Emergency stop time at alarm", 200),
            ("Pr6.15", "2nd over-speed level setup", 0),
            ("Pr6.16", "Running mode", 0),
            ("Pr6.17", "Front panel parameter writing selection", 0),
            ("Pr6.18", "Power-up wait time", 0),
            ("Pr6.19", "Encoder Z phase setup", 0),
            ("Pr6.20", "Trial running distance", 10),
            ("Pr6.21", "Trial running wait time", 200),
            ("Pr6.22", "Trial running cycle times", 1),
            ("Pr6.23", "Disturbance torque compensating gain", 0),
            ("Pr6.24", "Disturbance observer filter", 0),
            ("Pr6.25", "Reserved parameter", 0),
            ("Pr6.26", "Reserved parameter", 0),
            ("Pr6.27", "Alarm latch time selection", 0),
            ("Pr6.28", "Reserved parameter", 0),
            ("Pr6.29", "Reserved parameter", 0),
            ("Pr6.30", "Reserved parameter", 0),
            ("Pr6.31", "Real-time auto tuning estimation speed", 0),
            ("Pr6.32", "Real-time auto tuning custom setup", 0),
            ("Pr6.33", "Reserved parameter", 25),
            ("Pr6.34", "Reserved parameter", 0),
            ("Pr6.35", "Reserved parameter", 0),
            ("Pr6.36", "Reserved parameter", 0),
            ("Pr6.37", "Oscillation detection level", 0),
            ("Pr6.38", "Alarm mask setup", 0),
            ("Pr6.39", "Reserved parameter", 0),
            ("Pr7.00", "Current loop gain", 500),
            ("Pr7.01", "Current loop integral time", 50),
            ("Pr7.02", "Motor rotor initial position angle compensation", 106),
            ("Pr7.03", "Reserved parameter", 0),
            ("Pr7.04", "Motor rated voltage", 360),
            ("Pr7.05", "Motor pole pairs", 4),
            ("Pr7.06", "Motor phase resistor", 82),
            ("Pr7.07", "Motor D/Q inductance", 84),
            ("Pr7.08", "Motor back EMF coefficient", 56),
            ("Pr7.09", "Motor torque coefficient", 80),
            ("Pr7.10", "Motor rated speed", 3000),
            ("Pr7.11", "Motor maximum speed", 4000),
            ("Pr7.12", "Motor rated current", 763),
            ("Pr7.13", "Motor rotor inertia", 40),
            ("Pr7.14", "Motor power selection", 130),
            ("Pr7.15", "Motor model input", 2),
            ("Pr7.16", "Encoder selection", 0),
            ("Pr7.17", "Motor maximum current", 250),
            ("Pr7.18", "Encoder Index Angle compensation", 338),
            ("Pr7.19", "Reserved parameter", 0),
            ("Pr7.20", "Drive model input", 0),
            ("Pr7.21", "Servo model input", 0),
            ("Pr7.22", "Reserved parameter", 304),
            ("Pr7.23", "Reserved parameter", 15),
            ("Pr7.24", "Fan control mode setting", 0),
            ("Pr7.25", "Fan open temperature setting", 50),
            ("Pr7.26", "Fan temperature control hysteresis", 5),
            ("Pr7.27", "Drive over-temperature alarm threshold setting", 80),
            ("Pr7.28", "Time of Bleeder alarm window", 30),
            ("Pr7.29", "DC bus voltage detection filter", 0),
            ("Pr7.30", "Under-voltage point set", 16),
            ("Pr7.31", "Bleeder control mode setting", 0),
            ("Pr7.32", "Bleeder open the threshold set", 40),
            ("Pr7.33", "Bleeder control hysteresis", 1),
            ("Pr7.34", "Overvoltage point set", 72),
            ("Pr7.35", "Relay control mode setting", 1),
            ("Pr7.36", "Threshold setting of relay suction", 18),
            ("Pr7.37", "Analog quantity AI3 hardware zero drift compensation", 2048),
            ("Pr7.38", "Analog quantity AI3 hardware zero drift compensation", 2048),
            ("Pr7.39", "Analog quantity AI3 hardware zero drift compensation", 2048),
            ("Pr7.40", "RS232 communication mode settings", 21),
            ("Pr7.41", "RS485 communication baud rate settings", 4),
            ("Pr7.42", "RS232-ID", 63),
            ("Pr7.43", "DC bus voltage hardware zero drift compensation", 0),
            ("Pr7.44", "Temperature measurement hardware zero drift compensation", 0),
            ("Pr7.45", "DC bus voltage hardware slope coefficient", 10000),
            ("Pr7.46", "Current U phase sampling hardware slope coefficient", 10000),
            ("Pr7.47", "Current V phase sampling hardware slope coefficient", 10000),
            ("Pr7.48", "Reserved parameter", 18),
            ("Pr7.49", "Reserved parameter", 500),
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

            // Populate the register table with recommended values as defaults
            int idx = 0;
            foreach (var def in s_registerDefs)
            {
                ServoRegisters.Add(new ServoRegisterEntry
                {
                    Index            = idx++,
                    Address          = def.addr,
                    Name             = def.name,
                    CurrentValue     = def.recommended,
                    RecommendedValue = def.recommended,
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
        /// Raised when the user clicks "Load current from servo".
        /// The parent UI should subscribe and send a READ attr packet for each entry's Modbus address.
        /// </summary>
        public event EventHandler LoadFromServoRequested;

        /// <summary>
        /// Raised when the user clicks "Flash to servo".
        /// The parent UI should send the NVM-save command (register 0x019A = 0x5555) to the servo.
        /// </summary>
        public event EventHandler FlashToServoRequested;

        // ---------------------------------------------------------------
        // Button handlers
        // ---------------------------------------------------------------

        /// <summary>
        /// Triggered when the user clicks "Load current from servo".
        /// Resets all live values to unknown and raises the request event.
        /// </summary>
        private void Btn_LoadFromServo_Click(object sender, RoutedEventArgs e)
        {
            // Reset all live values to unknown (pending state)
            foreach (var entry in ServoRegisters)
                entry.LiveValue = null;

            LoadFromServoRequested?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Called when the ESP32 returns a ServoModbus ACK packet.
        /// Finds the register entry with the matching Modbus address and updates its LiveValue.
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
        }

        /// <summary>
        /// Resets every CurrentValue back to its RecommendedValue.
        /// </summary>
        private void Btn_ResetToRecommended_Click(object sender, RoutedEventArgs e)
        {
            foreach (var entry in ServoRegisters)
                entry.CurrentValue = entry.RecommendedValue;

            // Refresh the DataGrid so the new values are visible
            ServoRegisterGrid.Items.Refresh();
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

        // ---------------------------------------------------------------
        // DataGrid cell-edit handler
        // ---------------------------------------------------------------
        private void ServoRegisterGrid_CellEditEnding(object sender, DataGridCellEditEndingEventArgs e)
        {
            if (e.EditAction != DataGridEditAction.Commit) return;
            if (!(e.Row.Item is ServoRegisterEntry entry)) return;

            // Read the edited text from the cell
            if (e.EditingElement is TextBox tb)
            {
                if (int.TryParse(tb.Text, out int newValue))
                {
                    entry.CurrentValue = newValue;
                }
            }

            // TODO: implement actual servo register write logic here
            ServoRegisterValueChanged?.Invoke(this, entry);
        }

        // ---------------------------------------------------------------
        // Existing slider handlers (unchanged)
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
