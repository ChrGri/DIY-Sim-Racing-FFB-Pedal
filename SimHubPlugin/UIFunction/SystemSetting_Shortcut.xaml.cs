using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;
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
using static System.Windows.Forms.VisualStyles.VisualStyleElement.Header;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// SystemSetting_Shortcut.xaml 的互動邏輯
    /// </summary>
    public partial class SystemSetting_Shortcut : UserControl
    {
        private DIY_FFB_Pedal _plugin;
        public DIY_FFB_Pedal Plugin
        {
            get { return _plugin; }
            set
            {
                if (_plugin != value)
                {
                    _plugin = value;
                    InitializePlugin(_plugin);
                    OnPropertyChanged();
                    OnPropertyChanged(nameof(ShortcutProfile0));
                    OnPropertyChanged(nameof(ShortcutProfile1));
                    OnPropertyChanged(nameof(ShortcutProfile2));
                    OnPropertyChanged(nameof(ShortcutProfile3));
                    OnPropertyChanged(nameof(ShortcutProfile4));
                    OnPropertyChanged(nameof(ShortcutProfile5));
                }
            }
        }
        public string ShortcutProfile0
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[0] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[0] = value;
                    OnPropertyChanged();
                }
            }
        }
        public string ShortcutProfile1
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[1] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[1] = value;
                    OnPropertyChanged();
                }
            }
        }
        public string ShortcutProfile2
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[2] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[2] = value;
                    OnPropertyChanged();
                }
            }
        }
        public string ShortcutProfile3
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[3] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[3] = value;
                    OnPropertyChanged();
                }
            }
        }
        public string ShortcutProfile4
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[4] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[4] = value;
                    OnPropertyChanged();
                }
            }
        }
        public string ShortcutProfile5
        {
            get => _settings?.ProfileShortcut?.Length > 0 ? _settings.ProfileShortcut[5] : null;
            set
            {
                if (_settings?.ProfileShortcut != null && _settings.ProfileShortcut.Length > 0)
                {
                    _settings.ProfileShortcut[5] = value;
                    OnPropertyChanged();
                }
            }
        }
        private DIYFFBPedalSettings _settings { get; set; }

        private ObservableCollection<ProfileListItem> _profileList;
        public ObservableCollection<ProfileListItem> ProfileList
        {
            get { return _profileList; }
            set { _profileList = value; OnPropertyChanged(); }
        }
        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }
        public SystemSetting_Shortcut()
        {
            InitializeComponent();
            this.DataContext = this;
        }
        private void InitializePlugin(DIY_FFB_Pedal data)
        {
            if (data != null)
            {

                //this.DataContext = data;
                ProfileList = data.ProfileServicePlugin.ProfileList;
                _settings = data.Settings;
            }
        }
        private void Btn_Shortcut_0_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile0 = null;            
            ComboBox0.SelectedValue=null;
            Textbox_ShortcutName0.Text = "";
            _settings.ProfileShortcut[0] = "";
        }

        private void Btn_Shortcut_1_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile1 = null;
            ComboBox1.SelectedValue = null;
            Textbox_ShortcutName1.Text = "";
            _settings.ProfileShortcut[1] = "";
        }

        private void Btn_Shortcut_2_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile2 = null;
            ComboBox2.SelectedValue = null;
            Textbox_ShortcutName2.Text = "";
            _settings.ProfileShortcut[2] = "";
        }

        private void Btn_Shortcut_3_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile3 = null;
            ComboBox3.SelectedValue = null;
            Textbox_ShortcutName3.Text = "";
            _settings.ProfileShortcut[3] = "";
        }

        private void Btn_Shortcut_4_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile4 = null;
            ComboBox4.SelectedValue = null;
            Textbox_ShortcutName4.Text = "";
            _settings.ProfileShortcut[4] = "";
        }
        private void Btn_Shortcut_5_Click(object sender, RoutedEventArgs e)
        {
            ShortcutProfile5 = null;
            ComboBox5.SelectedValue = null;
            Textbox_ShortcutName5.Text = "";
            _settings.ProfileShortcut[5] = "";

        }
    }
}
