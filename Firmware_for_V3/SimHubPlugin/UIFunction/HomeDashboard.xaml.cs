using System;
using System.Collections.Generic;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace User.PluginSdkDemo.UIFunction
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
            refreshTimer = new DispatcherTimer(DispatcherPriority.Render) { Interval = TimeSpan.FromMilliseconds(1000.0 / 60.0) };
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
                uint slot = plugin._calculations.profile_index > 5 ? 5 : plugin._calculations.profile_index;
                string name = plugin.Settings.Profile_name[slot];
                ActiveProfile.Text = String.Format("Slot {0}{1}", (char)('A' + slot), String.IsNullOrWhiteSpace(name) ? "" : " · " + name);
                UpdateQuickButtons();
            }
            int connectedCount = 0;
            for (int pedal = 0; pedal < 3; pedal++)
            {
                bool connected = parent.IsHomePedalConnected(pedal);
                if (connected) connectedCount++;
                UpdatePedal(pedal, connected, parent.GetHomePedalPercent(pedal), refreshSummary);
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
            while (pedalHistory.Count > 0 && now - pedalHistory.Peek().Key > TimeSpan.FromSeconds(10)) pedalHistory.Dequeue();
            PointCollection points = new PointCollection(pedalHistory.Count);
            foreach (KeyValuePair<DateTime, double> sample in pedalHistory) points.Add(new Point(700 - (now - sample.Key).TotalSeconds * 70, 78 - sample.Value * 0.78));
            points.Freeze();
            graph.Points = points;
            if (!refreshSummary) return;
            payloadPedalConfig config = DIYFFBPedalControlUI.dap_config_st[pedal].payloadPedalConfig_;
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
            return names.Count == 0 ? "none" : String.Join(", ", names.ToArray());
        }

        private Button GetQuickButton(int index) { return index == 0 ? Quick0 : index == 1 ? Quick1 : index == 2 ? Quick2 : index == 3 ? Quick3 : index == 4 ? Quick4 : Quick5; }

        private void UpdateQuickButtons()
        {
            for (int slot = 0; slot < 6; slot++)
            {
                string name = plugin.Settings.Profile_name[slot];
                GetQuickButton(slot).Content = String.Format("{0} Apply", String.IsNullOrWhiteSpace(name) ? "Slot " + (char)('A' + slot) : name);
            }
        }

        private void QuickApply_Click(object sender, RoutedEventArgs e)
        {
            if (plugin == null || parent == null) return;
            uint slot = Convert.ToUInt32(((Button)sender).Tag);
            int enabled = 0;
            for (int pedal = 0; pedal < 3; pedal++)
            {
                if (plugin.Settings.file_enable_check[slot, pedal] != 1) continue;
                enabled++;
                string path = plugin.Settings.Pedal_file_string[slot, pedal];
                if (String.IsNullOrWhiteSpace(path) || !File.Exists(path))
                {
                    ApplyFeedback.Text = String.Format("Slot {0}: a pedal config file is missing. Check System > Profiles.", (char)('A' + slot));
                    return;
                }
            }
            if (enabled == 0)
            {
                ApplyFeedback.Text = String.Format("Slot {0} has no enabled pedal files. Set it up in System > Profiles.", (char)('A' + slot));
                return;
            }
            try
            {
                plugin._calculations.profile_index = slot;
                parent.Profile_change(slot);
                parent.Sendconfigtopedal_shortcut();
                string name = plugin.Settings.Profile_name[slot];
                ApplyFeedback.Text = String.Format("{0} applied · {1} pedal(s) updated", String.IsNullOrWhiteSpace(name) ? "Slot " + (char)('A' + slot) : name, enabled);
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
