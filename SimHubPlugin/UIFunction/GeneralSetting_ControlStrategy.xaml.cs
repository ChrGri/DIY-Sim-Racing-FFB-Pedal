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
    /// GeneralSetting_ControlStrategy.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_ControlStrategy : UserControl
    {
        public GeneralSetting_ControlStrategy()
        {
            InitializeComponent();
            DataContext = this;
            //ControlStrategy = 0;
        }
        public static readonly DependencyProperty ControlStrategyProperty =
DependencyProperty.Register(nameof(ControlStrategy), typeof(int), typeof(GeneralSetting_ControlStrategy),
new FrameworkPropertyMetadata(0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));
        public int ControlStrategy
        {
            get => (int)GetValue(ControlStrategyProperty);
            set {
                SetValue(ControlStrategyProperty, value);
                ControlStrategy_Sel_1.IsChecked = (value == 0);
                ControlStrategy_Sel_2.IsChecked = (value == 1);
                ControlStrategy_Sel_3.IsChecked = (value == 2);
                //ControlStrategyChanged?.Invoke(control, newValue);
            }  
        }
        public event EventHandler<int> ControlStrategyChanged;
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_ControlStrategy control)
            {
                
                int newValue = (int)e.NewValue;
                control.ControlStrategy_Sel_1.IsChecked = (newValue == 0);
                control.ControlStrategy_Sel_2.IsChecked = (newValue == 1);
                control.ControlStrategy_Sel_3.IsChecked = (newValue == 2);
                control.ControlStrategyChanged?.Invoke(control, newValue);
                
                control.ControlStrategyChangedEvent(control.ControlStrategy);
            }
        }

        private void ControlStrategy_Sel_Checked(object sender, RoutedEventArgs e)
        {
            if (sender == ControlStrategy_Sel_1) ControlStrategy = 0;
            else if (sender == ControlStrategy_Sel_2) ControlStrategy = 1;
            else if (sender == ControlStrategy_Sel_3) ControlStrategy = 2;

            ControlStrategyChanged?.Invoke(this, ControlStrategy);
        }

        
        private void ControlStrategyChangedEvent(int newValue)
        {
            ControlStrategyChanged?.Invoke(this, newValue);
        }

    }
}
