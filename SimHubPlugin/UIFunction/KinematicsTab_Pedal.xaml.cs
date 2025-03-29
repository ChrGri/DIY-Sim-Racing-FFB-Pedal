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

namespace User.PluginSdkDemo.UIFunction
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
        private void CanvasDraw()
        {
            //A= kinematic joint C
            //B= Kinematic joint A
            //C= Kinematic joint B
            //O=O
            //D=D

            Label_kinematic_b_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_b;
            Label_kinematic_c_hort_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            Label_kinematic_c_vert_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            Label_kinematic_a_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_a;
            Label_kinematic_d_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_d;
            Label_travel_canvas.Text = "" + dap_config_st.payloadPedalConfig_.lengthPedal_travel;
            Label_kinematic_scale.Content = Math.Round(Settings.kinematicDiagram_zeroPos_scale, 1);

            //parameter calculation
            double OA_length = dap_config_st.payloadPedalConfig_.lengthPedal_b;
            double OB_length = dap_config_st.payloadPedalConfig_.lengthPedal_c_horizontal;
            double BC_length = dap_config_st.payloadPedalConfig_.lengthPedal_c_vertical;
            double Travel_length = dap_config_st.payloadPedalConfig_.lengthPedal_travel;
            double CA_length = dap_config_st.payloadPedalConfig_.lengthPedal_a;
            double OD_length = OA_length + dap_config_st.payloadPedalConfig_.lengthPedal_d;
            double Current_travel_position;
            double OC_length;
            double pedal_angle_1;
            double pedal_angle_2;
            double pedal_angle;
            if (OA_length != 0 && OB_length != 0 && BC_length != 0 && CA_length != 0)
            {

                Current_travel_position = Travel_length / 100 * dap_config_st.payloadPedalConfig_.pedalStartPosition;
                OC_length = Math.Sqrt((OB_length + Current_travel_position) * (OB_length + Current_travel_position) + BC_length * BC_length);
                pedal_angle_1 = Math.Acos((OA_length * OA_length + OC_length * OC_length - CA_length * CA_length) / (2 * OA_length * OC_length));
                pedal_angle_2 = Math.Atan2(BC_length, (OB_length + Current_travel_position));
                pedal_angle = pedal_angle_1 + pedal_angle_2;

            }
            else
            {
                OA_length = 220;
                OB_length = 215;
                BC_length = 60;
                CA_length = 220;
                Travel_length = 60;
                Current_travel_position =  Travel_length / 100 * dap_config_st.payloadPedalConfig_.pedalStartPosition;
                OC_length = Math.Sqrt((OB_length + Current_travel_position) * (OB_length + Current_travel_position) + BC_length * BC_length);
                pedal_angle_1 = Math.Acos((OA_length * OA_length + OC_length * OC_length - CA_length * CA_length) / (2 * OA_length * OC_length));
                pedal_angle_2 = Math.Atan2(BC_length, (OB_length + Current_travel_position));
                pedal_angle = pedal_angle_1 + pedal_angle_2;
            }
            double OB_Max = OB_length + Travel_length;
            double OC_Max = Math.Sqrt((OB_Max) * (OB_Max) + BC_length * BC_length);
            double min_angle_1 = Math.Acos((OA_length * OA_length + OC_Max * OC_Max - CA_length * CA_length) / (2 * OA_length * OC_Max));
            double min_angle_2 = Math.Atan2(BC_length, OB_Max);
            double OC_Min = Math.Sqrt((OB_length) * (OB_length) + BC_length * BC_length);
            double max_angle_1 = Math.Acos((OA_length * OA_length + OC_Min * OC_Min - CA_length * CA_length) / (2 * OA_length * OC_Min));
            double max_angle_2 = Math.Atan2(BC_length, OB_length);

            double angle_beta_max = Math.Acos((OC_Max * OC_Max + CA_length * CA_length - OA_length * OA_length) / (2 * OC_Max * CA_length));
            double angle_gamma = Math.Acos((OA_length * OA_length + CA_length * CA_length - OC_Max * OC_Max) / (2 * OA_length * CA_length));
            Label_kinematic_pedal_angle.Content = "Current Pedal Angle: " + Math.Round(pedal_angle / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Max Pedal Angle:" + Math.Round((max_angle_1 + max_angle_2) / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Min Pedal Angle:" + Math.Round((min_angle_1 + min_angle_2) / Math.PI * 180) + "°,";
            Label_kinematic_pedal_angle.Content = Label_kinematic_pedal_angle.Content + " Angle Travel:" + Math.Round((max_angle_1 + max_angle_2 - min_angle_1 - min_angle_2) / Math.PI * 180) + "°";

            double A_X = OA_length * Math.Cos(pedal_angle);
            double A_Y = OA_length * Math.Sin(pedal_angle);
            double D_X = OD_length * Math.Cos(pedal_angle);
            double D_Y = OD_length * Math.Sin(pedal_angle);
            double scale_factor = Settings.kinematicDiagram_zeroPos_scale;
            double shifting_OX = Settings.kinematicDiagram_zeroPos_OX;
            double shifting_OY = Settings.kinematicDiagram_zeroPos_OY;
            //set rect position
            Canvas.SetLeft(rect_joint_O, shifting_OX - rect_joint_O.Width / 2);
            Canvas.SetTop(rect_joint_O, canvas_kinematic.Height - shifting_OY - rect_joint_O.Height / 2);
            Canvas.SetLeft(rect_joint_C, A_X / scale_factor - rect_joint_C.Width / 2 + shifting_OX);
            Canvas.SetTop(rect_joint_C, canvas_kinematic.Height - A_Y / scale_factor - rect_joint_C.Height / 2 - shifting_OY);
            Canvas.SetLeft(rect_joint_A, OB_length / scale_factor - rect_joint_A.Width / 2 + shifting_OX);
            Canvas.SetTop(rect_joint_A, canvas_kinematic.Height - 0 / scale_factor - rect_joint_A.Height / 2 - shifting_OY);
            Canvas.SetLeft(rect_joint_B, OB_length / scale_factor - rect_joint_B.Width / 2 + Current_travel_position / scale_factor + shifting_OX);
            Canvas.SetTop(rect_joint_B, canvas_kinematic.Height - BC_length / scale_factor - rect_joint_B.Height / 2 - shifting_OY);
            Canvas.SetLeft(rect_joint_D, D_X / scale_factor - rect_joint_A.Width / 2 + shifting_OX);
            Canvas.SetTop(rect_joint_D, canvas_kinematic.Height - D_Y / scale_factor - rect_joint_A.Height / 2 - shifting_OY);

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
            Canvas.SetTop(SP_kinematic_d_canvas, (Canvas.GetTop(rect_joint_C) + +Canvas.GetTop(rect_joint_D)) / 2 - Label_kinematic_d_canvas.Height / 2);
            Canvas.SetLeft(SP_travel_canvas, (Canvas.GetLeft(rect_joint_A) + (OB_length + Travel_length) / scale_factor + shifting_OX) / 2 - SP_travel_canvas.Width / 2);
            Canvas.SetTop(SP_travel_canvas, (Canvas.GetTop(rect_joint_A) + canvas_kinematic.Height - shifting_OY) / 2 + Label_travel_canvas.Height / 2 - 5);

            this.Line_kinematic_b.X1 = shifting_OX;
            this.Line_kinematic_b.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_b.X2 = A_X / scale_factor + shifting_OX;
            this.Line_kinematic_b.Y2 = canvas_kinematic.Height - A_Y / scale_factor - shifting_OY;

            this.Line_kinematic_c_hort.X1 = shifting_OX;
            this.Line_kinematic_c_hort.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_c_hort.X2 = OB_length / scale_factor + shifting_OX;
            this.Line_kinematic_c_hort.Y2 = canvas_kinematic.Height - shifting_OY;

            this.Line_kinematic_c_vert.X1 = (OB_length + Current_travel_position) / scale_factor + shifting_OX;
            this.Line_kinematic_c_vert.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_kinematic_c_vert.X2 = (OB_length + Current_travel_position) / scale_factor + shifting_OX;
            this.Line_kinematic_c_vert.Y2 = canvas_kinematic.Height - BC_length / scale_factor - shifting_OY;

            this.Line_kinematic_a.X1 = (OB_length + Current_travel_position) / scale_factor + shifting_OX;
            this.Line_kinematic_a.Y1 = canvas_kinematic.Height - BC_length / scale_factor - shifting_OY;
            this.Line_kinematic_a.X2 = A_X / scale_factor + shifting_OX;
            this.Line_kinematic_a.Y2 = canvas_kinematic.Height - A_Y / scale_factor - shifting_OY;

            this.Line_kinematic_d.X1 = A_X / scale_factor + shifting_OX;
            this.Line_kinematic_d.Y1 = canvas_kinematic.Height - A_Y / scale_factor - shifting_OY;
            this.Line_kinematic_d.X2 = D_X / scale_factor + shifting_OX;
            this.Line_kinematic_d.Y2 = canvas_kinematic.Height - D_Y / scale_factor - shifting_OY;

            this.Line_Pedal_Travel.X1 = OB_length / scale_factor + shifting_OX;
            this.Line_Pedal_Travel.Y1 = canvas_kinematic.Height - shifting_OY;
            this.Line_Pedal_Travel.X2 = (OB_length + Travel_length) / scale_factor + shifting_OX;
            this.Line_Pedal_Travel.Y2 = canvas_kinematic.Height - shifting_OY;
            PedalServoForceCheck();
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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
                        TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
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

                TextBlock_Warning_kinematics.Text = "Pedal Kinematic calculation error";
            }
        }

        private void btn_plus_kinematic_d_canvas_Click(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.lengthPedal_d > 0)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.lengthPedal_d = (Int16)(tmp.payloadPedalConfig_.lengthPedal_d + 1);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
                PedalServoForceCheck();
                CanvasDraw();
            }
            else
            {
                
            }

        }

        private void btn_minus_kinematic_d_canvas_Click(object sender, RoutedEventArgs e)
        {
            if (dap_config_st.payloadPedalConfig_.lengthPedal_d > 2)
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
                TextBlock_Warning_kinematics.Text = "Reach min vale";
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
            c_hort_max = c_hor + dap_config_st.payloadPedalConfig_.lengthPedal_travel;
            oc_max = Math.Sqrt((c_hort_max) * (c_hort_max) + c_vert * c_vert);
            min_angle_1 = Math.Acos((b * b + oc_max * oc_max - a * a) / (2 * b * oc_max));
            min_angle_2 = Math.Atan2(c_vert, c_hort_max);
            angle_beta_max = Math.Acos((oc_max * oc_max + a * a - b * b) / (2 * oc_max * a));
            angle_gamma = Math.Acos((b * b + a * a - oc_max * oc_max) / (2 * b * a));
            double servo_max_force_output_in_kg = Servo_max_force * Math.Sin(angle_gamma) * b / od / Math.Cos(angle_beta_max - min_angle_2);

            TextBlock_Warning_kinematics.Text = "Expected max force at max travel:" + Math.Round(servo_max_force_output_in_kg) + "kg";
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
