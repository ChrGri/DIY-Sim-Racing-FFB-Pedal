#pragma warning disable CS0067
using SimHub.Plugins.OutputPlugins.GraphicalDash.UI;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Effects;
using System.Windows.Shapes;
using Point = System.Windows.Point;
using Rectangle = System.Windows.Shapes.Rectangle;

namespace DiyFfbPedal.UIFunction
{
    public partial class CurveTab_RudderJoystickMapping : UserControl
    {
        private const double RectSize = 8;
        private const int MinControlQuantity = 6;
        private const int MaxControlQuantity = 11;

        // Yaw curve state (50% to 100%)
        private int yawRectCount = 6;
        private List<double> yawRectPosX = new List<double>();
        private List<double> yawRectPosY = new List<double>();
        private Rectangle yawDraggingRect = null;
        private Point yawDragOffset;
        public byte[] yawJoystickOrig = new byte[MaxControlQuantity];
        public byte[] yawJoystickMapped = new byte[MaxControlQuantity];
        private double[] yawTable = new double[1001]; // Index 0..1000 representing 50.0% to 100.0%

        // Toe brake curve state (0% to 100%)
        private int toeRectCount = 6;
        private List<double> toeRectPosX = new List<double>();
        private List<double> toeRectPosY = new List<double>();
        private Rectangle toeDraggingRect = null;
        private Point toeDragOffset;
        public byte[] toeJoystickOrig = new byte[MaxControlQuantity];
        public byte[] toeJoystickMapped = new byte[MaxControlQuantity];
        private double[] toeTable = new double[1001]; // Index 0..1000 representing 0.0% to 100.0%

        public CurveTab_RudderJoystickMapping()
        {
            InitializeComponent();
            DrawGridLines(canvasYaw);
            DrawGridLines(canvasToe);

            // Default Yaw Points (50% to 100%)
            byte[] defaultYawX = new byte[] { 50, 60, 70, 80, 90, 100 };
            byte[] defaultYawY = new byte[] { 50, 60, 70, 80, 90, 100 };
            for (int i = 0; i < 6; i++)
            {
                yawJoystickOrig[i] = defaultYawX[i];
                yawJoystickMapped[i] = defaultYawY[i];
            }

            // Default Toe Points (0% to 100%)
            byte[] defaultToeX = new byte[] { 0, 20, 40, 60, 80, 100 };
            byte[] defaultToeY = new byte[] { 0, 20, 40, 60, 80, 100 };
            for (int i = 0; i < 6; i++)
            {
                toeJoystickOrig[i] = defaultToeX[i];
                toeJoystickMapped[i] = defaultToeY[i];
            }

            InitYawRectangles();
            InitToeRectangles();

            UpdateYawCurve();
            UpdateToeCurve();
            UpdateYawState(0.5);
            UpdateToeBrakeState(0.0);
        }

