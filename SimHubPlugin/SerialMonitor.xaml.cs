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

namespace User.PluginSdkDemo
{
    public partial class SerialMonitor_Window : Window
    {
        DIYFFBPedalControlUI _main_UI;
        public SerialMonitor_Window(DIYFFBPedalControlUI Main_UI)
        {
            InitializeComponent();
            _main_UI = Main_UI;
            if (_main_UI.Plugin.Settings.Serial_auto_clean)
            { 
                Checkbox_Serial_Window_Auto_Remove_Serial_Text.IsChecked = true;
            }
            else
            {
                Checkbox_Serial_Window_Auto_Remove_Serial_Text.IsChecked = false;
            }

        }

        private void btn_Serial_Window_clear_Click(object sender, RoutedEventArgs e)
        {
            TextBox_SerialMonitor.Clear();
        }

        private void Checkbox_Serial_Window_Auto_Remove_Serial_Text_Checked(object sender, RoutedEventArgs e)
        {
            _main_UI.Plugin.Settings.Serial_auto_clean = true;
        }

        private void Checkbox_Serial_Window_Auto_Remove_Serial_Text_Unchecked(object sender, RoutedEventArgs e)
        {
            _main_UI.Plugin.Settings.Serial_auto_clean = false;
        }

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            StackPanel_UI.Width = this.Width;
            StackPanel_UI.Height = this.Height-12;
            TextBox_SerialMonitor.Height=this.Height-47;
            TextBox_SerialMonitor.Width=this.Width-7;

        }
    }
}
