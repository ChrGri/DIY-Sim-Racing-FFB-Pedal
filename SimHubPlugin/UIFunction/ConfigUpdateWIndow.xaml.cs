using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
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
using static System.Windows.Forms.VisualStyles.VisualStyleElement.Header;

namespace DiyFfbPedal.UIFunction
{
    /// <summary>
    /// ConfigUpdateWIndow.xaml ?????
    /// </summary>

    
    public partial class ConfigUpdateWIndow : Window
    {
        public ObservableCollection<ConfigListItem> ItemList { get; set; }
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
        bool compatibleMode = false;
        public ICommand AddNewItemCommand { get; }
        public ICommand SetCurrentItemCommand { get; }
        public ICommand SetDefaultItemCommand { get; }
        public ICommand RemoveItemCommand { get; }
        
        public ConfigUpdateWIndow(DIY_FFB_Pedal Plugin)
        {

            InitializeComponent();
            _plugin = Plugin;
            ItemList=_plugin.ConfigList;
            AddNewItemCommand = new RelayCommand(AddNewItem);
            SetCurrentItemCommand = new RelayCommand(SetCurrentItem);
            SetDefaultItemCommand = new RelayCommand(SetDefaultItem);
            RemoveItemCommand = new RelayCommand(RemoveItem);
            InitRectangles();
            force = new byte[maxQuantity];
            travel = new byte[maxQuantity];
            compatibleForce = new byte[minQuantity];
            for (int i = 0; i < maxQuantity; i++)
            {
                force[i] = 0;
                travel[i] = 0;
            }
            this.DataContext = this;
        }

        private void AddNewItem(object parameter)
        {
            ItemList.Add(new ConfigListItem { ListName = $"New Item {ItemList.Count + 1}" });
        }

        private void SetCurrentItem(object parameter)
        {
            if (parameter is ConfigListItem item)
            {

                foreach (var otherItem in ItemList.Where(i => i.IsCurrent))
                {
                    otherItem.IsCurrent = false;
                }
                item.IsCurrent = true;
            }
        }

        private void SetDefaultItem(object parameter)
        {
            if (parameter is ConfigListItem item)
            {

                foreach (var otherItem in ItemList.Where(i => i.IsDefault))
                {
                    otherItem.IsDefault = false;
                }
                item.IsDefault = true;
            }
        }

        private void RemoveItem(object parameter)
        {
            if (parameter is ConfigListItem item)
            {
                ItemList.Remove(item);
            }
        }

        private void ProfilesListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (sender is ListBox listBox)
            {
                if (listBox.SelectedItem is ConfigListItem item)
                {             
                    tmp_config = _plugin.ReadConfig(item.FullPath);
                    Update_ForceCurve();
                }
            }

        }

        private void Btn_Apply_Click(object sender, RoutedEventArgs e)
        {

        }

        private void Btn_Online_profile_leave_Click(object sender, RoutedEventArgs e)
        {

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
    }
}
