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
using System.Windows.Media.Imaging;
using System.Windows.Shapes;

namespace User.PluginSdkDemo
{
    /// <summary>
    /// ConfigPreviewWindow.xaml 的互動邏輯
    /// </summary>
    
    public partial class ConfigPreviewWindow : Window
    {
        private DIY_FFB_Pedal _plugin;
        private DAP_config_st tmp_config;
        private byte[] force;
        private byte[] compatibleForce;
        private byte[] travel;
        private int maxQuantity = 11;
        private int minQuantity = 6;
        private const double RectSize = 6;
        public int rectCount = 0;
        public List<double> rectPositionX = new List<double>();
        public List<double> rectPositionY = new List<double>();
        public ConfigPreviewWindow(DIY_FFB_Pedal Plugin, ConfigListItem item)
        {
            InitializeComponent();
            _plugin = Plugin;
            this.Title = "Config Preview - " + item.ListName;
            string filePath = item.FullPath;
            InitRectangles();
            
            force = new byte[maxQuantity];
            travel = new byte[maxQuantity];
            compatibleForce = new byte[minQuantity];
            for (int i = 0; i < maxQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            tmp_config = _plugin.ConfigService.ReadConfig(filePath);
            Update_ForceCurve();
            Textbox_Online_Profile_Description.Content = item.ListName;
            Textbox_Online_Profile_Description.Content += " Preview:\n";
            Textbox_Online_Profile_Description.Content += "DAP Version: " + tmp_config.payloadHeader_.version + "\n";
            Textbox_Online_Profile_Description.Content += "Max force: " + tmp_config.payloadPedalConfig_.maxForce + "\n";
            Textbox_Online_Profile_Description.Content += "Preload: " + tmp_config.payloadPedalConfig_.preloadForce + "\n";
            //Textbox_Online_Profile_Description.Text += "Max force: " + tmp_config.payloadPedalConfig_.maxForce + "\n";
            Textbox_Online_Profile_Description.Content += "Travel: " + ((float)(tmp_config.payloadPedalConfig_.pedalEndPosition - tmp_config.payloadPedalConfig_.pedalStartPosition) / 100.0f * tmp_config.payloadPedalConfig_.lengthPedal_travel) + "\n";
            Textbox_Online_Profile_Description.Content += "Damping: " + tmp_config.payloadPedalConfig_.dampingPress + "\n";
            switch (tmp_config.payloadPedalConfig_.kf_modelOrder)
            {
                case 0:
                    Textbox_Online_Profile_Description.Content += "KF Model: Const. Vel\n";
                    break;
                case 1:
                    Textbox_Online_Profile_Description.Content += "KF Model: Const. Acc\n";
                    break;
                case 2:
                    Textbox_Online_Profile_Description.Content += "KF Model: EXP.\n";
                    break;
                case 4:
                    Textbox_Online_Profile_Description.Content += "KF Model: None\n";
                    break;
            }
            Textbox_Online_Profile_Description.Content += "KF :" + tmp_config.payloadPedalConfig_.kf_modelNoise + "\n";
            Textbox_Online_Profile_Description.Content += "Serrvo smoothing :" + tmp_config.payloadPedalConfig_.positionSmoothingFactor_u8 + "\n";

        }
        private void Update_ForceCurve()
        {

            double[] x = new double[tmp_config.payloadPedalConfig_.quantityOfControl];
            double[] y = new double[tmp_config.payloadPedalConfig_.quantityOfControl];
            double x_quantity = 101;
            double y_max = 101;
            double dx = canvas_Online_Config_curve.Width / (x_quantity - 1);
            double dy = canvas_Online_Config_curve.Height / (y_max - 1);
            checkExistingRect(tmp_config.payloadPedalConfig_.quantityOfControl);

            //int count =
            //draw pedal force-travel curve
            //read all parameter in
            travel[0] = tmp_config.payloadPedalConfig_.relativeTravel00;
            travel[1] = tmp_config.payloadPedalConfig_.relativeTravel01;
            travel[2] = tmp_config.payloadPedalConfig_.relativeTravel02;
            travel[3] = tmp_config.payloadPedalConfig_.relativeTravel03;
            travel[4] = tmp_config.payloadPedalConfig_.relativeTravel04;
            travel[5] = tmp_config.payloadPedalConfig_.relativeTravel05;
            travel[6] = tmp_config.payloadPedalConfig_.relativeTravel06;
            travel[7] = tmp_config.payloadPedalConfig_.relativeTravel07;
            travel[8] = tmp_config.payloadPedalConfig_.relativeTravel08;
            travel[9] = tmp_config.payloadPedalConfig_.relativeTravel09;
            travel[10] = tmp_config.payloadPedalConfig_.relativeTravel10;

            force[0] = tmp_config.payloadPedalConfig_.relativeForce00;
            force[1] = tmp_config.payloadPedalConfig_.relativeForce01;
            force[2] = tmp_config.payloadPedalConfig_.relativeForce02;
            force[3] = tmp_config.payloadPedalConfig_.relativeForce03;
            force[4] = tmp_config.payloadPedalConfig_.relativeForce04;
            force[5] = tmp_config.payloadPedalConfig_.relativeForce05;
            force[6] = tmp_config.payloadPedalConfig_.relativeForce06;
            force[7] = tmp_config.payloadPedalConfig_.relativeForce07;
            force[8] = tmp_config.payloadPedalConfig_.relativeForce08;
            force[9] = tmp_config.payloadPedalConfig_.relativeForce09;
            force[10] = tmp_config.payloadPedalConfig_.relativeForce10;

            for (int i = 0; i < tmp_config.payloadPedalConfig_.quantityOfControl; i++)
            {
                x[i] = travel[i];
                y[i] = force[i];
            }

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, 100);
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();


            for (int pointIdx = 0; pointIdx < 100; pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * xs2[pointIdx], dy * ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);

            }
            this.Polyline_Online_Config_ForceCurve.Points = myPointCollection2;
            //set the rect

