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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// EffectsTab_Custom2.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_Custom2 : UserControl
    {
        public DIY_FFB_Pedal Plugin { get; set; }
        public EffectsTab_Custom2()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_Custom2),
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
            typeof(EffectsTab_Custom2),
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
            typeof(EffectsTab_Custom2),
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
                    checkbox_enable_CV2.IsChecked = (Settings.CV2_enable_flag[Settings.table_selected] == true);
                    Slider_CV2_trigger.SliderValue = Settings.CV2_trigger[Settings.table_selected];
                    if (calculation.Update_CV2_textbox)
                    {
                        calculation.Update_CV2_textbox = false;
                        textBox_CV2_string.Text = Settings.CV2_bindings[Settings.table_selected];
                    }

                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_Custom2;
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
            var control = d as EffectsTab_Custom2;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_CV2_AMP.SliderValue = (double)control.dap_config_st.payloadPedalConfig_.CV_amp_2 / (double)20.0f;
                    control.Slider_CV2_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.CV_freq_2;
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_Custom2;
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

        private void textBox_CV2_string_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                try
                {
                    string var1 = "";
                    var1 = Plugin.Ncalc_reading(textBox_CV2_string.Text.ToString());
                    Label_NCALC_CUS2.Content = var1;
                }
                catch { }
            }
            catch
            {

            }
        }

        private void Bind_CV2_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (Plugin.Ncalc_reading(textBox_CV2_string.Text) != "Error")
                {
                    Settings.CV2_bindings[Settings.table_selected] = (string)textBox_CV2_string.Text;
                    Settings.CV2_enable_flag[Settings.table_selected] = true;
                    SettingsChangedEvent(Settings);
                }
                else
                {
                    Plugin.Settings.CV2_enable_flag[Settings.table_selected] = false;
                    string MSG_tmp = "ERROR! String can not be evaluated";
                    System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

                }
            }
            catch
            {

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
            tmp.payloadPedalConfig_.CV_amp_2 = (Byte)(e.NewValue * 20);
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
    }
}