        #region Dependency Properties

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_RudderJoystickMapping),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public DAP_config_st dap_config_st
        {
            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set => SetValue(DAP_Config_Property, value);
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(CurveTab_RudderJoystickMapping),
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

        public static readonly DependencyProperty Calculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(CurveTab_RudderJoystickMapping),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Calculation_Property);
            set => SetValue(Calculation_Property, value);
        }

        public event EventHandler<DAP_config_st> ConfigChanged;
        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        public event EventHandler<CalculationVariables> CalculationChanged;

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CurveTab_RudderJoystickMapping control && e.NewValue is DIYFFBPedalSettings)
            {
                control.updateUI();
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CurveTab_RudderJoystickMapping control && e.NewValue is DAP_config_st)
            {
                control.updateRectControlFromConfig();
            }
        }

        #endregion

        #region Grid Lines

        private void DrawGridLines(Canvas canvas)
        {
            int rowCount = 5;
            int columnCount = 5;
            double cellWidth = canvas.Width / columnCount;
            double cellHeight = canvas.Height / rowCount;

            for (int i = 1; i < rowCount; i++)
            {
                Line line = new Line
                {
                    X1 = 0,
                    Y1 = i * cellHeight,
                    X2 = canvas.Width,
                    Y2 = i * cellHeight,
                    Stroke = Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                canvas.Children.Add(line);
            }

            for (int i = 1; i < columnCount; i++)
            {
                Line line = new Line
                {
                    X1 = i * cellWidth,
                    Y1 = 0,
                    X2 = i * cellWidth,
                    Y2 = canvas.Height,
                    Stroke = Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                canvas.Children.Add(line);
            }
        }

        #endregion

        #region Yaw Mapping Logic (50% - 100%)

        private void InitYawRectangles()
        {
            for (int i = 0; i < MinControlQuantity; i++)
            {
                double xFrac = (double)i / (MinControlQuantity - 1);
                double px = xFrac * canvasYaw.Width - 0.5 * RectSize;
                double py = canvasYaw.Height - xFrac * canvasYaw.Height - 0.5 * RectSize;
                AddYawRectAt(px, py);
            }
            UpdateYawRectState();
        }

        private void AddYawRectAt(double x, double y)
        {
            Rectangle rect = new Rectangle
            {
                Width = RectSize,
                Height = RectSize,
                StrokeThickness = 2,
                RadiusX = 1,
                RadiusY = 1,
                Fill = Brushes.Transparent,
                Opacity = 1.0,
                Cursor = Cursors.Hand
            };
            rect.Tag = -1;
            rect.SetResourceReference(Shape.StrokeProperty, "AccentColorBrush");
            Canvas.SetLeft(rect, x);
            Canvas.SetTop(rect, y);

            rect.MouseRightButtonDown += YawRect_MouseRightButtonDown;
            rect.MouseLeftButtonDown += YawRect_MouseLeftButtonDown;
            rect.MouseLeftButtonUp += YawRect_MouseLeftButtonUp;
            rect.MouseMove += YawRect_MouseMove;

            canvasYaw.Children.Add(rect);
        }

        private void CanvasYaw_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (yawRectCount < MaxControlQuantity)
            {
                Point pos = e.GetPosition(canvasYaw);
                if (pos.X > yawRectPosX[0] && pos.X < yawRectPosX[yawRectCount - 1])
                {
                    AddYawRectAt(pos.X - 0.5 * RectSize, pos.Y - 0.5 * RectSize);
                    UpdateYawRectState();
                    UpdateYawCurve();
                    WriteConfig();
                }
            }
        }

        private void YawRect_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (sender is Rectangle rect && rect.Tag is int tag)
            {
                if (yawRectCount > MinControlQuantity && tag > 0 && tag < (yawRectCount - 1))
                {
                    canvasYaw.Children.Remove(rect);
                    UpdateYawRectState();
                    UpdateYawCurve();
                    WriteConfig();
                }
            }
            e.Handled = true;
        }

        private void YawRect_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            UpdateYawRectState();
            if (sender is Rectangle rect && rect.Tag is int tag && tag > -1)
            {
                yawDraggingRect = rect;
                Point pos = e.GetPosition(canvasYaw);
                yawDragOffset = new Point(pos.X - Canvas.GetLeft(rect), pos.Y - Canvas.GetTop(rect));
                rect.CaptureMouse();
                rect.Effect = new DropShadowEffect { ShadowDepth = 0, BlurRadius = 15, Color = Colors.White, Opacity = 1 };

                textYawPointPos.Visibility = Visibility.Visible;
                textYawPointPos.Text = $"#{tag}\nOrig: {yawJoystickOrig[tag]}%\nMap: {yawJoystickMapped[tag]}%";
            }
        }

        private void YawRect_MouseMove(object sender, MouseEventArgs e)
        {
            if (yawDraggingRect != null && e.LeftButton == MouseButtonState.Pressed)
            {
                int tag = (int)yawDraggingRect.Tag;
                Point pos = e.GetPosition(canvasYaw);
                double newLeft = pos.X - yawDragOffset.X;
                double newTop = pos.Y - yawDragOffset.Y;

                List<Rectangle> taggedRects = canvasYaw.Children.OfType<Rectangle>()
                    .Where(r => r.Tag != null && (int)r.Tag >= 0)
                    .OrderBy(r => Canvas.GetLeft(r))
                    .ToList();

                if (tag == 0)
                {
                    // Point 0: Center Deadzone. Can only move in X between 0 and next point; Y locked at bottom (50% output)
                    double leftBound = -0.5 * RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[1]) - RectSize;
                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = canvasYaw.Height - 0.5 * RectSize;
                }
                else if (tag == yawRectCount - 1)
                {
                    // Point Last: Locked at (100%, 100%)
                    newLeft = canvasYaw.Width - 0.5 * RectSize;
                    newTop = -0.5 * RectSize;
                }
                else
                {
                    // Intermediate points: Free to move within neighbors
                    double leftBound = Canvas.GetLeft(taggedRects[tag - 1]) + RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[tag + 1]) - RectSize;
                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = Math.Max(-0.5 * RectSize, Math.Min(newTop, canvasYaw.Height - 0.5 * RectSize));
                }

                Canvas.SetLeft(yawDraggingRect, newLeft);
                Canvas.SetTop(yawDraggingRect, newTop);

                UpdateYawRectState();
                UpdateYawCurve();

                textYawPointPos.Visibility = Visibility.Visible;
                textYawPointPos.Text = $"#{tag}\nOrig: {yawJoystickOrig[tag]}%\nMap: {yawJoystickMapped[tag]}%";
            }
        }

        private void YawRect_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (yawDraggingRect != null)
            {
                yawDraggingRect.Effect = null;
                yawDraggingRect.ReleaseMouseCapture();
                yawDraggingRect = null;
                WriteConfig();
            }
            textYawPointPos.Visibility = Visibility.Hidden;
        }

        private void UpdateYawRectState()
        {
            List<Rectangle> taggedRects = canvasYaw.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => Canvas.GetLeft(r))
                .ToList();

            yawRectCount = taggedRects.Count;
            yawRectPosX.Clear();
            yawRectPosY.Clear();

            for (int i = 0; i < taggedRects.Count; i++)
            {
                taggedRects[i].Tag = i;
                double cx = Canvas.GetLeft(taggedRects[i]) + 0.5 * RectSize;
                double cy = Canvas.GetTop(taggedRects[i]) + 0.5 * RectSize;
                yawRectPosX.Add(cx);
                yawRectPosY.Add(cy);

                // Yaw mapping domain: 50% to 100%
                yawJoystickOrig[i] = (byte)Math.Round(50.0 + (cx / canvasYaw.Width) * 50.0);
                yawJoystickMapped[i] = (byte)Math.Round(50.0 + ((canvasYaw.Height - cy) / canvasYaw.Height) * 50.0);
            }
        }

        private void UpdateYawCurve()
        {
            if (yawRectCount < 2) return;

            double[] x = new double[yawRectCount];
            double[] y = new double[yawRectCount];
            double[] x_pct = new double[yawRectCount];
            double[] y_pct = new double[yawRectCount];

            for (int i = 0; i < yawRectCount; i++)
            {
                x[i] = yawRectPosX[i] - yawRectPosX[0];
                y[i] = yawRectPosY[i];
                x_pct[i] = (double)(yawJoystickOrig[i] - yawJoystickOrig[0]);
                y_pct[i] = (double)yawJoystickMapped[i];
            }

            int xQuantity = 101;
            (double[] xs2, double[] ys2, _, _) = Cubic.Interpolate1D(x, y, xQuantity);

            PointCollection polyPoints = new PointCollection();
            PointCollection fillPoints = new PointCollection();

            Point ptStart = new Point(0, canvasYaw.Height);
            polyPoints.Add(ptStart);
            fillPoints.Add(ptStart);

            // Deadzone flat line from 0 to Point 0
            if (yawRectPosX[0] > 0)
            {
                Point ptDZ = new Point(yawRectPosX[0], canvasYaw.Height);
                polyPoints.Add(ptDZ);
                fillPoints.Add(ptDZ);
            }

            for (int i = 0; i < xQuantity; i++)
            {
                Point pt = new Point(xs2[i] + yawRectPosX[0], ys2[i]);
                polyPoints.Add(pt);
                fillPoints.Add(pt);
            }

            Point ptEnd = new Point(canvasYaw.Width, 0);
            polyPoints.Add(ptEnd);
            fillPoints.Add(ptEnd);

            Point ptEndBottom = new Point(canvasYaw.Width, canvasYaw.Height);
            fillPoints.Add(ptEndBottom);
            fillPoints.Add(ptStart);

            polylineYawCurve.Points = polyPoints;
            polygonYawBackground.Points = fillPoints;

            // Build lookup table: index 0..1000 corresponds to input 50.0% to 100.0%
            int origRange = (yawJoystickOrig[yawRectCount - 1] - yawJoystickOrig[0]) * 10;
            if (origRange > 0)
            {
                (double[] _, double[] ys3, _, _) = Cubic.Interpolate1D(x_pct, y_pct, origRange + 1);

                int initialPos = (yawJoystickOrig[0] - 50) * 20; // 0..1000
                int endPos = (yawJoystickOrig[yawRectCount - 1] - 50) * 20;

                for (int i = 0; i < Math.Min(initialPos, 1001); i++)
                {
                    yawTable[i] = 50.0;
                }
                for (int i = initialPos; i < Math.Min(endPos, 1001); i++)
                {
                    int idx = (int)((double)(i - initialPos) / (endPos - initialPos) * origRange);
                    idx = Math.Max(0, Math.Min(idx, ys3.Length - 1));
                    yawTable[i] = ys3[idx];
                }
                for (int i = endPos; i <= 1000; i++)
                {
                    yawTable[i] = 100.0;
                }
            }
            else
            {
                for (int i = 0; i <= 1000; i++)
                {
                    yawTable[i] = 50.0 + (i / 1000.0) * 50.0;
                }
            }
        }

        private void CheckExistingYawRect(int targetCount)
        {
            var taggedRects = canvasYaw.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => (int)r.Tag)
                .ToList();
            int currentCount = taggedRects.Count;

            if (targetCount < currentCount)
            {
                for (int i = targetCount; i < currentCount; i++)
                {
                    canvasYaw.Children.Remove(taggedRects[i]);
                }
            }
            else if (targetCount > currentCount)
            {
                for (int i = currentCount; i < targetCount; i++)
                {
                    double xFrac = (double)i / (targetCount - 1);
                    AddYawRectAt(xFrac * canvasYaw.Width - 0.5 * RectSize, canvasYaw.Height - xFrac * canvasYaw.Height - 0.5 * RectSize);
                }
            }

            var remainingRects = canvasYaw.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => Canvas.GetLeft(r))
                .ToList();
            for (int i = 0; i < remainingRects.Count; i++)
            {
                remainingRects[i].Tag = i;
            }
            yawRectCount = remainingRects.Count;
        }

        private void RedrawYawFromArrays()
        {
            CheckExistingYawRect(yawRectCount);
            var taggedRects = canvasYaw.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => (int)r.Tag)
                .ToList();

            for (int i = 0; i < taggedRects.Count; i++)
            {
                double cx = ((double)(yawJoystickOrig[i] - 50) / 50.0) * canvasYaw.Width - 0.5 * RectSize;
                double cy = canvasYaw.Height - ((double)(yawJoystickMapped[i] - 50) / 50.0) * canvasYaw.Height - 0.5 * RectSize;
                Canvas.SetLeft(taggedRects[i], cx);
                Canvas.SetTop(taggedRects[i], cy);
            }
            UpdateYawRectState();
            UpdateYawCurve();
        }

        public void UpdateYawState(double yawNormalized01)
        {
            if (Settings != null && Settings.rudderMode == 3)
            {
                yawNormalized01 = 0.5;
            }
            double clamped = Math.Max(0.0, Math.Min(1.0, yawNormalized01));
            double mappedOutput;
            double canvasX;
            double deflectionPct;

            if (clamped >= 0.5)
            {
                // Right rudder deflection: 50% to 100%
                int tableIdx = (int)Math.Round((clamped - 0.5) * 2000.0);
                tableIdx = Math.Max(0, Math.Min(1000, tableIdx));
                mappedOutput = yawTable[tableIdx];
                canvasX = (clamped - 0.5) * 2.0 * canvasYaw.Width;
                deflectionPct = (mappedOutput - 50.0) / 50.0;
                textStateYaw.Text = (clamped == 0.5) ? "50%" : $"R: {Math.Round(mappedOutput)}%";
            }
            else
            {
                // Symmetrical Left rudder deflection: Mirrored
                double deflection = 0.5 - clamped;
                int tableIdx = (int)Math.Round(deflection * 2000.0);
                tableIdx = Math.Max(0, Math.Min(1000, tableIdx));
                double mappedRight = yawTable[tableIdx];
                mappedOutput = 50.0 - (mappedRight - 50.0);
                canvasX = deflection * 2.0 * canvasYaw.Width;
                deflectionPct = (mappedRight - 50.0) / 50.0;
                textStateYaw.Text = $"L: {Math.Round(mappedOutput)}%";
            }

            deflectionPct = Math.Max(0.0, Math.Min(1.0, deflectionPct));
            canvasX = Math.Max(0.0, Math.Min(canvasYaw.Width, canvasX));
            double canvasY = canvasYaw.Height - deflectionPct * canvasYaw.Height;
            canvasY = Math.Max(0.0, Math.Min(canvasYaw.Height, canvasY));

            Canvas.SetLeft(rectStateYaw, canvasX - rectStateYaw.Width / 2);
            Canvas.SetTop(rectStateYaw, canvasY - rectStateYaw.Height / 2);

            double textX = canvasX - 10;
            if (textX < 2) textX = 2;
            if (textX > canvasYaw.Width - 35) textX = canvasYaw.Width - 35;
            double textY = canvasY - rectStateYaw.Height - 4;
            if (textY < 2) textY = canvasY + rectStateYaw.Height + 2;

            Canvas.SetLeft(textStateYaw, textX);
            Canvas.SetTop(textStateYaw, textY);
        }

        #endregion

        #region Toe Brake Mapping Logic (0% - 100%)

        private void InitToeRectangles()
        {
            for (int i = 0; i < MinControlQuantity; i++)
            {
                double xFrac = (double)i / (MinControlQuantity - 1);
                double px = xFrac * canvasToe.Width - 0.5 * RectSize;
                double py = canvasToe.Height - xFrac * canvasToe.Height - 0.5 * RectSize;
                AddToeRectAt(px, py);
            }
            UpdateToeRectState();
        }

        private void AddToeRectAt(double x, double y)
        {
            Rectangle rect = new Rectangle
            {
                Width = RectSize,
                Height = RectSize,
                StrokeThickness = 2,
                RadiusX = 1,
                RadiusY = 1,
                Fill = Brushes.Transparent,
                Opacity = 1.0,
                Cursor = Cursors.Hand
            };
            rect.Tag = -1;
            rect.SetResourceReference(Shape.StrokeProperty, "AccentColorBrush");
            Canvas.SetLeft(rect, x);
            Canvas.SetTop(rect, y);

            rect.MouseRightButtonDown += ToeRect_MouseRightButtonDown;
            rect.MouseLeftButtonDown += ToeRect_MouseLeftButtonDown;
            rect.MouseLeftButtonUp += ToeRect_MouseLeftButtonUp;
            rect.MouseMove += ToeRect_MouseMove;

            canvasToe.Children.Add(rect);
        }

        private void CanvasToe_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (toeRectCount < MaxControlQuantity)
            {
                Point pos = e.GetPosition(canvasToe);
                if (pos.X > toeRectPosX[0] && pos.X < toeRectPosX[toeRectCount - 1])
                {
                    AddToeRectAt(pos.X - 0.5 * RectSize, pos.Y - 0.5 * RectSize);
                    UpdateToeRectState();
                    UpdateToeCurve();
            UpdateYawState(0.5);
            UpdateToeBrakeState(0.0);
                    WriteConfig();
                }
            }
        }

        private void ToeRect_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (sender is Rectangle rect && rect.Tag is int tag)
            {
                if (toeRectCount > MinControlQuantity && tag > 0 && tag < (toeRectCount - 1))
                {
                    canvasToe.Children.Remove(rect);
                    UpdateToeRectState();
                    UpdateToeCurve();
            UpdateYawState(0.5);
            UpdateToeBrakeState(0.0);
                    WriteConfig();
                }
            }
            e.Handled = true;
        }

        private void ToeRect_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            UpdateToeRectState();
            if (sender is Rectangle rect && rect.Tag is int tag && tag > -1)
            {
                toeDraggingRect = rect;
                Point pos = e.GetPosition(canvasToe);
                toeDragOffset = new Point(pos.X - Canvas.GetLeft(rect), pos.Y - Canvas.GetTop(rect));
                rect.CaptureMouse();
                rect.Effect = new DropShadowEffect { ShadowDepth = 0, BlurRadius = 15, Color = Colors.White, Opacity = 1 };

                textToePointPos.Visibility = Visibility.Visible;
                textToePointPos.Text = $"#{tag}\nOrig: {toeJoystickOrig[tag]}%\nMap: {toeJoystickMapped[tag]}%";
            }
        }

        private void ToeRect_MouseMove(object sender, MouseEventArgs e)
        {
            if (toeDraggingRect != null && e.LeftButton == MouseButtonState.Pressed)
            {
                int tag = (int)toeDraggingRect.Tag;
                Point pos = e.GetPosition(canvasToe);
                double newLeft = pos.X - toeDragOffset.X;
                double newTop = pos.Y - toeDragOffset.Y;

                List<Rectangle> taggedRects = canvasToe.Children.OfType<Rectangle>()
                    .Where(r => r.Tag != null && (int)r.Tag >= 0)
                    .OrderBy(r => Canvas.GetLeft(r))
                    .ToList();

                if (tag == 0)
                {
                    // Point 0: Deadzone at bottom (Y = 0% output)
                    double leftBound = -0.5 * RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[1]) - RectSize;
                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = canvasToe.Height - 0.5 * RectSize;
                }
                else if (tag == toeRectCount - 1)
                {
                    // Point Last: Locked at (100%, 100%)
                    newLeft = canvasToe.Width - 0.5 * RectSize;
                    newTop = -0.5 * RectSize;
                }
                else
                {
                    double leftBound = Canvas.GetLeft(taggedRects[tag - 1]) + RectSize;
                    double rightBound = Canvas.GetLeft(taggedRects[tag + 1]) - RectSize;
                    newLeft = Math.Max(leftBound, Math.Min(newLeft, rightBound));
                    newTop = Math.Max(-0.5 * RectSize, Math.Min(newTop, canvasToe.Height - 0.5 * RectSize));
                }

                Canvas.SetLeft(toeDraggingRect, newLeft);
                Canvas.SetTop(toeDraggingRect, newTop);

                UpdateToeRectState();
                UpdateToeCurve();
            UpdateYawState(0.5);
            UpdateToeBrakeState(0.0);

                textToePointPos.Visibility = Visibility.Visible;
                textToePointPos.Text = $"#{tag}\nOrig: {toeJoystickOrig[tag]}%\nMap: {toeJoystickMapped[tag]}%";
            }
        }

        private void ToeRect_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (toeDraggingRect != null)
            {
                toeDraggingRect.Effect = null;
                toeDraggingRect.ReleaseMouseCapture();
                toeDraggingRect = null;
                WriteConfig();
            }
            textToePointPos.Visibility = Visibility.Hidden;
        }

        private void UpdateToeRectState()
        {
            List<Rectangle> taggedRects = canvasToe.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => Canvas.GetLeft(r))
                .ToList();

            toeRectCount = taggedRects.Count;
            toeRectPosX.Clear();
            toeRectPosY.Clear();

            for (int i = 0; i < taggedRects.Count; i++)
            {
                taggedRects[i].Tag = i;
                double cx = Canvas.GetLeft(taggedRects[i]) + 0.5 * RectSize;
                double cy = Canvas.GetTop(taggedRects[i]) + 0.5 * RectSize;
                toeRectPosX.Add(cx);
                toeRectPosY.Add(cy);

                // Toe mapping domain: 0% to 100%
                toeJoystickOrig[i] = (byte)Math.Round((cx / canvasToe.Width) * 100.0);
                toeJoystickMapped[i] = (byte)Math.Round(((canvasToe.Height - cy) / canvasToe.Height) * 100.0);
            }
        }

        private void UpdateToeCurve()
        {
            if (toeRectCount < 2) return;

            double[] x = new double[toeRectCount];
            double[] y = new double[toeRectCount];
            double[] x_pct = new double[toeRectCount];
            double[] y_pct = new double[toeRectCount];

            for (int i = 0; i < toeRectCount; i++)
            {
                x[i] = toeRectPosX[i] - toeRectPosX[0];
                y[i] = toeRectPosY[i];
                x_pct[i] = (double)(toeJoystickOrig[i] - toeJoystickOrig[0]);
                y_pct[i] = (double)toeJoystickMapped[i];
            }

            int xQuantity = 101;
            (double[] xs2, double[] ys2, _, _) = Cubic.Interpolate1D(x, y, xQuantity);

            PointCollection polyPoints = new PointCollection();
            PointCollection fillPoints = new PointCollection();

            Point ptStart = new Point(0, canvasToe.Height);
            polyPoints.Add(ptStart);
            fillPoints.Add(ptStart);

            if (toeRectPosX[0] > 0)
            {
                Point ptDZ = new Point(toeRectPosX[0], canvasToe.Height);
                polyPoints.Add(ptDZ);
                fillPoints.Add(ptDZ);
            }

            for (int i = 0; i < xQuantity; i++)
            {
                Point pt = new Point(xs2[i] + toeRectPosX[0], ys2[i]);
                polyPoints.Add(pt);
                fillPoints.Add(pt);
            }

            Point ptEnd = new Point(canvasToe.Width, 0);
            polyPoints.Add(ptEnd);
            fillPoints.Add(ptEnd);

            Point ptEndBottom = new Point(canvasToe.Width, canvasToe.Height);
            fillPoints.Add(ptEndBottom);
            fillPoints.Add(ptStart);

            polylineToeCurve.Points = polyPoints;
            polygonToeBackground.Points = fillPoints;

            int origRange = (toeJoystickOrig[toeRectCount - 1] - toeJoystickOrig[0]) * 10;
            if (origRange > 0)
            {
                (double[] _, double[] ys3, _, _) = Cubic.Interpolate1D(x_pct, y_pct, origRange + 1);

                int initialPos = toeJoystickOrig[0] * 10;
                int endPos = toeJoystickOrig[toeRectCount - 1] * 10;

                for (int i = 0; i < Math.Min(initialPos, 1001); i++)
                {
                    toeTable[i] = 0.0;
                }
                for (int i = initialPos; i < Math.Min(endPos, 1001); i++)
                {
                    int idx = (int)((double)(i - initialPos) / (endPos - initialPos) * origRange);
                    idx = Math.Max(0, Math.Min(idx, ys3.Length - 1));
                    toeTable[i] = ys3[idx];
                }
                for (int i = endPos; i <= 1000; i++)
                {
                    toeTable[i] = 100.0;
                }
            }
            else
            {
                for (int i = 0; i <= 1000; i++)
                {
                    toeTable[i] = (i / 1000.0) * 100.0;
                }
            }
        }

        private void CheckExistingToeRect(int targetCount)
        {
            var taggedRects = canvasToe.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => (int)r.Tag)
                .ToList();
            int currentCount = taggedRects.Count;

            if (targetCount < currentCount)
            {
                for (int i = targetCount; i < currentCount; i++)
                {
                    canvasToe.Children.Remove(taggedRects[i]);
                }
            }
            else if (targetCount > currentCount)
            {
                for (int i = currentCount; i < targetCount; i++)
                {
                    double xFrac = (double)i / (targetCount - 1);
                    AddToeRectAt(xFrac * canvasToe.Width - 0.5 * RectSize, canvasToe.Height - xFrac * canvasToe.Height - 0.5 * RectSize);
                }
            }

            var remainingRects = canvasToe.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => Canvas.GetLeft(r))
                .ToList();
            for (int i = 0; i < remainingRects.Count; i++)
            {
                remainingRects[i].Tag = i;
            }
            toeRectCount = remainingRects.Count;
        }

        private void RedrawToeFromArrays()
        {
            CheckExistingToeRect(toeRectCount);
            var taggedRects = canvasToe.Children.OfType<Rectangle>()
                .Where(r => r.Tag != null)
                .OrderBy(r => (int)r.Tag)
                .ToList();

            for (int i = 0; i < taggedRects.Count; i++)
            {
                double cx = ((double)toeJoystickOrig[i] / 100.0) * canvasToe.Width - 0.5 * RectSize;
                double cy = canvasToe.Height - ((double)toeJoystickMapped[i] / 100.0) * canvasToe.Height - 0.5 * RectSize;
                Canvas.SetLeft(taggedRects[i], cx);
                Canvas.SetTop(taggedRects[i], cy);
            }
            UpdateToeRectState();
            UpdateToeCurve();
        }

        public void JoystickStateUpdate(ushort val)
        {
            UpdateYawState((double)val / 65535.0);
        }

        public void UpdateToeBrakeState(double toeBrakeNormalized01)
        {
            if (Settings != null && (Settings.rudderMode == 0 || Settings.rudderMode == 1))
            {
                toeBrakeNormalized01 = 0.0;
            }
            double clamped = Math.Max(0.0, Math.Min(1.0, toeBrakeNormalized01));
            int tableIdx = (int)Math.Round(clamped * 1000.0);
            tableIdx = Math.Max(0, Math.Min(1000, tableIdx));

            double mappedOutput = toeTable[tableIdx];
            double canvasX = clamped * canvasToe.Width;
            double canvasY = canvasToe.Height - (mappedOutput / 100.0) * canvasToe.Height;

            canvasX = Math.Max(0.0, Math.Min(canvasToe.Width, canvasX));
            canvasY = Math.Max(0.0, Math.Min(canvasToe.Height, canvasY));

            Canvas.SetLeft(rectStateToe, canvasX - rectStateToe.Width / 2);
            Canvas.SetTop(rectStateToe, canvasY - rectStateToe.Height / 2);

            double textX = canvasX - 10;
            if (textX < 2) textX = 2;
            if (textX > canvasToe.Width - 35) textX = canvasToe.Width - 35;
            double textY = canvasY - rectStateToe.Height - 4;
            if (textY < 2) textY = canvasY + rectStateToe.Height + 2;

            Canvas.SetLeft(textStateToe, textX);
            Canvas.SetTop(textStateToe, textY);

            textStateToe.Text = $"{Math.Round(mappedOutput)}%";
        }

        #endregion

        #region Presets (Linear, S-Curve, Exponential, Logarithmic)

        private double SmoothStep(double x) => x * x * (3 - 2 * x);
        private double Clamp(double val, double min, double max) => val < min ? min : (val > max ? max : val);
        private double ExponentialTo100(double x, double a = 5.0)
        {
            x = Clamp(x, 0.0, 1.0);
            return (Math.Pow(a, x) - 1.0) / (a - 1.0);
        }
        private double LogarithmicTo100(double x, double a = 7.0)
        {
            x = Clamp(x, 0.0, 1.0);
            double scaled = x * (a - 1) + 1;
            return Math.Log(scaled) / Math.Log(a);
        }

        // Yaw Presets (50% - 100%)
        private void btn_yaw_linear_Click(object sender, RoutedEventArgs e)
        {
            yawRectCount = 6;
            for (int i = 0; i < yawRectCount; i++)
            {
                double t = (double)i / (yawRectCount - 1);
                yawJoystickOrig[i] = (byte)Math.Round(50.0 + t * 50.0);
                yawJoystickMapped[i] = (byte)Math.Round(50.0 + t * 50.0);
            }
            for (int i = yawRectCount; i < MaxControlQuantity; i++)
            {
                yawJoystickOrig[i] = 0;
                yawJoystickMapped[i] = 0;
            }
            RedrawYawFromArrays();
            WriteConfig();
        }

        private void btn_yaw_scurve_Click(object sender, RoutedEventArgs e)
        {
            yawRectCount = 6;
            for (int i = 0; i < yawRectCount; i++)
            {
                double t = (double)i / (yawRectCount - 1);
                yawJoystickOrig[i] = (byte)Math.Round(50.0 + t * 50.0);
                yawJoystickMapped[i] = (byte)Math.Round(50.0 + SmoothStep(t) * 50.0);
            }
            for (int i = yawRectCount; i < MaxControlQuantity; i++)
            {
                yawJoystickOrig[i] = 0;
                yawJoystickMapped[i] = 0;
            }
            RedrawYawFromArrays();
            WriteConfig();
        }

        private void btn_yaw_expo_Click(object sender, RoutedEventArgs e)
        {
            yawRectCount = 6;
            for (int i = 0; i < yawRectCount; i++)
            {
                double t = (double)i / (yawRectCount - 1);
                yawJoystickOrig[i] = (byte)Math.Round(50.0 + t * 50.0);
                yawJoystickMapped[i] = (byte)Math.Round(50.0 + ExponentialTo100(t) * 50.0);
            }
            for (int i = yawRectCount; i < MaxControlQuantity; i++)
            {
                yawJoystickOrig[i] = 0;
                yawJoystickMapped[i] = 0;
            }
            RedrawYawFromArrays();
            WriteConfig();
        }

        private void btn_yaw_log_Click(object sender, RoutedEventArgs e)
        {
            yawRectCount = 6;
            for (int i = 0; i < yawRectCount; i++)
            {
                double t = (double)i / (yawRectCount - 1);
                yawJoystickOrig[i] = (byte)Math.Round(50.0 + t * 50.0);
                yawJoystickMapped[i] = (byte)Math.Round(50.0 + LogarithmicTo100(t) * 50.0);
            }
            for (int i = yawRectCount; i < MaxControlQuantity; i++)
            {
                yawJoystickOrig[i] = 0;
                yawJoystickMapped[i] = 0;
            }
            RedrawYawFromArrays();
            WriteConfig();
        }

        // Toe Presets (0% - 100%)
        private void btn_toe_linear_Click(object sender, RoutedEventArgs e)
        {
            toeRectCount = 6;
            for (int i = 0; i < toeRectCount; i++)
            {
                double t = (double)i / (toeRectCount - 1);
                toeJoystickOrig[i] = (byte)Math.Round(t * 100.0);
                toeJoystickMapped[i] = (byte)Math.Round(t * 100.0);
            }
            for (int i = toeRectCount; i < MaxControlQuantity; i++)
            {
                toeJoystickOrig[i] = 0;
                toeJoystickMapped[i] = 0;
            }
            RedrawToeFromArrays();
            WriteConfig();
        }

        private void btn_toe_scurve_Click(object sender, RoutedEventArgs e)
        {
            toeRectCount = 6;
            for (int i = 0; i < toeRectCount; i++)
            {
                double t = (double)i / (toeRectCount - 1);
                toeJoystickOrig[i] = (byte)Math.Round(t * 100.0);
                toeJoystickMapped[i] = (byte)Math.Round(SmoothStep(t) * 100.0);
            }
            for (int i = toeRectCount; i < MaxControlQuantity; i++)
            {
                toeJoystickOrig[i] = 0;
                toeJoystickMapped[i] = 0;
            }
            RedrawToeFromArrays();
            WriteConfig();
        }

        private void btn_toe_expo_Click(object sender, RoutedEventArgs e)
        {
            toeRectCount = 6;
            for (int i = 0; i < toeRectCount; i++)
            {
                double t = (double)i / (toeRectCount - 1);
                toeJoystickOrig[i] = (byte)Math.Round(t * 100.0);
                toeJoystickMapped[i] = (byte)Math.Round(ExponentialTo100(t) * 100.0);
            }
            for (int i = toeRectCount; i < MaxControlQuantity; i++)
            {
                toeJoystickOrig[i] = 0;
                toeJoystickMapped[i] = 0;
            }
            RedrawToeFromArrays();
            WriteConfig();
        }

        private void btn_toe_log_Click(object sender, RoutedEventArgs e)
        {
            toeRectCount = 6;
            for (int i = 0; i < toeRectCount; i++)
            {
                double t = (double)i / (toeRectCount - 1);
                toeJoystickOrig[i] = (byte)Math.Round(t * 100.0);
                toeJoystickMapped[i] = (byte)Math.Round(LogarithmicTo100(t) * 100.0);
            }
            for (int i = toeRectCount; i < MaxControlQuantity; i++)
            {
                toeJoystickOrig[i] = 0;
                toeJoystickMapped[i] = 0;
            }
            RedrawToeFromArrays();
            WriteConfig();
        }

        #endregion

        #region Settings & Config Serialization

        public void updateUI()
        {
            if (Settings == null) return;

            // Load Yaw Curve
            if (Settings.rudderYawJoystickMapOrig != null && Settings.rudderYawJoystickMapOrig.Length == 11 && Settings.rudderYawNumOfJoystickMapControl >= MinControlQuantity)
            {
                yawRectCount = Settings.rudderYawNumOfJoystickMapControl;
                for (int i = 0; i < 11; i++)
                {
                    yawJoystickOrig[i] = Settings.rudderYawJoystickMapOrig[i];
                    yawJoystickMapped[i] = Settings.rudderYawJoystickMapMapped[i];
                }
                RedrawYawFromArrays();
            }
            else if (Settings.rudderJoystickMapOrig != null && Settings.rudderJoystickMapOrig.Length == 11 && Settings.rudderJoystickMapOrig[0] >= 40)
            {
                // Fallback migration from older rudderJoystickMapOrig if it was 50-100%
                yawRectCount = Settings.rudderNumOfJoystickMapControl;
                for (int i = 0; i < 11; i++)
                {
                    yawJoystickOrig[i] = Settings.rudderJoystickMapOrig[i];
                    yawJoystickMapped[i] = Settings.rudderJoystickMapMapped[i];
                }
                RedrawYawFromArrays();
            }

            // Load Toe Curve
            if (Settings.rudderToeJoystickMapOrig != null && Settings.rudderToeJoystickMapOrig.Length == 11 && Settings.rudderToeNumOfJoystickMapControl >= MinControlQuantity)
            {
                toeRectCount = Settings.rudderToeNumOfJoystickMapControl;
                for (int i = 0; i < 11; i++)
                {
                    toeJoystickOrig[i] = Settings.rudderToeJoystickMapOrig[i];
                    toeJoystickMapped[i] = Settings.rudderToeJoystickMapMapped[i];
                }
                RedrawToeFromArrays();
            }
        }

        private void updateRectControlFromConfig()
        {
            // Sync with dap_config_st if needed
        }

        private void WriteConfig()
        {
            if (Settings != null)
            {
                // Save Yaw to Settings
                Settings.rudderYawNumOfJoystickMapControl = (byte)yawRectCount;
                if (Settings.rudderYawJoystickMapOrig == null || Settings.rudderYawJoystickMapOrig.Length != 11)
                    Settings.rudderYawJoystickMapOrig = new byte[11];
                if (Settings.rudderYawJoystickMapMapped == null || Settings.rudderYawJoystickMapMapped.Length != 11)
                    Settings.rudderYawJoystickMapMapped = new byte[11];

                for (int i = 0; i < 11; i++)
                {
                    Settings.rudderYawJoystickMapOrig[i] = yawJoystickOrig[i];
                    Settings.rudderYawJoystickMapMapped[i] = yawJoystickMapped[i];
                }

                // Save Toe to Settings
                Settings.rudderToeNumOfJoystickMapControl = (byte)toeRectCount;
                if (Settings.rudderToeJoystickMapOrig == null || Settings.rudderToeJoystickMapOrig.Length != 11)
                    Settings.rudderToeJoystickMapOrig = new byte[11];
                if (Settings.rudderToeJoystickMapMapped == null || Settings.rudderToeJoystickMapMapped.Length != 11)
                    Settings.rudderToeJoystickMapMapped = new byte[11];

                for (int i = 0; i < 11; i++)
                {
                    Settings.rudderToeJoystickMapOrig[i] = toeJoystickOrig[i];
                    Settings.rudderToeJoystickMapMapped[i] = toeJoystickMapped[i];
                }

                SettingsChanged?.Invoke(this, Settings);
            }

            // Send BOTH curves unconditionally - the device now has separate
            // slots for yaw/regular (joystickMapOrig/Mapped) and toe-brake
            // (joystickMapOrigToe/MappedToe), and picks which one is live
            // based on the Toe Brake toggle, not on which one the plugin last
            // happened to send. Previously this only sent one curve based on
            // rudderMode == 3, so editing the toe curve in "Airplane with Toe
            // Brake" mode (mode 2, which also uses toe braking) silently
            // never reached the device.
            var cfg = dap_config_st;

            cfg.payloadPedalConfig_.numOfJoystickMapControl = (byte)yawRectCount;
            cfg.payloadPedalConfig_.joystickMapOrig00 = yawJoystickOrig[0];
            cfg.payloadPedalConfig_.joystickMapOrig01 = yawJoystickOrig[1];
            cfg.payloadPedalConfig_.joystickMapOrig02 = yawJoystickOrig[2];
            cfg.payloadPedalConfig_.joystickMapOrig03 = yawJoystickOrig[3];
            cfg.payloadPedalConfig_.joystickMapOrig04 = yawJoystickOrig[4];
            cfg.payloadPedalConfig_.joystickMapOrig05 = yawJoystickOrig[5];
            cfg.payloadPedalConfig_.joystickMapOrig06 = yawJoystickOrig[6];
            cfg.payloadPedalConfig_.joystickMapOrig07 = yawJoystickOrig[7];
            cfg.payloadPedalConfig_.joystickMapOrig08 = yawJoystickOrig[8];
            cfg.payloadPedalConfig_.joystickMapOrig09 = yawJoystickOrig[9];
            cfg.payloadPedalConfig_.joystickMapOrig10 = yawJoystickOrig[10];

            cfg.payloadPedalConfig_.joystickMapMapped00 = yawJoystickMapped[0];
            cfg.payloadPedalConfig_.joystickMapMapped01 = yawJoystickMapped[1];
            cfg.payloadPedalConfig_.joystickMapMapped02 = yawJoystickMapped[2];
            cfg.payloadPedalConfig_.joystickMapMapped03 = yawJoystickMapped[3];
            cfg.payloadPedalConfig_.joystickMapMapped04 = yawJoystickMapped[4];
            cfg.payloadPedalConfig_.joystickMapMapped05 = yawJoystickMapped[5];
            cfg.payloadPedalConfig_.joystickMapMapped06 = yawJoystickMapped[6];
            cfg.payloadPedalConfig_.joystickMapMapped07 = yawJoystickMapped[7];
            cfg.payloadPedalConfig_.joystickMapMapped08 = yawJoystickMapped[8];
            cfg.payloadPedalConfig_.joystickMapMapped09 = yawJoystickMapped[9];
            cfg.payloadPedalConfig_.joystickMapMapped10 = yawJoystickMapped[10];

            cfg.payloadPedalConfig_.numOfJoystickMapControlToe = (byte)toeRectCount;
            cfg.payloadPedalConfig_.joystickMapOrigToe00 = toeJoystickOrig[0];
            cfg.payloadPedalConfig_.joystickMapOrigToe01 = toeJoystickOrig[1];
            cfg.payloadPedalConfig_.joystickMapOrigToe02 = toeJoystickOrig[2];
            cfg.payloadPedalConfig_.joystickMapOrigToe03 = toeJoystickOrig[3];
            cfg.payloadPedalConfig_.joystickMapOrigToe04 = toeJoystickOrig[4];
            cfg.payloadPedalConfig_.joystickMapOrigToe05 = toeJoystickOrig[5];
            cfg.payloadPedalConfig_.joystickMapOrigToe06 = toeJoystickOrig[6];
            cfg.payloadPedalConfig_.joystickMapOrigToe07 = toeJoystickOrig[7];
            cfg.payloadPedalConfig_.joystickMapOrigToe08 = toeJoystickOrig[8];
            cfg.payloadPedalConfig_.joystickMapOrigToe09 = toeJoystickOrig[9];
            cfg.payloadPedalConfig_.joystickMapOrigToe10 = toeJoystickOrig[10];

            cfg.payloadPedalConfig_.joystickMapMappedToe00 = toeJoystickMapped[0];
            cfg.payloadPedalConfig_.joystickMapMappedToe01 = toeJoystickMapped[1];
            cfg.payloadPedalConfig_.joystickMapMappedToe02 = toeJoystickMapped[2];
            cfg.payloadPedalConfig_.joystickMapMappedToe03 = toeJoystickMapped[3];
            cfg.payloadPedalConfig_.joystickMapMappedToe04 = toeJoystickMapped[4];
            cfg.payloadPedalConfig_.joystickMapMappedToe05 = toeJoystickMapped[5];
            cfg.payloadPedalConfig_.joystickMapMappedToe06 = toeJoystickMapped[6];
            cfg.payloadPedalConfig_.joystickMapMappedToe07 = toeJoystickMapped[7];
            cfg.payloadPedalConfig_.joystickMapMappedToe08 = toeJoystickMapped[8];
            cfg.payloadPedalConfig_.joystickMapMappedToe09 = toeJoystickMapped[9];
            cfg.payloadPedalConfig_.joystickMapMappedToe10 = toeJoystickMapped[10];

            dap_config_st = cfg;
            ConfigChanged?.Invoke(this, dap_config_st);
        }

        #endregion
    }
}

#pragma warning restore CS0067
