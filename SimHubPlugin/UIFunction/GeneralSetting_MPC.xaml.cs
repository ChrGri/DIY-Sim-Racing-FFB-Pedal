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
    /// GeneralSetting_MPC.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_MPC : UserControl
    {
        public GeneralSetting_MPC()
        {
            InitializeComponent();
            DataContext = this;
        }

        public static readonly DependencyProperty MPCGain_Property =
DependencyProperty.Register(nameof(MPC_Gain), typeof(double), typeof(GeneralSetting_MPC),
new FrameworkPropertyMetadata(10.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double MPC_Gain
        {
            get => (double)GetValue(MPCGain_Property);
            set {
                SetValue(MPCGain_Property, value);
                //Slider_MPC_0th_gain.SliderValue= value;
                
            } 
        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_MPC control)
            {
                //control.UpdateLabelContent();
            }
        }
        public event RoutedPropertyChangedEventHandler<double> MPCGain_ValueChanged;
        public void MPCGainValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            MPC_Gain = e.NewValue;
            MPCGain_ValueChanged?.Invoke(this, e);
        }
    }
}
