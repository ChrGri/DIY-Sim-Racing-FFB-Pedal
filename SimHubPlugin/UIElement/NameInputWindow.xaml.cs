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
    /// <summary>
    /// NameInputWindow.xaml 的互動邏輯
    /// </summary>
    public partial class NameInputWindow : Window
    {
        public string input { get; private set; }
        public NameInputWindow()
        {
            InitializeComponent();
        }

        
        private void Btn_OK_Click(object sender, RoutedEventArgs e)
        {
            input = Textbox_Input.Text;
            DialogResult = true;
        }

        private void Btn_Leave_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}
