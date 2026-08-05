using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;
using DiyFfbPedal.UIElement;

namespace DiyFfbPedal.UIFunction
{
    public partial class GeneralSetting_RudderDynamics : UserControl
    {
        // 雙向更新的防護鎖
        private bool _isUpdatingUIFromConfig = false;
        private bool _isUpdatingConfigFromUI = false;

        public GeneralSetting_RudderDynamics()
        {
            InitializeComponent();
            this.Loaded += GeneralSetting_Loaded; // 註冊載入完成事件
        }

        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_RudderDynamics),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(GeneralSetting_RudderDynamics),
            new FrameworkPropertyMetadata(new DIYFFBPedalSettings(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnSettingsChanged));

        public double PosSmoothing_value { get; set; }

        public DAP_config_st dap_config_st
        {
            get { return (DAP_config_st)GetValue(DAP_Config_Property); }
            set { SetValue(DAP_Config_Property, value); }
        }

        public DIYFFBPedalSettings Settings
        {
            get => (DIYFFBPedalSettings)GetValue(Settings_Property);
            set
            {
                SetValue(Settings_Property, value);
                updateUI();
            }
        }

        public event EventHandler<DAP_config_st> ConfigChanged;
        protected void ConfigChangedEvent(DAP_config_st newValue)
        {
            ConfigChanged?.Invoke(this, newValue);
        }

