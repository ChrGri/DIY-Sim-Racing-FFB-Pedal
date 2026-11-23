using System;
using System.Collections.Generic;
using System.Globalization;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Live plot of the effect pipeline offsets of the last 5 s, one subplot per
    // output type (pedal angle, force, relative force), one line per pedal channel.
    public class EffectPipelinePlotWindow : Window
    {
        private static EffectPipelinePlotWindow _instance;

        // Opens the window or brings the existing one to the front.
        public static void ShowFor(DIY_FFB_Pedal plugin, Window owner)
        {
            if (_instance == null)
            {
                _instance = new EffectPipelinePlotWindow(plugin);
                if (owner != null) _instance.Owner = owner;
                _instance.Closed += (s, e) => _instance = null;
                _instance.Show();
            }
            else
            {
                _instance.Activate();
            }
        }

        private static readonly string[] PedalNames = { "Clutch", "Brake", "Throttle" };
        private static readonly Color[] SeriesColors =
        {
            Color.FromRgb(0x4F, 0xC3, 0xF7), // clutch
            Color.FromRgb(0xEF, 0x53, 0x50), // brake
            Color.FromRgb(0x66, 0xBB, 0x6A), // throttle
        };

        private class Subplot
        {
            public EffectPipelineOffsetType Type;
            public string Unit;
            public double MinRange;
            public Canvas Canvas;
            public TextBlock Title;
            public double Scale;
        }

        private readonly DIY_FFB_Pedal _plugin;
        private readonly List<Subplot> _subplots = new List<Subplot>();
        private readonly double[] _buffer = new double[EffectPipelineEngine.HistoryLength];
        private readonly DispatcherTimer _timer;
        private const double WindowSeconds = 5.0;

        private EffectPipelinePlotWindow(DIY_FFB_Pedal plugin)
        {
            _plugin = plugin;
            Title = "Script effects - live offsets (last 5 s)";
            Width = 800;
            Height = 600;
            Background = new SolidColorBrush(Color.FromRgb(0x25, 0x25, 0x25));

            var grid = new Grid { Margin = new Thickness(8) };
            AddSubplot(grid, EffectPipelineOffsetType.Angle_deg, "Pedal angle offset", "deg", 0.05);
            AddSubplot(grid, EffectPipelineOffsetType.Force_N, "Force offset", "N", 1.0);
            AddSubplot(grid, EffectPipelineOffsetType.ForceRelative, "Relative force", "", 0.05);

            var legend = new WrapPanel { Margin = new Thickness(0, 4, 0, 0) };
            for (int pedal = 0; pedal < 3; pedal++)
            {
                var item = new StackPanel { Orientation = Orientation.Horizontal, Margin = new Thickness(0, 0, 14, 0) };
                item.Children.Add(new Rectangle { Width = 14, Height = 3, Fill = new SolidColorBrush(SeriesColors[pedal]), VerticalAlignment = VerticalAlignment.Center });
                item.Children.Add(new TextBlock { Text = " " + PedalNames[pedal] + " (sum of its scripts)", Foreground = Brushes.LightGray, FontSize = 11 });
                legend.Children.Add(item);
            }
            grid.RowDefinitions.Add(new RowDefinition { Height = GridLength.Auto });
            Grid.SetRow(legend, 3);
            grid.Children.Add(legend);
            Content = grid;

            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(50) };
            _timer.Tick += (s, e) => Redraw();
            Loaded += (s, e) => _timer.Start();
            Closed += (s, e) => _timer.Stop();
        }

        private void AddSubplot(Grid grid, EffectPipelineOffsetType type, string title, string unit, double minRange)
        {
            int row = grid.RowDefinitions.Count;
            grid.RowDefinitions.Add(new RowDefinition { Height = new GridLength(1, GridUnitType.Star) });

            var panel = new DockPanel { Margin = new Thickness(0, 0, 0, 6) };
            var titleBlock = new TextBlock { Text = title, Foreground = Brushes.White, FontWeight = FontWeights.Bold, Margin = new Thickness(0, 0, 0, 2) };
            DockPanel.SetDock(titleBlock, Dock.Top);
            panel.Children.Add(titleBlock);
            var border = new Border { BorderBrush = new SolidColorBrush(Color.FromRgb(0x55, 0x55, 0x55)), BorderThickness = new Thickness(1) };
            var canvas = new Canvas { ClipToBounds = true, Background = new SolidColorBrush(Color.FromRgb(0x1B, 0x1B, 0x1B)) };
            border.Child = canvas;
            panel.Children.Add(border);
            Grid.SetRow(panel, row);
            grid.Children.Add(panel);

            _subplots.Add(new Subplot { Type = type, Unit = unit, MinRange = minRange, Canvas = canvas, Title = titleBlock, Scale = minRange });
        }

        private void Redraw()
        {
            EffectPipelineEngine engine = _plugin?.EffectPipelines;
            if (engine == null) return;
            uint endIndex = engine.CurrentSampleIndex;

            foreach (Subplot sp in _subplots) sp.Canvas.Children.Clear();
            var maxAbs = new Dictionary<EffectPipelineOffsetType, double>();
            var series = new List<Tuple<Subplot, int, double[]>>();

            for (int pedal = 0; pedal < 3; pedal++)
            {
                foreach (Subplot sp in _subplots)
                {
                    EffectPipelineOffsetType type = sp.Type;
                    var values = new double[_buffer.Length];
                    engine.CopyHistory(pedal, type, endIndex, values);
                    double m = 0.0;
                    foreach (double v in values) m = Math.Max(m, Math.Abs(v));
                    if (m <= 0.0) continue; // nothing to show for this channel
                    double prev;
                    maxAbs.TryGetValue(type, out prev);
                    maxAbs[type] = Math.Max(prev, m);
                    series.Add(Tuple.Create(sp, pedal, values));
                }
            }

            foreach (Subplot sp in _subplots)
            {
                double m;
                maxAbs.TryGetValue(sp.Type, out m);
                double target = Math.Max(sp.MinRange, m * 1.1);
                // grow immediately, shrink slowly
                sp.Scale = target > sp.Scale ? target : sp.Scale + (target - sp.Scale) * 0.05;
                DrawAxes(sp);
            }

            foreach (var s in series) DrawSeries(s.Item1, s.Item3, SeriesColors[s.Item2]);
        }

        private void DrawAxes(Subplot sp)
        {
            double w = sp.Canvas.ActualWidth, h = sp.Canvas.ActualHeight;
            if (w <= 0 || h <= 0) return;
            var gridBrush = new SolidColorBrush(Color.FromRgb(0x3A, 0x3A, 0x3A));
            for (int sec = 1; sec < (int)WindowSeconds; sec++)
            {
                double x = w * sec / WindowSeconds;
                sp.Canvas.Children.Add(new Line { X1 = x, X2 = x, Y1 = 0, Y2 = h, Stroke = gridBrush, StrokeThickness = 1 });
            }
            sp.Canvas.Children.Add(new Line { X1 = 0, X2 = w, Y1 = h / 2, Y2 = h / 2, Stroke = Brushes.Gray, StrokeThickness = 1 });
            string unit = string.IsNullOrEmpty(sp.Unit) ? "" : " " + sp.Unit;
            AddLabel(sp.Canvas, "+" + sp.Scale.ToString("0.###", CultureInfo.InvariantCulture) + unit, 4, 2);
            AddLabel(sp.Canvas, "-" + sp.Scale.ToString("0.###", CultureInfo.InvariantCulture) + unit, 4, h - 16);
            AddLabel(sp.Canvas, "-5 s", 4, h / 2 + 2);
            AddLabel(sp.Canvas, "now", w - 28, h / 2 + 2);
        }

        private static void AddLabel(Canvas canvas, string text, double x, double y)
        {
            var tb = new TextBlock { Text = text, Foreground = Brushes.Gray, FontSize = 10 };
            Canvas.SetLeft(tb, x);
            Canvas.SetTop(tb, y);
            canvas.Children.Add(tb);
        }

        // One min/max pair per pixel column, so a fast vibration shows as its envelope.
        private static void DrawSeries(Subplot sp, double[] values, Color color)
        {
            double w = sp.Canvas.ActualWidth, h = sp.Canvas.ActualHeight;
            if (w <= 1 || h <= 0) return;
            int columns = (int)w;
            var points = new PointCollection(columns * 2);
            for (int c = 0; c < columns; c++)
            {
                int from = (int)((long)c * values.Length / columns);
                int to = Math.Max(from + 1, (int)((long)(c + 1) * values.Length / columns));
                double min = double.MaxValue, max = double.MinValue;
                for (int i = from; i < to && i < values.Length; i++)
                {
                    min = Math.Min(min, values[i]);
                    max = Math.Max(max, values[i]);
                }
                points.Add(new Point(c, ToY(min, sp.Scale, h)));
                if (max != min) points.Add(new Point(c, ToY(max, sp.Scale, h)));
            }
            sp.Canvas.Children.Add(new Polyline { Points = points, Stroke = new SolidColorBrush(color), StrokeThickness = 1.2 });
        }

        private static double ToY(double value, double scale, double h)
        {
            double y = h / 2 - value / scale * (h / 2 - 2);
            return Math.Max(0, Math.Min(h, y));
        }
    }
}
