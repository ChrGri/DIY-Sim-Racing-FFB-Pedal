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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// InfoSection_Pedal.xaml 的互動邏輯
    /// </summary>
    public partial class InfoSection_Pedal : UserControl
    {
        public InfoSection_Pedal()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(InfoSection_Pedal),
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
            typeof(InfoSection_Pedal),
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
            var control = d as InfoSection_Pedal;
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
            var control = d as InfoSection_Pedal;
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
            if (calculation != null)
            {
                calculation.PedalStatusString = "Waiting...";
                if (calculation.PedalSerialAvailability[Settings.table_selected])
                {
                    calculation.PedalStatusString = "Usb Connected";
                }
                else
                {
                    if (Settings.auto_connect_flag[Settings.table_selected] == 1)
                    {
                        calculation.PedalStatusString = calculation.PedalConnetingString;
                    }
                }
                if (Settings.Pedal_ESPNow_Sync_flag[Settings.table_selected])
                {
                    if (calculation.PedalAvailability[0]  && Settings.table_selected == 0)
                    {
                        calculation.PedalStatusString = "Wireless";
                    }
                    if (calculation.PedalAvailability[1] && Settings.table_selected == 1)
                    {
                        calculation.PedalStatusString = "Wireless";
                    }
                    if (calculation.PedalAvailability[2] && Settings.table_selected == 2)
                    {
                        calculation.PedalStatusString = "Wireless";
                    }
                }
                calculation.PedalStatusString += "\n" + Constants.pedalConfigPayload_version + "\n" + Constants.pluginVersion;
                if (calculation.PedalFirmwareVersion[Settings.table_selected, 2] != 0)
                {
                    calculation.PedalStatusString += "\n" + calculation.PedalFirmwareVersion[Settings.table_selected, 0] + "." + calculation.PedalFirmwareVersion[Settings.table_selected, 1] + ".";
                    if (calculation.PedalFirmwareVersion[Settings.table_selected, 2] < 10)
                    {
                        calculation.PedalStatusString += "0" + calculation.PedalFirmwareVersion[Settings.table_selected, 2];
                    }
                    else
                    {
                        calculation.PedalStatusString += "" + calculation.PedalFirmwareVersion[Settings.table_selected, 2];
                    }
                }
                else
                {
                    calculation.PedalStatusString += "\n" + "No data";
                }


            }
            if(info_label!=null) info_label.Content = "State:\nDAP Version:\nPlugin Version:\nPedal Version:";
            if (info_label_2 != null) info_label_2.Content = calculation.PedalStatusString;
        }
    }
}
