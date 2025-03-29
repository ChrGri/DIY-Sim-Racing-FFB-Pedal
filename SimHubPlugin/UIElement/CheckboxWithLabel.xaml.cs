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

namespace User.PluginSdkDemo.UIElement
{
    /// <summary>
    /// CheckboxWithLabel.xaml 的互動邏輯
    /// </summary>
    public partial class CheckboxWithLabel : UserControl
    {
        public CheckboxWithLabel()
        {
            InitializeComponent();
        }
        // Dependency Property for LabelText
        public static readonly DependencyProperty LabelTextProperty =
            DependencyProperty.Register(nameof(LabelText), typeof(string), typeof(CheckboxWithLabel),
                new PropertyMetadata("Label"));

        public string LabelText
        {
            get => (string)GetValue(LabelTextProperty);
            set => SetValue(LabelTextProperty, value);
        }

        // Dependency Property for IsChecked
        public static readonly DependencyProperty IsCheckedProperty =
            DependencyProperty.Register(nameof(IsChecked), typeof(bool), typeof(CheckboxWithLabel),
                new PropertyMetadata(false));

        public bool IsChecked
        {
            get => (bool)GetValue(IsCheckedProperty);
            set => SetValue(IsCheckedProperty, value);
        }

        // Dependency Property for ControlWidth
        public static readonly DependencyProperty ControlWidthProperty =
            DependencyProperty.Register(nameof(ControlWidth), typeof(double), typeof(CheckboxWithLabel),
                new PropertyMetadata(300.0, OnWidthChanged));

        public double ControlWidth
        {
            get => (double)GetValue(ControlWidthProperty);
            set => SetValue(ControlWidthProperty, value);
        }

        // Label width automatically calculated as ControlWidth - 30
        public double LabelWidth => ControlWidth - 14;

        private static void OnWidthChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CheckboxWithLabel control)
            {
                control.Width = (double)e.NewValue;
                control.LabelElement.Width = control.LabelWidth;
            }
        }

        // Events for checkbox
        public event RoutedEventHandler Checked;
        public event RoutedEventHandler Unchecked;

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            Checked?.Invoke(this, e);
        }

        private void CheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            Unchecked?.Invoke(this, e);
        }
    }
}
