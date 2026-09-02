using System;
using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;

namespace DiyFfbPedal.UIFunction
{
    public partial class CurveTab_Rudder : UserControl
    {
        public CurveTab_Rudder()
        {
            InitializeComponent();
            Update_BrakeForceCurve();
            UpdateLiveDeflection(0.5f);
        }

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(CurveTab_Rudder),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public DAP_config_st dap_config_st
        {
            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set => SetValue(DAP_Config_Property, value);
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(CurveTab_Rudder),
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

        public static readonly DependencyProperty Calculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(CurveTab_Rudder),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Calculation_Property);
            set => SetValue(Calculation_Property, value);
        }

        public void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    bool isHeli = (Settings.rudderMode == 1);

                    // Toggle mode-specific visualization elements
                    if (panel_AirplaneProfiles != null) panel_AirplaneProfiles.Visibility = isHeli ? Visibility.Collapsed : Visibility.Visible;
                    if (border_HeliBadge != null) border_HeliBadge.Visibility = isHeli ? Visibility.Visible : Visibility.Collapsed;
                    if (border_HeliOverlay != null) border_HeliOverlay.Visibility = isHeli ? Visibility.Visible : Visibility.Collapsed;

                    if (txt_HeliDampingDisplay != null) txt_HeliDampingDisplay.Text = $"Viscous Damping: {Settings.rudderHeliDamping}%";
                    if (txt_HeliFrictionDisplay != null) txt_HeliFrictionDisplay.Text = $"Coulomb Friction: {Math.Round(Settings.rudderHeliFriction, 1)} N";

                    if (Rangeslider_rudder_force_range != null)
                    {
                        Rangeslider_rudder_force_range.UpperValue = Settings.rudderCenteringForce;
                        Rangeslider_rudder_force_range.LowerValue = 0;
                    }

                    if (Rangeslider_rudder_travel_range != null)
                    {
                        Rangeslider_rudder_travel_range.LowerValue = Settings.rudderMinTravel;
                        Rangeslider_rudder_travel_range.UpperValue = Settings.rudderMaxTravel;
                    }

                    if (Label_min_pos_rudder != null) Label_min_pos_rudder.Content = $"MIN\n{Settings.rudderMinTravel}%";
                    if (Label_max_pos_rudder != null) Label_max_pos_rudder.Content = $"MAX\n{Settings.rudderMaxTravel}%";
                    if (Label_max_force_rudder != null) Label_max_force_rudder.Content = $"Max Force:\n{Math.Round(Settings.rudderCenteringForce, 1)} kg";

                    // Update profile button highlighting
                    HighlightActiveProfileButton();

