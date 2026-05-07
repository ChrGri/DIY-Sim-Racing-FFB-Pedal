using SimHub.Plugins.Devices.Registry.RexingMayaris2;
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
using System.Windows.Shapes;

namespace DiyFfbPedal
{
    /// <summary>
    /// NameInputWindow.xaml 的互動邏輯
    /// </summary>
    public partial class NcalcScriptEditor : Window
    {
        public string input { get; set; }
        private DIY_FFB_Pedal _plugin;
        private int customEffectIndex;
        private int pedalIndex;
        public NcalcScriptEditor(DIY_FFB_Pedal Plugin, int CustomEffectIndex, int PedalIndex)
        {
            _plugin = Plugin;
            customEffectIndex = CustomEffectIndex;
            pedalIndex = PedalIndex;
            InitializeComponent();
            this.DataContext = this;
        }

        
        private void Btn_OK_Click(object sender, RoutedEventArgs e)
        {
            if (TextBox_Ncalc_Input.Text != string.Empty && _plugin.Ncalc_reading(TextBox_Ncalc_Input.Text).ToString() != "Error")
            {
                input = TextBox_Ncalc_Input.Text.ToString();
                if(customEffectIndex==1) _plugin.Settings.CV1_bindings[pedalIndex] = input;
                else if(customEffectIndex==2) _plugin.Settings.CV2_bindings[pedalIndex] = input;
                else if (customEffectIndex == 3) _plugin.Settings.CV2_bindings[pedalIndex] = input;
                else if (customEffectIndex == 4) _plugin.Settings.CV4_bindings[pedalIndex] = input;
                DialogResult = true;
            }
            else
            {
                System.Windows.MessageBox.Show("Please input the correct script inside textbox.", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);
            }

        }

        private void Btn_Leave_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        private void Btn_Clear_Click(object sender, RoutedEventArgs e)
        {
            if(TextBox_Ncalc_Input!=null) TextBox_Ncalc_Input.Text = string.Empty;
        }

        private void TextBox_Ncalc_Input_TextChanged(object sender, TextChangedEventArgs e)
        {
            string var1 = "";
            if (_plugin != null) var1 = _plugin.Ncalc_reading(TextBox_Ncalc_Input.Text.ToString());
            Label_Result.Content = "Script Parse Result = "+var1;
        }
    }
}
