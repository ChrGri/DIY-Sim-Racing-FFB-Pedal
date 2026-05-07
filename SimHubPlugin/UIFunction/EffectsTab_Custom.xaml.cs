using NCalc;
using System;
using System.Collections.Generic;
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

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// EffectsTab_Custom1.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_Custom : UserControl
    {
        public DIY_FFB_Pedal Plugin { get; set; }
        public EffectsTab_Custom()
        {
            InitializeComponent();
        }

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_Custom),
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
            typeof(EffectsTab_Custom),
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
            typeof(EffectsTab_Custom),
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
                if (Settings != null)
                {
                    checkbox_enable_CV1.IsChecked = (Settings.CV1_enable_flag[Settings.table_selected] == true);
                    Slider_CV1_trigger.SliderValue = Settings.CV1_trigger[Settings.table_selected];
                    if (calculation.Update_CV1_textbox)
                    {
                        calculation.Update_CV1_textbox = false;
                        textBox_CV1_string.Text = Settings.CV1_bindings[Settings.table_selected];
                    }
                    checkbox_enable_CV2.IsChecked = (Settings.CV2_enable_flag[Settings.table_selected] == true);
                    Slider_CV2_trigger.SliderValue = Settings.CV2_trigger[Settings.table_selected];
                    if (calculation.Update_CV2_textbox)
                    {
                        calculation.Update_CV2_textbox = false;
                        textBox_CV2_string.Text = Settings.CV2_bindings[Settings.table_selected];
                    }
                    checkbox_enable_CV3.IsChecked = (Settings.CV3_enable_flag[Settings.table_selected] == true);
                    Slider_CV3_trigger.SliderValue = Settings.CV3_trigger[Settings.table_selected];
                    if (calculation.Update_CV3_textbox)
                    {
                        calculation.Update_CV3_textbox = false;
                        textBox_CV3_string.Text = Settings.CV3_bindings[Settings.table_selected];
                    }
                    checkbox_enable_CV4.IsChecked = (Settings.CV4_enable_flag[Settings.table_selected] == true);
                    Slider_CV4_trigger.SliderValue = Settings.CV4_trigger[Settings.table_selected];
                    if (calculation.Update_CV4_textbox)
                    {
                        calculation.Update_CV4_textbox = false;
                        textBox_CV4_string.Text = Settings.CV4_bindings[Settings.table_selected];
                    }

                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_Custom;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                try
                {
                    control.updateUI();
                }
                catch
                {
                }
            }

        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_Custom;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_CV1_AMP.SliderValue = (double)control.dap_config_st.payloadPedalConfig_.CV_amp_1 / 1000.0d * 100.0d;
                    control.Slider_CV1_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.CV_freq_1;
                    control.Slider_CV2_AMP.SliderValue = (double)control.dap_config_st.payloadPedalConfig_.CV_amp_2 / 1000.0d * 100.0d;
                    control.Slider_CV2_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.CV_freq_2;
                    control.Slider_CV3_AMP.SliderValue = (double)control.dap_config_st.payloadPedalConfig_.CV_amp_3 / 1000.0d * 100.0d;
                    control.Slider_CV3_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.CV_freq_3;
                    control.Slider_CV4_AMP.SliderValue = (double)control.dap_config_st.payloadPedalConfig_.CV_amp_4 / 1000.0d * 100.0d;
                    control.Slider_CV4_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.CV_freq_4;
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_Custom;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
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

        private void checkbox_enable_CV1_Checked(object sender, RoutedEventArgs e)
        {
            Settings.CV1_enable_flag[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_CV1_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.CV1_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void Bind_CV1_Click(object sender, RoutedEventArgs e)
        {
            NcalcScriptEditor sideWindow = new NcalcScriptEditor(Plugin, 1, (int)Plugin.Settings.table_selected);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            sideWindow.input = Plugin.Settings.CV1_bindings[Plugin.Settings.table_selected];
            if (sideWindow.ShowDialog() == true)
            {
                textBox_CV1_string.Text = sideWindow.input;
                Plugin.Settings.CV1_bindings[Plugin.Settings.table_selected] = sideWindow.input;
            }
        }

        private void Clear_CV1_Click(object sender, RoutedEventArgs e)
        {
            textBox_CV1_string.Text = "";
            Settings.CV1_bindings[Settings.table_selected] = (string)textBox_CV1_string.Text;
            Settings.CV1_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        

        private void Slider_CV1_trigger_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.CV1_trigger[Settings.table_selected] = (Byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV1_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_amp_1 = (Byte)(e.NewValue * 1000.0d / 100.0d);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_CV1_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_freq_1 = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
        //CV2
        private void checkbox_enable_CV2_Checked(object sender, RoutedEventArgs e)
        {
            Settings.CV2_enable_flag[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_CV2_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.CV2_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }


        private void Bind_CV2_Click(object sender, RoutedEventArgs e)
        {
            NcalcScriptEditor sideWindow = new NcalcScriptEditor(Plugin, 2, (int)Plugin.Settings.table_selected);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            sideWindow.input = Plugin.Settings.CV2_bindings[Plugin.Settings.table_selected];
            if (sideWindow.ShowDialog() == true)
            {
                textBox_CV2_string.Text = sideWindow.input;
                Plugin.Settings.CV2_bindings[Plugin.Settings.table_selected] = sideWindow.input;
            }
        }

        private void Clear_CV2_Click(object sender, RoutedEventArgs e)
        {
            textBox_CV2_string.Text = "";
            Settings.CV2_bindings[Settings.table_selected] = (string)textBox_CV2_string.Text;
            Settings.CV2_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV2_trigger_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.CV2_trigger[Settings.table_selected] = (Byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV2_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_amp_2 = (Byte)(e.NewValue * 1000.0d / 100.0d);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_CV2_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_freq_2 = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        //CV3
        private void checkbox_enable_CV3_Checked(object sender, RoutedEventArgs e)
        {
            Settings.CV3_enable_flag[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_CV3_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.CV3_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }


        private void Bind_CV3_Click(object sender, RoutedEventArgs e)
        {
            NcalcScriptEditor sideWindow = new NcalcScriptEditor(Plugin, 2, (int)Plugin.Settings.table_selected);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            sideWindow.input = Plugin.Settings.CV3_bindings[Plugin.Settings.table_selected];
            if (sideWindow.ShowDialog() == true)
            {
                textBox_CV3_string.Text = sideWindow.input;
                Plugin.Settings.CV3_bindings[Plugin.Settings.table_selected] = sideWindow.input;
            }
        }

        private void Clear_CV3_Click(object sender, RoutedEventArgs e)
        {
            textBox_CV3_string.Text = "";
            Settings.CV3_bindings[Settings.table_selected] = (string)textBox_CV3_string.Text;
            Settings.CV3_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV3_trigger_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.CV3_trigger[Settings.table_selected] = (Byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV3_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_amp_3 = (Byte)(e.NewValue * 1000.0d / 100.0d);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_CV3_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_freq_3 = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        //CV4
        private void checkbox_enable_CV4_Checked(object sender, RoutedEventArgs e)
        {
            Settings.CV4_enable_flag[Settings.table_selected] = true;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_CV4_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.CV4_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }



        private void Bind_CV4_Click(object sender, RoutedEventArgs e)
        {
            NcalcScriptEditor sideWindow = new NcalcScriptEditor(Plugin, 2, (int)Plugin.Settings.table_selected);
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            sideWindow.input = Plugin.Settings.CV4_bindings[Plugin.Settings.table_selected];
            if (sideWindow.ShowDialog() == true)
            {
                textBox_CV4_string.Text = sideWindow.input;
                Plugin.Settings.CV4_bindings[Plugin.Settings.table_selected] = sideWindow.input;
            }
        }

        private void Clear_CV4_Click(object sender, RoutedEventArgs e)
        {
            textBox_CV4_string.Text = "";
            Settings.CV4_bindings[Settings.table_selected] = (string)textBox_CV4_string.Text;
            Settings.CV4_enable_flag[Settings.table_selected] = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV4_trigger_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.CV4_trigger[Settings.table_selected] = (Byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_CV4_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_amp_4 = (Byte)(e.NewValue * 1000.0d / 100.0d);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_CV4_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.CV_freq_4 = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}
