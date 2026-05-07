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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace DiyFfbPedal.UIElement
{

    
    public partial class RSSIBar : UserControl
    {
        List<Rectangle> rssiRects = new List<Rectangle>();
        public RSSIBar()
        {
            InitializeComponent();
            RSSI_Canvas.Children.Clear();
            for (int i = 0; i < 4; i++)
            {
                Rectangle rect = new Rectangle
                {
                    Width = 3.5,
                    Height = 3 + i * 4,
                    Fill = (Brush)this.FindResource("AccentColorBrush"),
                    Visibility = Visibility.Hidden,
                    StrokeThickness = 0,
                };
                Canvas.SetLeft(rect, i * 4);
                Canvas.SetBottom(rect, 0);
                RSSI_Canvas.Children.Add(rect);
                rssiRects.Add(rect);
            }

        }
        public void updateRSSI(int _rssiValue)
        {
            int bars = 0;
            for (int i = 0; i < 4; i++)
            {
                rssiRects[i].Visibility = Visibility.Hidden;
            }
            if (_rssiValue>-85 && _rssiValue<-20) bars = 1;
            if (_rssiValue > -75 && _rssiValue < -20) bars = 2;
            if (_rssiValue > -65 && _rssiValue < -20) bars = 3;
            if (_rssiValue > -60 && _rssiValue < -20) bars = 4;

            for (int i = 0; i < bars; i++)
            {
                rssiRects[i].Visibility = Visibility.Visible;
                if (bars == 1 && i == 0)
                {
                    rssiRects[i].Fill = Brushes.Red;
                }
            }

        }
    }
}
