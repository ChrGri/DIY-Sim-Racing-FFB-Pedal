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
    /// EffectsTab_BitePoint.xaml 的互動邏輯
    /// </summary>
    public partial class EffectsTab_BitePoint : UserControl
    {
        public EffectsTab_BitePoint()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
           nameof(dap_config_st),
           typeof(DAP_config_st),
           typeof(EffectsTab_BitePoint),
           new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));


        public DAP_config_st dap_config_st
        {

            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
                checkbox_enable_bite_point.IsChecked = (value.payloadPedalConfig_.BP_trigger == 1);
            }
        }
        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(EffectsTab_BitePoint),
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

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_BitePoint;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.Slider_BP_freq.SliderValue = control.dap_config_st.payloadPedalConfig_.BP_freq;
                    control.Slider_BP_AMP.SliderValue = (double)(control.dap_config_st.payloadPedalConfig_.BP_amp) / (double)100.0f;
                    control.checkbox_enable_bite_point.IsChecked = (control.dap_config_st.payloadPedalConfig_.BP_trigger == 1);
                    control.update_plot_BP();
                }
                catch
                {

                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_BitePoint;
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
        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
        }

        private void update_plot_BP()
        {
            int x_quantity = 200;
            double[] x = new double[x_quantity];
            double[] y = new double[x_quantity];

            double y_max = 50;
            double dx = canvas_plot_BP.Width / x_quantity;
            double dy = canvas_plot_BP.Height / y_max;
            double freq = dap_config_st.payloadPedalConfig_.BP_freq;
            double max_force = 200 / 20;
            double amp = ((double)dap_config_st.payloadPedalConfig_.BP_amp) / 20;
            double peroid = x_quantity / freq;
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            for (int idx = 0; idx < x_quantity; idx++)
            {
                x[idx] = idx;
                y[idx] = -1 * amp / max_force * Math.Sin(2 * x[idx] / peroid * Math.PI) * y_max / 2;
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * x[idx], dy * y[idx] + 25);
                myPointCollection2.Add(Pointlcl);
            }
            this.Polyline_plot_BP.Points = myPointCollection2;
        }

        private void checkbox_enable_bite_point_Checked(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.BP_trigger != 1)
            {
                calculation.IsUIRefreshNeeded = true;

                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.BP_trigger = 1;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
            }


        }

        private void checkbox_enable_bite_point_Unchecked(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.BP_trigger != 0)
            {
                calculation.IsUIRefreshNeeded = true;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.BP_trigger = 0;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
            }


        }

        private void Slider_BP_AMP_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.BP_amp = (Byte)(e.NewValue * 100);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_BP();
        }

        private void Slider_BP_freq_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.BP_freq = (Byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            update_plot_BP();
        }
    }
}
