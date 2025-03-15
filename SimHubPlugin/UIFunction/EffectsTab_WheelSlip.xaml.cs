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
    /// EffectsTab_WheelSlip.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_WheelSlip : UserControl
    {
        public EffectsTab_WheelSlip()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_WheelSlip),
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
            typeof(EffectsTab_WheelSlip),
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



        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    Slider_WS_trigger.SliderValue = Settings.WS_trigger;
                    textBox_wheelslip_effect_string.Text = Settings.WSeffect_bind;
                    checkbox_enable_wheelslip.IsChecked = (Settings.WS_enable_flag[Settings.table_selected] == 1);
                }

            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_WheelSlip;
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
            var control = d as EffectsTab_WheelSlip;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_WS_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.WS_freq;
                    control.Slider_WS_AMP.SliderValue = (double)(control.dap_config_st.payloadPedalConfig_.WS_amp) / (double)20.0f;
                    control.update_plot_WS();
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

        private void checkbox_enable_wheelslip_Checked(object sender, RoutedEventArgs e)
        {
            Settings.WS_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }
        private void checkbox_enable_wheelslip_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.WS_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void Bind_WSeffect_Click(object sender, RoutedEventArgs e)
        {
            Settings.WSeffect_bind = (string)textBox_wheelslip_effect_string.Text;
            Settings.WS_enable_flag[Settings.table_selected] = 1;
            checkbox_enable_wheelslip.IsChecked = true;
            SettingsChangedEvent(Settings);
        }

        private void Clear_WSeffect_Click(object sender, RoutedEventArgs e)
        {
            Settings.WSeffect_bind = "";
            textBox_wheelslip_effect_string.Text = "";
            Settings.WS_enable_flag[Settings.table_selected] = 0;
            checkbox_enable_wheelslip.IsChecked = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_WS_trigger_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            Settings.WS_trigger = (int)(e.NewValue);
            SettingsChangedEvent(Settings);
        }

        private void Slider_WS_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.WS_amp = (Byte)(e.NewValue * 20);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_WS();
        }

        private void Slider_WS_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.WS_freq = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_WS();
        }
        private void update_plot_WS()
        {
            int x_quantity = 200;
            double[] x = new double[x_quantity];
            double[] y = new double[x_quantity];

            double y_max = 50;
            double dx = canvas_plot_WS.Width / x_quantity;
            double dy = canvas_plot_WS.Height / y_max;
            double freq = dap_config_st.payloadPedalConfig_.WS_freq;
            double max_force = 250 / 20;
            double amp = ((double)dap_config_st.payloadPedalConfig_.WS_amp) / 20;
            double peroid = x_quantity / freq;
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            for (int idx = 0; idx < x_quantity; idx++)
            {
                x[idx] = idx;
                y[idx] = -1 * amp / max_force * Math.Sin(2 * x[idx] / peroid * Math.PI) * y_max / 2;
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * x[idx], dy * y[idx] + 25);
                myPointCollection2.Add(Pointlcl);
            }
            this.Polyline_plot_WS.Points = myPointCollection2;
        }


    }
}
