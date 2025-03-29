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
using WoteverCommon;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// InfoSection_System.xaml 的互動邏輯
    /// </summary>
    public partial class InfoSection_System : UserControl
    {
        public InfoSection_System()
        {
            InitializeComponent();
            //Label_RSSI.Visibility = Visibility.Hidden;
        }
        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(InfoSection_System),
            new FrameworkPropertyMetadata(new CalculationVariables(), FrameworkPropertyMetadataOptions.BindsTwoWayByDefault, OnCalculationChanged));

        public CalculationVariables calculation
        {
            get => (CalculationVariables)GetValue(Cauculation_Property);
            set
            {
                SetValue(Cauculation_Property, value);
                updateUI();
            }
        }

        public static readonly DependencyProperty Settings_Property = DependencyProperty.Register(
            nameof(Settings),
            typeof(DIYFFBPedalSettings),
            typeof(InfoSection_System),
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
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as InfoSection_System;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
                    control.updateUI();
                }
                catch
                {

                }
            }
        }
        private static void OnSettingsChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as InfoSection_System;
            if (control != null && e.NewValue is DIYFFBPedalSettings newData)
            {
                try
                {
                    control.updateUI();
                }
                catch
                {
                }
            }

        }
        private void updateUI()
        { 
            if(info_label_system!=null) info_label_system.Content= "Bridge:\nDAP Version:\nPlugin Version:\nBridge Verison:";
            if (calculation != null)
            {
                calculation.SystemStatusString = "Waiting...";
                if (calculation.BridgeSerialAvailability)
                {
                    calculation.SystemStatusString = "Connected";
                }
                else
                {
                    if (Settings.Pedal_ESPNow_auto_connect_flag)
                    {
                        calculation.SystemStatusString = calculation.BridgeConnetingString;
                    }
                }
                calculation.SystemStatusString += "\n" + Constants.pedalConfigPayload_version + "\n" + Constants.pluginVersion;
                if (calculation.BridgeFirmwareVersion[2] != 0)
                {
                    calculation.SystemStatusString += "\n" + calculation.BridgeFirmwareVersion[0] + "." + calculation.BridgeFirmwareVersion[1] + ".";
                    if (calculation.BridgeFirmwareVersion[2] < 10)
                    {
                        calculation.SystemStatusString += "0" + calculation.BridgeFirmwareVersion[2];
                    }
                    else
                    {
                        calculation.SystemStatusString += "" + calculation.BridgeFirmwareVersion[2];
                    }
                }
                else
                {
                    calculation.SystemStatusString += "\nNo Data";
                }



            } 
            if (calculation.BridgeSerialConnectionStatus)
            {
                if(RSSI_canvas!=null) RSSI_canvas.Visibility = Visibility.Visible;
                //Label_RSSI.Visibility = Visibility.Visible;
                //if (Label_RSSI != null && Settings.advanced_b) Label_RSSI.Visibility = Visibility.Visible;
                //else Label_RSSI.Visibility = Visibility.Hidden;
            }
            else
            {
                if (RSSI_canvas != null) RSSI_canvas.Visibility = Visibility.Hidden;
                //if (Label_RSSI != null) Label_RSSI.Visibility = Visibility.Hidden;
            }
            if (info_label_2_system != null) info_label_2_system.Content = calculation.SystemStatusString;

            if (Label_RSSI != null && calculation.RSSI_Value != 0)
            {
                Label_RSSI.Content = "" + ((int)calculation.RSSI_Value - 100) + "dBm";
                Label_RSSI.Visibility = Visibility.Visible;
            } 
            else Label_RSSI.Visibility = Visibility.Hidden;

            if (RSSI_1 != null && RSSI_2 != null && RSSI_3 != null && RSSI_4 != null)
            {
                if (calculation.RSSI_Value < 25)
                {
                    RSSI_1.Visibility = Visibility.Visible;
                    RSSI_2.Visibility = Visibility.Hidden;
                    RSSI_3.Visibility = Visibility.Hidden;
                    RSSI_4.Visibility = Visibility.Hidden;
                }
                if (calculation.RSSI_Value > 25 && calculation.RSSI_Value < 30)
                {
                    RSSI_1.Visibility = Visibility.Visible;
                    RSSI_2.Visibility = Visibility.Visible;
                    RSSI_3.Visibility = Visibility.Hidden;
                    RSSI_4.Visibility = Visibility.Hidden;
                }
                if (calculation.RSSI_Value > 30 && calculation.RSSI_Value < 35)
                {
                    RSSI_1.Visibility = Visibility.Visible;
                    RSSI_2.Visibility = Visibility.Visible;
                    RSSI_3.Visibility = Visibility.Visible;
                    RSSI_4.Visibility = Visibility.Hidden;
                }
                if (calculation.RSSI_Value > 35)
                {
                    RSSI_1.Visibility = Visibility.Visible;
                    RSSI_2.Visibility = Visibility.Visible;
                    RSSI_3.Visibility = Visibility.Visible;
                    RSSI_4.Visibility = Visibility.Visible;
                }
            }
            
        }
    }
}
