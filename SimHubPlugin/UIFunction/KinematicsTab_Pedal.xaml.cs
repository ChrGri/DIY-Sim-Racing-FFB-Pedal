using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
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
using WoteverLocalization;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// KinematicsTab_Pedal.xaml 的互動邏輯
    /// </summary>
    public partial class KinematicsTab_Pedal : UserControl
    {
        private int gridline_kinematic_count_original = 0;
        public KinematicsTab_Pedal()
        {
            InitializeComponent();
            if (Settings != null)
            {
                DrawGridLines_kinematicCanvas(Settings.kinematicDiagram_zeroPos_OX, Settings.kinematicDiagram_zeroPos_OY, Settings.kinematicDiagram_zeroPos_scale);
            }
            

        }

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(KinematicsTab_Pedal),
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
            typeof(KinematicsTab_Pedal),
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
            typeof(KinematicsTab_Pedal),
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
                    DrawGridLines_kinematicCanvas(Settings.kinematicDiagram_zeroPos_OX, Settings.kinematicDiagram_zeroPos_OY, Settings.kinematicDiagram_zeroPos_scale);
                }
            }
            catch
            {
            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as KinematicsTab_Pedal;
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
            var control = d as KinematicsTab_Pedal;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                try
                {
                    control.CanvasDraw();

                }
                catch
                {
                }

            }
        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as KinematicsTab_Pedal;
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
        public void updatePedalState(ushort pedalPosition_u16)
        {
            // Only update when the control is visible and its tab is selected to save resources
            if (!this.IsVisible) return;

            var parentTab = this.Parent as TabItem;
            if (parentTab != null && !parentTab.IsSelected) return;

            if (!Dispatcher.CheckAccess())
            {
                Dispatcher.BeginInvoke(new Action(() => updatePedalState(pedalPosition_u16)));
                return;
            }

            double liveRatio = Math.Max(0.0, Math.Min(1.0, (double)pedalPosition_u16 / 65535.0));
            CanvasDraw(liveRatio);
        }

        private void CanvasDraw(double? livePositionRatio = null)
        {
            // Only update text input fields when NOT receiving live pedal movement updates
            if (livePositionRatio == null)
            {
                Label_kinematic_b_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_b;
                Label_kinematic_c_hort_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
                Label_kinematic_c_vert_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
                Label_kinematic_a_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_a;
                Label_kinematic_d_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_d;
                Label_travel_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_travel;
                Label_kinematic_scale.Content = Math.Round(Settings.kinematicDiagram_zeroPos_scale, 1);
            }

            //parameter calculation
            double OA_length = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB_length = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC_length = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double Travel_length = dap_config_st.payloadPedalConfig_.lengthPedal_travel;
            double CA_length = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            double OD_length = OA_length + dap_config_st.payloadPedalConfig_.lengthPedal_d;
            if (OA_length == 0 || OB_length == 0 || BC_length == 0 || CA_length == 0)
            {
                OA_length = 220;
                OB_length = 215;
                BC_length = 60;
                CA_length = 220;
                Travel_length = 60;
                OD_length = OA_length + dap_config_st.payloadPedalConfig_.lengthPedal_d;
            }

            double Current_travel_position;
            if (livePositionRatio.HasValue)
            {
                double startRatio = (double)dap_config_st.payloadPedalConfig_.pedalStartPosition / 100.0;
                double endRatio = (double)dap_config_st.payloadPedalConfig_.pedalEndPosition / 100.0;
                if (endRatio <= startRatio) endRatio = 1.0;
                double effectiveRatio = startRatio + livePositionRatio.Value * (endRatio - startRatio);
                Current_travel_position = Travel_length * effectiveRatio;
            }
            else
            {
                Current_travel_position = Travel_length / 100.0 * dap_config_st.payloadPedalConfig_.pedalStartPosition;
            }

            double OC_length = Math.Sqrt((OB_length + Current_travel_position) * (OB_length + Current_travel_position) + BC_length * BC_length);
            double cosAngle1 = (OA_length * OA_length + OC_length * OC_length - CA_length * CA_length) / (2 * OA_length * OC_length);
            cosAngle1 = Math.Max(-1.0, Math.Min(1.0, cosAngle1));
            double pedal_angle_1 = Math.Acos(cosAngle1);
            double pedal_angle_2 = Math.Atan2(BC_length, (OB_length + Current_travel_position));
            double pedal_angle = pedal_angle_1 + pedal_angle_2;

            double OB_Max = OB_length + Travel_length;
            double OC_Max = Math.Sqrt((OB_Max) * (OB_Max) + BC_length * BC_length);
            double cosMin1 = (OA_length * OA_length + OC_Max * OC_Max - CA_length * CA_length) / (2 * OA_length * OC_Max);
            cosMin1 = Math.Max(-1.0, Math.Min(1.0, cosMin1));
            double min_angle_1 = Math.Acos(cosMin1);
            double min_angle_2 = Math.Atan2(BC_length, OB_Max);

            double OC_Min = Math.Sqrt((OB_length) * (OB_length) + BC_length * BC_length);
            double cosMax1 = (OA_length * OA_length + OC_Min * OC_Min - CA_length * CA_length) / (2 * OA_length * OC_Min);
            cosMax1 = Math.Max(-1.0, Math.Min(1.0, cosMax1));
            double max_angle_1 = Math.Acos(cosMax1);
            double max_angle_2 = Math.Atan2(BC_length, OB_length);

            Label_kinematic_pedal_angle.Content = "Current Pedal Angle: " + Math.Round(pedal_angle / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Max Pedal Angle:" + Math.Round((max_angle_1 + max_angle_2) / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Min Pedal Angle:" + Math.Round((min_angle_1 + min_angle_2) / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Angle Travel:" + Math.Round((max_angle_1 + max_angle_2 - min_angle_1 - min_angle_2) / Math.PI * 180) + "°";

            double A_X = OA_length * Math.Cos(pedal_angle);
            double A_Y = OA_length * Math.Sin(pedal_angle);
            double D_X = OD_length * Math.Cos(pedal_angle);
            double D_Y = OD_length * Math.Sin(pedal_angle);
            double scale_factor = Settings.kinematicDiagram_zeroPos_scale;
            if (scale_factor <= 0.01) scale_factor = 1.0;
            double shifting_OX = Settings.kinematicDiagram_zeroPos_OX;
            double shifting_OY = Settings.kinematicDiagram_zeroPos_OY;

            double O_cx = shifting_OX;
            double O_cy = canvas_kinematic.Height - shifting_OY;
            double C_cx = A_X / scale_factor + shifting_OX;
            double C_cy = canvas_kinematic.Height - A_Y / scale_factor - shifting_OY;
            double A_cx = OB_length / scale_factor + shifting_OX;
            double A_cy = canvas_kinematic.Height - shifting_OY;
            double B_cx = (OB_length + Current_travel_position) / scale_factor + shifting_OX;
            double B_cy = canvas_kinematic.Height - BC_length / scale_factor - shifting_OY;
            double D_cx = D_X / scale_factor + shifting_OX;
            double D_cy = canvas_kinematic.Height - D_Y / scale_factor - shifting_OY;

            // Draw schematic shapes
            DrawSchematicShapes(O_cx, O_cy, C_cx, C_cy, B_cx, B_cy, D_cx, D_cy, pedal_angle, scale_factor, shifting_OX, shifting_OY, OB_length, Travel_length);

            // set rect position
            Canvas.SetLeft(rect_joint_O, O_cx - rect_joint_O.Width / 2);
            Canvas.SetTop(rect_joint_O, O_cy - rect_joint_O.Height / 2);
            Canvas.SetLeft(rect_joint_C, C_cx - rect_joint_C.Width / 2);
            Canvas.SetTop(rect_joint_C, C_cy - rect_joint_C.Height / 2);
            Canvas.SetLeft(rect_joint_A, A_cx - rect_joint_A.Width / 2);
            Canvas.SetTop(rect_joint_A, A_cy - rect_joint_A.Height / 2);
            Canvas.SetLeft(rect_joint_B, B_cx - rect_joint_B.Width / 2);
            Canvas.SetTop(rect_joint_B, B_cy - rect_joint_B.Height / 2);
            Canvas.SetLeft(rect_joint_D, D_cx - rect_joint_D.Width / 2);
            Canvas.SetTop(rect_joint_D, D_cy - rect_joint_D.Height / 2);

            Canvas.SetLeft(Label_joint_C, Canvas.GetLeft(rect_joint_C) - Label_joint_C.Width);
            Canvas.SetTop(Label_joint_C, Canvas.GetTop(rect_joint_C));
            Canvas.SetLeft(Label_joint_A, Canvas.GetLeft(rect_joint_A) + rect_joint_A.Width / 2 - Label_joint_A.Width / 2);
            Canvas.SetTop(Label_joint_A, Canvas.GetTop(rect_joint_A) - Label_joint_A.Height);
            Canvas.SetLeft(Label_joint_B, Canvas.GetLeft(rect_joint_B) + Label_joint_B.Width);
            Canvas.SetTop(Label_joint_B, Canvas.GetTop(rect_joint_B));
            Canvas.SetLeft(Label_joint_D, Canvas.GetLeft(rect_joint_D) - Label_joint_D.Width);
            Canvas.SetTop(Label_joint_D, Canvas.GetTop(rect_joint_D));
            Canvas.SetLeft(Label_joint_O, Canvas.GetLeft(rect_joint_O) - Label_joint_O.Width);
            Canvas.SetTop(Label_joint_O, Canvas.GetTop(rect_joint_O));

            Canvas.SetLeft(SP_kinematic_b_canvas, (Canvas.GetLeft(rect_joint_C) + shifting_OX) / 2 - SP_kinematic_b_canvas.Width / 2 - Label_kinematic_b_canvas.Width / 2);
            Canvas.SetTop(SP_kinematic_b_canvas, (Canvas.GetTop(rect_joint_C) + canvas_kinematic.Height - shifting_OY) / 2 - Label_kinematic_b_canvas.Height / 2);
            Canvas.SetLeft(SP_kinematic_c_hort_canvas, (Canvas.GetLeft(rect_joint_A) + shifting_OX) / 2 - SP_kinematic_c_hort_canvas.Width / 2 - 5);
            Canvas.SetTop(SP_kinematic_c_hort_canvas, (Canvas.GetTop(rect_joint_A) + canvas_kinematic.Height - shifting_OY) / 2 + Label_kinematic_c_hort_canvas.Height / 2 - 5);
            Canvas.SetLeft(SP_kinematic_c_vert_canvas, Canvas.GetLeft(rect_joint_B) - rect_joint_B.Width - SP_kinematic_c_vert_canvas.Width / 2 + Label_kinematic_c_vert_canvas.Width);
            Canvas.SetTop(SP_kinematic_c_vert_canvas, (Canvas.GetTop(rect_joint_A) + Canvas.GetTop(rect_joint_B)) / 2 - Label_kinematic_c_vert_canvas.Height / 2 + 5);
            Canvas.SetLeft(SP_kinematic_a_canvas, (Canvas.GetLeft(rect_joint_A) + Canvas.GetLeft(rect_joint_C)) / 2 - SP_kinematic_a_canvas.Width / 2 + Label_kinematic_a_canvas.Width / 2);
            Canvas.SetTop(SP_kinematic_a_canvas, (Canvas.GetTop(rect_joint_A) + Canvas.GetTop(rect_joint_C)) / 2 - Label_kinematic_a_canvas.Height);
            Canvas.SetLeft(SP_kinematic_d_canvas, (Canvas.GetLeft(rect_joint_C) + Canvas.GetLeft(rect_joint_D)) / 2 - SP_kinematic_d_canvas.Width / 2 - Label_kinematic_d_canvas.Width / 2);
            Canvas.SetTop(SP_kinematic_d_canvas, (Canvas.GetTop(rect_joint_C) + Canvas.GetTop(rect_joint_D)) / 2 - Label_kinematic_d_canvas.Height / 2);
            Canvas.SetLeft(SP_travel_canvas, (Canvas.GetLeft(rect_joint_A) + (OB_length + Travel_length) / scale_factor + shifting_OX) / 2 - SP_travel_canvas.Width / 2);
            Canvas.SetTop(SP_travel_canvas, (Canvas.GetTop(rect_joint_A) + canvas_kinematic.Height - shifting_OY) / 2 + Label_travel_canvas.Height / 2 - 5);

            this.Line_kinematic_b.X1 = shifting_OX;
            this.Line_kinematic_b.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_b.X2 = C_cx;
            this.Line_kinematic_b.Y2 = C_cy;

            this.Line_kinematic_c_hort.X1 = shifting_OX;
            this.Line_kinematic_c_hort.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_c_hort.X2 = A_cx;
            this.Line_kinematic_c_hort.Y2 = canvas_kinematic.Height - shifting_OY;

            this.Line_kinematic_c_vert.X1 = B_cx;
            this.Line_kinematic_c_vert.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_c_vert.X2 = B_cx;
            this.Line_kinematic_c_vert.Y2 = B_cy;

            this.Line_kinematic_a.X1 = B_cx;
            this.Line_kinematic_a.Y1 = B_cy;
            this.Line_kinematic_a.X2 = C_cx;
            this.Line_kinematic_a.Y2 = C_cy;

            this.Line_kinematic_d.X1 = C_cx;
            this.Line_kinematic_d.Y1 = C_cy;
            this.Line_kinematic_d.X2 = D_cx;
            this.Line_kinematic_d.Y2 = D_cy;

            this.Line_Pedal_Travel.X1 = A_cx;
            this.Line_Pedal_Travel.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_Pedal_Travel.X2 = (OB_length + Travel_length) / scale_factor + shifting_OX;
            this.Line_Pedal_Travel.Y2 = canvas_kinematic.Height - shifting_OY;

            if (livePositionRatio == null)
            {
                PedalServoForceCheck();
            }
        }

        private void DrawSchematicShapes(double O_cx, double O_cy, double C_cx, double C_cy, double B_cx, double B_cy, double D_cx, double D_cy,
            double pedal_angle, double scale_factor, double shifting_OX, double shifting_OY, double OB_length, double Travel_length)
        {
            if (scale_factor <= 0.01) scale_factor = 1.0;

            // 1. Base rail (scales dynamically with zoom in lockstep with stroke)
            double railLeft = shifting_OX - (25.0 / scale_factor);
            double railRight = shifting_OX + (OB_length + Travel_length + 25.0) / scale_factor;
            railRight = Math.Min(railRight, canvas_kinematic.Width - 5);
            double railWidth = Math.Max(40.0, railRight - railLeft);
            double railHeight = Math.Max(8.0, 24.0 / scale_factor);

            Canvas.SetLeft(schematic_base_rail, railLeft);
            Canvas.SetTop(schematic_base_rail, O_cy);
            schematic_base_rail.Width = railWidth;
            schematic_base_rail.Height = railHeight;

            // Rail groove
            double grooveY = O_cy + (railHeight * 0.5);
            schematic_rail_groove.X1 = railLeft + 4;
            schematic_rail_groove.Y1 = grooveY;
            schematic_rail_groove.X2 = railLeft + railWidth - 4;
            schematic_rail_groove.Y2 = grooveY;

            // 2. Base pivot bracket at Joint O (holds pedal upright arm)
            PointCollection bracketPts = new PointCollection
            {
                new Point(O_cx - (18.0 / scale_factor), O_cy + 1),
                new Point(O_cx + (16.0 / scale_factor), O_cy + 1),
                new Point(O_cx + (10.0 / scale_factor), O_cy - (16.0 / scale_factor)),
                new Point(O_cx - (12.0 / scale_factor), O_cy - (16.0 / scale_factor))
            };
            schematic_pivot_bracket.Points = bracketPts;

            // 3. Stepper motor AT THE FRONT (scales dynamically with zoom)
            double motorWidth = 55.0 / scale_factor;
            double motorHeight = 44.0 / scale_factor;
            double motorLeft = O_cx + (18.0 / scale_factor);
            double motorTop = O_cy - motorHeight;
            Canvas.SetLeft(schematic_motor, motorLeft);
            Canvas.SetTop(schematic_motor, motorTop);
            schematic_motor.Width = motorWidth;
            schematic_motor.Height = motorHeight;

            // Motor bearing / coupler block (white bracket in CAD)
            double bearingWidth = 14.0 / scale_factor;
            double bearingHeight = 26.0 / scale_factor;
            double bearingLeft = motorLeft + motorWidth;
            double bearingTop = O_cy - bearingHeight;
            Canvas.SetLeft(schematic_motor_bearing, bearingLeft);
            Canvas.SetTop(schematic_motor_bearing, bearingTop);
            schematic_motor_bearing.Width = bearingWidth;
            schematic_motor_bearing.Height = bearingHeight;

            // 4. Lead screw / ballscrew from bearing block to carriage front
            double screwY = O_cy - (13.0 / scale_factor);
            schematic_ballscrew.X1 = bearingLeft + bearingWidth;
            schematic_ballscrew.Y1 = screwY;
            schematic_ballscrew.X2 = Math.Max(bearingLeft + bearingWidth, B_cx - (16.0 / scale_factor));
            schematic_ballscrew.Y2 = screwY;

            // 5. Slider carriage around Joint B
            PointCollection carriagePts = new PointCollection
            {
                new Point(B_cx - (18.0 / scale_factor), O_cy),
                new Point(B_cx + (20.0 / scale_factor), O_cy),
                new Point(B_cx + (20.0 / scale_factor), O_cy - (16.0 / scale_factor)),
                new Point(B_cx + (10.0 / scale_factor), B_cy - (6.0 / scale_factor)),
                new Point(B_cx - (10.0 / scale_factor), B_cy - (6.0 / scale_factor)),
                new Point(B_cx - (18.0 / scale_factor), O_cy - (16.0 / scale_factor))
            };
            schematic_carriage.Points = carriagePts;

            // 6. Scaled Loadcell bar (pushrod connecting Joint B to Joint C)
            double rod_dx = C_cx - B_cx;
            double rod_dy = C_cy - B_cy;
            double rod_len = Math.Sqrt(rod_dx * rod_dx + rod_dy * rod_dy);
            if (rod_len < 1e-4) rod_len = 1.0;
            double u_rod_x = rod_dx / rod_len;
            double u_rod_y = rod_dy / rod_len;
            double n_rod_x = -u_rod_y;
            double n_rod_y = u_rod_x;
            double w_rod = Math.Max(2.5, 7.5 / scale_factor);
            double end_ext = 6.0 / scale_factor;

            PointCollection pushrodPts = new PointCollection
            {
                new Point(B_cx - end_ext * u_rod_x - w_rod * n_rod_x, B_cy - end_ext * u_rod_y - w_rod * n_rod_y),
                new Point(C_cx + end_ext * u_rod_x - w_rod * n_rod_x, C_cy + end_ext * u_rod_y - w_rod * n_rod_y),
                new Point(C_cx + end_ext * u_rod_x + w_rod * n_rod_x, C_cy + end_ext * u_rod_y + w_rod * n_rod_y),
                new Point(B_cx - end_ext * u_rod_x + w_rod * n_rod_x, B_cy - end_ext * u_rod_y + w_rod * n_rod_y)
            };
            schematic_pushrod.Points = pushrodPts;

            schematic_pushrod_spine.X1 = B_cx;
            schematic_pushrod_spine.Y1 = B_cy;
            schematic_pushrod_spine.X2 = C_cx;
            schematic_pushrod_spine.Y2 = C_cy;

            // 7. Scaled Pedal upright arm from O up past C to D
            double u_arm_x = Math.Cos(pedal_angle);
            double u_arm_y = -Math.Sin(pedal_angle);
            double n_arm_x = u_arm_y;
            double n_arm_y = -u_arm_x;
            double w_arm = Math.Max(3.0, 10.5 / scale_factor);
            double top_extend = 8.0 / scale_factor;
            double bot_extend = 10.0 / scale_factor;

            // Distance from C to D along arm in canvas units:
            double d_length = dap_config_st.payloadPedalConfig_.lengthPedal_d;
            double d_canvas = d_length / scale_factor;

            PointCollection armPts;
            if (d_canvas >= 12.0)
            {
                // C is distinctly below D
                armPts = new PointCollection
                {
                    new Point(O_cx - bot_extend * u_arm_x + w_arm * n_arm_x, O_cy - bot_extend * u_arm_y + w_arm * n_arm_y),
                    new Point(D_cx + top_extend * u_arm_x + w_arm * n_arm_x, D_cy + top_extend * u_arm_y + w_arm * n_arm_y),
                    new Point(D_cx + top_extend * u_arm_x - w_arm * n_arm_x, D_cy + top_extend * u_arm_y - w_arm * n_arm_y),
                    new Point(C_cx + (12.0 / scale_factor) * u_arm_x - w_arm * n_arm_x, C_cy + (12.0 / scale_factor) * u_arm_y - w_arm * n_arm_y),
                    new Point(C_cx - (w_arm + 8.0 / scale_factor) * n_arm_x, C_cy - (w_arm + 8.0 / scale_factor) * n_arm_y),
                    new Point(C_cx - (12.0 / scale_factor) * u_arm_x - w_arm * n_arm_x, C_cy - (12.0 / scale_factor) * u_arm_y - w_arm * n_arm_y),
                    new Point(O_cx - bot_extend * u_arm_x - w_arm * n_arm_x, O_cy - bot_extend * u_arm_y - w_arm * n_arm_y)
                };
            }
            else
            {
                // C and D are close or coincident (d = 0): clean upper clevis without horns
                armPts = new PointCollection
                {
                    new Point(O_cx - bot_extend * u_arm_x + w_arm * n_arm_x, O_cy - bot_extend * u_arm_y + w_arm * n_arm_y),
                    new Point(D_cx + top_extend * u_arm_x + w_arm * n_arm_x, D_cy + top_extend * u_arm_y + w_arm * n_arm_y),
                    new Point(D_cx + top_extend * u_arm_x - (w_arm + 4.0 / scale_factor) * n_arm_x, D_cy + top_extend * u_arm_y - (w_arm + 4.0 / scale_factor) * n_arm_y),
                    new Point(C_cx - (w_arm + 8.0 / scale_factor) * n_arm_x, C_cy - (w_arm + 8.0 / scale_factor) * n_arm_y),
                    new Point(C_cx - (14.0 / scale_factor) * u_arm_x - w_arm * n_arm_x, C_cy - (14.0 / scale_factor) * u_arm_y - w_arm * n_arm_y),
                    new Point(O_cx - bot_extend * u_arm_x - w_arm * n_arm_x, O_cy - bot_extend * u_arm_y - w_arm * n_arm_y)
                };
            }
            schematic_pedal_arm.Points = armPts;

            // 8. Scaled Footplate with convex curved front facing driver
            double pad_h_up = 36.0 / scale_factor;
            double pad_h_down = 28.0 / scale_factor;
            double pad_back = w_arm + (1.0 / scale_factor);
            double c_tip = 8.0 / scale_factor;
            double c_mid = 15.0 / scale_factor;
            double c_max = 18.0 / scale_factor;

            PointCollection footplatePts = new PointCollection
            {
                // Top-back
                new Point(D_cx + pad_h_up * u_arm_x + pad_back * n_arm_x, D_cy + pad_h_up * u_arm_y + pad_back * n_arm_y),
                // Top rounded tip
                new Point(D_cx + pad_h_up * u_arm_x + (pad_back + c_tip) * n_arm_x, D_cy + pad_h_up * u_arm_y + (pad_back + c_tip) * n_arm_y),
                // Upper-mid curve forward
                new Point(D_cx + (pad_h_up * 0.55) * u_arm_x + (pad_back + c_mid) * n_arm_x, D_cy + (pad_h_up * 0.55) * u_arm_y + (pad_back + c_mid) * n_arm_y),
                // Center crown (maximum forward curve towards driver's foot)
                new Point(D_cx + (pad_back + c_max) * n_arm_x, D_cy + (pad_back + c_max) * n_arm_y),
                // Lower-mid curve forward
                new Point(D_cx - (pad_h_down * 0.5) * u_arm_x + (pad_back + c_mid) * n_arm_x, D_cy - (pad_h_down * 0.5) * u_arm_y + (pad_back + c_mid) * n_arm_y),
                // Bottom rounded tip
                new Point(D_cx - pad_h_down * u_arm_x + (pad_back + c_tip) * n_arm_x, D_cy - pad_h_down * u_arm_y + (pad_back + c_tip) * n_arm_y),
                // Bottom-back
                new Point(D_cx - pad_h_down * u_arm_x + pad_back * n_arm_x, D_cy - pad_h_down * u_arm_y + pad_back * n_arm_y)
            };
            schematic_footplate.Points = footplatePts;
        }

        private void btn_plus_kinematic_b_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA + 1, OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_b = (Int16)(tmp.payloadPedalConfig_.lengthPedal_b + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void NumericTextBox_PreviewTextInput(object sender, TextCompositionEventArgs e)
        {
            Regex regex = new Regex("^[.][0-9]+$|^[0-9]*[.]{0,4}[0-9]*$");

            System.Windows.Controls.TextBox textBox = (System.Windows.Controls.TextBox)sender;

            e.Handled = !regex.IsMatch(textBox.Text + e.Text);
        }

        private void Kinematic_TextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            var textbox = sender as System.Windows.Controls.TextBox;
            if (textbox.Name == "Label_kinematic_b_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    double OA = result;
                    double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
                    double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
                    double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
                    if (Kinematic_check(OA, OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_b = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
            if (textbox.Name == "Label_kinematic_c_hort_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
                    double OB = result;
                    double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
                    double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
                    if (Kinematic_check(OA, OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_c_horizontal = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
            if (textbox.Name == "Label_kinematic_c_vert_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
                    double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
                    double BC = result;
                    double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
                    if (Kinematic_check(OA, OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_c_vertical = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
            if (textbox.Name == "Label_kinematic_a_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
                    double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
                    double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
                    double CA = result;
                    if (Kinematic_check(OA, OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_a = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
            if (textbox.Name == "Label_kinematic_d_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    if (result >= 0 && result <= 100)
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_d = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
            if (textbox.Name == "Label_travel_canvas")
            {
                if (int.TryParse(textbox.Text, out int result))
                {
                    if (result >= 10 && result <= 200)
                    {
                        var tmp = dap_config_st;
                        tmp.payloadPedalConfig_.lengthPedal_travel = (Int16)(result);
                        dap_config_st = tmp;
                        CanvasDraw();
                        PedalServoForceCheck();
                        ConfigChangedEvent(dap_config_st);
                    }
                    else
                    {
                        TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
                    }
                }
            }
        }

        private void btn_minus_kinematic_b_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA -1 , OB, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_b = (Int16)(tmp.payloadPedalConfig_.lengthPedal_b - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_plus_kinematic_c_hort_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA , OB+1, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_c_horizontal = (Int16)(tmp.payloadPedalConfig_.lengthPedal_c_horizontal + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_minus_kinematic_c_hort_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA, OB - 1, BC, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_c_horizontal = (Int16)(tmp.payloadPedalConfig_.lengthPedal_c_horizontal - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }


        private void btn_plus_kinematic_c_vert_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA, OB , BC+1, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_c_vertical = (Int16)(tmp.payloadPedalConfig_.lengthPedal_c_vertical +1 );
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_minus_kinematic_c_vert_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA, OB, BC - 1, CA, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_c_vertical = (Int16)(tmp.payloadPedalConfig_.lengthPedal_c_vertical - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_plus_kinematic_a_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA, OB, BC , CA+1, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_a = (Int16)(tmp.payloadPedalConfig_.lengthPedal_a + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_minus_kinematic_a_canvas_Click(object sender, RoutedEventArgs e)
        {
            double OA = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double CA = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            if (Kinematic_check(OA, OB, BC, CA - 1, dap_config_st.payloadPedalConfig_.lengthPedal_travel))
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_a = (Int16)(tmp.payloadPedalConfig_.lengthPedal_a - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();

            }
            else
            {

                TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsError", "Pedal Kinematic calculation error");
            }
        }

        private void btn_plus_kinematic_d_canvas_Click(object sender, RoutedEventArgs e)
        {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_d = (Int16)(tmp.payloadPedalConfig_.lengthPedal_d + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();
        }

        private void btn_minus_kinematic_d_canvas_Click(object sender, RoutedEventArgs e)
        {
            // check whether lower limit is reached already
            if (dap_config_st.payloadPedalConfig_.lengthPedal_d > 0)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_d = (Int16)(tmp.payloadPedalConfig_.lengthPedal_d - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();
            }
            else
            {
                TextBlock_Warning_kinematics.Text = "Reached min value";
            }
        }

        private void btn_plus_travel_canvas_Click(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.lengthPedal_travel <= 200)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_travel = (Int16)(tmp.payloadPedalConfig_.lengthPedal_travel + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();
            }
            else
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_travel = 200;
                dap_config_st = tmp;
                CanvasDraw();
            }

        }

        private void btn_minus_travel_canvas_Click(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.lengthPedal_travel >=30)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_travel = (Int16)(tmp.payloadPedalConfig_.lengthPedal_travel - 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();
            }
            else
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_travel = 30;
                dap_config_st = tmp;
                CanvasDraw();
            }
        }

        private void btn_plus_kinematic_scale_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.kinematicDiagram_zeroPos_scale <2)
            {
                Settings.kinematicDiagram_zeroPos_scale = Settings.kinematicDiagram_zeroPos_scale + 0.1;
                DrawGridLines_kinematicCanvas(Settings.kinematicDiagram_zeroPos_OX, Settings.kinematicDiagram_zeroPos_OY, Settings.kinematicDiagram_zeroPos_scale);
                CanvasDraw();
                SettingsChangedEvent(Settings);

            }
        }

        private void btn_minus_kinematic_scale_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.kinematicDiagram_zeroPos_scale > 0.7)
            {
                Settings.kinematicDiagram_zeroPos_scale = Settings.kinematicDiagram_zeroPos_scale - 0.1;
                DrawGridLines_kinematicCanvas(Settings.kinematicDiagram_zeroPos_OX, Settings.kinematicDiagram_zeroPos_OY, Settings.kinematicDiagram_zeroPos_scale);
                CanvasDraw();
                SettingsChangedEvent(Settings);
                
            }
        }

        private void SP_canvas_MouseEnter(object sender, MouseEventArgs e)
        {
            btn_plus_kinematic_b_canvas.Visibility = Visibility.Visible;
            btn_minus_kinematic_b_canvas.Visibility = Visibility.Visible;
            btn_plus_kinematic_c_hort_canvas.Visibility = Visibility.Visible;
            btn_minus_kinematic_c_hort_canvas.Visibility = Visibility.Visible;
            btn_plus_kinematic_c_vert_canvas.Visibility = Visibility.Visible;
            btn_minus_kinematic_c_vert_canvas.Visibility = Visibility.Visible;
            btn_plus_kinematic_a_canvas.Visibility = Visibility.Visible;
            btn_minus_kinematic_a_canvas.Visibility = Visibility.Visible;
            btn_plus_kinematic_d_canvas.Visibility = Visibility.Visible;
            btn_minus_kinematic_d_canvas.Visibility = Visibility.Visible;
            btn_plus_travel_canvas.Visibility = Visibility.Visible;
            btn_minus_travel_canvas.Visibility = Visibility.Visible;
        }
        private void SP_canvas_MouseLeave(object sender, MouseEventArgs e)
        {
            btn_plus_kinematic_b_canvas.Visibility = Visibility.Hidden;
            btn_minus_kinematic_b_canvas.Visibility = Visibility.Hidden;
            btn_plus_kinematic_c_hort_canvas.Visibility = Visibility.Hidden;
            btn_minus_kinematic_c_hort_canvas.Visibility = Visibility.Hidden;
            btn_plus_kinematic_c_vert_canvas.Visibility = Visibility.Hidden;
            btn_minus_kinematic_c_vert_canvas.Visibility = Visibility.Hidden;
            btn_plus_kinematic_a_canvas.Visibility = Visibility.Hidden;
            btn_minus_kinematic_a_canvas.Visibility = Visibility.Hidden;
            btn_plus_kinematic_d_canvas.Visibility = Visibility.Hidden;
            btn_minus_kinematic_d_canvas.Visibility = Visibility.Hidden;
            btn_plus_travel_canvas.Visibility = Visibility.Hidden;
            btn_minus_travel_canvas.Visibility = Visibility.Hidden;
        }
        private bool Kinematic_check(double OA, double OB, double BC, double CA, double travel)
        {

            double OC = Math.Sqrt((OB + travel) * (OB + travel) + BC * BC);
            double pedal_angle_1 = Math.Acos((OA * OA + OC * OC - CA * CA) / (2 * OA * OC));
            double pedal_angle_2 = Math.Atan2(BC, (OB + travel));


            double pedal_angle = pedal_angle_1 + pedal_angle_2;
            if (pedal_angle_1 != double.NaN && pedal_angle_2 != double.NaN)
            {
                if (pedal_angle <= Math.PI * 0.6)
                {
                    if ((OA + CA) > OC)
                    {
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            else
            {
                return false;
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
            TextBlock_Warning_kinematics.Text = SLoc.GetValue("DIYFFBPedalPlugin_TextPedalKinematicsExpectedMaxForce", "Expected max force at max travel")+":" + Math.Round(servo_max_force_output_in_kg) + "kg";
            //TextBlock_Warning_kinematics.Text = "Expected max force at max travel:" + Math.Round(servo_max_force_output_in_kg) + "kg";
        }
        private void DrawGridLines_kinematicCanvas(double OX, double OY, double scale_i)
        {

            if (gridline_kinematic_count_original > 0)
            {
                for (int i = 0; i < gridline_kinematic_count_original; i++)
                {
                    if (canvas_kinematic.Children.Count != 0)
                    {
                        canvas_kinematic.Children.RemoveAt(canvas_kinematic.Children.Count - 1);
                    }
                }
            }
            double scale = scale_i;
            double gridlineSpacing = 50 / scale;

            double cellWidth = gridlineSpacing;
            double cellHeight = gridlineSpacing;

            // we want the gridlines to be centered at pedal position O
            // --> calculate an offset
            double xOffset = OX % gridlineSpacing;
            double yOffset = OY % gridlineSpacing;


            int rowCount = (int)Math.Floor((canvas_kinematic.Height - 0 * yOffset) / gridlineSpacing);
            int columnCount = (int)Math.Floor((canvas_kinematic.Width - 0 * xOffset) / gridlineSpacing);


            // Draw horizontal gridlines
            for (int i = 0; i < rowCount; i++)
            {

                Line line2 = new Line
                {
                    X1 = 0,
                    Y1 = canvas_kinematic.Height - (yOffset + i * cellHeight),
                    X2 = 400,
                    Y2 = canvas_kinematic.Height - (yOffset + i * cellHeight),
                    //Stroke = Brush.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1

                };
                canvas_kinematic.Children.Add(line2);
            }

            // Draw vertical gridlines
            for (int i = 0; i < columnCount; i++)
            {

                Line line2 = new Line
                {
                    X1 = xOffset + i * cellWidth,
                    Y1 = 0,
                    X2 = xOffset + i * cellWidth,
                    Y2 = canvas_kinematic.Height,
                    //Stroke = Brushes.Black,
                    Stroke = System.Windows.Media.Brushes.LightSteelBlue,
                    StrokeThickness = 1,
                    Opacity = 0.1
                };
                canvas_kinematic.Children.Add(line2);

            }
            gridline_kinematic_count_original = columnCount + rowCount;
        }
    }
    

}