            var toDraw = canvas_Online_Config_curve.Children
             .OfType<Rectangle>()
             .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue < rectCount)
             .ToList();
            //fix the latest one to 100%
            Canvas.SetLeft(toDraw[rectCount - 1], canvas_Online_Config_curve.Width - 0.5 * RectSize);
            Canvas.SetTop(toDraw[rectCount - 1], 0 - 0.5 * RectSize);

            for (int i = 1; i < rectCount - 1; i++)
            {
                //fill with new loaded value, only fill from 1 to rectNewCount-2
                Canvas.SetLeft(toDraw[i], (double)travel[i] * dx - 0.5 * RectSize);
                Canvas.SetTop(toDraw[i], canvas_Online_Config_curve.Height - (double)force[i] * dy - 0.5 * RectSize);

            }
        }


        private void InitRectangles()
        {
            for (int i = 0; i < minQuantity; i++)
            {
                AddRectAt(i * canvas_Online_Config_curve.Width / 5 - 0.5 * RectSize, canvas_Online_Config_curve.Height - i * canvas_Online_Config_curve.Height / 5 - 0.5 * RectSize);
            }
            UpdateRectState();
        }

        private void AddRectAt(double x, double y)
        {
            Rectangle rect = new Rectangle
            {
                Width = RectSize,
                Height = RectSize,
                StrokeThickness = 0,
                Opacity = 0.8
            };
            rect.Tag = (int)-1;
            rect.SetResourceReference(Shape.FillProperty, "AccentColorBrush");
            Canvas.SetLeft(rect, x);
            Canvas.SetTop(rect, y);


            canvas_Online_Config_curve.Children.Add(rect);

        }
        private void checkExistingRect(byte rectNewCount)
        {
            if (rectNewCount < rectCount)
            {
                var toRemove = canvas_Online_Config_curve.Children
                 .OfType<Rectangle>()
                 .Where(r => r.Tag != null && r.Tag is int tagValue && tagValue > (rectNewCount - 1))
                 .ToList();

                foreach (var rect in toRemove)
                {
                    canvas_Online_Config_curve.Children.Remove(rect);
                }
            }
            if (rectNewCount > rectCount)
            {
                for (int i = rectCount; i < rectNewCount; i++)
                {
                    AddRectAt(i * 80 - 0.5 * RectSize, canvas_Online_Config_curve.Height - i * 40 - 0.5 * RectSize);
                }

            }
            if (rectNewCount < minQuantity)
            {
                //do nothing
            }
            UpdateRectState();
            //Update_BrakeForceCurve();


        }
        private void UpdateRectState()
        {
            List<Rectangle> taggedRects = canvas_Online_Config_curve.Children
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

        private void Btn_Leave_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
        private void Window_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
           
            if (e.ClickCount == 1)
            {
                
                this.DragMove();
            }
        }
    }
}
