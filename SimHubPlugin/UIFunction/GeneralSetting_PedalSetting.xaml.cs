using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// GeneralSetting_PedalSetting.xaml 的互動邏輯
    /// </summary>
    public partial class GeneralSetting_PedalSetting : System.Windows.Controls.UserControl
    {
        public GeneralSetting_PedalSetting()
        {
            InitializeComponent();
            dap_config_st = default; 
        }
        
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(GeneralSetting_PedalSetting),
            new FrameworkPropertyMetadata(new DAP_config_st(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnPropertyChanged));
        

        public DAP_config_st dap_config_st
        {
            
            get => (DAP_config_st)GetValue(DAP_Config_Property);
            set
            {
                SetValue(DAP_Config_Property, value);
            }
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(GeneralSetting_PedalSetting),
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

        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(GeneralSetting_PedalSetting),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Cauculation_Property);
            set
            {
                SetValue(Cauculation_Property, value);
                //updateUI();
            }
        }

        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {

        }
        private void updateUI()
        {
            try 
            {
                if (Settings != null)
                {
                    if (Settings.advanced_b)
                    {
                        Slider_LC_rate.TickFrequency = 1;
                    }
                    else
                    {
                        Slider_LC_rate.TickFrequency = 10;
                    }
                }
            }
            catch 
            {  
            }
        }
        private static void OnPropertyChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            //UI update here
            var control = d as GeneralSetting_PedalSetting;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                //slider

                if (control != null)
                {
                    try
                    {
                        control.Slider_damping.SliderValue = (double)(newData.payloadPedalConfig_.dampingPress * (double)control.Slider_damping.TickFrequency);
                        control.Slider_LC_rate.SliderValue = newData.payloadPedalConfig_.loadcell_rating * 2;
                        control.Slider_maxgame_output.SliderValue = newData.payloadPedalConfig_.maxGameOutput;
                    }
                    catch (Exception caughtEx)
                    {
                    }
                    
                    //combobox
                    try
                    {
                        if (newData.payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                        {
                            control.ComboboxPitchSelection.SelectedIndex = 5;
                        }
                        else
                        {
                            control.ComboboxPitchSelection.SelectedIndex = newData.payloadPedalConfig_.spindlePitch_mmPerRev_u8;
                        }
                    }
                    catch (Exception caughtEx)
                    {
                    }
                    try
                    {
                        //checkbox
                        control.CheckBox_JoystickOutput.IsChecked = newData.payloadPedalConfig_.travelAsJoystickOutput_u8 == 1;
                        control.CheckBox_InvertLoadcellReading.IsChecked = newData.payloadPedalConfig_.invertLoadcellReading_u8 == 1;
                        control.CheckBox_InvertMotorDir.IsChecked = newData.payloadPedalConfig_.invertMotorDirection_u8 == 1;
                        if (((newData.payloadPedalConfig_.stepLossFunctionFlags_u8 >> 0) & 1) == 1)
                        {
                            control.CheckBox_StepLossRecov.IsChecked = true;
                        }
                        else
                        {
                            control.CheckBox_StepLossRecov.IsChecked = false;
                        }
                        if (((newData.payloadPedalConfig_.stepLossFunctionFlags_u8 >> 1) & 1) == 1)
                        {
                            control.CheckBox_CrashDetection.IsChecked = true;
                        }
                        else
                        {
                            control.CheckBox_CrashDetection.IsChecked = false;
                        }
                    }
                    catch (Exception caughtEx)
                    {                         
                    }

                }
            }

        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as EffectsTab_ABS;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
                }
                catch
                {
                }
            }
        }

        public event EventHandler<CalculationVariables> CalculationChanged;
        protected void CalculationChangedEvent(CalculationVariables newValue)
        {
            CalculationChanged?.Invoke(this, newValue);
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
        private void ComboboxPitchSelection_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            byte pitch_last = dap_config_st.payloadPedalConfig_.spindlePitch_mmPerRev_u8;
            if (pitch_last != (byte)ComboboxPitchSelection.SelectedIndex)
            {
                var tmp = dap_config_st;
                tmp.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = (byte)ComboboxPitchSelection.SelectedIndex;
                if (tmp.payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                {
                    tmp.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
                    //ComboboxPitchSelection.SelectedIndex = 5;
                }
                calculation.IsUIRefreshNeeded = true;
                dap_config_st = tmp;
                ConfigChangedEvent(dap_config_st);
            }


        }

        private void CheckBox_StepLossRecov_Checked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.stepLossFunctionFlags_u8 |= (1 << 0);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
        private void CheckBox_StepLossRecov_Unchecked(object sender, RoutedEventArgs e)
        {
            
            var tmp_class = dap_config_st;
            byte tmp = dap_config_st.payloadPedalConfig_.stepLossFunctionFlags_u8;
            tmp = (byte)(tmp & ~(1 << 0));
            tmp_class.payloadPedalConfig_.stepLossFunctionFlags_u8 = tmp;
            dap_config_st = tmp_class;
            ConfigChangedEvent(dap_config_st);
        }
        private void CheckBox_CrashDetection_Checked(object sender, RoutedEventArgs e)
        {
            var tmp_class = dap_config_st;
            tmp_class.payloadPedalConfig_.stepLossFunctionFlags_u8 |= (1 << 1);
            dap_config_st = tmp_class;
            ConfigChangedEvent(dap_config_st);
        }
        private void CheckBox_CrashDetection_Unchecked(object sender, RoutedEventArgs e)
        {

            byte tmp = dap_config_st.payloadPedalConfig_.stepLossFunctionFlags_u8;
            tmp = (byte)(tmp & ~(1 << 1));
            var tmp_class = dap_config_st;
            tmp_class.payloadPedalConfig_.stepLossFunctionFlags_u8 = tmp;
            dap_config_st = tmp_class;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_damping_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.dampingPress = (Byte)(e.NewValue / (double)Slider_damping.TickFrequency);
            tmp.payloadPedalConfig_.dampingPull = (Byte)(e.NewValue / (double)Slider_damping.TickFrequency);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_LC_rate_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.loadcell_rating = (Byte)(e.NewValue / 2);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void Slider_maxgame_output_SliderValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.maxGameOutput = (Byte)(e.NewValue);
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }



        private void CheckBox_JoystickOutput_Checked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.travelAsJoystickOutput_u8 = (byte)1;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void CheckBox_JoystickOutput_Unchecked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.travelAsJoystickOutput_u8 = (byte)0;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void CheckBox_InvertLoadcellReading_Checked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.invertLoadcellReading_u8 = (byte)1;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void CheckBox_InvertLoadcellReading_Unchecked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.invertLoadcellReading_u8 = (byte)0;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void CheckBox_InvertMotorDir_Checked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.invertMotorDirection_u8 = (byte)1;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }

        private void CheckBox_InvertMotorDir_Unchecked(object sender, RoutedEventArgs e)
        {
            var tmp = dap_config_st;
            tmp.payloadPedalConfig_.invertMotorDirection_u8 = (byte)1;
            dap_config_st = tmp;
            ConfigChangedEvent(dap_config_st);
        }
    }
}

