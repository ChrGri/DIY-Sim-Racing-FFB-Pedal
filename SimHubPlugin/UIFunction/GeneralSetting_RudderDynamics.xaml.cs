using System;
using System.Windows;
using System.Windows.Controls;

namespace DiyFfbPedal.UIFunction
{
    public partial class GeneralSetting_RudderDynamics : UserControl
    {
        public GeneralSetting_RudderDynamics()
        {
            InitializeComponent();
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(GeneralSetting_RudderDynamics),
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

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_RudderDynamics),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public DAP_config_st dap_config_st
        {
            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set => SetValue(DAP_Config_Property, value);
        }

        public void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    bool isHeli = (Settings.rudderMode == 1);

                    // Mode Adaptive sections visibility
                    if (panel_AirplaneDynamics != null) panel_AirplaneDynamics.Visibility = isHeli ? Visibility.Collapsed : Visibility.Visible;
                    if (panel_HeliDynamics != null) panel_HeliDynamics.Visibility = isHeli ? Visibility.Visible : Visibility.Collapsed;

                    if (lbl_DynamicsHeader != null)
                        lbl_DynamicsHeader.Content = isHeli ? "Helicopter Anti-Torque Dynamics" : "Flight Dynamics & Bilateral Coupling";

                    // Airplane Controls
                    if (Slider_CenteringDeadzone != null) Slider_CenteringDeadzone.SliderValue = Settings.rudderDeadzone;
                    if (Slider_RudderTrim != null) Slider_RudderTrim.SliderValue = Settings.rudderTrimOffset;
                    if (Slider_AeroQGain != null) Slider_AeroQGain.SliderValue = Settings.rudderAeroQGain;
                    if (chk_AeroQScaling != null) chk_AeroQScaling.IsChecked = Settings.rudderAeroQScaling;

                    // Helicopter Controls
                    if (Slider_HeliViscousDamping != null) Slider_HeliViscousDamping.SliderValue = Settings.rudderHeliDamping;
                    if (Slider_HeliCoulombFriction != null) Slider_HeliCoulombFriction.SliderValue = Settings.rudderHeliFriction;
                    if (Slider_HoverBias != null) Slider_HoverBias.SliderValue = Settings.rudderTrimOffset;

                    // Shared Bilateral Controls
                    if (Slider_PushPullSyncForce != null) Slider_PushPullSyncForce.SliderValue = Settings.rudderBilateralSyncForce;
                    if (Slider_VirtualPedalMass != null) Slider_VirtualPedalMass.SliderValue = (double)Settings.rudderVirtualPedalMass / 100.0;
                    if (Slider_SoftEndstopTravelRange != null) Slider_SoftEndstopTravelRange.SliderValue = Settings.rudderEndstopTravelRange;
                    if (Slider_SoftEndstopStiffness != null) Slider_SoftEndstopStiffness.SliderValue = Settings.rudderEndstopStiffness;
                }
            }
            catch { }
        }

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (d is GeneralSetting_RudderDynamics control && e.NewValue is DIYFFBPedalSettings)
            {
                control.updateUI();
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e) { }

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue) => SettingsChanged?.Invoke(this, newValue);

        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue) => ConfigChanged?.Invoke(this, newValue);

        private void Slider_CenteringDeadzone_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderDeadzone = (float)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_RudderTrim_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderTrimOffset = (float)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_AeroQGain_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderAeroQGain = (byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void chk_AeroQScaling_Checked(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderAeroQScaling = true;
            SettingsChangedEvent(Settings);
        }

        private void chk_AeroQScaling_Unchecked(object sender, RoutedEventArgs e)
        {
            if (Settings == null) return;
            Settings.rudderAeroQScaling = false;
            SettingsChangedEvent(Settings);
        }

        private void Slider_HeliViscousDamping_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderHeliDamping = (byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_HeliCoulombFriction_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderHeliFriction = (float)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_HoverBias_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderTrimOffset = (float)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_PushPullSyncForce_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderBilateralSyncForce = (float)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_VirtualPedalMassChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderVirtualPedalMass = (byte)(e.NewValue * 100);
            SettingsChangedEvent(Settings);
        }

        private void Slider_SoftEndstopTravelRangeChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderEndstopTravelRange = (byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }

        private void Slider_SoftEndstopStiffnessChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (Settings == null) return;
            Settings.rudderEndstopStiffness = (byte)e.NewValue;
            SettingsChangedEvent(Settings);
        }
    }
}
