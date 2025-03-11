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
    /// GeneralSetting_PID.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_PID : UserControl
    {
        public GeneralSetting_PID()
        {
            InitializeComponent();
            DataContext = this;
        }
        public static readonly DependencyProperty P_valueProperty =
DependencyProperty.Register(nameof(P_value), typeof(double), typeof(GeneralSetting_PID),
new FrameworkPropertyMetadata(0.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double P_value
        {
            get => (double)GetValue(P_valueProperty);
            set => SetValue(P_valueProperty, value);
        }

        public static readonly DependencyProperty I_valueProperty =
DependencyProperty.Register(nameof(I_value), typeof(double), typeof(GeneralSetting_PID),
new FrameworkPropertyMetadata(0.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double I_value
        {
            get => (double)GetValue(I_valueProperty);
            set => SetValue(I_valueProperty, value);
        }

        public static readonly DependencyProperty D_valueProperty =
DependencyProperty.Register(nameof(D_value), typeof(double), typeof(GeneralSetting_PID),
new FrameworkPropertyMetadata(0.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double D_value
        {
            get => (double)GetValue(D_valueProperty);
            set => SetValue(D_valueProperty, value);
        }

        public static readonly DependencyProperty FeedForwardGainProperty =
DependencyProperty.Register(nameof(FeedForwardGain), typeof(double), typeof(GeneralSetting_PID),
new FrameworkPropertyMetadata(0.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double FeedForwardGain
        {
            get => (double)GetValue(FeedForwardGainProperty);
            set => SetValue(FeedForwardGainProperty, value);
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_PID control)
            {
                //control.UpdateLabelContent();
            }
        }
        public event RoutedPropertyChangedEventHandler<double> P_ValueChanged;
        private void Slider_Pgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            P_value = e.NewValue;
            P_ValueChanged?.Invoke(this, e);
        }
        public event RoutedPropertyChangedEventHandler<double> D_ValueChanged;
        private void Slider_Dgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            D_value = e.NewValue;
            D_ValueChanged?.Invoke(this, e);
        }
        public event RoutedPropertyChangedEventHandler<double> FeedForwardGainChanged;
        private void Slider_VFgain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            FeedForwardGain = e.NewValue;
            FeedForwardGainChanged?.Invoke(this, e);
        }
        public event RoutedPropertyChangedEventHandler<double> I_ValueChanged;
        private void Slider_Igain_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            I_value = e.NewValue;
            I_ValueChanged?.Invoke(this, e);
        }
    }
}
