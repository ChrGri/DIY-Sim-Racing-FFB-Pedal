using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace DiyFfbPedal.UIElement
{
    public partial class SliderWithLabel : UserControl
    {
        private bool _isCommittingEdit = false;

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
            if (_isCommittingEdit) return;

            SliderValue = e.NewValue;
            SliderValueChanged?.Invoke(this, e);
        }

        private void LabelDisplay_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (!this.IsEnabled) return;
            BeginInlineEdit();
            e.Handled = true;
        }

        public void BeginInlineEdit()
        {
            if (PanelEdit == null || LabelDisplay == null || BoxEditValue == null) return;
            TxtEditPrefix.Text = (SliderName ?? "") + ": ";
            TxtEditUnit.Text = string.IsNullOrEmpty(Unit) ? "" : " " + Unit;
            BoxEditValue.Text = Math.Round(SliderValue, 4).ToString("0.####", System.Globalization.CultureInfo.InvariantCulture);
            LabelDisplay.Visibility = Visibility.Collapsed;
            PanelEdit.Visibility = Visibility.Visible;
            BoxEditValue.Focus();
            BoxEditValue.SelectAll();
        }

        private void CommitInlineEdit()
        {
            if (PanelEdit == null || PanelEdit.Visibility != Visibility.Visible) return;

            string text = (BoxEditValue.Text ?? string.Empty).Trim().Replace(',', '.');
            if (double.TryParse(text, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out double value))
            {
                value = Math.Max(MinValue, Math.Min(MaxValue, value));
                if (TickFrequency > 0)
                {
                    double ticks = Math.Round((value - MinValue) / TickFrequency);
                    value = MinValue + ticks * TickFrequency;
                    value = Math.Max(MinValue, Math.Min(MaxValue, value));
                    value = Math.Round(value, 4);
                }

                _isCommittingEdit = true;
                try
                {
                    double oldVal = SliderValue;
                    SliderValue = value;
                    if (SliderElement != null)
                    {
                        SliderElement.Value = value;
                    }
                    UpdateLabelContent();
                    SliderValueChanged?.Invoke(this, new RoutedPropertyChangedEventArgs<double>(oldVal, value));
                }
                finally
                {
                    _isCommittingEdit = false;
                }
            }
            EndInlineEdit();
        }

        private void EndInlineEdit()
        {
            if (PanelEdit != null) PanelEdit.Visibility = Visibility.Collapsed;
            if (LabelDisplay != null) LabelDisplay.Visibility = Visibility.Visible;
        }

        private void BoxEditValue_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                CommitInlineEdit();
                e.Handled = true;
            }
            else if (e.Key == Key.Escape)
            {
                EndInlineEdit();
                e.Handled = true;
            }
        }

        private void BoxEditValue_LostFocus(object sender, RoutedEventArgs e)
        {
            CommitInlineEdit();
        }
    }
}
