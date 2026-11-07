using System;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;

namespace DiyFfbPedal.UIFunction
{
    public partial class LivePlotWindow : Window
    {
        public LivePlotWindow()
        {
            InitializeComponent();
        }

        public LivePlotWindow(DIY_FFB_Pedal plugin, DIYFFBPedalControlUI parentUI) : this()
        {
            if (LivePlotControl != null)
            {
                LivePlotControl.SetReferences(plugin, parentUI);
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            this.Topmost = true;
            UpdateTopmostButton();
            if (slider_opacity != null)
            {
                ApplyOpacity(slider_opacity.Value);
            }
            if (LivePlotControl?.btn_popout != null)
            {
                LivePlotControl.btn_popout.Visibility = Visibility.Collapsed;
            }
            LivePlotControl?.OnTabSelected(true);
        }

        private void Window_StateChanged(object sender, EventArgs e)
        {
            if (WindowState == WindowState.Minimized)
            {
                LivePlotControl?.OnTabSelected(false);
            }
            else
            {
                LivePlotControl?.OnTabSelected(true);
                if (btn_maximize != null)
                {
                    btn_maximize.Content = (WindowState == WindowState.Maximized) ? "❐" : "□";
                }
            }
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            LivePlotControl?.OnTabSelected(false);
            if (LivePlotControl?.Plugin?._calculations != null)
            {
                LivePlotControl.Plugin._calculations.OnExtendedStateReceived -= LivePlotControl.OnPacketReceived;
            }
        }

        private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                ToggleMaximize();
                return;
            }
            if (e.ButtonState == MouseButtonState.Pressed)
            {
                this.DragMove();
            }
        }

        private void ToggleMaximize()
        {
            WindowState = (WindowState == WindowState.Maximized) ? WindowState.Normal : WindowState.Maximized;
            if (btn_maximize != null)
            {
                btn_maximize.Content = (WindowState == WindowState.Maximized) ? "❐" : "□";
            }
        }

        private void SliderOpacity_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            double val = e.NewValue;
            if (tb_opacity_val != null)
            {
                tb_opacity_val.Text = $"{(int)(val * 100)}%";
            }
            ApplyOpacity(val);
        }

        private void ApplyOpacity(double alpha)
        {
            byte bgA = (byte)Math.Max(30, Math.Min(255, (int)(alpha * 230)));
            byte titleA = (byte)Math.Max(40, Math.Min(255, (int)(alpha * 240)));
            byte borderA = (byte)Math.Max(30, Math.Min(255, (int)(alpha * 180)));

            if (border_window_root != null)
            {
                border_window_root.Background = new SolidColorBrush(Color.FromArgb(bgA, 0x16, 0x16, 0x16));
                border_window_root.BorderBrush = new SolidColorBrush(Color.FromArgb(borderA, 0x55, 0x55, 0x55));
            }
            if (border_title_bar != null)
            {
                border_title_bar.Background = new SolidColorBrush(Color.FromArgb(titleA, 0x24, 0x24, 0x24));
            }
            LivePlotControl?.SetBackgroundOpacity(alpha);
        }

        private void BtnTopmost_Click(object sender, RoutedEventArgs e)
        {
            this.Topmost = !this.Topmost;
            UpdateTopmostButton();
        }

        private void UpdateTopmostButton()
        {
            if (btn_topmost == null) return;
            if (this.Topmost)
            {
                btn_topmost.Content = "📌 On Top";
                btn_topmost.Foreground = new SolidColorBrush(Color.FromRgb(0x00, 0xE5, 0xFF));
            }
            else
            {
                btn_topmost.Content = "📍 Normal";
                btn_topmost.Foreground = new SolidColorBrush(Color.FromRgb(0xAA, 0xAA, 0xAA));
            }
        }

        private void BtnMinimize_Click(object sender, RoutedEventArgs e)
        {
            WindowState = WindowState.Minimized;
        }

        private void BtnMaximize_Click(object sender, RoutedEventArgs e)
        {
            ToggleMaximize();
        }

        private void BtnClose_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
