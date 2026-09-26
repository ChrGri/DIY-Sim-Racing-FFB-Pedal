using Newtonsoft.Json;
using SimHub.Plugins.Styles;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
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
using UserControl = System.Windows.Controls.UserControl;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// SystemSetting_Profiles.xaml 的互動邏輯
    /// </summary>
    
    public partial class SystemSetting_Profiles : UserControl
    {
        public System.Windows.Controls.CheckBox[,] Effect_status_profile = new System.Windows.Controls.CheckBox[3, 8];
        public uint profile_select = 0;
        public SystemSetting_Profiles()
        {
            InitializeComponent();
            //initialize profile effect status
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    Effect_status_profile[i, j] = new System.Windows.Controls.CheckBox();
                    Effect_status_profile[i, j].Width = 45;
                    Effect_status_profile[i, j].Height = 20;
                    Effect_status_profile[i, j].FontSize = 8;
                    switch (j)
                    {
                        case 0:

                            Effect_status_profile[i, j].Margin = new Thickness(20, 0, 0, 0);
                            if (i == 0 || i == 2)
                            {
                                Effect_status_profile[i, j].Content = "TC";

                            }
                            else
                            {
                                Effect_status_profile[i, j].Content = "ABS";
                            }

                            break;
                        case 1:
                            Effect_status_profile[i, j].Content = "RPM";
                            break;
                        case 2:
                            Effect_status_profile[i, j].Content = "B.P";
                            Effect_status_profile[i, j].Width = 40;
                            break;
                        case 3:
                            Effect_status_profile[i, j].Content = "G-F";
                            Effect_status_profile[i, j].Width = 40;
                            if (i == 0 || i == 2)
                            {
                                Effect_status_profile[i, j].IsEnabled = false;
                            }
                            break;
                        case 4:
                            Effect_status_profile[i, j].Content = "W.S";
                            Effect_status_profile[i, j].Width = 40;
                            break;
                        case 5:
                            Effect_status_profile[i, j].Content = "IMAPCT";
                            Effect_status_profile[i, j].Width = 60;
                            break;
                        case 6:
                            Effect_status_profile[i, j].Content = "CUS-1";
                            Effect_status_profile[i, j].Width = 50;
                            break;
                        case 7:
                            Effect_status_profile[i, j].Content = "CUS-2";
                            Effect_status_profile[i, j].Width = 50;
                            break;
                    }
                    switch (i)
                    {
                        case 0:
                            StackPanel_Effects_Status_0.Children.Add(Effect_status_profile[i, j]);
                            break;
                        case 1:
                            StackPanel_Effects_Status_1.Children.Add(Effect_status_profile[i, j]);
                            break;
                        case 2:
                            StackPanel_Effects_Status_2.Children.Add(Effect_status_profile[i, j]);
                            break;
                    }
                }
            }
        }
        public static readonly DependencyProperty DAP_Config_Property = DependencyProperty.Register(
            nameof(dap_config_st),
            typeof(DAP_config_st),
            typeof(SystemSetting_Profiles),
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
            typeof(SystemSetting_Profiles),
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
            typeof(SystemSetting_Profiles),
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
                    if (this != null)
                    {
                        if (calculation.Update_Profile_Checkbox_b)
                        {
                            for (int j = 0; j < 3; j++)
                            {
                                for (int k = 0; k < 8; k++)
                                {
                                    if (Settings.Effect_status_prolife[profile_select, j, k]){Effect_status_profile[j, k].IsChecked = true;}
                                    else{Effect_status_profile[j, k].IsChecked = false;}
                                }
                            }
                            calculation.Update_Profile_Checkbox_b = false;
                        }
                        if (textbox_profile_name != null) textbox_profile_name.Text = Settings.Profile_name[profile_select];
                        if (Settings.file_enable_check[profile_select, 0] == 1)
                        {
                            if (Label_clutch_file != null) Label_clutch_file.Content = Settings.Pedal_file_string[profile_select, 0];
                            Clutch_file_check.IsChecked = true;
                        }
                        else
                        {
                            if (Label_clutch_file != null) Label_clutch_file.Content = "";
                            Clutch_file_check.IsChecked = false;
                        }
                        if (Settings.file_enable_check[profile_select, 1] == 1)
                        {
                            if (Label_brake_file != null) Label_brake_file.Content = Settings.Pedal_file_string[profile_select, 1];
                            Brake_file_check.IsChecked = true;
                        }
                        else
                        {
                            if (Label_brake_file != null) Label_brake_file.Content = "";
                            Brake_file_check.IsChecked = false;
                        }

                        if (Settings.file_enable_check[profile_select, 2] == 1)
                        {
                            if (Label_gas_file != null) Label_gas_file.Content = Settings.Pedal_file_string[profile_select, 2];
                            Gas_file_check.IsChecked = true;
                        }
                        else
                        {
                            if (Label_gas_file != null) Label_gas_file.Content = "";
                            Gas_file_check.IsChecked = false;
                        }
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
            var control = d as SystemSetting_Profiles;
            if (control != null && e.NewValue is DAP_config_st newData)
            {
                if (control != null)
                {

                }
            }

        }
        private static void OnCalculationChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            var control = d as SystemSetting_Profiles;
            if (control != null && e.NewValue is CalculationVariables newData)
            {
                try
                {
                    if(control.ProfileTab!=null) control.ProfileTab.SelectedIndex = (int)newData.profile_index;

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

        private void ProfileTab_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            profile_select = (uint)ProfileTab.SelectedIndex;
            calculation.profile_index = profile_select;
            //Profile_change(profile_select);
            //Plugin.Settings.table_selected = (uint)MyTab.SelectedIndex;
            // update the sliders & serial port selection accordingly
            calculation.Update_Profile_Checkbox_b = true;
            CalculationChangedEvent(calculation);
            updateUI();
        }
        public void Profile_change(uint profile_index)
        {
            profile_select = profile_index;
            ProfileTab.SelectedIndex = (int)profile_index;
            //if (Plugin.Settings.file_enable_check[profile_select])
            //Parsefile(profile_index);
            string tmp;
            switch (profile_index)
            {
                case 0:
                    tmp = "A:" + Settings.Profile_name[profile_index];
                    break;
                case 1:
                    tmp = "B:" + Settings.Profile_name[profile_index];
                    break;
                case 2:
                    tmp = "C:" + Settings.Profile_name[profile_index];
                    break;
                case 3:
                    tmp = "D:" + Settings.Profile_name[profile_index];
                    break;
                case 4:
                    tmp = "E:" + Settings.Profile_name[profile_index];
                    break;
                case 5:
                    tmp = "F:" + Settings.Profile_name[profile_index];
                    break;
                default:
                    tmp = "No Profile";
                    break;
            }
            calculation.current_profile = tmp;
            for (int j = 0; j < 3; j++)
            {
                for (int k = 0; k < 8; k++)
                {
                    if (Settings.Effect_status_prolife[profile_select, j, k])
                    {
                        switch (k)
                        {
                            case 0:
                                Settings.ABS_enable_flag[j] = 1;
                                break;
                            case 1:
                                Settings.RPM_enable_flag[j] = 1;
                                break;
                            case 2:
                                //Plugin.Settings. = 1;
                                break;
                            case 3:
                                Settings.G_force_enable_flag[j] = 1;
                                break;
                            case 4:
                                Settings.WS_enable_flag[j] = 1;
                                break;
                            case 5:
                                Settings.Road_impact_enable_flag[j] = 1;
                                break;
                            case 6:
                                Settings.CV1_enable_flag[j] = true;
                                break;
                            case 7:
                                Settings.CV2_enable_flag[j] = true;
                                break;
                        }
                    }
                    else
                    {
                        switch (k)
                        {
                            case 0:
                                Settings.ABS_enable_flag[j] = 0;
                                break;
                            case 1:
                                Settings.RPM_enable_flag[j] = 0;
                                break;
                            case 2:
                                //Plugin.Settings. = 1;
                                break;
                            case 3:
                                Settings.G_force_enable_flag[j] = 0;
                                break;
                            case 4:
                                Settings.WS_enable_flag[j] = 0;
                                break;
                            case 5:
                                Settings.Road_impact_enable_flag[j] = 0;
                                break;
                            case 6:
                                Settings.CV1_enable_flag[j] = false;
                                break;
                            case 7:
                                Settings.CV2_enable_flag[j] = false;
                                break;
                        }
                    }

                }
            }
            //effect profile change

        }

        

        private void textbox_profile_name_TextChanged(object sender, TextChangedEventArgs e)
        {
            var textbox = sender as System.Windows.Controls.TextBox;
            Settings.Profile_name[profile_select] = textbox.Text;
            SettingsChangedEvent(Settings);
        }

        private void Read_for_slot(object sender, RoutedEventArgs e)
        {
            var Button = sender as SHButtonPrimary;

            using (System.Windows.Forms.OpenFileDialog openFileDialog = new System.Windows.Forms.OpenFileDialog())
            {
                openFileDialog.Title = "Datei auswählen";
                openFileDialog.Filter = "Configdateien (*.json)|*.json";
                string currentDirectory = Directory.GetCurrentDirectory();
                openFileDialog.InitialDirectory = currentDirectory + "\\PluginsData\\Common";

                if (openFileDialog.ShowDialog() == DialogResult.OK)
                {
                    string content = (string)openFileDialog.FileName;


                    string filePath = openFileDialog.FileName;
                    //TextBox_debugOutput.Text =  Button.Name;
                    //
                    uint i = profile_select;
                    uint j = 0;
                    if (Button.Name == "Reading_clutch")
                    {

                        Settings.Pedal_file_string[profile_select, 0] = filePath;
                        Label_clutch_file.Content = Settings.Pedal_file_string[profile_select, 0];
                        Settings.file_enable_check[profile_select, 0] = 1;
                        Clutch_file_check.IsChecked = true;
                        j = 0;
                    }
                    if (Button.Name == "Reading_brake")
                    {
                        Settings.Pedal_file_string[profile_select, 1] = filePath;
                        Label_brake_file.Content = Settings.Pedal_file_string[profile_select, 1];
                        Settings.file_enable_check[profile_select, 1] = 1;
                        Brake_file_check.IsChecked = true;
                        j = 1;
                    }
                    if (Button.Name == "Reading_gas")
                    {
                        Settings.Pedal_file_string[profile_select, 2] = filePath;
                        Label_gas_file.Content = Settings.Pedal_file_string[profile_select, 2];
                        Settings.file_enable_check[profile_select, 2] = 1;
                        Gas_file_check.IsChecked = true;
                        j = 2;
                    }

                    //write to setting
                    for (int k = 0; k < 8; k++)
                    {
                        if (Effect_status_profile[j, k].IsChecked == true)
                        {
                            Settings.Effect_status_prolife[i, j, k] = true;
                        }
                        else
                        {
                            Settings.Effect_status_prolife[i, j, k] = false;
                        }

                    }


                }
            }
            SettingsChangedEvent(Settings);
        }

        private void Clear_slot(object sender, RoutedEventArgs e)
        {
            var Button = sender as SHButtonPrimary;
            uint i = profile_select;
            uint j = 0;
            if (Button.Name == "Clear_clutch")
            {

                Settings.Pedal_file_string[profile_select, 0] = "";
                Label_clutch_file.Content = Settings.Pedal_file_string[profile_select, 0];
                Settings.file_enable_check[profile_select, 0] = 0;
                Clutch_file_check.IsChecked = false;
                j = 0;

            }
            if (Button.Name == "Clear_brake")
            {

                Settings.Pedal_file_string[profile_select, 1] = "";
                Label_brake_file.Content = Settings.Pedal_file_string[profile_select, 1];
                Settings.file_enable_check[profile_select, 1] = 0;
                Brake_file_check.IsChecked = false;
                j = 1;

            }
            if (Button.Name == "Clear_gas")
            {

                Settings.Pedal_file_string[profile_select, 2] = "";
                Label_gas_file.Content = Settings.Pedal_file_string[profile_select, 2];
                Settings.file_enable_check[profile_select, 2] = 0;
                Gas_file_check.IsChecked = false;
                j = 2;

            }
            //write to setting
            for (int k = 0; k < 8; k++)
            {
                Settings.Effect_status_prolife[i, j, k] = false;
                Effect_status_profile[j, k].IsChecked = false;
            }
            //updateTheGuiFromConfig();
            SettingsChangedEvent(Settings);
        }
        public event EventHandler btn_apply_profile_Click_event;
        private void btn_apply_profile_Click(object sender, RoutedEventArgs e)
        {
            btn_apply_profile_Click_event?.Invoke(this, EventArgs.Empty);
        }
        public event EventHandler btn_send_profile_Click_event;
        private void btn_send_profile_Click(object sender, RoutedEventArgs e)
        {
            btn_send_profile_Click_event?.Invoke(this, EventArgs.Empty);
        }

        private void file_check_Checked(object sender, RoutedEventArgs e)
        {
            var checkbox = sender as System.Windows.Controls.CheckBox;
            if (checkbox.Name == "Clutch_file_check")
            {
                Settings.file_enable_check[profile_select, 0] = 1;
            }

            if (checkbox.Name == "Brake_file_check")
            {
                Settings.file_enable_check[profile_select, 1] = 1;
            }

            if (checkbox.Name == "Gas_file_check")
            {
                Settings.file_enable_check[profile_select, 2] = 1;
            }
        }

        private void file_check_Unchecked(object sender, RoutedEventArgs e)
        {
            var checkbox = sender as System.Windows.Controls.CheckBox;
            if (checkbox.Name == "Clutch_file_check")
            {
                Settings.file_enable_check[profile_select, 0] = 0;
            }

            if (checkbox.Name == "Brake_file_check")
            {
                Settings.file_enable_check[profile_select, 1] = 0;
            }

            if (checkbox.Name == "Gas_file_check")
            {
                Settings.file_enable_check[profile_select, 2] = 0;
            }
        }
    }
}