        public event EventHandler<DIYFFBPedalSettings> SettingsChanged;
        protected void SettingsChangedEvent(DIYFFBPedalSettings newValue)
        {
            SettingsChanged?.Invoke(this, newValue);
        }

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as GeneralSetting_RudderDynamics;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                control.updateUI();
            }
        }

        private void updateUI()
        {
            try
            {
                if (Settings != null)
                {
                    _isUpdatingUIFromConfig = true;

                    if (Slider_VirtualPedalMass != null) Slider_VirtualPedalMass.SliderValue = (double)Settings.rudderVirtualPedalMass * Slider_VirtualPedalMass.TickFrequency;
                    if (Slider_CoulombFriction != null) Slider_CoulombFriction.SliderValue = (double)Settings.rudderCoulombFriction * Slider_CoulombFriction.TickFrequency;
                    if (Slider_VirtualDamping != null) Slider_VirtualDamping.SliderValue = (double)Settings.rudderVirtualDamping * Slider_VirtualDamping.TickFrequency;
                    if (Slider_DampingProgression != null) Slider_DampingProgression.SliderValue = (double)Settings.rudderDampingProgression * Slider_DampingProgression.TickFrequency;
                    if (Slider_SoftEndstopTravelRange != null) Slider_SoftEndstopTravelRange.SliderValue = (double)Settings.rudderEndstopTravelRange * Slider_SoftEndstopTravelRange.TickFrequency;
                    if (Slider_SoftEndstopStiffness != null) Slider_SoftEndstopStiffness.SliderValue = (double)Settings.rudderEndstopStiffness * Slider_SoftEndstopStiffness.TickFrequency;
                    
                    _isUpdatingUIFromConfig = false;
                }
            }
            catch
            {
                _isUpdatingUIFromConfig = false;
                _isUpdatingConfigFromUI = false;
            }
        }

        // 畫面載入完成時，若設定檔為空，則將 XAML 的預設值寫入設定檔中
        private void GeneralSetting_Loaded(object sender, RoutedEventArgs e)
        {
            if (DesignerProperties.GetIsInDesignMode(this)) return;

            if (dap_config_st.payloadPedalConfig_.virtualPedalMass_u8 == 0)
            {
                _isUpdatingConfigFromUI = true;
                try
                {
                    var tmp = dap_config_st;
                    if (Slider_VirtualPedalMass != null) tmp.payloadPedalConfig_.virtualPedalMass_u8 = (Byte)(Slider_VirtualPedalMass.SliderValue / Slider_VirtualPedalMass.TickFrequency);
                    if (Slider_CoulombFriction != null) tmp.payloadPedalConfig_.coulombFrictionIn0p1N_u8 = (Byte)(Slider_CoulombFriction.SliderValue / Slider_CoulombFriction.TickFrequency);
                    if (Slider_VirtualDamping != null) tmp.payloadPedalConfig_.virtualPedalDamping_u8 = (Byte)(Slider_VirtualDamping.SliderValue / Slider_VirtualDamping.TickFrequency);
                    if (Slider_DampingProgression != null) tmp.payloadPedalConfig_.dampingProgression_u8 = (Byte)(Slider_DampingProgression.SliderValue / Slider_DampingProgression.TickFrequency);
                    if (Slider_SoftEndstopTravelRange != null) tmp.payloadPedalConfig_.endstopTravelRange_mm_u8 = (Byte)(Slider_SoftEndstopTravelRange.SliderValue / Slider_SoftEndstopTravelRange.TickFrequency);
                    if (Slider_SoftEndstopStiffness != null) tmp.payloadPedalConfig_.endstopStiffness_kg_mm_u8 = (Byte)(Slider_SoftEndstopStiffness.SliderValue / Slider_SoftEndstopStiffness.TickFrequency);

                    dap_config_st = tmp;
                    ConfigChangedEvent(dap_config_st);

                    if (Settings != null)
                    {
                        Settings.rudderVirtualPedalMass = tmp.payloadPedalConfig_.virtualPedalMass_u8;
                        Settings.rudderCoulombFriction = tmp.payloadPedalConfig_.coulombFrictionIn0p1N_u8;
                        Settings.rudderVirtualDamping = tmp.payloadPedalConfig_.virtualPedalDamping_u8;
                        Settings.rudderDampingProgression = tmp.payloadPedalConfig_.dampingProgression_u8;
                        Settings.rudderEndstopTravelRange = tmp.payloadPedalConfig_.endstopTravelRange_mm_u8;
                        Settings.rudderEndstopStiffness = tmp.payloadPedalConfig_.endstopStiffness_kg_mm_u8;
                        SettingsChangedEvent(Settings);
                    }
                }
                finally
                {
                    _isUpdatingConfigFromUI = false;
                }
            }
        }

        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            if (DesignerProperties.GetIsInDesignMode(d)) return;

            if (d is GeneralSetting_RudderDynamics control && e.NewValue is DAP_config_st newData)
            {
                if (control._isUpdatingConfigFromUI) return;
                if (newData.payloadPedalConfig_.virtualPedalMass_u8 == 0) return;

                control._isUpdatingUIFromConfig = true;
                try
                {
                    if (control.Slider_VirtualPedalMass != null)
                        control.Slider_VirtualPedalMass.SliderValue = (double)newData.payloadPedalConfig_.virtualPedalMass_u8 * control.Slider_VirtualPedalMass.TickFrequency;

                    if (control.Slider_CoulombFriction != null)
                        control.Slider_CoulombFriction.SliderValue = (double)newData.payloadPedalConfig_.coulombFrictionIn0p1N_u8 * control.Slider_CoulombFriction.TickFrequency;

                    if (control.Slider_VirtualDamping != null)
                        control.Slider_VirtualDamping.SliderValue = (double)newData.payloadPedalConfig_.virtualPedalDamping_u8 * control.Slider_VirtualDamping.TickFrequency;

                    if (control.Slider_DampingProgression != null)
                        control.Slider_DampingProgression.SliderValue = (double)newData.payloadPedalConfig_.dampingProgression_u8 * control.Slider_DampingProgression.TickFrequency;

                    if (control.Slider_SoftEndstopTravelRange != null)
                        control.Slider_SoftEndstopTravelRange.SliderValue = (double)newData.payloadPedalConfig_.endstopTravelRange_mm_u8 * control.Slider_SoftEndstopTravelRange.TickFrequency;

                    if (control.Slider_SoftEndstopStiffness != null)
                        control.Slider_SoftEndstopStiffness.SliderValue = (double)newData.payloadPedalConfig_.endstopStiffness_kg_mm_u8 * control.Slider_SoftEndstopStiffness.TickFrequency;
                }
                finally
                {
                    control._isUpdatingUIFromConfig = false;
                }
            }
        }

        private void Slider_VirtualPedalMassChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_VirtualPedalMass.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.virtualPedalMass_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderVirtualPedalMass = tmp.payloadPedalConfig_.virtualPedalMass_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        private void Slider_CoulombFrictionChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_CoulombFriction.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.coulombFrictionIn0p1N_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderCoulombFriction = tmp.payloadPedalConfig_.coulombFrictionIn0p1N_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        public void Slider_VirtualPedalDampingChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_VirtualDamping.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.virtualPedalDamping_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderVirtualDamping = tmp.payloadPedalConfig_.virtualPedalDamping_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        public void Slider_DampingProgressionChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_DampingProgression.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.dampingProgression_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderDampingProgression = tmp.payloadPedalConfig_.dampingProgression_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        public void Slider_SoftEndstopTravelRangeChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_SoftEndstopTravelRange.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.endstopTravelRange_mm_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderEndstopTravelRange = tmp.payloadPedalConfig_.endstopTravelRange_mm_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        public void Slider_SoftEndstopStiffnessChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (DesignerProperties.GetIsInDesignMode(this) || !this.IsLoaded) return;
            if (_isUpdatingUIFromConfig) return;

            _isUpdatingConfigFromUI = true;
            try
            {
                double targetValue_fl64 = e.NewValue / (double)Slider_SoftEndstopStiffness.TickFrequency;
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.endstopStiffness_kg_mm_u8 = (Byte)(targetValue_fl64);
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);

                if (Settings != null)
                {
                    Settings.rudderEndstopStiffness = tmp.payloadPedalConfig_.endstopStiffness_kg_mm_u8;
                    SettingsChangedEvent(Settings);
                }
            }
            finally
            {
                _isUpdatingConfigFromUI = false;
            }
        }

        private void Slider_DampingProgression_Loaded(object sender, RoutedEventArgs e)
        {
            // 保留原本的空事件防報錯
        }
    }
}
