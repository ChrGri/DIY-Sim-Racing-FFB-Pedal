using FMOD;
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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// CurveTab_PedalForceTravel.xaml 的互動邏輯
    /// </summary>
    public partial class CurveTab_PedalForceTravel_Modified : UserControl
    {
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
        DateTime forceUpdate_currentTime = DateTime.Now;
        DateTime forceUpdate_lastTime = DateTime.Now;
        public CurveTab_PedalForceTravel_Modified()
        {
            InitializeComponent();
            DrawGridLines();
            InitRectangles();
            force=new byte[maxControlQuantity];
            travel=new byte[maxControlQuantity];
            Update_BrakeForceCurve();
            
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_PedalForceTravel_Modified),
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
            typeof(CurveTab_PedalForceTravel_Modified),
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
            typeof(CurveTab_PedalForceTravel_Modified),
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
                /*
                if (Settings != null)
                {
                    if (Settings.table_selected != 1)
                    {
                        if(Rangeslider_force_range!=null) Rangeslider_force_range.Maximum = 50;
                    }
                    else
                    {
                        if (Rangeslider_force_range != null) Rangeslider_force_range.Maximum = 200;
                    }
                }*/
            }
            catch
            {
            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_PedalForceTravel_Modified;
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
            var control = d as CurveTab_PedalForceTravel_Modified;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {

                    if (control.mainCanvas != null) control.CanvasDraw();
                    if (control.mainCanvas != null) control.updateRectControlFromConfig();
                    if (control.Label_min_force != null) control.Label_min_force.Content = "Preload:\n" + Math.Round((float)control.dap_config_st.payloadPedalConfig_.preloadForce,1) + "kg";
                    if (control.Label_max_force != null) control.Label_max_force.Content = "Max force:\n" + Math.Round((float)control.dap_config_st.payloadPedalConfig_.maxForce,1) + "kg";
                    if (control.Label_max_pos != null) control.Label_max_pos.Content = "MAX\n" + control.dap_config_st.payloadPedalConfig_.pedalEndPosition + "%\n" + Math.Round((float)(control.dap_config_st.payloadPedalConfig_.lengthPedal_travel * control.dap_config_st.payloadPedalConfig_.pedalEndPosition) / 100) + "mm";
                    if (control.Label_min_pos != null) control.Label_min_pos.Content = "MIN\n" + control.dap_config_st.payloadPedalConfig_.pedalStartPosition + "%\n" + Math.Round((float)(control.dap_config_st.payloadPedalConfig_.lengthPedal_travel * control.dap_config_st.payloadPedalConfig_.pedalStartPosition) / 100) + "mm";
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
                    if (control.Rangeslider_travel_range != null) control.Rangeslider_travel_range.LowerValue = control.dap_config_st.payloadPedalConfig_.pedalStartPosition;
                    if (control.Rangeslider_travel_range != null) control.Rangeslider_travel_range.UpperValue = control.dap_config_st.payloadPedalConfig_.pedalEndPosition;
                    if (control.Rangeslider_force_range != null) control.Rangeslider_force_range.UpperValue = control.dap_config_st.payloadPedalConfig_.maxForce;
                    if (control.Rangeslider_force_range != null) control.Rangeslider_force_range.LowerValue = control.dap_config_st.payloadPedalConfig_.preloadForce;
                    control.PedalServoForceCheck();
                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as CurveTab_PedalForceTravel_Modified;
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
            double dyy = mainCanvas.Height / control_rect_value_max;
            //Canvas.SetTop(rect0, mainCanvas.Height - dyy * dap_config_st.payloadPedalConfig_.relativeForce_p000 - rect0.Height / 2);
            //Canvas.SetLeft(rect0, 0 * mainCanvas.Width / 5 - rect0.Width / 2);
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
            Canvas.SetTop(rect_State, mainCanvas.Height - rect_State.Height / 2);
            Canvas.SetLeft(rect_State, -rect_State.Width / 2);
            Canvas.SetLeft(text_state, Canvas.GetLeft(rect_State) /*+ rect_State.Width*/);
            Canvas.SetTop(text_state, Canvas.GetTop(rect_State) - rect_State.Height);
            text_state.Text = "0%";
            //set for ABS slider
            Canvas.SetTop(rect_SABS_Control, (control_rect_value_max - dap_config_st.payloadPedalConfig_.Simulate_ABS_value) * dyy - rect_SABS_Control.Height / 2);
            Canvas.SetLeft(rect_SABS_Control, 0);
            Canvas.SetTop(rect_SABS, 0);
            Canvas.SetLeft(rect_SABS, 0);
            rect_SABS.Height = mainCanvas.Height - dap_config_st.payloadPedalConfig_.Simulate_ABS_value * dyy;
            Canvas.SetTop(text_SABS, Canvas.GetTop(rect_SABS_Control) - text_SABS.Height - rect_SABS_Control.Height);
            Canvas.SetLeft(text_SABS, mainCanvas.Width - text_SABS.Width);
            text_SABS.Text = "ABS trigger value: " + dap_config_st.payloadPedalConfig_.Simulate_ABS_value + "%";

            //set for travel slider;
            double dx = 0;
            //Bite point control
            double BP_max = 100;
            dx = (double)mainCanvas.Width / BP_max;
            text_BP.Text = "Bite Point:\n" + ((float)dap_config_st.payloadPedalConfig_.BP_trigger_value) + "%";
            Canvas.SetLeft(rect_BP_Control, dap_config_st.payloadPedalConfig_.BP_trigger_value * dx - rect_BP_Control.Width / 2);
            Canvas.SetLeft(text_BP, Canvas.GetLeft(rect_BP_Control) + rect_BP_Control.Width + 3);
            Canvas.SetTop(text_BP, mainCanvas.Height - text_BP.Height - 15);
            Update_BrakeForceCurve();
        }

        private void btn_linearcurve_Click(object sender, RoutedEventArgs e)
        {
            
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
                travel[i]= 0;
            }
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
        }

        private void btn_Scurve_Click(object sender, RoutedEventArgs e)
        {
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
            updateRectFromInternalSetting((byte)6);
            writeForceAndTravelToConfig();
        }

        private void btn_logcurve_Click(object sender, RoutedEventArgs e)
        {
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

        private void btn_10xcurve_Click(object sender, RoutedEventArgs e)
        {
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

        private void rect_SABS_Control_MouseMove(object sender, MouseEventArgs e)
        {
            if (calculation.isDragging)
            {
                var rectangle = sender as Rectangle;
                //double x = e.GetPosition(canvas).X - offset.X;
                double y = e.GetPosition(mainCanvas).Y - calculation.offset.Y;

                // Ensure the rectangle stays within the canvas
                double dy = mainCanvas.Height / 100;
                double min_posiiton = 5 * dy;
                double max_position = 50 * dy;
                //min position: 50%, max 95%
                //double dx = 100 / (canvas_horz_slider.Width - 10);
                y = Math.Max(min_posiiton, Math.Min(y, max_position));
                //Canvas.SetTop(rect_SABS, y);
                rect_SABS.Height = y;
                double actual_y = (mainCanvas.Height - y) / dy;
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
                    //rectangle.Fill = calculation.defaultcolor;
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
                    double x = e.GetPosition(mainCanvas).X - calculation.offset.X;
                    double BP_max = 100;
                    double dx = (mainCanvas.Width) / BP_max;
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
                    Canvas.SetTop(text_BP, mainCanvas.Height - text_BP.Height - 15);

                    
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
                double y = e.GetPosition(mainCanvas).Y - calculation.offset.Y;

                // Ensure the rectangle stays within the canvas
                //x = Math.Max(0, Math.Min(x, canvas.ActualWidth - rectangle.ActualWidth));
                y = Math.Max(-1 * rectangle.Height / 2, Math.Min(y, mainCanvas.Height - rectangle.Height / 2));

                //Canvas.SetLeft(rectangle, x);
                Canvas.SetTop(rectangle, y);
                double y_max = 100;
                double dx = mainCanvas.Height / y_max;
                double y_actual = (mainCanvas.Height - y - rectangle.Height / 2) / dx;
                var tmp = dap_config_st;
                /*
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
                */
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
            if(Label_min_force!=null) Label_min_force.Content = "Preload:\n" + Math.Round((float)dap_config_st.payloadPedalConfig_.preloadForce,1) + "kg";
            ConfigChangedEvent(dap_config_st);
            
        }

        private void Rangeslider_force_range_UpperValueChanged(object sender, MahApps.Metro.Controls.RangeParameterChangedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.maxForce = (float)e.NewValue;
            dap_config_st = tmp;
            if(Label_max_force!=null) Label_max_force.Content = "Max force:\n" + Math.Round((float)dap_config_st.payloadPedalConfig_.maxForce, 1) + "kg";
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

            double[] x = new double[rectCount];
            double[] y = new double[rectCount];
            double x_quantity = 101;
            double y_max = 101;
            double dx = mainCanvas.Width / (x_quantity-1);
            double dy = mainCanvas.Height / (y_max-1);
            //draw pedal force-travel curve
            for (int i = 0; i < rectCount; i++)
            {
                x[i]=rectPositionX[i];
                travel[i] = (byte)(((rectPositionX[i]) / mainCanvas.Width )* 100);
                y[i]=rectPositionY[i];
                force[i] = (byte)(((mainCanvas.Height- rectPositionY[i] ) / mainCanvas.Height) * 100);
            }
            /*
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
            */

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, (int)x_quantity);



            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();
            System.Windows.Media.PointCollection myPointCollection3 = new System.Windows.Media.PointCollection();

            for (int pointIdx = 0; pointIdx < (int)x_quantity; pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(xs2[pointIdx], ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);
                myPointCollection3.Add(Pointlcl);
                calculation.Force_curve_Y[pointIdx] = ys2[pointIdx];
            }
            this.Polyline_BrakeForceCurve.Points = myPointCollection2;
            
            System.Windows.Point Pointend1 = new System.Windows.Point(mainCanvas.Width, mainCanvas.Height);
            System.Windows.Point Pointend2 = new System.Windows.Point(0, mainCanvas.Height);
            myPointCollection3.Add(Pointend1);
            myPointCollection3.Add(Pointend2);
            polygonCurveBackground.Points= myPointCollection3;



        }

        public void updatePedalState(ushort pedalPosition_u16, ushort pedalForce_u16)
        {
            text_point_pos.Visibility = Visibility.Hidden;
            double control_rect_value_max = 65535;
            double dyy = mainCanvas.Height / control_rect_value_max;
            double dxx = mainCanvas.Width / control_rect_value_max;

            if (Settings.advanced_b)
            {
                Canvas.SetLeft(rect_State, dxx * pedalPosition_u16 - rect_State.Width / 2);
                Canvas.SetTop(rect_State, mainCanvas.Height - dyy * pedalForce_u16 - rect_State.Height / 2);

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
                Canvas.SetTop(rect_State, calculation.Force_curve_Y[round_x] - rect_State.Height / 2);
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
            double servoMaxForceCorrectionFactor_d = 1.6;
            Servo_max_force *= servoMaxForceCorrectionFactor_d; // We empirically identified that the max pedal force typically is 1.6 times the value given by the formula above.

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

        private void InitRectangles()
        {
            for (int i = 0; i < minControlQuantity; i++)
            {
                AddRectAt(i * 80-0.5*RectSize, mainCanvas.Height-i*40-0.5*RectSize);
            }
            UpdateRectState();
        }

        
        private void AddRectAt(double x, double y)
        {
            /*
            Border border = new Border
            {
                Width = RectSize,
                Height = RectSize,
                BorderThickness = new Thickness(2),
                Background = System.Windows.Media.Brushes.Transparent,
                CornerRadius = new CornerRadius(2),
                Opacity = 1.0
            };
            border.SetResourceReference(Border.BorderBrushProperty, "AccentColorBrush");
            border.Tag = -1;
            Canvas.SetLeft(border, x);
            Canvas.SetTop(border, y);
            mainCanvas.Children.Add(border);
            */
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
                Update_BrakeForceCurve();
                writeForceAndTravelToConfig();

            }
            e.Handled = true;
            
        }

        private void Canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (rectCount < (maxControlQuantity ))
            {
                Point pos = e.GetPosition(mainCanvas);
                AddRectAt(pos.X, pos.Y);
                UpdateRectState();
                Update_BrakeForceCurve();
                writeForceAndTravelToConfig();
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
                text_point_pos.Text += "\nTravel:" + travel[(int)rect.Tag] + "%";
                text_point_pos.Text += "\nForce: " + force[(int)rect.Tag] + "%";


            }
        }

        private void Rect_MouseMove(object sender, MouseEventArgs e)
        {
            if (draggingRect != null && e.LeftButton == MouseButtonState.Pressed)
            {
                int myTag = (int)draggingRect.Tag;

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
                UpdateRectState();
                Update_BrakeForceCurve();
                text_point_pos.Visibility = Visibility.Visible;
                text_point_pos.Text = "#"+draggingRect.Tag.ToString();
                text_point_pos.Text += "\nTravel:" + travel[(int)draggingRect.Tag ] + "%";
                text_point_pos.Text += "\nForce: " + force[(int)draggingRect.Tag ] + "%";
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
            force[0] = dap_config_st.payloadPedalConfig_.relativeForce00;
            force[1] = dap_config_st.payloadPedalConfig_.relativeForce01;
            force[2] = dap_config_st.payloadPedalConfig_.relativeForce02;
            force[3] = dap_config_st.payloadPedalConfig_.relativeForce03;
            force[4] = dap_config_st.payloadPedalConfig_.relativeForce04;
            force[5] = dap_config_st.payloadPedalConfig_.relativeForce05;
            force[6] = dap_config_st.payloadPedalConfig_.relativeForce06;
            force[7] = dap_config_st.payloadPedalConfig_.relativeForce07;
            force[8] = dap_config_st.payloadPedalConfig_.relativeForce08;
            force[9] = dap_config_st.payloadPedalConfig_.relativeForce09;
            force[10] = dap_config_st.payloadPedalConfig_.relativeForce10;

            travel[0] = dap_config_st.payloadPedalConfig_.relativeTravel00;
            travel[1] = dap_config_st.payloadPedalConfig_.relativeTravel01;
            travel[2] = dap_config_st.payloadPedalConfig_.relativeTravel02;
            travel[3] = dap_config_st.payloadPedalConfig_.relativeTravel03;
            travel[4] = dap_config_st.payloadPedalConfig_.relativeTravel04;
            travel[5] = dap_config_st.payloadPedalConfig_.relativeTravel05;
            travel[6] = dap_config_st.payloadPedalConfig_.relativeTravel06;
            travel[7] = dap_config_st.payloadPedalConfig_.relativeTravel07;
            travel[8] = dap_config_st.payloadPedalConfig_.relativeTravel08;
            travel[9] = dap_config_st.payloadPedalConfig_.relativeTravel09;
            travel[10] = dap_config_st.payloadPedalConfig_.relativeTravel10;
        }
        private void writeForceAndTravelToConfig()
        {
            UpdateRectState();
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.relativeForce00 = force[0];
            tmp.payloadPedalConfig_.relativeForce01 = force[1];
            tmp.payloadPedalConfig_.relativeForce02 = force[2];
            tmp.payloadPedalConfig_.relativeForce03 = force[3];
            tmp.payloadPedalConfig_.relativeForce04 = force[4];
            tmp.payloadPedalConfig_.relativeForce05 = force[5];
            tmp.payloadPedalConfig_.relativeForce06 = force[6];
            tmp.payloadPedalConfig_.relativeForce07 = force[7];
            tmp.payloadPedalConfig_.relativeForce08 = force[8];
            tmp.payloadPedalConfig_.relativeForce09 = force[9];
            tmp.payloadPedalConfig_.relativeForce10 = force[10];

            tmp.payloadPedalConfig_.relativeTravel00 = travel[0];
            tmp.payloadPedalConfig_.relativeTravel01 = travel[1];
            tmp.payloadPedalConfig_.relativeTravel02 = travel[2];
            tmp.payloadPedalConfig_.relativeTravel03 = travel[3];
            tmp.payloadPedalConfig_.relativeTravel04 = travel[4];
            tmp.payloadPedalConfig_.relativeTravel05 = travel[5];
            tmp.payloadPedalConfig_.relativeTravel06 = travel[6];
            tmp.payloadPedalConfig_.relativeTravel07 = travel[7];
            tmp.payloadPedalConfig_.relativeTravel08 = travel[8];
            tmp.payloadPedalConfig_.relativeTravel09 = travel[9];
            tmp.payloadPedalConfig_.relativeTravel10 = travel[10];
            
            tmp.payloadPedalConfig_.quantityOfControl = (byte)rectCount;
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
            if (minControlQuantity > dap_config_st.payloadPedalConfig_.quantityOfControl) return;
            checkExistingRect(dap_config_st.payloadPedalConfig_.quantityOfControl);
            readForceAndTravelFromConfig();
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
                Canvas.SetLeft(toDraw[i], (double)travel[i]*dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], mainCanvas.Height - (double)force[i]*dy - 0.5 * RectSize);
                
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
                Canvas.SetLeft(toDraw[i], (double)travel[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], mainCanvas.Height - (double)force[i] * dy - 0.5 * RectSize);

            }
            UpdateRectState();
            Update_BrakeForceCurve();
        }
    }
}
