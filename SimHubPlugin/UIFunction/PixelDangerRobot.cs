using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Small animated 8-bit robot (angry eyes, teeth, horns) holding a danger sign,
    // shown above the script effects. Original pixel art drawn in code (no image files).
    // Two frames: the sign bobs, eyes and chest core blink alternately.
    public class PixelDangerRobot : UserControl
    {
        private const int Px = 3; // screen pixels per art pixel
        private const int ArtWidth = 33;
        private const int ArtHeight = 22;

        // . transparent  K outline  D dark metal  G body  S highlight  W teeth
        // E eyes  A chest core  Y yellow  B black  P pole
        private static readonly string[] Robot =
        {
            ".K............K.",
            ".KK..........KK.",
            "..KK.KKKKKK.KK..",
            "...KKDDDDDDKK...",
            "...KEEDDDDEEK...",
            "...KDEEDDEEDK...",
            "...KDDDDDDDDK...",
            "...KDWDWDWDWK...",
            "...KWKWKWKWDK...",
            "....KKKKKKKK....",
            "KK...KGGGGK...KK",
            "KGKKKKKKKKKKKKGK",
            "KGGGGGGGGGGGGGGK",
            "KGGSGGGAAGGGSGGK",
            "KGGGSGGAAGGSGGGK",
            "KKGGGGGGGGGGGGKK",
            ".KKKKKKKKKKKKKK.",
            "..KGGK....KGGK..",
            "..KGGK....KGGK..",
            ".KKGGKK..KKGGKK.",
            ".KSSSSK..KSSSSK.",
        };

        private static readonly string[] Sign =
        {
            "......K......",
            ".....KYK.....",
            ".....KYK.....",
            "....KYYYK....",
            "....KYBYK....",
            "...KYYBYYK...",
            "...KYYBYYK...",
            "..KYYYYYYYK..",
            "..KYYYBYYYK..",
            ".KYYYYYYYYYK.",
            "KKKKKKKKKKKKK",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
            "......P......",
        };

        private static readonly Dictionary<char, Color> Palette = new Dictionary<char, Color>
        {
            { 'K', Color.FromRgb(0x1E, 0x1E, 0x1E) },
            { 'D', Color.FromRgb(0x45, 0x4D, 0x55) },
            { 'G', Color.FromRgb(0x6B, 0x76, 0x80) },
            { 'S', Color.FromRgb(0x9A, 0xA4, 0xAE) },
            { 'W', Color.FromRgb(0xE8, 0xE8, 0xE8) },
            { 'Y', Color.FromRgb(0xFF, 0xC1, 0x07) },
            { 'B', Color.FromRgb(0x1E, 0x1E, 0x1E) },
            { 'P', Color.FromRgb(0x79, 0x55, 0x48) },
        };
        private static readonly Color EyeOn = Color.FromRgb(0xFF, 0x30, 0x00);
        private static readonly Color EyeOff = Color.FromRgb(0x70, 0x10, 0x00);

        private readonly Canvas[] _frames = new Canvas[2];
        private readonly DispatcherTimer _timer;
        private int _frame;

        public PixelDangerRobot()
        {
            var root = new Grid();
            for (int f = 0; f < 2; f++)
            {
                _frames[f] = BuildFrame(f);
                _frames[f].Visibility = f == 0 ? Visibility.Visible : Visibility.Collapsed;
                root.Children.Add(_frames[f]);
            }
            Content = root;
            Width = ArtWidth * Px;
            Height = ArtHeight * Px;
            SnapsToDevicePixels = true;

            _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(450) };
            _timer.Tick += (s, e) =>
            {
                _frames[_frame].Visibility = Visibility.Collapsed;
                _frame ^= 1;
                _frames[_frame].Visibility = Visibility.Visible;
            };
            Loaded += (s, e) => _timer.Start();
            Unloaded += (s, e) => _timer.Stop();
        }

        private static Canvas BuildFrame(int frame)
        {
            var canvas = new Canvas { Width = ArtWidth * Px, Height = ArtHeight * Px };
            DrawArt(canvas, Robot, 0, 1, frame);
            // the sign bobs up by one pixel in the second frame
            DrawArt(canvas, Sign, 19, 1, frame, frame == 0 ? 0 : 1);
            // arm from the shoulder to the pole, claw around it
            const int armY = 13;
            for (int x = 16; x < 25; x++) AddPixel(canvas, x, armY, Palette['G']);
            AddPixel(canvas, 24, armY - 1, Palette['K']);
            AddPixel(canvas, 25, armY - 1, Palette['K']);
            AddPixel(canvas, 24, armY + 1, Palette['K']);
            AddPixel(canvas, 25, armY + 1, Palette['K']);
            return canvas;
        }

        private static void DrawArt(Canvas canvas, string[] art, int offsetX, int offsetY, int frame, int lift = 0)
        {
            for (int y = 0; y < art.Length; y++)
            {
                for (int x = 0; x < art[y].Length; x++)
                {
                    char c = art[y][x];
                    if (c == '.') continue;
                    Color color;
                    if (c == 'E') color = frame == 0 ? EyeOn : EyeOff;
                    else if (c == 'A') color = frame == 0 ? EyeOff : EyeOn;
                    else color = Palette[c];
                    AddPixel(canvas, offsetX + x, offsetY + y - lift, color);
                }
            }
        }

        private static void AddPixel(Canvas canvas, int x, int y, Color color)
        {
            var r = new Rectangle { Width = Px, Height = Px, Fill = new SolidColorBrush(color) };
            Canvas.SetLeft(r, x * Px);
            Canvas.SetTop(r, y * Px);
            canvas.Children.Add(r);
        }
    }
}
