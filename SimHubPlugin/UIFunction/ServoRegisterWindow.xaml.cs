using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.Windows.Media;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// Interaction logic for ServoRegisterWindow.xaml
    /// Provides an independent window for managing, tuning, importing/exporting, and flashing iSV57 servo registers.
    /// </summary>
    public partial class ServoRegisterWindow : Window
    {
        private readonly DIYFFBPedalControlUI _mainUI;

        // Static register definition table (address + name)
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
            ("Pr3.15", "Speed zero-clamp level"),
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
            ("Pr4.10", "Input selection SI11"),
            ("Pr4.11", "Output selection SO1"),
            ("Pr4.12", "Output selection SO2"),
            ("Pr4.13", "Output selection SO3"),
            ("Pr4.14", "Output selection SO4"),
            ("Pr4.15", "Output selection SO5"),
            ("Pr4.16", "Output selection SO6"),
            ("Pr4.17", "Output selection SO7"),
            ("Pr4.18", "Output selection SO8"),
            ("Pr4.19", "Output selection SO9"),
            ("Pr4.20", "Reserved parameter"),
            ("Pr4.21", "Reserved parameter"),
            ("Pr4.22", "Mechanical brake action at stalling setup"),
            ("Pr4.23", "Mechanical brake action at running setup"),
            ("Pr4.24", "Braking release delay time"),
            ("Pr4.25", "Positioning complete range"),
            ("Pr4.26", "Positioning complete output setup"),
            ("Pr4.27", "Positioning complete hold time"),
            ("Pr4.28", "Reserved parameter"),
            ("Pr4.29", "Reserved parameter"),
            ("Pr4.30", "Reserved parameter"),
            ("Pr4.31", "Positioning command pass range"),
            ("Pr4.32", "Speed arrival output setup"),
            ("Pr4.33", "Speed arrival output range"),
            ("Pr4.34", "Speed reach output setup"),
            ("Pr4.35", "Speed reach range"),
            ("Pr4.36", "Speed reach output condition setup"),
            ("Pr4.37", "Torque arrival output setup"),
            ("Pr4.38", "Torque arrival level"),
            ("Pr4.39", "Torque arrival hysteresis"),
            ("Pr4.40", "Zero-speed clamp speed"),
            ("Pr4.41", "Zero-speed clamp hysteresis"),
            ("Pr4.42", "Reserved parameter"),
            ("Pr4.43", "Reserved parameter"),
            ("Pr4.44", "Reserved parameter"),
            ("Pr4.45", "Reserved parameter"),
            ("Pr4.46", "Reserved parameter"),
            ("Pr4.47", "Reserved parameter"),
            ("Pr4.48", "Reserved parameter"),
            ("Pr4.49", "Reserved parameter"),
            ("Pr5.00", "Reserved parameter"),
            ("Pr5.01", "Reserved parameter"),
            ("Pr5.02", "Reserved parameter"),
            ("Pr5.03", "Reserved parameter"),
            ("Pr5.04", "Drive inhibit input setup"),
            ("Pr5.05", "Sequence at drive inhibit input"),
            ("Pr5.06", "Sequence at servo off"),
            ("Pr5.07", "Main power off stop mode"),
            ("Pr5.08", "LV-trip selection at main power off"),
            ("Pr5.09", "The main power off detection time"),
            ("Pr5.10", "Sequence at alarm"),
            ("Pr5.11", "Torque setup for emergency stop"),
            ("Pr5.12", "Over-load level setup"),
            ("Pr5.13", "Over-speed level setup"),
            ("Pr5.14", "Motor working over-temperature alarm threshold setting"),
            ("Pr5.15", "Control power supply under-voltage alarm threshold setting"),
            ("Pr5.16", "Continuous regeneration processing time limit"),
            ("Pr5.17", "Internal braking resistor resistance"),
            ("Pr5.18", "Internal braking resistor power"),
            ("Pr5.19", "External braking resistor power"),
            ("Pr5.20", "Position setup unit select"),
            ("Pr5.21", "Selection of torque limit"),
            ("Pr5.22", "2nd torque limit"),
            ("Pr5.23", "Torque limit positive"),
            ("Pr5.24", "Torque limit negative"),
            ("Pr5.25", "Reserved parameter"),
            ("Pr5.26", "Reserved parameter"),
            ("Pr5.27", "Reserved parameter"),
            ("Pr5.28", "Reserved parameter"),
            ("Pr5.29", "Reserved parameter"),
            ("Pr5.30", "Reserved parameter"),
            ("Pr5.31", "Speed axis pulse output division numerator"),
            ("Pr5.32", "Reserved parameter"),
            ("Pr5.33", "Pulse output division denominator"),
            ("Pr5.34", "Reserved parameter"),
            ("Pr5.35", "Front panel lock"),
            ("Pr5.36", "Reserved parameter"),
            ("Pr5.37", "Reserved parameter"),
            ("Pr5.38", "Reserved parameter"),
            ("Pr5.39", "Reserved parameter"),
            ("Pr6.00", "Reserved parameter"),
            ("Pr6.01", "Reserved parameter"),
            ("Pr6.02", "Reserved parameter"),
            ("Pr6.03", "Reserved parameter"),
            ("Pr6.04", "Drive velocity limit"),
            ("Pr6.05", "Position 3rd gain valid setup"),
            ("Pr6.06", "Position 3rd gain multiplication"),
            ("Pr6.07", "Additional torque filter"),
            ("Pr6.08", "Torque compensation value"),
            ("Pr6.09", "Torque compensation direction"),
            ("Pr6.10", "Function extension setup"),
            ("Pr6.11", "Current response setup"),
            ("Pr6.12", "Current response setup"),
            ("Pr6.13", "Reserved parameter"),
            ("Pr6.14", "Emergency stop time at alarm"),
            ("Pr6.15", "Reserved parameter"),
            ("Pr6.16", "Reserved parameter"),
            ("Pr6.17", "Reserved parameter"),
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

        public ObservableCollection<ServoRegisterEntry> ServoRegisters { get; }
            = new ObservableCollection<ServoRegisterEntry>();

        private readonly Queue<ServoReadBatch> _pendingBatchQueue = new Queue<ServoReadBatch>();
        private readonly System.Windows.Threading.DispatcherTimer _readTimeoutTimer;
        private int _totalReads;

        private readonly Queue<ServoWriteBatch> _pendingWriteBatchQueue = new Queue<ServoWriteBatch>();
        private readonly System.Windows.Threading.DispatcherTimer _writeTimeoutTimer;
        private int _totalWrites;

        private bool _smoothSetShown;

        public byte SelectedPedalIndex
        {
            get
            {
                if (CboPedalSelect != null && CboPedalSelect.SelectedIndex >= 0)
                    return (byte)CboPedalSelect.SelectedIndex;
                return (byte)(_mainUI != null ? _mainUI.indexOfSelectedPedal_u : 1);
            }
        }

        public ServoRegisterWindow(DIYFFBPedalControlUI mainUI)
        {
            InitializeComponent();
            _mainUI = mainUI;

            // Timer fires if an ACK is not received within 2 s — skips the stalled batch
            _readTimeoutTimer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(2)
            };
            _readTimeoutTimer.Tick += OnReadTimeout;

            // Write timer fires if an ACK is not received within 1 s — skips the stalled batch
            _writeTimeoutTimer = new System.Windows.Threading.DispatcherTimer
            {
                Interval = TimeSpan.FromSeconds(1)
            };
            _writeTimeoutTimer.Tick += OnWriteTimeout;

            // Populate table
            int idx = 0;
            foreach (var def in s_registerDefs)
            {
                Isv57TunedParameters.TryGetValue(def.addr, false, out int recommended);
                ServoRegisters.Add(new ServoRegisterEntry
                {
                    Index = idx++,
                    Address = def.addr,
                    Name = def.name,
                    RecommendedValue = recommended,
                });
            }

            // Apply default sort by Index
            var view = CollectionViewSource.GetDefaultView(ServoRegisters);
            view.SortDescriptions.Clear();
            view.SortDescriptions.Add(new SortDescription(
                nameof(ServoRegisterEntry.Index),
                ListSortDirection.Ascending));

            // Select current pedal
            if (_mainUI != null)
            {
                int pedalIdx = (int)_mainUI.indexOfSelectedPedal_u;
                if (pedalIdx >= 0 && pedalIdx < CboPedalSelect.Items.Count)
                    CboPedalSelect.SelectedIndex = pedalIdx;
            }

            UpdateSummaryText();
        }

        private void UpdateSummaryText()
        {
            int liveCount = ServoRegisters.Count(r => r.LiveValue.HasValue);
            TxtSummary.Text = $"Total: {ServoRegisters.Count} registers | Live: {liveCount}";
        }

        private void CboPedalSelect_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            string pedalName = "Pedal " + SelectedPedalIndex;
            if (CboPedalSelect.SelectedItem is ComboBoxItem item)
                pedalName = item.Content.ToString();
            Title = $"Servo Register Editor - iSV57 [{pedalName}]";
            TxtStatus.Text = $"Target pedal set to: {pedalName}";
        }

        // ---------------------------------------------------------------
        // Search & Filter
        // ---------------------------------------------------------------
        private void TxtSearch_TextChanged(object sender, TextChangedEventArgs e)
        {
            ApplyFilter();
        }

        private void BtnClearSearch_Click(object sender, RoutedEventArgs e)
        {
            TxtSearch.Text = string.Empty;
        }

        private void ApplyFilter()
        {
            var view = CollectionViewSource.GetDefaultView(ServoRegisters);
            if (string.IsNullOrWhiteSpace(TxtSearch.Text))
            {
                view.Filter = null;
            }
            else
            {
                string query = TxtSearch.Text.Trim();
                view.Filter = obj =>
                {
                    if (obj is ServoRegisterEntry entry)
                    {
                        return (entry.Address != null && entry.Address.IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0)
                            || (entry.Name != null && entry.Name.IndexOf(query, StringComparison.OrdinalIgnoreCase) >= 0);
                    }
                    return false;
                };
            }
        }

        // ---------------------------------------------------------------
        // Sequential batch load (Read)
        // ---------------------------------------------------------------
        private void Btn_LoadFromServo_Click(object sender, RoutedEventArgs e)
        {
            _readTimeoutTimer.Stop();
            _pendingBatchQueue.Clear();

            foreach (var entry in ServoRegisters)
                entry.LiveValue = null;

            ServoReadBatch currentBatch = new ServoReadBatch();
            foreach (var entry in ServoRegisters)
            {
                if (DapAttrHelper.TryParseServoAddress(entry.Address, out ushort addr))
                {
                    currentBatch.Addresses.Add(addr);
                    currentBatch.PendingAddresses.Add(addr);

                    if (currentBatch.Addresses.Count == 10)
                    {
                        _pendingBatchQueue.Enqueue(currentBatch);
                        currentBatch = new ServoReadBatch();
                    }
                }
            }
            if (currentBatch.Addresses.Count > 0)
            {
                _pendingBatchQueue.Enqueue(currentBatch);
            }

            _totalReads = _pendingBatchQueue.Count;
            ProgressBarMain.Minimum = 0;
            ProgressBarMain.Maximum = _totalReads;
            ProgressBarMain.Value = 0;
            ProgressBarMain.Visibility = Visibility.Visible;
            TxtStatus.Text = $"Starting read from servo (Pedal {SelectedPedalIndex})...";

            SendNextPendingBatch();
        }

        private void SendNextPendingBatch()
        {
            _readTimeoutTimer.Stop();
            if (_pendingBatchQueue.Count == 0)
            {
                ProgressBarMain.Visibility = Visibility.Collapsed;
                TxtStatus.Text = "Finished loading registers from servo.";
                UpdateSummaryText();
                ShowParameterSetOfServo();
                return;
            }

            int processed = _totalReads - _pendingBatchQueue.Count;
            ProgressBarMain.Value = processed;
            TxtStatus.Text = $"Reading from servo... Batch {processed + 1} of {_totalReads}";

            ServoReadBatch nextBatch = _pendingBatchQueue.Peek();
            _readTimeoutTimer.Start();

            _mainUI?.SendServoModbusBatchRead(SelectedPedalIndex, nextBatch.Addresses);
        }

        private void OnReadTimeout(object sender, EventArgs e)
        {
            _readTimeoutTimer.Stop();
            if (_pendingBatchQueue.Count > 0)
                _pendingBatchQueue.Dequeue(); // skip stalled batch
            TxtStatus.Text = "Read timeout on batch, proceeding...";
            SendNextPendingBatch();
        }

        /// <summary>
        /// Called when the ESP32 returns a ServoModbus ACK packet.
        /// </summary>
        public void HandleServoModbusAck(ushort addr, short value)
        {
            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.Invoke(() => HandleServoModbusAck(addr, value));
                return;
            }

            foreach (var entry in ServoRegisters)
            {
                if (DapAttrHelper.TryParseServoAddress(entry.Address, out ushort entryAddr) && entryAddr == addr)
                {
                    entry.LiveValue = (int)value;
                    break;
                }
            }
            UpdateSummaryText();

            // Advance read queue
            if (_pendingBatchQueue.Count > 0)
            {
                ServoReadBatch currentBatch = _pendingBatchQueue.Peek();
                if (currentBatch.PendingAddresses.Contains(addr))
                {
                    currentBatch.PendingAddresses.Remove(addr);
                    if (currentBatch.PendingAddresses.Count == 0)
                    {
                        _pendingBatchQueue.Dequeue();
                        SendNextPendingBatch();
                    }
                }
            }

            // Advance write queue
            if (_pendingWriteBatchQueue.Count > 0)
            {
                ServoWriteBatch currentWriteBatch = _pendingWriteBatchQueue.Peek();
                if (currentWriteBatch.PendingAddresses.Contains(addr))
                {
                    currentWriteBatch.PendingAddresses.Remove(addr);
                    if (currentWriteBatch.PendingAddresses.Count == 0)
                    {
                        _pendingWriteBatchQueue.Dequeue();
                        SendNextPendingWriteBatch();
                    }
                }
            }
        }

        // ---------------------------------------------------------------
        // Sequential batch write
        // ---------------------------------------------------------------
        private void WriteAllRegistersToServo()
        {
            _writeTimeoutTimer.Stop();
            _pendingWriteBatchQueue.Clear();

            ServoWriteBatch currentBatch = new ServoWriteBatch();
            foreach (var entry in ServoRegisters)
            {
                if (entry.LiveValue.HasValue)
                {
                    if (DapAttrHelper.TryParseServoAddress(entry.Address, out ushort addr))
                    {
                        currentBatch.Entries.Add(entry);
                        currentBatch.PendingAddresses.Add(addr);

                        if (currentBatch.Entries.Count == 10)
                        {
                            _pendingWriteBatchQueue.Enqueue(currentBatch);
                            currentBatch = new ServoWriteBatch();
                        }
                    }
                }
            }
            if (currentBatch.Entries.Count > 0)
            {
                _pendingWriteBatchQueue.Enqueue(currentBatch);
            }

            if (_pendingWriteBatchQueue.Count == 0)
            {
                _mainUI?.SendServoFlash(SelectedPedalIndex);
                TxtStatus.Text = "No values to write. Flash command sent.";
                return;
            }

            _totalWrites = _pendingWriteBatchQueue.Count;
            ProgressBarMain.Minimum = 0;
            ProgressBarMain.Maximum = _totalWrites;
            ProgressBarMain.Value = 0;
            ProgressBarMain.Visibility = Visibility.Visible;
            TxtStatus.Text = $"Starting write to servo (Pedal {SelectedPedalIndex})...";

            SendNextPendingWriteBatch();
        }

        private void SendNextPendingWriteBatch()
        {
            _writeTimeoutTimer.Stop();
            if (_pendingWriteBatchQueue.Count == 0)
            {
                ProgressBarMain.Visibility = Visibility.Collapsed;
                _mainUI?.SendServoFlash(SelectedPedalIndex);
                TxtStatus.Text = "Finished writing registers. Flash command sent. Please power-cycle the servo!";
                MessageBox.Show("All registers written and stored to servo NVM successfully!\n\nPlease power-cycle the servo for settings to take effect.",
                                "Servo Flash Complete", MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }

            int processed = _totalWrites - _pendingWriteBatchQueue.Count;
            ProgressBarMain.Value = processed;
            TxtStatus.Text = $"Writing to servo... Batch {processed + 1} of {_totalWrites}";

            ServoWriteBatch nextBatch = _pendingWriteBatchQueue.Peek();
            _writeTimeoutTimer.Start();

            _mainUI?.SendServoBatchWrite(SelectedPedalIndex, nextBatch.Entries.ToArray());
        }

        private void OnWriteTimeout(object sender, EventArgs e)
        {
            _writeTimeoutTimer.Stop();
            if (_pendingWriteBatchQueue.Count > 0)
                _pendingWriteBatchQueue.Dequeue();
            TxtStatus.Text = "Write timeout on batch, proceeding...";
            SendNextPendingWriteBatch();
        }

        // ---------------------------------------------------------------
        // Buttons
        // ---------------------------------------------------------------
        private void Btn_ResetToRecommended_Click(object sender, RoutedEventArgs e)
        {
            bool? smooth = AskParameterSet();
            if (!smooth.HasValue) return;

            ShowRecommendedSet(smooth.Value);
            foreach (var entry in ServoRegisters)
                entry.LiveValue = entry.RecommendedValue;

            ServoRegisterGrid.Items.Refresh();
            UpdateSummaryText();
            WriteAllRegistersToServo();
        }

        private void Btn_FlashToServo_Click(object sender, RoutedEventArgs e)
        {
            var res = MessageBox.Show("Save all current live parameters to servo non-volatile memory (NVM)?\nA power-cycle of the servo is required afterwards.",
                                      "Flash to Servo", MessageBoxButton.YesNo, MessageBoxImage.Question);
            if (res == MessageBoxResult.Yes)
            {
                WriteAllRegistersToServo();
            }
        }

        private void Btn_ResetToFactory_Click(object sender, RoutedEventArgs e)
        {
            var res = MessageBox.Show($"Reset servo parameters on Pedal {SelectedPedalIndex} to factory defaults?\nA power-cycle of the servo is required afterwards.",
                                      "Reset to Factory", MessageBoxButton.YesNo, MessageBoxImage.Warning);
            if (res == MessageBoxResult.Yes)
            {
                _mainUI?.SendServoResetToFactory(SelectedPedalIndex);
                TxtStatus.Text = "Factory reset command sent. Please power-cycle the servo.";
            }
        }

        private void ShowRecommendedSet(bool smooth)
        {
            _smoothSetShown = smooth;
            foreach (var entry in ServoRegisters)
            {
                Isv57TunedParameters.TryGetValue(entry.Address, smooth, out int recommended);
                entry.RecommendedValue = recommended;
            }
            if (ServoRegisterGrid.Columns.Count > 3)
                ServoRegisterGrid.Columns[3].Header = smooth ? "Recommended (smooth)" : "Recommended (silent)";
        }

        private void ShowParameterSetOfServo()
        {
            var mode = ServoRegisters.FirstOrDefault(r => r.Address == "Pr1.15");
            if (mode == null || !mode.LiveValue.HasValue) return;
            Isv57TunedParameters.TryGetValue("Pr1.15", true, out int smoothMode);
            Isv57TunedParameters.TryGetValue("Pr1.15", false, out int silentMode);
            if (smoothMode == silentMode) return;
            ShowRecommendedSet(mode.LiveValue.Value == smoothMode);
        }

        private bool? AskParameterSet()
        {
            bool? result = null;
            var dialog = new Window
            {
                Title = "Reset to recommended",
                Width = 460,
                SizeToContent = SizeToContent.Height,
                WindowStartupLocation = WindowStartupLocation.CenterScreen,
                WindowStyle = WindowStyle.ToolWindow,
                ResizeMode = ResizeMode.NoResize,
                Topmost = true,
                Background = new SolidColorBrush(Color.FromRgb(0x2A, 0x2A, 0x2A))
            };
            var panel = new StackPanel { Margin = new Thickness(16) };
            panel.Children.Add(new TextBlock
            {
                TextWrapping = TextWrapping.Wrap,
                Foreground = new SolidColorBrush(Color.FromRgb(0xF0, 0xF0, 0xF0)),
                FontSize = 13,
                Margin = new Thickness(0, 0, 0, 12),
                Text = "Which servo parameter set should be written to the servo and stored in its NVM?\n\n" +
                       "Silent: one fixed gain set. Quiet, slightly rougher pedal feel (motor cogging).\n\n" +
                       "Smooth: gain switching. A fast gain set while the pedal moves rejects the motor " +
                       "cogging better (smoother feel), but the servo is louder while moving. At " +
                       "standstill a quiet gain set is used.\n\n" +
                       "The servo needs a power-cycle afterwards."
            });
            var buttons = new StackPanel { Orientation = Orientation.Horizontal, HorizontalAlignment = HorizontalAlignment.Right };
            Button MakeButton(string text, bool? value)
            {
                var b = new Button { Content = text, MinWidth = 90, Margin = new Thickness(6, 0, 0, 0), Padding = new Thickness(8, 3, 8, 3) };
                b.Click += (s, ev) => { result = value; dialog.Close(); };
                buttons.Children.Add(b);
                return b;
            }
            MakeButton("Silent", false).IsDefault = !_smoothSetShown;
            MakeButton("Smooth", true).IsDefault = _smoothSetShown;
            MakeButton("Cancel", null).IsCancel = true;
            panel.Children.Add(buttons);
            dialog.Content = panel;
            dialog.ShowDialog();
            return result;
        }

        // ---------------------------------------------------------------
        // Export & Import
        // ---------------------------------------------------------------
        private static string ServoParameterDirectory()
        {
            string simhubDirectory = System.IO.Directory.GetCurrentDirectory();
            string servoDirectory = System.IO.Path.Combine(simhubDirectory, "PluginsData", "Common", "DiyFfbPedal", "servo");
            try
            {
                System.IO.Directory.CreateDirectory(servoDirectory);
                return servoDirectory;
            }
            catch (Exception)
            {
                return simhubDirectory;
            }
        }

        private void Btn_ExportServoParameters_Click(object sender, RoutedEventArgs e)
        {
            var known = ServoRegisters.Where(r => r.LiveValue.HasValue).ToList();
            if (known.Count == 0)
            {
                MessageBox.Show("No live values yet. Click 'Load from servo' first.",
                                "Export servo parameters", MessageBoxButton.OK, MessageBoxImage.Information);
                return;
            }
            int unknown = ServoRegisters.Count - known.Count;
            if (unknown > 0 &&
                MessageBox.Show($"{unknown} registers have not been read from the servo. Export only the {known.Count} read values?",
                                "Export servo parameters", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
            {
                return;
            }

            var dlg = new Microsoft.Win32.SaveFileDialog
            {
                Title = "Export servo parameters",
                Filter = "Servo parameters (*.csv)|*.csv|All files (*.*)|*.*",
                DefaultExt = ".csv",
                AddExtension = true,
                InitialDirectory = ServoParameterDirectory(),
                FileName = $"ServoParameters_Pedal{SelectedPedalIndex}_{DateTime.Now:yyyyMMdd_HHmmss}.csv"
            };
            if (dlg.ShowDialog() != true) return;

            var sb = new StringBuilder();
            sb.AppendLine("register,value,name");
            foreach (var entry in known.OrderBy(r => r.Index))
            {
                sb.AppendLine($"{entry.Address},{entry.LiveValue.Value.ToString(System.Globalization.CultureInfo.InvariantCulture)},{entry.Name}");
            }
            try
            {
                System.IO.File.WriteAllText(dlg.FileName, sb.ToString(), Encoding.UTF8);
                TxtStatus.Text = $"Exported {known.Count} registers to {System.IO.Path.GetFileName(dlg.FileName)}";
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Could not write the file:\n{ex.Message}", "Export servo parameters",
                                MessageBoxButton.OK, MessageBoxImage.Error);
            }
        }

        private void Btn_ImportServoParameters_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new Microsoft.Win32.OpenFileDialog
            {
                Title = "Import servo parameters",
                Filter = "Servo parameters (*.csv)|*.csv|All files (*.*)|*.*",
                InitialDirectory = ServoParameterDirectory()
            };
            if (dlg.ShowDialog() != true) return;

            var values = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
            var invalidLines = new List<int>();
            try
            {
                string[] lines = System.IO.File.ReadAllLines(dlg.FileName);
                for (int i = 0; i < lines.Length; i++)
                {
                    string line = lines[i].Trim();
                    if (line.Length == 0 || line.StartsWith("register", StringComparison.OrdinalIgnoreCase))
                        continue;
                    string[] fields = line.Split(new[] { ',' }, 3);
                    if (fields.Length >= 2 &&
                        int.TryParse(fields[1].Trim(), System.Globalization.NumberStyles.Integer,
                                     System.Globalization.CultureInfo.InvariantCulture, out int value))
                    {
                        values[fields[0].Trim()] = value;
                    }
                    else
                    {
                        invalidLines.Add(i + 1);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($"Could not read the file:\n{ex.Message}", "Import servo parameters",
                                MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            var matched = ServoRegisters.Where(r => values.ContainsKey(r.Address)).ToList();
            int unknownRegisters = values.Count - matched.Count;
            if (matched.Count == 0)
            {
                MessageBox.Show("The file contains no known servo registers.", "Import servo parameters",
                                MessageBoxButton.OK, MessageBoxImage.Warning);
                return;
            }

            var changed = matched.Where(r => r.LiveValue != values[r.Address])
                                 .Select(r => $"{r.Address}: {(r.LiveValue.HasValue ? r.LiveValue.Value.ToString() : "?")} -> {values[r.Address]}")
                                 .ToList();
            var message = new StringBuilder();
            message.AppendLine($"Write {matched.Count} registers to the servo (Pedal {SelectedPedalIndex}) and store them in its NVM?");
            message.AppendLine("The servo needs a power-cycle afterwards.");
            if (unknownRegisters > 0)
                message.AppendLine($"\n{unknownRegisters} unknown registers in the file are skipped.");
            if (invalidLines.Count > 0)
                message.AppendLine($"Lines without a valid value are skipped: {string.Join(", ", invalidLines.Take(10))}{(invalidLines.Count > 10 ? ", ..." : "")}");
            if (changed.Count > 0)
            {
                message.AppendLine($"\nChanges against the table ({changed.Count}):");
                foreach (string c in changed.Take(15))
                    message.AppendLine("  " + c);
                if (changed.Count > 15)
                    message.AppendLine("  ...");
            }
            if (MessageBox.Show(message.ToString(), "Import servo parameters",
                                MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
                return;

            foreach (var entry in matched)
                entry.LiveValue = values[entry.Address];
            ServoRegisterGrid.Items.Refresh();
            UpdateSummaryText();

            WriteAllRegistersToServo();
        }

        // ---------------------------------------------------------------
        // DataGrid cell-edit handler
        // ---------------------------------------------------------------
        private void ServoRegisterGrid_CellEditEnding(object sender, DataGridCellEditEndingEventArgs e)
        {
            if (e.EditAction != DataGridEditAction.Commit) return;
            if (!(e.Row.Item is ServoRegisterEntry entry)) return;

            var editor = e.EditingElement as TextBox ?? FindDescendant<TextBox>(e.EditingElement);
            if (editor != null)
            {
                editor.GetBindingExpression(TextBox.TextProperty)?.UpdateSource();
                if (Validation.GetHasError(editor)) return;
            }

            _mainUI?.SendServoRegisterValue(SelectedPedalIndex, entry);
            TxtStatus.Text = $"Sent single write: {entry.Address} = {entry.LiveValue} to Pedal {SelectedPedalIndex}";
            UpdateSummaryText();
        }

        private static T FindDescendant<T>(DependencyObject root) where T : DependencyObject
        {
            if (root == null) return null;
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); i++)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                if (child is T match) return match;
                var nested = FindDescendant<T>(child);
                if (nested != null) return nested;
            }
            return null;
        }
    }
}
