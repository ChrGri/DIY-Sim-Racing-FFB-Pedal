using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    // Small animated 8-bit robot (boxy head, antennas, grinning teeth, glowing red eyes),
    // shown next to the script effects warning (its speech bubble). Original pixel art
    // drawn in code (no image files). Two frames: eyes, antenna tips and chest light pulse.
    public class PixelDangerRobot : UserControl
    {
        private const int Px = 3; // screen pixels per art pixel
        private const int ArtWidth = 17;
        private const int ArtHeight = 22;

        // . transparent  K outline  L face  G body  W teeth  Y yellow  B black  P pole
        // E eye core  e eye glow  A chest light  T antenna tip   (E, e, A, T pulse)
        private static readonly string[] Robot =
        {
            "..T...........T..",
            "...K.........K...",
            "....K.......K....",
            "..KKKKKKKKKKKKK..",
            "..KLLLLLLLLLLLK..",
            "..KLeeeLLLeeeLK..",
            "..KLeEeLLLeEeLK..",
            "..KLeeeLLLeeeLK..",
            "..KLLLLLLLLLLLK..",
            "..KLKWKWKWKWKLK..",
            "..KLWKWKWKWKWLK..",
            "..KKKKKKKKKKKKK..",
            ".....KKKKKKK.....",
            "KKKKKKKKKKKKKKKKK",
            "KGKGGGGGGGGGGGKGK",
            "KGKGGKKKKKKKGGKGK",
            "KGKGGKAAAAAKGGKGK",
            "KGKGGKKKKKKKGGKGK",
            "KGKGGGGGGGGGGGKGK",
            "KKKKKKKKKKKKKKKKK",
            "K.K..KGK.KGK..K.K",
            "....KKKK.KKKK....",
        };

        internal static readonly string[] Sign =
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
        };

        private static readonly Dictionary<char, Color> Palette = new Dictionary<char, Color>
        {
            { 'K', Color.FromRgb(0x16, 0x16, 0x16) },
            { 'L', Color.FromRgb(0xB8, 0xC0, 0xC8) },
            { 'G', Color.FromRgb(0x8E, 0x99, 0xA4) },
            { 'W', Color.FromRgb(0xF0, 0xF0, 0xF0) },
            { 'Y', Color.FromRgb(0xFF, 0xC1, 0x07) },
            { 'B', Color.FromRgb(0x16, 0x16, 0x16) },
            { 'P', Color.FromRgb(0x79, 0x55, 0x48) },
        };

        // pulsing colors per frame: eye core, eye glow, chest light, antenna tip
        private static readonly Dictionary<char, Color>[] FrameColors =
        {
            new Dictionary<char, Color>
            {
                { 'E', Color.FromRgb(0xFF, 0xEB, 0xC8) },
                { 'e', Color.FromRgb(0xFF, 0x28, 0x00) },
                { 'A', Color.FromRgb(0xFF, 0x28, 0x00) },
                { 'T', Color.FromRgb(0xFF, 0x28, 0x00) },
            },
            new Dictionary<char, Color>
            {
                { 'E', Color.FromRgb(0xFF, 0x78, 0x3C) },
                { 'e', Color.FromRgb(0xAA, 0x1E, 0x0A) },
                { 'A', Color.FromRgb(0x6E, 0x0F, 0x00) },
                { 'T', Color.FromRgb(0x5A, 0x14, 0x0A) },
            },
        };

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
            var canvas = new Canvas { Width = ArtWidth * Px, Height = ArtHeight * Px, ClipToBounds = true };
            DrawArt(canvas, Robot, 0, 0, frame);
            return canvas;
        }

        internal static void DrawArt(Canvas canvas, string[] art, int offsetX, int offsetY, int frame, int px = Px, int width = ArtWidth, int height = ArtHeight)
        {
            for (int y = 0; y < art.Length; y++)
            {
                for (int x = 0; x < art[y].Length; x++)
                {
                    char c = art[y][x];
                    if (c == '.') continue;
                    Color color;
                    if (!FrameColors[frame].TryGetValue(c, out color)) color = Palette[c];
                    AddPixel(canvas, offsetX + x, offsetY + y, color, px, width, height);
                }
            }
        }

        private static void AddPixel(Canvas canvas, int x, int y, Color color, int px, int width, int height)
        {
            if (x < 0 || y < 0 || x >= width || y >= height) return;
            var r = new Rectangle { Width = px, Height = px, Fill = new SolidColorBrush(color) };
            Canvas.SetLeft(r, x * px);
            Canvas.SetTop(r, y * px);
            canvas.Children.Add(r);
        }
    }

    // The danger sign from the robot's art as a small static icon (speech bubble title).
    public class PixelDangerSign : UserControl
    {
        private const int Px = 2;

        public PixelDangerSign()
        {
            int width = PixelDangerRobot.Sign[0].Length;
            int height = PixelDangerRobot.Sign.Length;
            var canvas = new Canvas { Width = width * Px, Height = height * Px };
            PixelDangerRobot.DrawArt(canvas, PixelDangerRobot.Sign, 0, 0, 0, Px, width, height);
            Content = canvas;
            Width = width * Px;
            Height = height * Px;
            SnapsToDevicePixels = true;
        }
    }
}
