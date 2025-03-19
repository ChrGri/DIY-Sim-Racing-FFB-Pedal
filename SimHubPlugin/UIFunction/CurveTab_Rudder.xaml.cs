using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using MouseEventArgs = System.Windows.Input.MouseEventArgs;
using System.Windows.Media;
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Input;


namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// CurveTab_Rudder.xaml 的互動邏輯
    /// </summary>
    /// 

    public partial class CurveTab_Rudder : UserControl
    {
        public bool isDragging { get; set; }

        public Point offset;

        public CurveTab_Rudder()
        {
            InitializeComponent();
            DrawGridLines();

        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_Rudder),
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
            typeof(CurveTab_Rudder),
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
            typeof(CurveTab_Rudder),
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

                }
            }
            catch
            {
            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_Rudder;
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
            var control = d as CurveTab_Rudder;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {

                    control.CanvasDraw();
                    control.Rangeslider_rudder_force_range.LowerValue = control.dap_config_st.payloadPedalConfig_.preloadForce;
                    control.Rangeslider_rudder_force_range.UpperValue = control.dap_config_st.payloadPedalConfig_.maxForce;
                    control.Rangeslider_rudder_travel_range.LowerValue = control.dap_config_st.payloadPedalConfig_.pedalStartPosition;
                    control.Rangeslider_rudder_travel_range.UpperValue = control.dap_config_st.payloadPedalConfig_.pedalEndPosition;
                    control.Label_min_pos_rudder.Content = "MIN\n" + control.dap_config_st.payloadPedalConfig_.pedalStartPosition + "%";
                    control.Label_max_pos_rudder.Content = "MAX\n" + control.dap_config_st.payloadPedalConfig_.pedalEndPosition + "%";
                    control.Label_max_force_rudder.Content = "Max force:\n" + control.dap_config_st.payloadPedalConfig_.maxForce + "kgf";
                    control.Label_min_force_rudder.Content = "Preload:\n" + control.dap_config_st.payloadPedalConfig_.preloadForce + "kgf";
                    

                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_Rudder;
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
            try
            {
                double control_rect_value_max = 100;

                double dyy_rudder = canvas_rudder_curve.Height / control_rect_value_max;
                Canvas.SetTop(rect0_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p000 - rect0_rudder.Height / 2);
                Canvas.SetLeft(rect0_rudder, 0 * canvas_rudder_curve.Width / 5 - rect0_rudder.Width / 2);

                Canvas.SetTop(rect1_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p020 - rect1_rudder.Height / 2);
                Canvas.SetLeft(rect1_rudder, 1 * canvas_rudder_curve.Width / 5 - rect1_rudder.Width / 2);

                Canvas.SetTop(rect2_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p040 - rect2_rudder.Height / 2);
                Canvas.SetLeft(rect2_rudder, 2 * canvas_rudder_curve.Width / 5 - rect2_rudder.Width / 2);

                Canvas.SetTop(rect3_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p060 - rect3_rudder.Height / 2);
                Canvas.SetLeft(rect3_rudder, 3 * canvas_rudder_curve.Width / 5 - rect3_rudder.Width / 2);

                Canvas.SetTop(rect4_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p080 - rect4_rudder.Height / 2);
                Canvas.SetLeft(rect4_rudder, 4 * canvas_rudder_curve.Width / 5 - rect4_rudder.Width / 2);

                Canvas.SetTop(rect5_rudder, canvas_rudder_curve.Height - dyy_rudder * dap_config_st.payloadPedalConfig_.relativeForce_p100 - rect5_rudder.Height / 2);
                Canvas.SetLeft(rect5_rudder, 5 * canvas_rudder_curve.Width / 5 - rect5_rudder.Width / 2);
                text_point_pos_rudder.Visibility = Visibility.Hidden;
                Update_BrakeForceCurve();
            }
            catch
            {
            }
        }

        private void btn_linearcurve_rudder_Click(object sender, RoutedEventArgs e)
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

        private void btn_Scurve_rudder_Click(object sender, RoutedEventArgs e)
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

        private void btn_10xcurve_rudder_Click(object sender, RoutedEventArgs e)
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

        private void btn_logcurve_rudder_Click(object sender, RoutedEventArgs e)
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
            try
            {
                isDragging = true;
                var rectangle = sender as Rectangle;
                offset = e.GetPosition(rectangle);
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
                    //rectangle.Fill = calculation.lightcolor;
                    rectangle.Effect = dropShadowEffect;
                }
            }
            catch { }
            
        }

        private void Rectangle_MouseMove_Rudder(object sender, MouseEventArgs e)
        {
            try 
            {
                if (isDragging)
                {
                    var rectangle = sender as Rectangle;
                    //double x = e.GetPosition(canvas).X - offset.X;
                    double y = e.GetPosition(canvas_rudder_curve).Y - offset.Y;

                    // Ensure the rectangle stays within the canvas
                    //x = Math.Max(0, Math.Min(x, canvas.ActualWidth - rectangle.ActualWidth));
                    y = Math.Max(-1 * rectangle.Height / 2, Math.Min(y, canvas_rudder_curve.Height - rectangle.Height / 2));

                    //Canvas.SetLeft(rectangle, x);
                    Canvas.SetTop(rectangle, y);
                    double y_max = 100;
                    double dx = canvas_rudder_curve.Height / y_max;
                    double y_actual = (canvas_rudder_curve.Height - y - rectangle.Height / 2) / dx;


                    //rudder
                    if (rectangle.Name == "rect0_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p000 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:0%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";

                    }
                    if (rectangle.Name == "rect1_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p020 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:20%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";
                    }
                    if (rectangle.Name == "rect2_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p040 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:40%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";
                    }
                    if (rectangle.Name == "rect3_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p060 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:60%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";
                    }
                    if (rectangle.Name == "rect4_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p080 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:80%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";
                    }
                    if (rectangle.Name == "rect5_rudder")
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.relativeForce_p100 = Convert.ToByte(y_actual);
                        dap_config_st = tmp;
                        ConfigChangedEvent(dap_config_st);
                        text_point_pos_rudder.Text = "Travel:100%";
                        text_point_pos_rudder.Text += "\nForce: " + (int)y_actual + "%";
                    }
                    text_point_pos_rudder.Visibility = Visibility.Visible;
                    Update_BrakeForceCurve();
                }
            }
            catch { }
            
        }

        private void Rectangle_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try             
            {
                if (isDragging)
                {
                    var rectangle = sender as Rectangle;
                    isDragging = false;
                    rectangle.ReleaseMouseCapture();
                    text_point_pos_rudder.Visibility = Visibility.Hidden;
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
                        //rectangle.Fill = calculation.defaultcolor;
                        rectangle.Effect = dropShadowEffect;
                    }
                }
            }
            catch { }

        }

        private void Rangeslider_rudder_force_range_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.preloadForce = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            try
            {
                Label_min_force_rudder.Content = "Preload:\n" + dap_config_st.payloadPedalConfig_.preloadForce + "kg";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_force_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.maxForce = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            try
            {
                Label_max_force_rudder.Content = "Max force:\n" + dap_config_st.payloadPedalConfig_.maxForce + "kg";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_travel_range_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalStartPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            try
            {
                Label_min_pos_rudder.Content = "MIN\n" + dap_config_st.payloadPedalConfig_.pedalStartPosition + "%";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_travel_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalEndPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            try
            {
                Label_max_pos_rudder.Content = "MAX\n" + dap_config_st.payloadPedalConfig_.pedalEndPosition + "%";
            }
            catch { }
            
        }

        private void Update_BrakeForceCurve()
        {
            try 
            {
                double[] x = new double[6];
                double[] y = new double[6];
                double x_quantity = 100;
                double y_max = 100;
                double dx = canvas_rudder_curve.Width / x_quantity;
                double dy = canvas_rudder_curve.Height / y_max;
                //draw pedal force-travel curve



                //draw rudder curve
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
                (double[] xs2_rudder, double[] ys2_rudder, double[] a_rudder, double[] b_rudder) = Cubic.Interpolate1D(x, y, 100);

                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.cubic_spline_param_a_0 = (float)a_rudder[0];
                tmp.payloadPedalConfig_.cubic_spline_param_a_1 = (float)a_rudder[1];
                tmp.payloadPedalConfig_.cubic_spline_param_a_2 = (float)a_rudder[2];
                tmp.payloadPedalConfig_.cubic_spline_param_a_3 = (float)a_rudder[3];
                tmp.payloadPedalConfig_.cubic_spline_param_a_4 = (float)a_rudder[4];

                tmp.payloadPedalConfig_.cubic_spline_param_b_0 = (float)b_rudder[0];
                tmp.payloadPedalConfig_.cubic_spline_param_b_1 = (float)b_rudder[1];
                tmp.payloadPedalConfig_.cubic_spline_param_b_2 = (float)b_rudder[2];
                tmp.payloadPedalConfig_.cubic_spline_param_b_3 = (float)b_rudder[3];
                tmp.payloadPedalConfig_.cubic_spline_param_b_4 = (float)b_rudder[4];
                dap_config_st = tmp;


                System.Windows.Media.PointCollection myPointCollection3 = new System.Windows.Media.PointCollection();


                for (int pointIdx = 0; pointIdx < 100; pointIdx++)
                {
                    System.Windows.Point Pointlcl = new System.Windows.Point(dx * xs2_rudder[pointIdx], dy * ys2_rudder[pointIdx]);
                    myPointCollection3.Add(Pointlcl);
                    //Force_curve_Y[pointIdx] = dy * ys2_rudder[pointIdx];
                }

                this.Polyline_RudderForceCurve.Points = myPointCollection3;
            }
            catch { }
            
        }

        private void DrawGridLines()
        {
            // Specify the number of rows and columns for the grid
            int rowCount = 5;
            int columnCount = 5;

            // Calculate the width and height of each cell
            double cellWidth = canvas_rudder_curve.Width / columnCount;
            double cellHeight = canvas_rudder_curve.Height / rowCount;



            // Draw horizontal gridlines
            for (int i = 1; i < rowCount; i++)
            {

                Line line2 = new Line
                {
                    X1 = 0,
                    Y1 = i * cellHeight,
                    X2 = canvas_rudder_curve.Width,
                    Y2 = i * cellHeight,
                    //Stroke = Brush.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1

                };
                canvas_rudder_curve.Children.Add(line2);
            }

            // Draw vertical gridlines
            for (int i = 1; i < columnCount; i++)
            {
                Line line2 = new Line
                {
                    X1 = i * cellWidth,
                    Y1 = 0,
                    X2 = i * cellWidth,
                    Y2 = canvas_rudder_curve.Height,
                    //Stroke = Brushes.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };

                canvas_rudder_curve.Children.Add(line2);

            }
        }
    }
}
