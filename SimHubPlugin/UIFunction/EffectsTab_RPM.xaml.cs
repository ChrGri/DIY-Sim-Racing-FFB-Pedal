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
    /// EffectsTab_RPM.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_RPM : UserControl
    {
        public EffectsTab_RPM()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(EffectsTab_RPM),
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
            typeof(EffectsTab_RPM),
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
                checkbox_enable_RPM.IsChecked = (Settings.RPM_enable_flag[Settings.table_selected] == 1);
                RPMeffecttype_Sel_1.IsChecked = (Settings.RPM_effect_type == 0);
                RPMeffecttype_Sel_2.IsChecked = (Settings.RPM_effect_type == 1);
                /*
                if (Settings.RPM_enable_flag[Settings.table_selected] == 1)
                {
                    checkbox_enable_RPM.IsChecked = true;
                }
                else
                {
                    checkbox_enable_RPM.IsChecked = false;          
                }
                */
            }
            catch
            {

            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RPM;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                try
                {
                    /*
                    if (newData.RPM_enable_flag[newData.table_selected] == 1)
                    {
                        control.checkbox_enable_RPM.IsChecked = true;
                        
                    }
                    else
                    {
                        control.checkbox_enable_RPM.IsChecked = false;
                       
                    }

                    if (newData.RPM_effect_type == 0)
                    {
                        control.RPMeffecttype_Sel_1.IsChecked = true;
                    }
                    else
                    {
                        control.RPMeffecttype_Sel_2.IsChecked = true;
                    }
                    */
                }
                catch
                {

                }
            }

        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_RPM;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Rangeslider_RPM_freq.LowerValue = control.dap_config_st.payloadPedalConfig_.RPM_min_freq;
                    control.Rangeslider_RPM_freq.UpperValue = control.dap_config_st.payloadPedalConfig_.RPM_max_freq;
                    control.label_RPM_freq_max.Content = "MAX:" + control.dap_config_st.payloadPedalConfig_.RPM_max_freq + "Hz";
                    control.label_RPM_freq_min.Content = "MIN:" + control.dap_config_st.payloadPedalConfig_.RPM_min_freq + "Hz";
                    control.Slider_RPM_AMP.SliderValue = (double)(control.dap_config_st.payloadPedalConfig_.RPM_AMP) / (double)100.0f;
                    control.update_plot_RPM();
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


        private void checkbox_enable_RPM_Checked(object sender, RoutedEventArgs e)
        {
            Settings.RPM_enable_flag[Settings.table_selected] = 1;
            SettingsChangedEvent(Settings);
        }

        private void checkbox_enable_RPM_Unchecked(object sender, RoutedEventArgs e)
        {
            Settings.RPM_enable_flag[Settings.table_selected] = 0;
            SettingsChangedEvent(Settings);
        }

        private void RPMeffecttype_Sel_1_Checked(object sender, RoutedEventArgs e)
        {
            if (RPMeffecttype_Sel_1.IsChecked==true) Settings.RPM_effect_type = 0;
            if (RPMeffecttype_Sel_2.IsChecked == true) Settings.RPM_effect_type = 1;
            SettingsChangedEvent(Settings);
        }

        private void Slider_RPM_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_AMP = (Byte)(e.NewValue * 100);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_RPM();
        }

        private void Rangeslider_RPM_freq_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_min_freq = (byte)e.NewValue;
            label_RPM_freq_min.Content = "MIN:" + tmp.payloadPedalConfig_.RPM_min_freq + "Hz";
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_RPM();
        }

        private void Rangeslider_RPM_freq_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.RPM_max_freq = (byte)e.NewValue;
            label_RPM_freq_max.Content = "MAX:" + tmp.payloadPedalConfig_.RPM_max_freq + "Hz";
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_RPM();
        }
        private void update_plot_RPM()
        {
            int x_quantity = 1601;
            double[] x = new double[x_quantity];
            double[] y = new double[x_quantity];
            double[] peroid_x = new double[x_quantity];
            double[] freq = new double[x_quantity];
            double[] amp = new double[x_quantity];
            double y_max = 50;
            double dx = canvas_plot_RPM.Width / (x_quantity - 1);
            double dy = canvas_plot_RPM.Height / y_max;
            double freq_max = dap_config_st.payloadPedalConfig_.RPM_max_freq;
            double freq_min = dap_config_st.payloadPedalConfig_.RPM_min_freq;
            double max_force = 200 / 20 * 1.3;
            double amp_base = ((double)dap_config_st.payloadPedalConfig_.RPM_AMP) / 20;
            //double peroid = x_quantity / freq;
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            for (int idx = 0; idx < x_quantity; idx++)
            {
                x[idx] = idx;
                freq[idx] = freq_min + (((double)idx) / (double)x_quantity) * (freq_max - freq_min);
                peroid_x[idx] = x_quantity / freq[idx];
                amp[idx] = amp_base + amp_base * idx / x_quantity * 0.3;
                y[idx] = -1 * amp[idx] / max_force * Math.Sin(2 * x[idx] / peroid_x[idx] * Math.PI) * y_max / 2;
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * x[idx], dy * y[idx] + 25);
                myPointCollection2.Add(Pointlcl);
            }
            this.Polyline_plot_RPM.Points = myPointCollection2;
        }
    }
}