                    Update_BrakeForceCurve();
                }
            }
            catch { }
        }

        private void HighlightActiveProfileButton()
        {
            if (btn_linearcurve_rudder == null || btn_progressive_rudder == null || btn_Scurve_rudder == null || Settings == null) return;

            btn_linearcurve_rudder.Opacity = (Settings.rudderCenteringProfile == 0) ? 1.0 : 0.6;
            btn_progressive_rudder.Opacity = (Settings.rudderCenteringProfile == 1) ? 1.0 : 0.6;
            btn_Scurve_rudder.Opacity = (Settings.rudderCenteringProfile == 2) ? 1.0 : 0.6;
        }

        public void Update_BrakeForceCurve()
        {
            try
            {
                if (canvas_rudder_curve == null || Polyline_RudderForceCurve == null || polygonCurveBackground == null || Settings == null) return;

                double canvasW = canvas_rudder_curve.Width;  // 430
                double canvasH = canvas_rudder_curve.Height; // 200
                double centerX = canvasW / 2.0;

                // Adjust center marker for trim offset
                double trimOffsetNormalized = (Settings.rudderTrimOffset / 100.0); // -0.5 to +0.5
                double effectiveCenterX = centerX + (trimOffsetNormalized * canvasW);
                effectiveCenterX = Math.Max(20, Math.Min(canvasW - 20, effectiveCenterX));

                if (line_CenterTrim != null)
                {
                    line_CenterTrim.X1 = effectiveCenterX;
                    line_CenterTrim.X2 = effectiveCenterX;
                }

                PointCollection curvePoints = new PointCollection();
                PointCollection polygonPoints = new PointCollection();

                if (Settings.rudderMode == 1)
                {
                    // Helicopter Mode: Zero Return Spring (flat at baseline)
                    curvePoints.Add(new Point(0, canvasH - 2));
                    curvePoints.Add(new Point(canvasW, canvasH - 2));

                    polygonPoints.Add(new Point(0, canvasH));
                    polygonPoints.Add(new Point(0, canvasH - 2));
                    polygonPoints.Add(new Point(canvasW, canvasH - 2));
                    polygonPoints.Add(new Point(canvasW, canvasH));
                }
                else
                {
                    // Airplane Mode: Symmetric Bipolar Centering Spring Curve
                    int samplePoints = 40;
                    double maxForce = Math.Max(Settings.rudderCenteringForce, 1.0);
                    double deadzone = Settings.rudderDeadzone / 100.0; // 0.0 to 0.05
                    uint profile = Settings.rudderCenteringProfile;     // 0=Linear, 1=Progressive, 2=S-Curve

                    polygonPoints.Add(new Point(0, canvasH));

                    for (int i = 0; i <= samplePoints; i++)
                    {
                        double x = (double)i / samplePoints * canvasW;
                        double normalizedX = (x - effectiveCenterX) / (canvasW / 2.0); // -1.0 to +1.0 relative to trim center
                        double absNormX = Math.Abs(normalizedX);

                        // Apply Deadzone
                        double effectiveU = 0.0;
                        if (absNormX > deadzone)
                        {
                            effectiveU = (absNormX - deadzone) / Math.Max(0.01, 1.0 - deadzone);
                            effectiveU = Math.Min(1.0, Math.Max(0.0, effectiveU));
                        }

                        // Apply Curve Profile
                        double forceFraction = 0.0;
                        if (profile == 0)
                        {
                            // Linear
                            forceFraction = effectiveU;
                        }
                        else if (profile == 1)
                        {
                            // Progressive (Cubic/Exponential ramp)
                            forceFraction = Math.Pow(effectiveU, 1.8);
                        }
                        else if (profile == 2)
                        {
                            // S-Curve (Sigmoid)
                            forceFraction = 0.5 * (1.0 - Math.Cos(effectiveU * Math.PI));
                        }

                        double y = canvasH - (forceFraction * (canvasH - 10)) - 2;
                        curvePoints.Add(new Point(x, y));
                        polygonPoints.Add(new Point(x, y));
                    }

                    polygonPoints.Add(new Point(canvasW, canvasH));
                }

                Polyline_RudderForceCurve.Points = curvePoints;
                polygonCurveBackground.Points = polygonPoints;
            }
            catch { }
        }

        public void UpdateLiveDeflection(float positionRatio01)
        {
            try
            {
                if (canvas_rudder_curve == null || line_LiveDeflection == null) return;

                double canvasW = canvas_rudder_curve.Width;
                if (canvasW <= 0 || double.IsNaN(canvasW)) canvasW = 430.0;
                double markerX = Math.Max(0, Math.Min(canvasW, positionRatio01 * canvasW));
                line_LiveDeflection.X1 = markerX;
                line_LiveDeflection.X2 = markerX;
            }
            catch { }
        }

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is CurveTab_Rudder control && e.NewValue is DIYFFBPedalSettings)
            {
                control.updateUI();
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) { }

        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            // Calculation updates do not override live differential rudder deflection
        }

        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue) => ConfigChanged?.Invoke(this, newValue);

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue) => SettingsChanged?.Invoke(this, newValue);

        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue) => CalculationChanged?.Invoke(this, newValue);

        private void btn_linearcurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderCenteringProfile = 0;
            HighlightActiveProfileButton();
            Update_BrakeForceCurve();
            SettingsChangedEvent(Settings);
        }

        private void btn_progressive_rudder_Click(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderCenteringProfile = 1;
            HighlightActiveProfileButton();
            Update_BrakeForceCurve();
            SettingsChangedEvent(Settings);
        }

        private void btn_Scurve_rudder_Click(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderCenteringProfile = 2;
            HighlightActiveProfileButton();
            Update_BrakeForceCurve();
            SettingsChangedEvent(Settings);
        }

        private void Rangeslider_rudder_force_range_UpperValueChanged(object sender, RoutedEventArgs e)
        {
            if (Settings == null || Rangeslider_rudder_force_range == null) return;
            Settings.rudderCenteringForce = (float)Rangeslider_rudder_force_range.UpperValue;
            Settings.rudderMaxForce = Settings.rudderCenteringForce;
            if (Label_max_force_rudder != null) Label_max_force_rudder.Content = $"Max Force:\n{Math.Round(Settings.rudderCenteringForce, 1)} kg";
            Update_BrakeForceCurve();
            SettingsChangedEvent(Settings);
        }

        private void Rangeslider_rudder_force_range_LowerValueChanged(object sender, RoutedEventArgs e) { }

        private void Rangeslider_rudder_travel_range_LowerValueChanged(object sender, RoutedEventArgs e)
        {
            if (Settings == null || Rangeslider_rudder_travel_range == null) return;
            Settings.rudderMinTravel = (byte)Rangeslider_rudder_travel_range.LowerValue;
            if (Label_min_pos_rudder != null) Label_min_pos_rudder.Content = $"MIN\n{Settings.rudderMinTravel}%";
            SettingsChangedEvent(Settings);
        }

        private void Rangeslider_rudder_travel_range_UpperValueChanged(object sender, RoutedEventArgs e)
        {
            if (Settings == null || Rangeslider_rudder_travel_range == null) return;
            Settings.rudderMaxTravel = (byte)Rangeslider_rudder_travel_range.UpperValue;
            if (Label_max_pos_rudder != null) Label_max_pos_rudder.Content = $"MAX\n{Settings.rudderMaxTravel}%";
            SettingsChangedEvent(Settings);
        }
         private void btn_RudderDocs_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                System.Diagnostics.Process.Start(new System.Diagnostics.ProcessStartInfo("https://github.com/ChrGri/DIY-Sim-Racing-FFB-Pedal/blob/rudder_test/docs/rudder_modes_flight_dynamics.md") { UseShellExecute = true });
            }
            catch { }
        }
    }
}