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
using System.Windows.Media.Effects;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using MouseEventArgs = System.Windows.Input.MouseEventArgs;


namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// CurveTab_Rudder.xaml 的互動邏輯
    /// </summary>
    /// 

    public partial class CurveTab_Rudder : System.Windows.Controls.UserControl
    {
        public bool isDragging { get; set; }

        public Point offset;
        private const double RectSize = 10;
        public int rectCount = 0;
        public List<double> rectPositionX = new List<double>();
        public List<double> rectPositionY = new List<double>();
        private Rectangle draggingRect = null;
        private Point dragOffset;
        private int minControlQuantity = 6;
        private int maxControlQuantity = 11;
        private byte[] force;
        private byte[] travel;
        public CurveTab_Rudder()
        {
            InitializeComponent();
            DrawGridLines();
            InitRectangles();
            force = new byte[maxControlQuantity];
            travel = new byte[maxControlQuantity];
            Update_BrakeForceCurve();
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
                    if (canvas_rudder_curve != null) CanvasDraw();
                    if (canvas_rudder_curve != null) updateRectControlFromConfig();
                    if (Rangeslider_rudder_force_range != null) Rangeslider_rudder_force_range.LowerValue = Settings.rudderMinForce;
                    if (Rangeslider_rudder_force_range != null) Rangeslider_rudder_force_range.UpperValue = Settings.rudderMaxForce;
                    if (Rangeslider_rudder_travel_range != null) Rangeslider_rudder_travel_range.LowerValue = Settings.rudderMinTravel;
                    if (Rangeslider_rudder_travel_range != null) Rangeslider_rudder_travel_range.UpperValue = Settings.rudderMaxTravel;
                    if (Label_min_pos_rudder != null) Label_min_pos_rudder.Content = "MIN\n" + Settings.rudderMinTravel + "%";
                    if (Label_max_pos_rudder != null) Label_max_pos_rudder.Content = "MAX\n" + Settings.rudderMaxTravel + "%";
                    if (Label_max_force_rudder != null) Label_max_force_rudder.Content = "Max force:\n" + Math.Round(Settings.rudderMaxForce,1) + "kg";
                    if (Label_min_force_rudder != null) Label_min_force_rudder.Content = "Preload:\n" + Math.Round(Settings.rudderMinForce, 1) + "kg";
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
                /*
                Canvas.SetTop(rect0_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[0] - rect0_rudder.Height / 2);
                Canvas.SetLeft(rect0_rudder, 0 * canvas_rudder_curve.Width / 5 - rect0_rudder.Width / 2);

                Canvas.SetTop(rect1_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[1] - rect1_rudder.Height / 2);
                Canvas.SetLeft(rect1_rudder, 1 * canvas_rudder_curve.Width / 5 - rect1_rudder.Width / 2);

                Canvas.SetTop(rect2_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[2] - rect2_rudder.Height / 2);
                Canvas.SetLeft(rect2_rudder, 2 * canvas_rudder_curve.Width / 5 - rect2_rudder.Width / 2);

                Canvas.SetTop(rect3_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[3] - rect3_rudder.Height / 2);
                Canvas.SetLeft(rect3_rudder, 3 * canvas_rudder_curve.Width / 5 - rect3_rudder.Width / 2);

                Canvas.SetTop(rect4_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[4] - rect4_rudder.Height / 2);
                Canvas.SetLeft(rect4_rudder, 4 * canvas_rudder_curve.Width / 5 - rect4_rudder.Width / 2);

                Canvas.SetTop(rect5_rudder, canvas_rudder_curve.Height - dyy_rudder * Settings.rudderForce[5] - rect5_rudder.Height / 2);
                Canvas.SetLeft(rect5_rudder, 5 * canvas_rudder_curve.Width / 5 - rect5_rudder.Width / 2);
                */
                text_point_pos_rudder.Visibility = Visibility.Hidden;
                //Update_BrakeForceCurve();
            }
            catch
            {
            }
        }

        private void btn_linearcurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce00 = 0;
            tmp.payloadPedalConfig_.relativeForce01 = 20;
            tmp.payloadPedalConfig_.relativeForce02 = 40;
            tmp.payloadPedalConfig_.relativeForce03 = 60;
            tmp.payloadPedalConfig_.relativeForce04 = 80;
            tmp.payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            */
            
            force[0] = 0;
            force[1] = 20;
            force[2] = 40;
            force[3] = 60;
            force[4] = 80;
            force[5] = 100;
            travel[0] = 0;
            travel[1] = 20;
            travel[2] = 40;
            travel[3] = 60;
            travel[4] = 80;
            travel[5] = 100;
            for (int i = 6; i < maxControlQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            //CanvasDraw();
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
            //SettingsChangedEvent(Settings);

        }

        private void btn_Scurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce00 = 0;
            tmp.payloadPedalConfig_.relativeForce01 = 7;
            tmp.payloadPedalConfig_.relativeForce02 = 28;
            tmp.payloadPedalConfig_.relativeForce03 = 70;
            tmp.payloadPedalConfig_.relativeForce04 = 93;
            tmp.payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
            */
            force[0] = 0;
            force[1] = 7;
            force[2] = 28;
            force[3] = 70;
            force[4] = 93;
            force[5] = 100;
            travel[0] = 0;
            travel[1] = 20;
            travel[2] = 40;
            travel[3] = 60;
            travel[4] = 80;
            travel[5] = 100;
            for (int i = 6; i < maxControlQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            //CanvasDraw();
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
        }

        private void btn_10xcurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce00 = 0;
            tmp.payloadPedalConfig_.relativeForce01 = 43;
            tmp.payloadPedalConfig_.relativeForce02 = 69;
            tmp.payloadPedalConfig_.relativeForce03 = 85;
            tmp.payloadPedalConfig_.relativeForce04 = 95;
            tmp.payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
            */
            force[0] = 0;
            force[1] = 43;
            force[2] = 69;
            force[3] = 85;
            force[4] = 95;
            force[5] = 100;
            travel[0] = 0;
            travel[1] = 20;
            travel[2] = 40;
            travel[3] = 60;
            travel[4] = 80;
            travel[5] = 100;
            for (int i = 6; i < maxControlQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
        }

        private void btn_logcurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce00 = 0;
            tmp.payloadPedalConfig_.relativeForce01 = 6;
            tmp.payloadPedalConfig_.relativeForce02 = 17;
            tmp.payloadPedalConfig_.relativeForce03 = 33;
            tmp.payloadPedalConfig_.relativeForce04 = 59;
            tmp.payloadPedalConfig_.relativeForce05 = 100;
            dap_config_st = tmp;
            Update_BrakeForceCurve();
            ConfigChangedEvent(dap_config_st);
            */
            force[0] = 0;
            force[1] = 6;
            force[2] = 17;
            force[3] = 33;
            force[4] = 59;
            force[5] = 100;
            travel[0] = 0;
            travel[1] = 20;
            travel[2] = 40;
            travel[3] = 60;
            travel[4] = 80;
            travel[5] = 100;
            for (int i = 6; i < maxControlQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
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
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.preloadForce = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            */
            Settings.rudderMinForce = (float)e.NewValue;
            try
            {
                if(Label_min_force_rudder!=null) Label_min_force_rudder.Content = "Preload:\n" + Math.Round(Settings.rudderMinForce, 1) + "kg";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_force_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.maxForce = (float)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            */
            Settings.rudderMaxForce = (float)e.NewValue;
            try
            {
                if (Label_max_force_rudder != null) Label_max_force_rudder.Content = "Max force:\n" + Math.Round(Settings.rudderMaxForce, 1) + "kg";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_travel_range_LowerValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalStartPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            */
            Settings.rudderMinTravel = (byte)e.NewValue;
            try
            {
                if (Label_min_pos_rudder != null) Label_min_pos_rudder.Content = "MIN\n" + Settings.rudderMinTravel + "%";
            }
            catch { }
            
        }

        private void Rangeslider_rudder_travel_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            /*
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.pedalEndPosition = (byte)e.NewValue;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
            */
            Settings.rudderMaxTravel = (byte)e.NewValue;
            try
            {
                if (Label_max_pos_rudder != null) Label_max_pos_rudder.Content = "MAX\n" + Settings.rudderMaxTravel + "%";
            }
            catch { }
            
        }

        private void Update_BrakeForceCurve()
        {
            try
            {
                double[] x = new double[rectCount];
                double[] y = new double[rectCount];
                double x_quantity = 101;
                double y_max = 101;
                double dx = canvas_rudder_curve.Width / x_quantity;
                double dy = canvas_rudder_curve.Height / y_max;
                //draw pedal force-travel curve

                for (int i = 0; i < rectCount; i++)
                {
                    x[i] = rectPositionX[i];
                    travel[i] = (byte)(((rectPositionX[i]) / canvas_rudder_curve.Width) * 100);
                    y[i] = rectPositionY[i];
                    force[i] = (byte)(((canvas_rudder_curve.Height - rectPositionY[i]) / canvas_rudder_curve.Height) * 100);
                }



                // Use cubic interpolation to smooth the original data
                (double[] xs2_rudder, double[] ys2_rudder, double[] a_rudder, double[] b_rudder) = Cubic.Interpolate1D(x, y, (int)x_quantity);

                System.Windows.Media.PointCollection myPointCollection3 = new System.Windows.Media.PointCollection();
                System.Windows.Media.PointCollection myPointCollection4 = new System.Windows.Media.PointCollection();

                for (int pointIdx = 0; pointIdx < x_quantity; pointIdx++)
                {
                    System.Windows.Point Pointlcl = new System.Windows.Point(xs2_rudder[pointIdx], ys2_rudder[pointIdx]);
                    myPointCollection3.Add(Pointlcl);
                    myPointCollection4.Add(Pointlcl);
                    //Force_curve_Y[pointIdx] = dy * ys2_rudder[pointIdx];
                }

                this.Polyline_RudderForceCurve.Points = myPointCollection3;
                System.Windows.Point Pointend1 = new System.Windows.Point(canvas_rudder_curve.Width, canvas_rudder_curve.Height);
                System.Windows.Point Pointend2 = new System.Windows.Point(0, canvas_rudder_curve.Height);
                myPointCollection4.Add(Pointend1);
                myPointCollection4.Add(Pointend2);
                polygonCurveBackground.Points = myPointCollection4;
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

        private void InitRectangles()
        {
            for (int i = 0; i < 6; i++)
            {
                AddRectAt(i * 80 - 0.5 * RectSize, canvas_rudder_curve.Height - i * 40 - 0.5 * RectSize);
            }
            UpdateRectState();
        }

        private void AddRectAt(double x, double y)
        {
            Rectangle rect = new Rectangle
            {
                Width = RectSize,
                Height = RectSize,
                StrokeThickness = 2,
                RadiusX = 1,
                RadiusY = 1,
                Fill = System.Windows.Media.Brushes.Transparent,
                Opacity = 1.0
            };
            rect.Tag = (int)-1;
            //rect.SetResourceReference(Shape.FillProperty, "AccentColorBrush");
            rect.SetResourceReference(Rectangle.StrokeProperty, "AccentColorBrush");
            Canvas.SetLeft(rect, x);
            Canvas.SetTop(rect, y);

            rect.MouseRightButtonDown += Rect_MouseRightButtonDown;
            rect.MouseLeftButtonDown += Rect_MouseLeftButtonDown;
            rect.MouseLeftButtonUp += Rect_MouseLeftButtonUp;
            rect.MouseMove += Rect_MouseMove;

            canvas_rudder_curve.Children.Add(rect);

        }

        private void Rect_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            Rectangle rect = sender as Rectangle;
            if (rect != null && rect.Tag != null && rectCount > (minControlQuantity) && (int)rect.Tag != 0 && (int)rect.Tag != (rectCount - 1))
            {
                canvas_rudder_curve.Children.Remove(rect);
                UpdateRectState();
                Update_BrakeForceCurve();
                writeForceAndTravelToConfig();

            }
            e.Handled = true;

        }

        private void Canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (rectCount < (maxControlQuantity))
            {
                Point pos = e.GetPosition(canvas_rudder_curve);
                AddRectAt(pos.X, pos.Y);
                UpdateRectState();
                Update_BrakeForceCurve();
                writeForceAndTravelToConfig();
            }

        }

        private void UpdateRectState()
        {
            List<Rectangle> taggedRects = canvas_rudder_curve.Children
                .OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => Canvas.GetLeft(r))
                .ToList();

            rectCount = taggedRects.Count;
            rectPositionX.Clear();
            rectPositionY.Clear();

            for (int i = 0; i < taggedRects.Count; i++)
            {
                taggedRects[i].Tag = i;
                rectPositionX.Add(Canvas.GetLeft(taggedRects[i]) + 0.5 * RectSize);
                rectPositionY.Add(Canvas.GetTop(taggedRects[i]) + 0.5 * RectSize);
            }

            //Title = "rectCount: " + rectCount + " | X: [" + string.Join(", ", rectPositionX) + "]";
        }

        private void Rect_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            System.Windows.Shapes.Rectangle rect = sender as Rectangle;

            if (rect != null && rect.Tag != null)
            {

                int tag = (int)rect.Tag;
                //MessageBox.Show("Tag: " + tag + ", Count: " + rectCount);
                if (tag > 0 && tag < rectCount - 1)
                {
                    draggingRect = rect;
                    Point pos = e.GetPosition(canvas_rudder_curve);
                    dragOffset = new Point(pos.X - Canvas.GetLeft(rect), pos.Y - Canvas.GetTop(rect));
                    rect.CaptureMouse();
                    var dropShadowEffect = new DropShadowEffect
                    {
                        ShadowDepth = 0,
                        BlurRadius = 15,
                        Color = Colors.White,
                        Opacity = 1
                    };
                    //rectangle.Fill = calculation.lightcolor;
                    rect.Effect = dropShadowEffect;

                }
                text_point_pos_rudder.Visibility = Visibility.Visible;
                text_point_pos_rudder.Text = "#" + rect.Tag.ToString();
                text_point_pos_rudder.Text += "\nTravel:" + travel[(int)rect.Tag] + "%";
                text_point_pos_rudder.Text += "\nForce: " + force[(int)rect.Tag] + "%";


            }
        }

        private void Rect_MouseMove(object sender, MouseEventArgs e)
        {
            if (draggingRect != null && e.LeftButton == MouseButtonState.Pressed)
            {
                int myTag = (int)draggingRect.Tag;

                Point pos = e.GetPosition(canvas_rudder_curve);
                double newLeft = pos.X - dragOffset.X;
                double newTop = pos.Y - dragOffset.Y;
                List<Rectangle> taggedRects = canvas_rudder_curve.Children
                    .OfType<Rectangle>()
                    .Where(r => r.Tag != null)
                    .OrderBy(r => Canvas.GetLeft(r))
                    .ToList();

                double leftBound = Canvas.GetLeft(taggedRects[myTag - 1]) + RectSize;
                double rightBound = Canvas.GetLeft(taggedRects[myTag + 1]) - RectSize;

                newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                newTop = Math.Max(0, Math.Min(newTop, canvas_rudder_curve.Height));
                Canvas.SetLeft(draggingRect, newLeft);
                Canvas.SetTop(draggingRect, newTop);
                UpdateRectState();
                Update_BrakeForceCurve();
                text_point_pos_rudder.Visibility = Visibility.Visible;
                text_point_pos_rudder.Text = "#" + draggingRect.Tag.ToString();
                text_point_pos_rudder.Text += "\nTravel:" + travel[(int)draggingRect.Tag] + "%";
                text_point_pos_rudder.Text += "\nForce: " + force[(int)draggingRect.Tag] + "%";
                //writeForceAndTravelToConfig();
            }
        }

        private void Rect_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (draggingRect != null)
            {
                var dropShadowEffect = new DropShadowEffect
                {
                    ShadowDepth = 0,
                    BlurRadius = 20,
                    Color = Colors.White,
                    Opacity = 0
                };
                //rectangle.Fill = calculation.defaultcolor;
                draggingRect.Effect = dropShadowEffect;
                draggingRect.ReleaseMouseCapture();
                draggingRect = null;
                UpdateRectState();
                Update_BrakeForceCurve();
                writeForceAndTravelToConfig();
            }
            text_point_pos_rudder.Visibility = Visibility.Hidden;
        }

        private void checkExistingRect(byte rectNewCount)
        {
            if (rectNewCount < rectCount)
            {
                var toRemove = canvas_rudder_curve.Children
                 .OfType<Rectangle>()
                 .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue > (rectNewCount - 1))
                 .ToList();

                foreach (var rect in toRemove)
                {
                    canvas_rudder_curve.Children.Remove(rect);
                }
            }
            if (rectNewCount > rectCount)
            {
                for (int i = rectCount; i < rectNewCount; i++)
                {
                    AddRectAt(i * 80 - 0.5 * RectSize, canvas_rudder_curve.Height - i * 40 - 0.5 * RectSize);
                }

            }
            if (rectNewCount < minControlQuantity)
            {
                //do nothing
            }
            UpdateRectState();
            //Update_BrakeForceCurve();


        }
        private void updateRectControlFromConfig()
        {
            //check existing rect
            if (minControlQuantity > Settings.rudderControlQuantity) return;
            checkExistingRect(Settings.rudderControlQuantity);
            readForceAndTravelFromConfig();
            UpdateRectState();
            var toDraw = canvas_rudder_curve.Children
             .OfType<Rectangle>()
             .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue < rectCount)
             .ToList();
            //fix the latest one to 100%
            Canvas.SetLeft(toDraw[rectCount - 1], canvas_rudder_curve.Width - 0.5 * RectSize);
            Canvas.SetTop(toDraw[rectCount - 1], 0 - 0.5 * RectSize);
            double dx = canvas_rudder_curve.Width / 100.0;
            double dy = canvas_rudder_curve.Height / 100.0;
            for (int i = 1; i < rectCount - 1; i++)
            {
                //fill with new loaded value, only fill from 1 to rectNewCount-2
                Canvas.SetLeft(toDraw[i], (double)travel[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], canvas_rudder_curve.Height - (double)force[i] * dy - 0.5 * RectSize);

            }
            UpdateRectState();
            Update_BrakeForceCurve();
        }

        private void updateRectFromInternalSetting(byte countSet)
        {
            //check existing rect
            if (minControlQuantity > countSet) return;
            checkExistingRect(countSet);
            UpdateRectState();
            var toDraw = canvas_rudder_curve.Children
             .OfType<Rectangle>()
             .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue < rectCount)
             .ToList();
            //fix the latest one to 100%
            Canvas.SetLeft(toDraw[rectCount - 1], canvas_rudder_curve.Width - 0.5 * RectSize);
            Canvas.SetTop(toDraw[rectCount - 1], 0 - 0.5 * RectSize);
            double dx = canvas_rudder_curve.Width / 100.0;
            double dy = canvas_rudder_curve.Height / 100.0;
            for (int i = 1; i < rectCount - 1; i++)
            {
                //fill with new loaded value, only fill from 1 to rectNewCount-2
                Canvas.SetLeft(toDraw[i], (double)travel[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], canvas_rudder_curve.Height - (double)force[i] * dy - 0.5 * RectSize);

            }
            UpdateRectState();
            Update_BrakeForceCurve();
        }

        private void writeForceAndTravelToConfig()
        {
            UpdateRectState();
            for (int i = 0; i < maxControlQuantity; i++)
            {
                Settings.rudderForce[i] = force[i];
                Settings.rudderTravel[i] = travel[i];
            }
            Settings.rudderControlQuantity = (byte)rectCount;


        }

        private void readForceAndTravelFromConfig()
        {
            for (int i = 0; i < maxControlQuantity; i++)
            {
                force[i]= Settings.rudderForce[i];
                travel[i]= Settings.rudderTravel[i];
            }

        }
       
    }
}
