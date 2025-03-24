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
    /// InfoSection_Rudder.xaml 的互動邏輯
    /// </summary>
    public partial class InfoSection_Rudder : UserControl
    {
        public InfoSection_Rudder()
        {
            InitializeComponent();
        }
        public static readonly DependencyProperty Cauculation_Property = DependencyProperty.Register(
            nameof(calculation),
            typeof(CalculationVariables),
            typeof(InfoSection_Rudder),
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
            typeof(InfoSection_Rudder),
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
            var control = d as InfoSection_Rudder;
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
            var control = d as InfoSection_Rudder;
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
            if (calculation.BridgeSerialAvailability)
            {
                calculation.RudderStatusString = "Online\n";
            }
            else
            {
                calculation.RudderStatusString = "Offline\n";
            }

            for (uint i = 0; i<3; i ++)
            {
                if (calculation.PedalAvailability[i])
                {
                    calculation.RudderStatusString += "Online\n";
                }
                else
                {
                    calculation.RudderStatusString += "Offline\n";
                }
            }

            if (calculation.Rudder_status)
            {

                calculation.RudderStatusString += "In action";
            }
            else
            {

                calculation.RudderStatusString += "Off";
            }
            if (info_rudder_label != null) info_rudder_label.Content = "Bridge State:\nClutch:\nBrake:\nThrottle:\nRudder:";
            if (info_rudder_label_2 != null) info_rudder_label_2.Content = calculation.RudderStatusString;
        }
    }
}
