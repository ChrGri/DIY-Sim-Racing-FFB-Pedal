using FMOD;
using SimHub.Plugins.OutputPlugins.GraphicalDash.UI;
using System;
using System.Collections.Generic;
using System.Drawing;
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
using WoteverCommon.WPF;
using Point = System.Windows.Point;
using Rectangle = System.Windows.Shapes.Rectangle;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// CurveTab_PedalForceTravel.xaml 的互動邏輯
    /// </summary>
    public partial class CurveTab_JoystickMapping : UserControl
    {
        private const double RectSize = 10;
        public int rectCount = 0;
        public List<double> rectPositionX = new List<double>();
        public List<double> rectPositionY = new List<double>();
        private Rectangle draggingRect = null;
        private Point dragOffset;
        private int minControlQuantity = 6;
        private int maxControlQuantity = 11;
        private double[] update_y = new double[1001] ;
        private int numOfMaxPointinLine = 1001;
        private byte[] joystickValueOrig;
        private byte[] joystickValueMapping;
        private double minpos = 0;
        private double maxpos = 400;
        DateTime forceUpdate_currentTime = DateTime.Now;
        DateTime forceUpdate_lastTime = DateTime.Now;
        public CurveTab_JoystickMapping()
        {
            InitializeComponent();
            DrawGridLines();
            InitRectangles();
          
            joystickValueOrig = new byte[maxControlQuantity];
            joystickValueMapping = new byte[maxControlQuantity];
            Update_BrakeForceCurve();
            maxpos = mainCanvas.Width;
            //update_y = new double[numOfMaxPointinLine];
            
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_JoystickMapping),
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
            typeof(CurveTab_JoystickMapping),
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
            typeof(CurveTab_JoystickMapping),
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
            var control = d as CurveTab_JoystickMapping;
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
            var control = d as CurveTab_JoystickMapping;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {

                    if (control.mainCanvas != null) control.CanvasDraw();
                    if (control.mainCanvas != null) control.updateRectControlFromConfig();
                    
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_JoystickMapping;
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
            text_point_pos.Visibility = Visibility.Hidden;
            double control_rect_value_max = 100;
            double dyy = mainCanvas.Height / control_rect_value_max;
            //Canvas.SetTop(rect0, mainCanvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p000 - rect0.Height / 2);
            //Canvas.SetLeft(rect0, 0 * mainCanvas.Width / 5 - rect0.Width / 2);
            Canvas.SetTop(rectState, mainCanvas.Height - rectState.Height / 2);
            Canvas.SetLeft(rectState, -rectState.Width / 2);
            Canvas.SetLeft(textState, Canvas.GetLeft(rectState) );
            Canvas.SetTop(textState, Canvas.GetTop(rectState) - rectState.Height);
            textState.Text = "0%";
            Update_BrakeForceCurve();
        }

        private void btn_linearcurve_Click(object sender, RoutedEventArgs e)
        {
            int count = 6;

            GetMaxMinpos();
            UpdateRectState();
            checkExistingRect((byte)count);
            double maxY = 100;
            double maxXCount = 100;
            double dx = (maxpos - minpos) / (count-1);
            double dxx =  mainCanvas.Width / maxXCount;
            double dyy = mainCanvas.Height / maxY;
            
            double dy = maxY / (count-1);
            joystickValueOrig[0] = (byte)(minpos / dxx);
            joystickValueMapping[0] = (byte)(0);
            joystickValueOrig[count-1] = (byte)(maxpos / dxx);
            joystickValueMapping[count - 1] = (byte)(100);
            for (int i = 1; i < count-1; i++)
            {
                
                joystickValueOrig[i] = (byte)((minpos + dx * i )/ dxx);
                joystickValueMapping[i] = (byte)(dy*i); 

            }
            for (int i = count; i < maxControlQuantity; i++)
            {
                joystickValueOrig[i] = 0;
                joystickValueMapping[i]= 0;
            }
            /*
            String MSG_tmp;
            MSG_tmp = "";
            MSG_tmp += "minpos=" + minpos + " maxpos" + maxpos + "\n";
            for (int i = 0; i < count; i++)
            {
                MSG_tmp += "joystickValueOrig" + i + "=" + joystickValueOrig[i] + "\n";
                MSG_tmp += "joystickValueMapping" + i + "=" + joystickValueMapping[i] + "\n";
            }
            */
            //MSG_tmp = "Pedal didnt connect to Bridge, please connect pedal to via Bridge then try again.";
            //System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            redrawUIOnly();
            writeForceAndTravelToConfig();
        }
        private double SmoothStep(double x)
        {
            return x * x * (3 - 2 * x);
        }
        private void btn_Scurve_Click(object sender, RoutedEventArgs e)
        {
            int count = 6;
            UpdateRectState();
            checkExistingRect((byte)count);
            double maxY = 100;
            double maxXCount = 100;
            double dx = (maxpos - minpos) / (count - 1);
            double x_range = (maxpos - minpos);
            double dxx = mainCanvas.Width / maxXCount;
            double dyy = mainCanvas.Height / maxY;

            double dy = maxY / (count - 1);
            joystickValueOrig[0] = (byte)(minpos / dxx);
            joystickValueMapping[0] = (byte)(0);
            joystickValueOrig[count - 1] = (byte)(maxpos / dxx);
            joystickValueMapping[count - 1] = (byte)(100);
            for (int i = 1; i < count - 1; i++)
            {

                joystickValueOrig[i] = (byte)((minpos + dx * i) / dxx);
                joystickValueMapping[i] = (byte)(maxY*SmoothStep(dx*i/x_range));

            }
            for (int i = count; i < maxControlQuantity; i++)
            {
                joystickValueOrig[i] = 0;
                joystickValueMapping[i] = 0;
            }
            /*
            String MSG_tmp;
            MSG_tmp = "";
            MSG_tmp += "minpos=" + minpos + " maxpos" + maxpos + "\n";
            for (int i = 0; i < count; i++)
            {
                MSG_tmp += "joystickValueOrig" + i + "=" + joystickValueOrig[i] + "\n";
                MSG_tmp += "joystickValueMapping" + i + "=" + joystickValueMapping[i] + "\n";
            }
            
            
            System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            */
            redrawUIOnly();
            writeForceAndTravelToConfig();
        }
        private double ExponentialTo100(double x, double a = 5.0)
        {
            x = Clamp(x, 0.0, 1.0);
            return (Math.Pow(a, x) - 1.0) / (a - 1.0);

        }
        private double Clamp(double value, double min, double max)
        {
            if (value < min) return min;
            if (value > max) return max;
            return value;
        }
        private double LogarithmicTo100(double x, double a = 7.0)
        {
            // Clamp x to [0,1] for safety
            x = Clamp(x, 0.0, 1.0);

            // Avoid log(0) by shifting domain
            double scaled = x * (a - 1) + 1;
            return Math.Log(scaled) / Math.Log(a);
        }
        private void btn_10xcurve_Click(object sender, RoutedEventArgs e)
        {
            int count = 6;
            checkExistingRect((byte)count);
            UpdateRectState();
            rectPositionX[0] = minpos;
            rectPositionX[count - 1] = maxpos;
            double maxY = 100;
            double maxXCount = 100;
            double dx = (maxpos - minpos) / (count - 1);
            double x_range = (maxpos - minpos);
            double dxx = mainCanvas.Width / maxXCount;
            double dyy = mainCanvas.Height / maxY;

            double dy = maxY / (count - 1);
            joystickValueOrig[0] = (byte)(minpos / dxx);
            joystickValueMapping[0] = (byte)(0);
            joystickValueOrig[count - 1] = (byte)(maxpos / dxx);
            joystickValueMapping[count - 1] = (byte)(100);
            for (int i = 1; i < count - 1; i++)
            {

                joystickValueOrig[i] = (byte)((minpos + dx * i) / dxx);
                joystickValueMapping[i] = (byte)(maxY * ExponentialTo100(dx * i / x_range));

            }
            for (int i = count; i < maxControlQuantity; i++)
            {
                joystickValueOrig[i] = 0;
                joystickValueMapping[i] = 0;
            }
            /*
            String MSG_tmp;
            MSG_tmp = "";
            MSG_tmp += "minpos=" + minpos + " maxpos" + maxpos + "\n";
            for (int i = 0; i < count; i++)
            {
                MSG_tmp += "joystickValueOrig" + i + "=" + joystickValueOrig[i] + "\n";
                MSG_tmp += "joystickValueMapping" + i + "=" + joystickValueMapping[i] + "\n";
            }
            */
            
            
            //System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            
            redrawUIOnly();
            writeForceAndTravelToConfig();
        }

        private void btn_logcurve_Click(object sender, RoutedEventArgs e)
        {
            int count = 6;
            checkExistingRect((byte)count);
            UpdateRectState();
            rectPositionX[0] = minpos;
            rectPositionX[count - 1] = maxpos;
            double maxY = 100;
            double maxXCount = 100;
            double dx = (maxpos - minpos) / (count - 1);
            double x_range = (maxpos - minpos);
            double dxx = mainCanvas.Width / maxXCount;
            double dyy = mainCanvas.Height / maxY;

            double dy = maxY / (count - 1);
            joystickValueOrig[0] = (byte)(minpos / dxx);
            joystickValueMapping[0] = (byte)(0);
            joystickValueOrig[count - 1] = (byte)(maxpos / dxx);
            joystickValueMapping[count - 1] = (byte)(100);
            for (int i = 1; i < count - 1; i++)
            {

                joystickValueOrig[i] = (byte)((minpos + dx * i) / dxx);
                joystickValueMapping[i] = (byte)(maxY * LogarithmicTo100(dx * i / x_range));

            }
            for (int i = count; i < maxControlQuantity; i++)
            {
                joystickValueOrig[i] = 0;
                joystickValueMapping[i] = 0;
            }
            /*
            String MSG_tmp;
            MSG_tmp = "";
            MSG_tmp += "minpos=" + minpos + " maxpos" + maxpos + "\n";
            for (int i = 0; i < count; i++)
            {
                MSG_tmp += "joystickValueOrig" + i + "=" + joystickValueOrig[i] + "\n";
                MSG_tmp += "joystickValueMapping" + i + "=" + joystickValueMapping[i] + "\n";
            }
            */


            //System.Windows.MessageBox.Show(MSG_tmp, "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

            redrawUIOnly();
            writeForceAndTravelToConfig();
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
                //rectangle.Fill = calculation.lightcolor;
                rectangle.Effect = dropShadowEffect;
            }
        }

     

 
        private void Update_BrakeForceCurve()
        {

            double[] x = new double[rectCount];
            double[] y = new double[rectCount];
            double[] x_update = new double[rectCount];
            double[] y_update = new double[rectCount];
            double x_quantity = 101;
            double y_max = 101;
            double x_width = rectPositionX[rectCount - 1]-rectPositionX[0];
            double dx = x_width / (x_quantity-1);
            double dxx = mainCanvas.Width / (x_quantity - 1);
            double dy = mainCanvas.Height / (y_max-1);
            //draw pedal joystickValueOrig-joystickValueMapping curve
            for (int i = 0; i < rectCount; i++)
            {
                x[i]=rectPositionX[i] - rectPositionX[0];
                joystickValueOrig[i] = (byte)((((rectPositionX[i])) / mainCanvas.Width) * 100);
                x_update[i] = (double)(joystickValueOrig[i]- joystickValueOrig[0]);
                y[i]=rectPositionY[i];
                joystickValueMapping[i] = (byte)(((mainCanvas.Height- rectPositionY[i] ) / mainCanvas.Height) * 100);
                y_update[i]= (double)(joystickValueMapping[i]);
            }
           

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, (int)x_quantity);
            //(double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, (int)(joystickValueOrig[rectCount-1]- joystickValueOrig[0]-1));


            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            System.Windows.Media.PointCollection myPointCollection3 = new System.Windows.Media.PointCollection();
            System.Windows.Point PointStart= new System.Windows.Point(0, mainCanvas.Height);
            myPointCollection2.Add(PointStart);
            myPointCollection3.Add(PointStart);
            for (int pointIdx = 0; pointIdx < (x_quantity); pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(xs2[pointIdx] + rectPositionX[0], ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);
                myPointCollection3.Add(Pointlcl);
                //calculation.Force_curve_Y[pointIdx] = ys2[pointIdx];
            }
            System.Windows.Point PointEnd = new System.Windows.Point(mainCanvas.Width, 0);
            myPointCollection2.Add(PointEnd);
            myPointCollection3.Add(PointEnd);
            this.Polyline_BrakeForceCurve.Points = myPointCollection2;
            System.Windows.Point Pointend1 = new System.Windows.Point(mainCanvas.Width, mainCanvas.Height);
            myPointCollection3.Add(Pointend1);
            myPointCollection3.Add(PointStart);
            polygonCurveBackground.Points = myPointCollection3;
            //calculate actual point
            (double[] xs3, double[] ys3, double[] a2, double[] b2) = Cubic.Interpolate1D(x_update, y_update, (int)(joystickValueOrig[rectCount - 1]*10 - joystickValueOrig[0]*10+1));

            int initialPos = joystickValueOrig[0] * 10;
            int endPos = joystickValueOrig[rectCount - 1] * 10;

            for (int i = 0; i < initialPos; i++)
            {
                update_y[i] = 0;    
            }
            for (int i = initialPos; i < endPos; i++)
            {
                update_y[i] = ys3[i- initialPos];
            }
            for (int i = endPos; i < numOfMaxPointinLine; i++)
            {
                update_y[i] = 100;
            }

        }



        private void DrawGridLines()
        {
            // Specify the number of rows and columns for the grid
            int rowCount = 5;
            int columnCount = 5;

            // Calculate the width and height of each cell
            double cellWidth = mainCanvas.Width / columnCount;
            double cellHeight = mainCanvas.Height / rowCount;



            // Draw horizontal gridlines
            for (int i = 1; i < rowCount; i++)
            {
                Line line = new Line
                {
                    X1 = 0,
                    Y1 = i * cellHeight,
                    X2 = mainCanvas.Width,
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
                    X2 = mainCanvas.Width,
                    Y2 = i * cellHeight,
                    //Stroke = Brush.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1

                };
                mainCanvas.Children.Add(line);
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
                    Y2 = mainCanvas.Height,
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
                    Y2 = mainCanvas.Height,
                    //Stroke = Brushes.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                mainCanvas.Children.Add(line);
                //canvas_rudder_curve.Children.Add(line2);

            }
        }

        

        private void InitRectangles()
        {
            //AddRectAt(40 - 0.5 * RectSize, mainCanvas.Height - 0 * 40 - 0.5 * RectSize);
            for (int i = 0; i < minControlQuantity; i++)
            {
                AddRectAt(i * 80-0.5*RectSize, mainCanvas.Height-i*40-0.5*RectSize);
            }
            //AddRectAt(360 - 0.5 * RectSize, mainCanvas.Height - 5 * 40 - 0.5 * RectSize);
            UpdateRectState();
            GetMaxMinpos();
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

            mainCanvas.Children.Add(rect);
            
        }

        private void Rect_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            Rectangle rect = sender as Rectangle;
            if (rect != null && rect.Tag != null && rectCount>(minControlQuantity) && (int)rect.Tag!=0 && (int)rect.Tag!=(rectCount-1))
            {
                mainCanvas.Children.Remove(rect);
                UpdateRectState();
                GetMaxMinpos();
                Update_BrakeForceCurve();
                //writeForceAndTravelToConfig();

            }
            e.Handled = true;
            
        }

        private void Canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (rectCount < (maxControlQuantity ))
            {

                Point pos = e.GetPosition(mainCanvas);
                if (pos.X<maxpos && pos.X>minpos)
                {
                    AddRectAt(pos.X, pos.Y);
                    UpdateRectState();
                    GetMaxMinpos();
                    UpdateRectState();
                    Update_BrakeForceCurve();
                }

                //writeForceAndTravelToConfig();
            }

        }

        private void UpdateRectState()
        {
            List<Rectangle> taggedRects = mainCanvas.Children
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
                rectPositionX.Add(Canvas.GetLeft(taggedRects[i])+0.5*RectSize);
                rectPositionY.Add(Canvas.GetTop(taggedRects[i])+0.5*RectSize);
            }
            //Title = "rectCount: " + rectCount + " | X: [" + string.Join(", ", rectPositionX) + "]";
        }
        private void GetMaxMinpos()
        {
            minpos = rectPositionX[0];
            maxpos = rectPositionX[rectCount - 1];
        }

        private void Rect_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            UpdateRectState ();
            System.Windows.Shapes.Rectangle rect = sender as Rectangle;

            if (rect != null && rect.Tag != null)
            {
                
                int tag = (int)rect.Tag;
                //MessageBox.Show("Tag: " + tag + ", Count: " + rectCount);
                //if (tag > 0 && tag < rectCount - 1)
                if (tag > -1)
                {
                    draggingRect = rect;
                    Point pos = e.GetPosition(mainCanvas);
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
                text_point_pos.Visibility = Visibility.Visible;
                text_point_pos.Text = "#" + rect.Tag.ToString();
                text_point_pos.Text += "\nOrig:" + joystickValueOrig[(int)rect.Tag] + "%";
                text_point_pos.Text += "\nMapping: " + joystickValueMapping[(int)rect.Tag] + "%";
                GetMaxMinpos();

            }
        }

        private void Rect_MouseMove(object sender, MouseEventArgs e)
        {
            if (draggingRect != null && e.LeftButton == MouseButtonState.Pressed)
            {

                int myTag = (int)draggingRect.Tag;
                // move freely
                if (myTag > 0 && myTag <rectCount - 1)
                {
                    Point pos = e.GetPosition(mainCanvas);
                    double newLeft = pos.X - dragOffset.X;
                    double newTop = pos.Y - dragOffset.Y;
                    List<Rectangle> taggedRects = mainCanvas.Children
                        .OfType<Rectangle>()
                        .Where(r => r.Tag != null)
                        .OrderBy(r => Canvas.GetLeft(r))
                        .ToList();

                    double leftBound = Canvas.GetLeft(taggedRects[myTag - 1]) + RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[myTag + 1]) - RectSize;

                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = Math.Max(0, Math.Min(newTop, mainCanvas.Height));
                    Canvas.SetLeft(draggingRect, newLeft);
                    Canvas.SetTop(draggingRect, newTop);
                }
                //only move x-axis
                //first one
                if (myTag == 0)
                {
                    Point pos = e.GetPosition(mainCanvas);
                    double newLeft = pos.X - dragOffset.X;
                    double newTop = pos.Y - dragOffset.Y;
                    List<Rectangle> taggedRects = mainCanvas.Children
                        .OfType<Rectangle>()
                        .Where(r => r.Tag != null)
                        .OrderBy(r => Canvas.GetLeft(r))
                        .ToList();

                    double leftBound = 0 - 0.5*RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[myTag + 1]) - RectSize;

                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = mainCanvas.Height - 0.5*RectSize;
                    Canvas.SetLeft(draggingRect, newLeft);
                    Canvas.SetTop(draggingRect, newTop);
                }
                //last one
                if (myTag == rectCount - 1)
                {
                    Point pos = e.GetPosition(mainCanvas);
                    double newLeft = pos.X - dragOffset.X;
                    double newTop = pos.Y - dragOffset.Y;
                    List<Rectangle> taggedRects = mainCanvas.Children
                        .OfType<Rectangle>()
                        .Where(r => r.Tag != null)
                        .OrderBy(r => Canvas.GetLeft(r))
                        .ToList();

                    double leftBound = Canvas.GetLeft(taggedRects[myTag - 1]) + RectSize; ;
                    double rightBound = mainCanvas.Width - 0.5*RectSize;

                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = 0 - 0.5 * RectSize; ;
                    Canvas.SetLeft(draggingRect, newLeft);
                    Canvas.SetTop(draggingRect, newTop);
                }
            

                UpdateRectState();
                GetMaxMinpos();
                Update_BrakeForceCurve();
                text_point_pos.Visibility = Visibility.Visible;
                text_point_pos.Text = "#"+draggingRect.Tag.ToString();
                text_point_pos.Text += "\nOrig:" + joystickValueOrig[myTag] + "%";
                text_point_pos.Text += "\nMapping: " + joystickValueMapping[myTag] + "%";
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
                
                writeForceAndTravelToConfig();
            }
            text_point_pos.Visibility = Visibility.Hidden;
        }
        private void readForceAndTravelFromConfig()
        {
            joystickValueOrig[0] = dap_config_st.payloadPedalConfig_.joystickMapOrig00;
            joystickValueOrig[1] = dap_config_st.payloadPedalConfig_.joystickMapOrig01;
            joystickValueOrig[2] = dap_config_st.payloadPedalConfig_.joystickMapOrig02;
            joystickValueOrig[3] = dap_config_st.payloadPedalConfig_.joystickMapOrig03;
            joystickValueOrig[4] = dap_config_st.payloadPedalConfig_.joystickMapOrig04;
            joystickValueOrig[5] = dap_config_st.payloadPedalConfig_.joystickMapOrig05;
            joystickValueOrig[6] = dap_config_st.payloadPedalConfig_.joystickMapOrig06;
            joystickValueOrig[7] = dap_config_st.payloadPedalConfig_.joystickMapOrig07;
            joystickValueOrig[8] = dap_config_st.payloadPedalConfig_.joystickMapOrig08;
            joystickValueOrig[9] = dap_config_st.payloadPedalConfig_.joystickMapOrig09;
            joystickValueOrig[10] = dap_config_st.payloadPedalConfig_.joystickMapOrig10;

            joystickValueMapping[0] = dap_config_st.payloadPedalConfig_.joystickMapMapped00;
            joystickValueMapping[1] = dap_config_st.payloadPedalConfig_.joystickMapMapped01;
            joystickValueMapping[2] = dap_config_st.payloadPedalConfig_.joystickMapMapped02;
            joystickValueMapping[3] = dap_config_st.payloadPedalConfig_.joystickMapMapped03;
            joystickValueMapping[4] = dap_config_st.payloadPedalConfig_.joystickMapMapped04;
            joystickValueMapping[5] = dap_config_st.payloadPedalConfig_.joystickMapMapped05;
            joystickValueMapping[6] = dap_config_st.payloadPedalConfig_.joystickMapMapped06;
            joystickValueMapping[7] = dap_config_st.payloadPedalConfig_.joystickMapMapped07;
            joystickValueMapping[8] = dap_config_st.payloadPedalConfig_.joystickMapMapped08;
            joystickValueMapping[9] = dap_config_st.payloadPedalConfig_.joystickMapMapped09;
            joystickValueMapping[10] = dap_config_st.payloadPedalConfig_.joystickMapMapped10;
        }
        private void writeForceAndTravelToConfig()
        {
            UpdateRectState();
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.joystickMapOrig00 = joystickValueOrig[0];
            tmp.payloadPedalConfig_.joystickMapOrig01 = joystickValueOrig[1];
            tmp.payloadPedalConfig_.joystickMapOrig02 = joystickValueOrig[2];
            tmp.payloadPedalConfig_.joystickMapOrig03 = joystickValueOrig[3];
            tmp.payloadPedalConfig_.joystickMapOrig04 = joystickValueOrig[4];
            tmp.payloadPedalConfig_.joystickMapOrig05 = joystickValueOrig[5];
            tmp.payloadPedalConfig_.joystickMapOrig06 = joystickValueOrig[6];
            tmp.payloadPedalConfig_.joystickMapOrig07 = joystickValueOrig[7];
            tmp.payloadPedalConfig_.joystickMapOrig08 = joystickValueOrig[8];
            tmp.payloadPedalConfig_.joystickMapOrig09 = joystickValueOrig[9];
            tmp.payloadPedalConfig_.joystickMapOrig10 = joystickValueOrig[10];

            tmp.payloadPedalConfig_.joystickMapMapped00 = joystickValueMapping[0];
            tmp.payloadPedalConfig_.joystickMapMapped01 = joystickValueMapping[1];
            tmp.payloadPedalConfig_.joystickMapMapped02 = joystickValueMapping[2];
            tmp.payloadPedalConfig_.joystickMapMapped03 = joystickValueMapping[3];
            tmp.payloadPedalConfig_.joystickMapMapped04 = joystickValueMapping[4];
            tmp.payloadPedalConfig_.joystickMapMapped05 = joystickValueMapping[5];
            tmp.payloadPedalConfig_.joystickMapMapped06 = joystickValueMapping[6];
            tmp.payloadPedalConfig_.joystickMapMapped07 = joystickValueMapping[7];
            tmp.payloadPedalConfig_.joystickMapMapped08 = joystickValueMapping[8];
            tmp.payloadPedalConfig_.joystickMapMapped09 = joystickValueMapping[9];
            tmp.payloadPedalConfig_.joystickMapMapped10 = joystickValueMapping[10];
            
            tmp.payloadPedalConfig_.numOfJoystickMapControl = (byte)rectCount;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);

        }
        private void checkExistingRect(byte rectNewCount)
        {
            if (rectNewCount < rectCount)
            {
                var toRemove = mainCanvas.Children
                 .OfType<Rectangle>()
                 .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue > (rectNewCount - 1))
                 .ToList();

                foreach (var rect in toRemove)
                {
                    mainCanvas.Children.Remove(rect);
                }
            }
            if (rectNewCount > rectCount)
            {
                for (int i = rectCount; i < rectNewCount; i++)
                {
                    AddRectAt(i * 80 - 0.5 * RectSize, mainCanvas.Height - i * 40 - 0.5 * RectSize);
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
            if (minControlQuantity > dap_config_st.payloadPedalConfig_.numOfJoystickMapControl) return;
            checkExistingRect(dap_config_st.payloadPedalConfig_.numOfJoystickMapControl);
            readForceAndTravelFromConfig();
            GetMaxMinpos();
            redrawUIOnly();
        }

        private void updateRectFromInternalSetting(byte countSet)
        {
            //check existing rect
            if (minControlQuantity > countSet) return;
            checkExistingRect(countSet);
            UpdateRectState();
            var toDraw = mainCanvas.Children
             .OfType<Rectangle>()
             .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue < rectCount)
             .ToList();
            //fix the latest one to 100%
            Canvas.SetLeft(toDraw[rectCount - 1], mainCanvas.Width - 0.5 * RectSize);
            Canvas.SetTop(toDraw[rectCount - 1], 0 - 0.5 * RectSize);
            double dx = mainCanvas.Width / 100.0;
            double dy = mainCanvas.Height / 100.0;
            for (int i = 1; i < rectCount - 1; i++)
            {
                //fill with new loaded value, only fill from 1 to rectNewCount-2
                Canvas.SetLeft(toDraw[i], (double)joystickValueOrig[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], mainCanvas.Height - (double)joystickValueMapping[i] * dy - 0.5 * RectSize);

            }
            UpdateRectState();
            Update_BrakeForceCurve();
        }
        private void redrawUIOnly()
        {
            UpdateRectState();
            var toDraw = mainCanvas.Children
             .OfType<Rectangle>()
             .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue < rectCount)
             .ToList();
            double dx = mainCanvas.Width / 100.0;
            double dy = mainCanvas.Height / 100.0;
            for (int i = 0; i < rectCount ; i++)
            {
                //fill with new loaded value, only fill from 1 to rectNewCount-2
                Canvas.SetLeft(toDraw[i], (double)joystickValueOrig[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], mainCanvas.Height - (double)joystickValueMapping[i] * dy - 0.5 * RectSize);

            }
            
            UpdateRectState();
            GetMaxMinpos();
            Update_BrakeForceCurve();
        }
        public void JoystickStateUpdate(ushort pedalJoystickPosition_u16)
        {
            
            double control_rect_value_max = 65535;
            double round_x = (1000 * (double)pedalJoystickPosition_u16 / control_rect_value_max);
            double xPosition=Clamp(round_x, 0, 1000);
            double controlRectValueMaxNormalized = 1000;
            double controlRectValueYMaxNormalized = 100;
            double dyy = mainCanvas.Height / controlRectValueYMaxNormalized;
            double dxx = mainCanvas.Width / controlRectValueMaxNormalized;
            Canvas.SetTop(rectState, mainCanvas.Height - update_y[(int)xPosition]*dyy - rectState.Height / 2);
            Canvas.SetLeft(rectState, xPosition*dxx-rectState.Width / 2);
            Canvas.SetLeft(textState, Canvas.GetLeft(rectState));
            Canvas.SetTop(textState, Canvas.GetTop(rectState) - rectState.Height);
            
            textState.Text = Math.Round(update_y[(int)xPosition])+"%";
            //textDebug.Text = "input:" + pedalJoystickPosition_u16 + " xPosition:"+xPosition;
        }
    }
}
