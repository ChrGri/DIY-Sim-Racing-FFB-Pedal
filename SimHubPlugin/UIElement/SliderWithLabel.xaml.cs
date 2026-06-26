using System;
using System.Windows;
using System.Windows.Controls;

namespace DiyFfbPedal.UIElement
{
    public partial class SliderWithLabel : UserControl
    {
        public SliderWithLabel()
        {
            InitializeComponent();
        }

        // Dependency Property for slider_name
        public static readonly DependencyProperty SliderNameProperty =
            DependencyProperty.Register(nameof(SliderName), typeof(string), typeof(SliderWithLabel),
                new PropertyMetadata("Slider", OnPropertyChanged));

        public string SliderName
        {
            get => (string)GetValue(SliderNameProperty);
            set => SetValue(SliderNameProperty, value);
        }

        // Dependency Property for Unit
        public static readonly DependencyProperty UnitProperty =
            DependencyProperty.Register(nameof(Unit), typeof(string), typeof(SliderWithLabel),
                new PropertyMetadata("", OnPropertyChanged));

        public string Unit
        {
            get => (string)GetValue(UnitProperty);
            set => SetValue(UnitProperty, value);
        }

        // Dependency Property for Value
        public static readonly DependencyProperty ValueProperty =
            DependencyProperty.Register(nameof(SliderValue), typeof(double), typeof(SliderWithLabel),
                new FrameworkPropertyMetadata(0.0, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public double SliderValue
        {
            get => (double)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        // Dependency Property for MinValue
        public static readonly DependencyProperty MinValueProperty =
            DependencyProperty.Register(nameof(MinValue), typeof(double), typeof(SliderWithLabel),
                new PropertyMetadata(0.0));

        public double MinValue
        {
            get => (double)GetValue(MinValueProperty);
            set => SetValue(MinValueProperty, value);
        }

        // Dependency Property for MaxValue
        public static readonly DependencyProperty MaxValueProperty =
            DependencyProperty.Register(nameof(MaxValue), typeof(double), typeof(SliderWithLabel),
                new PropertyMetadata(100.0));

        public double MaxValue
        {
            get => (double)GetValue(MaxValueProperty);
            set => SetValue(MaxValueProperty, value);
        }

        // Dependency Property for TickFrequency
        public static readonly DependencyProperty TickFrequencyProperty =
            DependencyProperty.Register(nameof(TickFrequency), typeof(double), typeof(SliderWithLabel),
                new PropertyMetadata(1.0));

        public double TickFrequency
        {
            get => (double)GetValue(TickFrequencyProperty);
            set => SetValue(TickFrequencyProperty, value);
        }

        // Dependency Property for Length
        public static readonly DependencyProperty LengthProperty =
            DependencyProperty.Register(nameof(SliderLength), typeof(double), typeof(SliderWithLabel),
                new PropertyMetadata(200.0));

        public double SliderLength
        {
            get => (double)GetValue(LengthProperty);
            set => SetValue(LengthProperty, value);
        }

        // Dependency Property for LabelContent (read-only)
        public static readonly DependencyProperty LabelContentProperty =
            DependencyProperty.Register(nameof(LabelContent), typeof(string), typeof(SliderWithLabel),
                new PropertyMetadata(""));

        public string LabelContent
        {
            get => (string)GetValue(LabelContentProperty);
            private set => SetValue(LabelContentProperty, value);
        }

        // Update LabelContent when relevant properties change
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is SliderWithLabel control)
            {
                control.UpdateLabelContent();
            }
        }

        private void UpdateLabelContent()
        {
            LabelContent = SliderName + ": " + Math.Round(SliderValue, 4) + " " + Unit;
        }

        // Event to notify the main window of slider value changes
        public event RoutedPropertyChangedEventHandler<double> SliderValueChanged;

        private void SliderElement_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            // 忽略初始化期間因 MinValue 設定造成的自動校正雜訊
            if (!this.IsLoaded) return;

            SliderValue = e.NewValue;
            SliderValueChanged?.Invoke(this, e);
        }
    }
}
