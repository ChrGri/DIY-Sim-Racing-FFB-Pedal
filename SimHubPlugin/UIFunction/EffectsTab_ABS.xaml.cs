using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using UserControl = System.Windows.Controls.UserControl;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// EffectsTab_ABS.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_ABS : UserControl
    {
        public EffectsTab_ABS()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_ABS),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(EffectsTab_ABS),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);
                updateUI();
            }
        }

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(EffectsTab_ABS),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Cauculation_Property);
            set
            {
                SetValue(Cauculation_Property, value);
                //updateUI();
            }
        }

        private void updateUI()
        {
            try
            {
                if (Settings.ABS_enable_flag[Settings.table_selected] == 1)
                {
                    checkbox_enable_ABS.IsChecked = true;
                    checkbox_enable_ABS.Content = "ABS/TC Effect Enabled";
                }
                else
                {
                    checkbox_enable_ABS.IsChecked = false;
                    checkbox_enable_ABS.Content = "ABS/TC Effect Disabled";
                }
            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_ABS;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                try
                {
                    if (newData.ABS_enable_flag[newData.table_selected] == 1)
                    {
                        control.checkbox_enable_ABS.IsChecked = true;
                        control.checkbox_enable_ABS.Content = "ABS/TC Effect Enabled";
                    }
                    else
                    {
                        control.checkbox_enable_ABS.IsChecked = false;
                        control.checkbox_enable_ABS.Content = "ABS/TC Effect Disabled";
                    }
                }
                catch
                {

                }
            }

        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_ABS;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_ABS_freq.SliderValue = newData.payloadPedalConfig_.absFrequency;
                    control.Slider_ABS_AMP.SliderValue = ((double)newData.payloadPedalConfig_.absAmplitude) / (double)20.0f;
                    switch (newData.payloadPedalConfig_.absForceOrTarvelBit)
                    {
                        case 0:
                            control.Slider_ABS_AMP.Unit = "kg";
                            break;
                        case 1:
                            control.Slider_ABS_AMP.Unit = "%";
                            break;
                        default:
                            break;
                    }
                    if (newData.payloadPedalConfig_.Simulate_ABS_trigger == 1)
                    {
                        control.Simulate_ABS_check.IsChecked = true;
                    }
                    else
                    {
                        control.Simulate_ABS_check.IsChecked = false;
                    }

                    control.AbsPattern.SelectedIndex = newData.payloadPedalConfig_.absPattern;
                    control.EffectAppliedOnForceOrTravel_combobox.SelectedIndex = newData.payloadPedalConfig_.absForceOrTarvelBit;
                    control.update_plot_ABS();
                }
                catch
                { 
                
                }
                
            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_ABS;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
                    if (newData.SendAbsSignal) control.TestAbs_check.IsChecked = true;
                    else control.TestAbs_check.IsChecked= false;
                }
                catch
                {

                }
            }
        }
        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue)
        {
            SettingsChanged?.Invoke(this, newValue);
        }
        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
        }

        private void Slider_ABS_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.absFrequency = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_ABS();
        }

        private void Slider_ABS_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.absAmplitude = (Byte)(e.NewValue * 20);
            dap_config_st = tmp;
            
            switch (dap_config_st.payloadPedalConfig_.absForceOrTarvelBit)
            {
                case 0:
                    Slider_ABS_AMP.Unit = "kg";
                    //label_ABS_AMP.Content = "ABS/TC Amplitude: " + (float)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absAmplitude / 20.0f + "kg";
                    break;
                case 1:
                    Slider_ABS_AMP.Unit = "%";
                    //label_ABS_AMP.Content = "ABS/TC Amplitude: " + (float)dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.absAmplitude / 20.0f + "%";
                    break;
                default:
                    break;
            }
            update_plot_ABS();
            ConfigChangedEvent(dap_config_st);
            
        }

        private void Simulate_ABS_check_Checked(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.Simulate_ABS_trigger != 1)
            {
                calculation.IsUIRefreshNeeded = true;
                CalculationChangedEvent(calculation);
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.Simulate_ABS_trigger = 1;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
            }


        }

        private void Simulate_ABS_check_Unchecked(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.Simulate_ABS_trigger != 0)
            {
                calculation.IsUIRefreshNeeded = true;
                CalculationChangedEvent(calculation);
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.Simulate_ABS_trigger = 0;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
            }


        }

        private void EffectAppliedOnForceOrTravel_combobox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.absForceOrTarvelBit = (byte)EffectAppliedOnForceOrTravel_combobox.SelectedIndex;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                if (Slider_ABS_AMP != null)
                {
                    switch (dap_config_st.payloadPedalConfig_.absForceOrTarvelBit)
                    {
                        case 0:
                            Slider_ABS_AMP.Unit = "kg";
                            break;
                        case 1:
                            Slider_ABS_AMP.Unit = "%";                          
                            break;
                        default:
                            break;
                    }
                }
            }
            catch (Exception caughtEx)
            {
                string errorMessage = caughtEx.Message;
            }
        }


        private void btn_testABS_Click(object sender, RoutedEventArgs e)
        {
            if (calculation.SendAbsSignal)
            {
                calculation.SendAbsSignal = false;
                TestAbs_check.IsChecked = false;
            }
            else
            { 
                calculation.SendAbsSignal = true; 
                TestAbs_check.IsChecked = true;
            }
            CalculationChangedEvent(calculation);
        }

        private void AbsPattern_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.absPattern = (byte)AbsPattern.SelectedIndex;
            dap_config_st = tmp;
           
            update_plot_ABS();
            ConfigChangedEvent(dap_config_st);
        }

        private void checkbox_enable_ABS_Checked(object sender, RoutedEventArgs e)
        {
            Settings.ABS_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_ABS_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.ABS_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void update_plot_ABS()
        {
            int x_quantity = 200;
            double[] x = new double[x_quantity];
            double[] y = new double[x_quantity];

            double y_max = 50;
            double dx = canvas_plot_ABS.Width / x_quantity;
            double dy = canvas_plot_ABS.Height / y_max;
            double freq = dap_config_st.payloadPedalConfig_.absFrequency;
            double max_force = 255 / 20;
            double amp = ((double)dap_config_st.payloadPedalConfig_.absAmplitude) / 20;
            double peroid = x_quantity / freq;
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            if (dap_config_st.payloadPedalConfig_.absPattern == 0)
            {
                for (int idx = 0; idx < x_quantity; idx++)
                {
                    x[idx] = idx;
                    y[idx] = -1 * amp / max_force * Math.Sin(2 * x[idx] / peroid * Math.PI) * y_max / 2;
                    System.Windows.Point Pointlcl = new System.Windows.Point(dx * x[idx], dy * y[idx] + 25);
                    myPointCollection2.Add(Pointlcl);
                }

            }
            if (dap_config_st.payloadPedalConfig_.absPattern == 1)
            {
                for (int idx = 0; idx < x_quantity; idx++)
                {
                    x[idx] = idx;
                    y[idx] = -1 * amp / max_force * y_max * (x[idx] % peroid) / peroid + 0.5 * amp / max_force * y_max;
                    System.Windows.Point Pointlcl = new System.Windows.Point(dx * x[idx], dy * y[idx] + 25);
                    myPointCollection2.Add(Pointlcl);
                }
            }

            this.Polyline_plot_ABS.Points = myPointCollection2;
        }
    }
}
