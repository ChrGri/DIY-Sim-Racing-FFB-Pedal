using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DiyFfbPedal.UIFunction
{
    public partial class HomeDashboard : UserControl
    {
        private static readonly Brush DisconnectedBrush = CreateFrozenBrush(171, 180, 190);

        private readonly Queue<KeyValuePair<DateTime, double>>[] history =
        {
            new Queue<KeyValuePair<DateTime, double>>(),
            new Queue<KeyValuePair<DateTime, double>>(),
            new Queue<KeyValuePair<DateTime, double>>()
        };
        // newest sample that fell out of the 10 s window, used to interpolate the graph's left edge
        private readonly KeyValuePair<DateTime, double>?[] lastExpired = new KeyValuePair<DateTime, double>?[3];
        private readonly bool?[] lastConnected = new bool?[3];
        private readonly int[] lastPercent = { -1, -1, -1 };
        private readonly DispatcherTimer refreshTimer;
        private DIY_FFB_Pedal plugin;
        private DIYFFBPedalControlUI parent;
        private int refreshTicks;

        private static Brush CreateFrozenBrush(byte r, byte g, byte b)
        {
            SolidColorBrush brush = new SolidColorBrush(Color.FromRgb(r, g, b));
            brush.Freeze();
            return brush;
        }

        // Render priority lets the tick ride along with WPF's own render pass, so it
        // never fires faster than the display can actually draw (effectively capped at
        // the monitor's refresh rate) while still staying in step with it for smooth motion.
        private const int SummaryRefreshEveryNthTick = 15; // ~250ms of text/status refresh at a 60fps tick rate

        public HomeDashboard()
        {
            InitializeComponent();
            refreshTimer = new DispatcherTimer(DispatcherPriority.Render)
            {
                Interval = TimeSpan.FromMilliseconds(1000.0 / 60.0)
            };
            refreshTimer.Tick += RefreshTimer_Tick;
            Loaded += delegate { refreshTimer.Start(); };
            Unloaded += delegate { refreshTimer.Stop(); };
        }

        public void Initialize(DIY_FFB_Pedal pluginInstance, DIYFFBPedalControlUI parentControl)
        {
            plugin = pluginInstance;
            parent = parentControl;
            UpdateDashboard(true);
            refreshTimer.Start();
        }

        private void RefreshTimer_Tick(object sender, EventArgs e)
        {
            if (IsVisible) UpdateDashboard(++refreshTicks % SummaryRefreshEveryNthTick == 0);
        }

        private void UpdateDashboard(bool refreshSummary)
        {
            if (plugin == null || parent == null) return;
            if (refreshSummary)
            {
                string profile = plugin._calculations.ProfileEditing;
                if (String.IsNullOrWhiteSpace(profile)) profile = plugin.ProfileServicePlugin.CurrentGameProfile;
                ActiveProfile.Text = String.IsNullOrWhiteSpace(profile) ? "None" : profile;
                UpdateQuickButtons();
            }

            int connectedCount = 0;
            for (int pedal = 0; pedal < 3; pedal++)
            {
                bool connected = parent.IsHomePedalConnected(pedal);
                if (connected) connectedCount++;
                double percent = parent.GetHomePedalPercent(pedal);
                UpdatePedal(pedal, connected, percent, refreshSummary);
            }
            ConnectionSummary.Text = String.Format("{0} of 3 pedals connected", connectedCount);
        }

        private void UpdatePedal(int pedal, bool connected, double percent, bool refreshSummary)
        {
            TextBlock status = pedal == 0 ? Status0 : pedal == 1 ? Status1 : Status2;
            TextBlock value = pedal == 0 ? Value0 : pedal == 1 ? Value1 : Value2;
            ProgressBar bar = pedal == 0 ? Bar0 : pedal == 1 ? Bar1 : Bar2;
            Polyline graph = pedal == 0 ? Graph0 : pedal == 1 ? Graph1 : Graph2;
            TextBlock configText = pedal == 0 ? Config0 : pedal == 1 ? Config1 : Config2;
            TextBlock effectsText = pedal == 0 ? Effects0 : pedal == 1 ? Effects1 : Effects2;

            percent = Math.Max(0, Math.Min(100, percent));
            int roundedPercent = (int)Math.Round(percent);

            if (lastConnected[pedal] != connected)
            {
                status.Text = connected ? "Connected" : "Disconnected";
                status.Foreground = connected ? Brushes.LightGreen : DisconnectedBrush;
                lastConnected[pedal] = connected;
            }
            bar.Value = percent;
            if (lastPercent[pedal] != roundedPercent)
            {
                value.Text = String.Format("{0:0}%", roundedPercent);
                lastPercent[pedal] = roundedPercent;
            }

            DateTime now = DateTime.UtcNow;
            Queue<KeyValuePair<DateTime, double>> pedalHistory = history[pedal];
            pedalHistory.Enqueue(new KeyValuePair<DateTime, double>(now, percent));
            DateTime windowStart = now - TimeSpan.FromSeconds(10);
            while (pedalHistory.Count > 0 && pedalHistory.Peek().Key < windowStart) lastExpired[pedal] = pedalHistory.Dequeue();
            PointCollection points = new PointCollection(pedalHistory.Count + 1);
            // pin the line's left end to x = 0 so it doesn't jitter by up to one sample interval
            if (lastExpired[pedal].HasValue && pedalHistory.Count > 0)
            {
                KeyValuePair<DateTime, double> before = lastExpired[pedal].Value;
                KeyValuePair<DateTime, double> after = pedalHistory.Peek();
                double span = (after.Key - before.Key).TotalSeconds;
                double t = span > 0 ? (windowStart - before.Key).TotalSeconds / span : 1;
                double edgeValue = before.Value + (after.Value - before.Value) * t;
                points.Add(new Point(0, 72 - edgeValue * 0.66));
            }
            foreach (KeyValuePair<DateTime, double> sample in pedalHistory)
            {
                double x = 700 - (now - sample.Key).TotalSeconds * 70;
                // keep 0 % / 100 % 6 units inside the canvas so the lines clear the border's rounded corners
                points.Add(new Point(x, 72 - sample.Value * 0.66));
            }
            points.Freeze();
            graph.Points = points;

            if (!refreshSummary) return;
            payloadPedalConfig config = parent.dap_config_st[pedal].payloadPedalConfig_;
            configText.Text = String.Format("Max {0:0.#} kg · Travel {1}–{2}%", config.maxForce, config.pedalStartPosition, config.pedalEndPosition);
            effectsText.Text = "Effects: " + GetEffects(pedal);
        }

        private string GetEffects(int pedal)
        {
            List<string> names = new List<string>();
            DIYFFBPedalSettings settings = plugin.Settings;
            if (settings.ABS_enable_flag[pedal] == 1) names.Add("ABS");
            if (settings.RPM_enable_flag[pedal] == 1) names.Add("RPM");
            if (settings.G_force_enable_flag[pedal] == 1) names.Add("G-force");
            if (settings.WS_enable_flag[pedal] == 1) names.Add("Slip");
            if (settings.Road_impact_enable_flag[pedal] == 1) names.Add("Road");
            if (settings.CV1_enable_flag[pedal]) names.Add("Custom 1");
            if (settings.CV2_enable_flag[pedal]) names.Add("Custom 2");
            if (settings.CV3_enable_flag[pedal]) names.Add("Custom 3");
            if (settings.CV4_enable_flag[pedal]) names.Add("Custom 4");
            return names.Count == 0 ? "none" : String.Join(", ", names.ToArray());
        }

        private Button GetQuickButton(int index)
        {
            return index == 0 ? Quick0 : index == 1 ? Quick1 : index == 2 ? Quick2 : index == 3 ? Quick3 : index == 4 ? Quick4 : Quick5;
        }

        private void UpdateQuickButtons()
        {
            int count = plugin.ProfileServicePlugin.ProfileList == null ? 0 : plugin.ProfileServicePlugin.ProfileList.Count;
            for (int i = 0; i < 6; i++)
            {
                Button button = GetQuickButton(i);
                button.Visibility = i < count ? Visibility.Visible : Visibility.Hidden;
                button.IsEnabled = i < count;
                if (i < count)
                {
                    ProfileListItem item = plugin.ProfileServicePlugin.ProfileList[i];
                    string name = String.IsNullOrWhiteSpace(item.ListNameOrig) ? item.FileName : item.ListNameOrig;
                    button.Content = name;
                }
            }
        }

        private void QuickApply_Click(object sender, RoutedEventArgs e)
        {
            if (plugin == null || parent == null) return;
            int index = Convert.ToInt32(((Button)sender).Tag);
            if (plugin.ProfileServicePlugin.ProfileList == null || index >= plugin.ProfileServicePlugin.ProfileList.Count) return;
            ProfileListItem item = plugin.ProfileServicePlugin.ProfileList[index];
            try
            {
                plugin.ProfileServicePlugin.ApplyProfile(item.FullPath);
                plugin._calculations.ProfileEditing = item.FileName;
                plugin._calculations.ProfileSelected = item.FileName;
                parent.SystemProfile_TabNew.ApplyProfileOnUiWithPath(item.FullPath);
                parent.updateTheGuiFromConfig();
                ApplyFeedback.Text = String.Format("{0} applied · settings and effects updated", String.IsNullOrWhiteSpace(item.ListNameOrig) ? item.FileName : item.ListNameOrig);
                UpdateDashboard(true);
            }
            catch (Exception ex)
            {
                ApplyFeedback.Text = "Profile could not be applied: " + ex.Message;
            }
        }

        private void OpenPedals_Click(object sender, RoutedEventArgs e) { parent.ShowHomeTarget(false); }
        private void OpenSystem_Click(object sender, RoutedEventArgs e) { parent.ShowHomeTarget(true); }
    }
}
