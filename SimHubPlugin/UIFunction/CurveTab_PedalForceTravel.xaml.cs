using FMOD;
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
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Media.TextFormatting;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// CurveTab_PedalForceTravel.xaml 的互動邏輯
    /// </summary>
    public partial class CurveTab_PedalForceTravel : UserControl
    {
        public CurveTab_PedalForceTravel()
        {
            InitializeComponent();
            DrawGridLines();
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_PedalForceTravel),
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
            typeof(CurveTab_PedalForceTravel),
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

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(CurveTab_PedalForceTravel),
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



        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    if (Settings.table_selected != 1)
                    {
                        Rangeslider_force_range.Maximum = 50;
                    }
                    else
                    {
                        Rangeslider_force_range.Maximum = 200;
                    }
                }
            }
            catch
            {
            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_PedalForceTravel;
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
            var control = d as CurveTab_PedalForceTravel;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.CanvasDraw();
                    control.Label_min_force.Content = "Preload:\n" + (float)control.dap_config_st.payloadPedalConfig_.preloadForce + "kg";
                    control.Label_max_force.Content = "Max force:\n" + (float)control.dap_config_st.payloadPedalConfig_.maxForce + "kg";
                    control.Label_max_pos.Content = "MAX\n" + control.dap_config_st.payloadPedalConfig_.pedalEndPosition + "%\n" + Math.Round((float)(control.dap_config_st.payloadPedalConfig_.lengthPedal_travel * control.dap_config_st.payloadPedalConfig_.pedalEndPosition) / 100) + "mm";
                    control.Label_min_pos.Content = "MIN\n" + control.dap_config_st.payloadPedalConfig_.pedalStartPosition + "%\n" + Math.Round((float)(control.dap_config_st.payloadPedalConfig_.lengthPedal_travel * control.dap_config_st.payloadPedalConfig_.pedalStartPosition) / 100) + "mm";
                    if (control.dap_config_st.payloadPedalConfig_.pedalStartPosition < 5)
                    {
                        var tmp = control.dap_config_st;
                        tmp.payloadPedalConfig_.pedalStartPosition = 5;
                        control.dap_config_st = tmp;
                    }
                    if (control.dap_config_st.payloadPedalConfig_.pedalEndPosition > 95)
                    {
                        var tmp = control.dap_config_st;
                        tmp.payloadPedalConfig_.pedalEndPosition = 95;
                        control.dap_config_st = tmp;
                    }
                    control.Rangeslider_travel_range.LowerValue = control.dap_config_st.payloadPedalConfig_.pedalStartPosition;
                    control.Rangeslider_travel_range.UpperValue = control.dap_config_st.payloadPedalConfig_.pedalEndPosition;
                    control.Rangeslider_force_range.UpperValue = control.dap_config_st.payloadPedalConfig_.maxForce;
                    control.Rangeslider_force_range.LowerValue = control.dap_config_st.payloadPedalConfig_.preloadForce;
                    control.PedalServoForceCheck();
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_PedalForceTravel;
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

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue)
        {
            SettingsChanged?.Invoke(this, newValue);
        }
        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
        }

        private void CanvasDraw()
        {
            if (dap_config_st.payloadPedalConfig_.BP_trigger == 1)
            {

                text_BP.Visibility = Visibility.Visible;
                rect_BP_Control.Visibility = Visibility.Visible;
            }
            else
            {
                text_BP.Visibility = Visibility.Hidden;
                rect_BP_Control.Visibility = Visibility.Hidden;
            }

            if (dap_config_st.payloadPedalConfig_.Simulate_ABS_trigger == 1)
            {
                rect_SABS.Visibility = Visibility.Visible;
                rect_SABS_Control.Visibility = Visibility.Visible;
                text_SABS.Visibility = Visibility.Visible;
            }
            else
            {
                rect_SABS.Visibility = Visibility.Hidden;
                rect_SABS_Control.Visibility = Visibility.Hidden;
                text_SABS.Visibility = Visibility.Hidden;
            }
            text_point_pos.Visibility = Visibility.Hidden;
            double control_rect_value_max = 100;
            double dyy = canvas.Height / control_rect_value_max;
            Canvas.SetTop(rect0, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p000 - rect0.Height / 2);
            Canvas.SetLeft(rect0, 0 * canvas.Width / 5 - rect0.Width / 2);
            Canvas.SetTop(rect1, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p020 - rect0.Height / 2);
            Canvas.SetLeft(rect1, 1 * canvas.Width / 5 - rect1.Width / 2);
            Canvas.SetTop(rect2, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p040 - rect0.Height / 2);
            Canvas.SetLeft(rect2, 2 * canvas.Width / 5 - rect2.Width / 2);
            Canvas.SetTop(rect3, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p060 - rect0.Height / 2);
            Canvas.SetLeft(rect3, 3 * canvas.Width / 5 - rect3.Width / 2);
            Canvas.SetTop(rect4, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p080 - rect0.Height / 2);
            Canvas.SetLeft(rect4, 4 * canvas.Width / 5 - rect4.Width / 2);
            Canvas.SetTop(rect5, canvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p100 - rect0.Height / 2);
            Canvas.SetLeft(rect5, 5 * canvas.Width / 5 - rect5.Width / 2);
            if (dap_config_st.payloadPedalConfig_.debug_flags_0 != 32)
            {
                rect_State.Visibility = Visibility.Visible;
                text_state.Visibility = Visibility.Visible;
            }
            else
            {
                rect_State.Visibility = Visibility.Hidden;
                text_state.Visibility = Visibility.Hidden;
            }
            Canvas.SetTop(rect_State, canvas.Height - rect_State.Height / 2);
            Canvas.SetLeft(rect_State, -rect_State.Width / 2);
            Canvas.SetLeft(text_state, Canvas.GetLeft(rect_State) /*+ rect_State.Width*/);
            Canvas.SetTop(text_state, Canvas.GetTop(rect_State) - rect_State.Height);
            text_state.Text = "0%";
            //set for ABS slider
            Canvas.SetTop(rect_SABS_Control, (control_rect_value_max - dap_config_st.payloadPedalConfig_.Simulate_ABS_value) * dyy - rect_SABS_Control.Height / 2);
            Canvas.SetLeft(rect_SABS_Control, 0);
            Canvas.SetTop(rect_SABS, 0);
            Canvas.SetLeft(rect_SABS, 0);
            rect_SABS.Height = canvas.Height - dap_config_st.payloadPedalConfig_.Simulate_ABS_value * dyy;
            Canvas.SetTop(text_SABS, Canvas.GetTop(rect_SABS_Control) - text_SABS.Height - rect_SABS_Control.Height);
            Canvas.SetLeft(text_SABS, canvas.Width - text_SABS.Width);
            text_SABS.Text = "ABS trigger value: " + dap_config_st.payloadPedalConfig_.Simulate_ABS_value + "%";

            //set for travel slider;
            double dx = 0;
            //Bite point control
            double BP_max = 100;
            dx = (double)canvas.Width / BP_max;
            text_BP.Text = "Bite Point:\n" + ((float)dap_config_st.payloadPedalConfig_.BP_trigger_value) + "%";
            Canvas.SetLeft(rect_BP_Control, dap_config_st.payloadPedalConfig_.BP_trigger_value * dx - rect_BP_Control.Width / 2);
            Canvas.SetLeft(text_BP, Canvas.GetLeft(rect_BP_Control) + rect_BP_Control.Width + 3);
            Canvas.SetTop(text_BP, canvas.Height - text_BP.Height - 15);
            Update_BrakeForceCurve();
        }

        private void btn_linearcurve_Click(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce_p000 = 0;
            tmp.payloadPedalConfig_.relativeForce_p020 = 20;
            tmp.payloadPedalConfig_.relativeForce_p040 = 40;
            tmp.payloadPedalConfig_.relativeForce_p060 = 60;
            tmp.payloadPedalConfig_.relativeForce_p080 = 80;
            tmp.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
        }

        private void btn_Scurve_Click(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce_p000 = 0;
            tmp.payloadPedalConfig_.relativeForce_p020 = 7;
            tmp.payloadPedalConfig_.relativeForce_p040 = 28;
            tmp.payloadPedalConfig_.relativeForce_p060 = 70;
            tmp.payloadPedalConfig_.relativeForce_p080 = 93;
            tmp.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
        }

        private void btn_10xcurve_Click(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce_p000 = 0;
            tmp.payloadPedalConfig_.relativeForce_p020 = 43;
            tmp.payloadPedalConfig_.relativeForce_p040 = 69;
            tmp.payloadPedalConfig_.relativeForce_p060 = 85;
            tmp.payloadPedalConfig_.relativeForce_p080 = 95;
            tmp.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
        }

        private void btn_logcurve_Click(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce_p000 = 0;
            tmp.payloadPedalConfig_.relativeForce_p020 = 6;
            tmp.payloadPedalConfig_.relativeForce_p040 = 17;
            tmp.payloadPedalConfig_.relativeForce_p060 = 33;
            tmp.payloadPedalConfig_.relativeForce_p080 = 59;
            tmp.payloadPedalConfig_.relativeForce_p100 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
        }



        private void Rectangle_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            calculation.isDragging = true;
            var rectangle = sender as Rectangle;
            calculation.offset = e.GetPosition(rectangle);
            rectangle.CaptureMouse();
            if (rectangle.Name != "rect_SABS_Control" & rectangle.Name != "rect_BP_Control")
            {
                var dropShadowEffect = new DropShadowEffect
                {
                    ShadowDepth = 0,
                    BlurRadius = 15,
                    Color = Colors.White,
                    Opacity = 1
                };
                rectangle.Fill = calculation.lightcolor;
                rectangle.Effect = dropShadowEffect;
            }
        }

        private void rect_SABS_Control_MouseMove(object sender, MouseEventArgs e)
        {
            if (calculation.isDragging)
            {
                var rectangle = sender as Rectangle;
                //double x = e.GetPosition(canvas).X - offset.X;
                double y = e.GetPosition(canvas).Y - calculation.offset.Y;

                // Ensure the rectangle stays within the canvas
                double dy = canvas.Height / 100;
                double min_posiiton = 5 * dy;
                double max_position = 50 * dy;
                //min position: 50%, max 95%
                //double dx = 100 / (canvas_horz_slider.Width - 10);
                y = Math.Max(min_posiiton, Math.Min(y, max_position));
                //Canvas.SetTop(rect_SABS, y);
                rect_SABS.Height = y;
                double actual_y = (canvas.Height - y) / dy;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.Simulate_ABS_value = Convert.ToByte(actual_y);
                dap_config_st = tmp;
                //TextBox_debugOutput.Text = "ABS trigger value: " + dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.Simulate_ABS_value + "%";
                text_SABS.Text = "ABS trigger value: " + dap_config_st.payloadPedalConfig_.Simulate_ABS_value + "%";
                Canvas.SetTop(text_SABS, y - rect_SABS_Control.Height - text_SABS.Height);
                Canvas.SetTop(rectangle, y - rect_SABS_Control.Height / 2);

                
                ConfigChangedEvent(dap_config_st);

            }
        }

        private void Rectangle_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (calculation.isDragging)
            {
                var rectangle = sender as Rectangle;
                calculation.isDragging = false;
                rectangle.ReleaseMouseCapture();
                text_point_pos.Visibility = Visibility.Hidden;
                //text_point_pos_rudder.Visibility = Visibility.Hidden;
                //SolidColorBrush buttonBackground = btn_update.Background as SolidColorBrush;
                //Color color = Color.FromArgb(150, buttonBackground.Color.R, buttonBackground.Color.G, buttonBackground.Color.B);
                //rectangle.Fill = btn_update.Background;
                if (rectangle.Name != "rect_SABS_Control" & rectangle.Name != "rect_BP_Control")
                {
                    var dropShadowEffect = new DropShadowEffect
                    {
                        ShadowDepth = 0,
                        BlurRadius = 20,
                        Color = Colors.White,
                        Opacity = 0
                    };
                    rectangle.Fill = calculation.defaultcolor;
                    rectangle.Effect = dropShadowEffect;
                }
            }
        }

        private void rect_BP_Control_MouseMove(object sender, MouseEventArgs e)
        {
            if (calculation.isDragging)
            {
                var rectangle = sender as Rectangle;
                //Bite point control
                if (rectangle.Name == "rect_BP_Control")
                {
                    // Ensure the rectangle stays within the canvas
                    double x = e.GetPosition(canvas).X - calculation.offset.X;
                    double BP_max = 100;
                    double dx = (canvas.Width) / BP_max;
                    double min_position = 10 * dx - rect_BP_Control.Width / 2;
                    double max_position = (BP_max - 10) * dx - rect_BP_Control.Width / 2;

                    x = Math.Max(min_position, Math.Min(x, max_position));
                    double actual_x = (x + rect_BP_Control.Width / 2) / dx;
                    var tmp = dap_config_st;
                    tmp.payloadPedalConfig_.BP_trigger_value = (byte)(actual_x);
                    dap_config_st = tmp;
                    text_BP.Text = "Bite Point:\n" + ((float)dap_config_st.payloadPedalConfig_.BP_trigger_value) + "%";
                    Canvas.SetLeft(rectangle, x);
                    Canvas.SetLeft(text_BP, Canvas.GetLeft(rect_BP_Control) + rect_BP_Control.Width + 3);
                    Canvas.SetTop(text_BP, canvas.Height - text_BP.Height - 15);

                    
                    ConfigChangedEvent(dap_config_st);

                }
            }
        }

        private void Rectangle_MouseMove(object sender, MouseEventArgs e)
        {
            if (calculation.isDragging)
            {
                var rectangle = sender as Rectangle;
                //double x = e.GetPosition(canvas).X - offset.X;
                double y = e.GetPosition(canvas).Y - calculation.offset.Y;

                // Ensure the rectangle stays within the canvas
                //x = Math.Max(0, Math.Min(x, canvas.ActualWidth - rectangle.ActualWidth));
                y = Math.Max(-1 * rectangle.Height / 2, Math.Min(y, canvas.Height - rectangle.Height / 2));

                //Canvas.SetLeft(rectangle, x);
                Canvas.SetTop(rectangle, y);
                double y_max = 100;
                double dx = canvas.Height / y_max;
                double y_actual = (canvas.Height - y - rectangle.Height / 2) / dx;
                var tmp = dap_config_st;
                if (rectangle.Name == "rect0")
                {
                    tmp.payloadPedalConfig_.relativeForce_p000 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:0%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";

                }
                if (rectangle.Name == "rect1")
                {

                    tmp.payloadPedalConfig_.relativeForce_p020 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:20%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";
                }
                if (rectangle.Name == "rect2")
                {
                    tmp.payloadPedalConfig_.relativeForce_p040 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:40%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";
                }
                if (rectangle.Name == "rect3")
                {
                    tmp.payloadPedalConfig_.relativeForce_p060 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:60%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";
                }
                if (rectangle.Name == "rect4")
                {
                    tmp.payloadPedalConfig_.relativeForce_p080 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:80%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";
                }
                if (rectangle.Name == "rect5")
                {
                    tmp.payloadPedalConfig_.relativeForce_p100 = Convert.ToByte(y_actual);
                    text_point_pos.Text = "Travel:100%";
                    text_point_pos.Text += "\nForce: " + (int)y_actual + "%";
                }

                text_point_pos.Visibility = Visibility.Visible; ;

                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                Update_BrakeForceCurve();
            }
        }

        private void btn_plus_maxforce_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.advanced_b)
            {
                Rangeslider_force_range.UpperValue = Rangeslider_force_range.UpperValue + 0.1;
            }
            else
            {
                Rangeslider_force_range.UpperValue = Rangeslider_force_range.UpperValue + 1;
            }
            PedalServoForceCheck();
        }

        private void btn_minus_maxforce_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.advanced_b)
            {
                Rangeslider_force_range.UpperValue = Rangeslider_force_range.UpperValue - 0.1;
            }
            else
            {
                Rangeslider_force_range.UpperValue = Rangeslider_force_range.UpperValue - 1;
            }
        }

        private void Rangeslider_force_range_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.preloadForce = (float)e.NewValue;
            dap_config_st = tmp;
            if(Label_min_force!=null) Label_min_force.Content = "Preload:\n" + (float)dap_config_st.payloadPedalConfig_.preloadForce + "kg";
            ConfigChangedEvent(dap_config_st);
            
        }

        private void Rangeslider_force_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.maxForce = (float)e.NewValue;
            dap_config_st = tmp;
            if(Label_max_force!=null) Label_max_force.Content = "Max force:\n" + (float)dap_config_st.payloadPedalConfig_.maxForce + "kg";
            ConfigChangedEvent(dap_config_st);
            PedalServoForceCheck();
        }

        private void btn_plus_preload_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.advanced_b)
            {
                Rangeslider_force_range.LowerValue = Rangeslider_force_range.LowerValue + 0.1;
            }
            else
            {
                Rangeslider_force_range.LowerValue = Rangeslider_force_range.LowerValue + 1;
            }
        }

        private void btn_minus_preload_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.advanced_b)
            {
                Rangeslider_force_range.LowerValue = Rangeslider_force_range.LowerValue - 0.1;
            }
            else
            {
                Rangeslider_force_range.LowerValue = Rangeslider_force_range.LowerValue - 1;
            }
        }

        private void Rangeslider_travel_range_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalStartPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            if(Label_min_pos!=null) Label_min_pos.Content = "MIN\n" + dap_config_st.payloadPedalConfig_.pedalStartPosition + "%\n" + Math.Round((float)(dap_config_st.payloadPedalConfig_.lengthPedal_travel * dap_config_st.payloadPedalConfig_.pedalStartPosition) / 100) + "mm";
            ConfigChangedEvent(dap_config_st);
        }

        private void Rangeslider_travel_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalEndPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            if(Label_max_pos!=null) Label_max_pos.Content = "MAX\n" + dap_config_st.payloadPedalConfig_.pedalEndPosition + "%\n" + Math.Round((float)(dap_config_st.payloadPedalConfig_.lengthPedal_travel * dap_config_st.payloadPedalConfig_.pedalEndPosition) / 100) + "mm";
            ConfigChangedEvent(dap_config_st);
        }
        private void Update_BrakeForceCurve()
        {

            double[] x = new double[6];
            double[] y = new double[6];
            double x_quantity = 100;
            double y_max = 100;
            double dx = canvas.Width / x_quantity;
            double dy = canvas.Height / y_max;
            //draw pedal force-travel curve
            x[0] = 0;
            x[1] = 20;
            x[2] = 40;
            x[3] = 60;
            x[4] = 80;
            x[5] = 100;

            y[0] = dap_config_st.payloadPedalConfig_.relativeForce_p000;
            y[1] = dap_config_st.payloadPedalConfig_.relativeForce_p020;
            y[2] = dap_config_st.payloadPedalConfig_.relativeForce_p040;
            y[3] = dap_config_st.payloadPedalConfig_.relativeForce_p060;
            y[4] = dap_config_st.payloadPedalConfig_.relativeForce_p080;
            y[5] = dap_config_st.payloadPedalConfig_.relativeForce_p100;

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, 100);

            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.cubic_spline_param_a_0 = (float)a[0];
            tmp.payloadPedalConfig_.cubic_spline_param_a_1 = (float)a[1];
            tmp.payloadPedalConfig_.cubic_spline_param_a_2 = (float)a[2];
            tmp.payloadPedalConfig_.cubic_spline_param_a_3 = (float)a[3];
            tmp.payloadPedalConfig_.cubic_spline_param_a_4 = (float)a[4];

            tmp.payloadPedalConfig_.cubic_spline_param_b_0 = (float)b[0];
            tmp.payloadPedalConfig_.cubic_spline_param_b_1 = (float)b[1];
            tmp.payloadPedalConfig_.cubic_spline_param_b_2 = (float)b[2];
            tmp.payloadPedalConfig_.cubic_spline_param_b_3 = (float)b[3];
            tmp.payloadPedalConfig_.cubic_spline_param_b_4 = (float)b[4];
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);


            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();


            for (int pointIdx = 0; pointIdx < 100; pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * xs2[pointIdx], dy * ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);
                calculation.Force_curve_Y[pointIdx] = dy * ys2[pointIdx];
            }
            this.Polyline_BrakeForceCurve.Points = myPointCollection2;
        }

        public void updatePedalState(ushort pedalPosition_u16, ushort pedalForce_u16)
        {
            text_point_pos.Visibility = Visibility.Hidden;
            double control_rect_value_max = 65535;
            double dyy = canvas.Height / control_rect_value_max;
            double dxx = canvas.Width / control_rect_value_max;

            if (Settings.advanced_b)
            {
                Canvas.SetLeft(rect_State, dxx * pedalPosition_u16 - rect_State.Width / 2);
                Canvas.SetTop(rect_State, canvas.Height - dyy * pedalForce_u16 - rect_State.Height / 2);

                Canvas.SetLeft(text_state, Canvas.GetLeft(rect_State) /*+ rect_State.Width*/);
                Canvas.SetTop(text_state, Canvas.GetTop(rect_State) - rect_State.Height);
                text_state.Text = Math.Round(pedalForce_u16 / control_rect_value_max * 100) + "%";
                int round_x = (int)(100 * pedalPosition_u16 / control_rect_value_max) - 1;
                int x_showed = round_x + 1;

                calculation.current_pedal_travel_state = x_showed;
                calculation.pedal_state_in_ratio = (byte)calculation.current_pedal_travel_state;
            }
            else
            {
                Canvas.SetLeft(rect_State, dxx * pedalPosition_u16 - rect_State.Width / 2);
                int round_x = (int)(100 * pedalPosition_u16 / control_rect_value_max) - 1;
                int x_showed = round_x + 1;
                round_x = Math.Max(0, Math.Min(round_x, 99));
                calculation.current_pedal_travel_state = x_showed;
                calculation.pedal_state_in_ratio = (byte)calculation.current_pedal_travel_state;
                Canvas.SetTop(rect_State, canvas.Height - calculation.Force_curve_Y[round_x] - rect_State.Height / 2);
                Canvas.SetLeft(text_state, Canvas.GetLeft(rect_State) /*+ rect_State.Width*/);
                Canvas.SetTop(text_state, Canvas.GetTop(rect_State) - rect_State.Height);
                text_state.Text = x_showed + "%";
                
            }
        }

        private void DrawGridLines()
        {
            // Specify the number of rows and columns for the grid
            int rowCount = 5;
            int columnCount = 5;

            // Calculate the width and height of each cell
            double cellWidth = canvas.Width / columnCount;
            double cellHeight = canvas.Height / rowCount;



            // Draw horizontal gridlines
            for (int i = 1; i < rowCount; i++)
            {
                Line line = new Line
                {
                    X1 = 0,
                    Y1 = i * cellHeight,
                    X2 = canvas.Width,
                    Y2 = i * cellHeight,
                    //Stroke = Brush.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1

                };
                Line line2 = new Line
                {
                    X1 = 0,
                    Y1 = i * cellHeight,
                    X2 = canvas.Width,
                    Y2 = i * cellHeight,
                    //Stroke = Brush.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1

                };
                canvas.Children.Add(line);
                //canvas_rudder_curve.Children.Add(line2);
            }

            // Draw vertical gridlines
            for (int i = 1; i < columnCount; i++)
            {
                Line line = new Line
                {
                    X1 = i * cellWidth,
                    Y1 = 0,
                    X2 = i * cellWidth,
                    Y2 = canvas.Height,
                    //Stroke = Brushes.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                Line line2 = new Line
                {
                    X1 = i * cellWidth,
                    Y1 = 0,
                    X2 = i * cellWidth,
                    Y2 = canvas.Height,
                    //Stroke = Brushes.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                canvas.Children.Add(line);
                //canvas_rudder_curve.Children.Add(line2);

            }
        }

        private void PedalServoForceCheck()
        {
            //parameter calculation
            double b = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double c_hor = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double c_vert = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double travel_setup_max = (double)dap_config_st.payloadPedalConfig_.lengthPedal_travel * (double)dap_config_st.payloadPedalConfig_.pedalEndPosition / 100.0;
            double a = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            double od = b + dap_config_st.payloadPedalConfig_.lengthPedal_d;
            double c_hort_max = c_hor + travel_setup_max;
            double oc_max = Math.Sqrt((c_hort_max) * (c_hort_max) + c_vert * c_vert);
            double min_angle_1 = Math.Acos((b * b + oc_max * oc_max - a * a) / (2 * b * oc_max));
            double min_angle_2 = Math.Atan2(c_vert, c_hort_max);
            double oc_min = Math.Sqrt((c_hor) * (c_hor) + c_vert * c_vert);
            double max_angle_1 = Math.Acos((b * b + oc_min * oc_min - a * a) / (2 * b * oc_min));
            double max_angle_2 = Math.Atan2(c_vert, c_hor);

            double angle_beta_max = Math.Acos((oc_max * oc_max + a * a - b * b) / (2 * oc_max * a));
            double angle_gamma = Math.Acos((b * b + a * a - oc_max * oc_max) / (2 * b * a));
            TextBlock_Warning.Text = "beta=" + Math.Round(angle_beta_max / Math.PI * 180);
            TextBlock_Warning.Text += "\nAlpha+=" + Math.Round(min_angle_2 / Math.PI * 180);
            TextBlock_Warning.Text += "\nE=" + Math.Round(((angle_beta_max - min_angle_2) / Math.PI * 180));
            double Force_calculated = dap_config_st.payloadPedalConfig_.maxForce * (Math.Cos(angle_beta_max - min_angle_2) / Math.Sin(angle_gamma)) * od / b;
            double Servo_max_force = 1.1 * 2 * Math.PI / (double)(dap_config_st.payloadPedalConfig_.spindlePitch_mmPerRev_u8 / 1000.0) * 0.83 / 9.8;
            c_hort_max = c_hor + dap_config_st.payloadPedalConfig_.lengthPedal_travel;
            oc_max = Math.Sqrt((c_hort_max) * (c_hort_max) + c_vert * c_vert);
            min_angle_1 = Math.Acos((b * b + oc_max * oc_max - a * a) / (2 * b * oc_max));
            min_angle_2 = Math.Atan2(c_vert, c_hort_max);
            angle_beta_max = Math.Acos((oc_max * oc_max + a * a - b * b) / (2 * oc_max * a));
            angle_gamma = Math.Acos((b * b + a * a - oc_max * oc_max) / (2 * b * a));
            double servo_max_force_output_in_kg = Servo_max_force * Math.Sin(angle_gamma) * b / od / Math.Cos(angle_beta_max - min_angle_2);
            if (dap_config_st.payloadPedalConfig_.maxForce > servo_max_force_output_in_kg)
            {
                TextBlock_Warning.Text = "Caution, the config and pedal kinematics may cause servo overloaded!!";
                //TextBlock_Warning.Text += "\nExpected max force= " + Math.Round(Force_calculated)+"kg";
                TextBlock_Warning.Text += "\nMax servo output force= " + Math.Round(servo_max_force_output_in_kg) + "kg";
                TextBlock_Warning.Visibility = Visibility.Visible;
                Label_max_force.Foreground = calculation.Red_Warning;
            }
            else
            {
                TextBlock_Warning.Text = "Expected max push force=" + Math.Round(Force_calculated);
                TextBlock_Warning.Text += "\nMax servo holding force=" + Math.Round(Servo_max_force);
                TextBlock_Warning.Visibility = Visibility.Hidden;
                Label_max_force.Foreground = calculation.White_Default;
            }

            
        }
    }
}
