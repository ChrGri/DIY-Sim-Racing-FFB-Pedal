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
using Newtonsoft.Json;
using System.Net.Http;
using System.ComponentModel;



namespace User.PluginSdkDemo
{

    public partial class OnlineProfile : Window
    {
        public string SelectedFileName { get; private set; }
        private DAP_config_st tmp_config;
        public OnlineProfile()
        {
            InitializeComponent();
            LoadProfiles();
        }

        private async void LoadProfiles()
        {
            string jsonUrl = "https://raw.githubusercontent.com/tcfshcrw/FFB_PEDAL_PROFILE/master/main.json";

            using (HttpClient client = new HttpClient())
            {
                string jsonString = await client.GetStringAsync(jsonUrl);
                var profilesData = JsonConvert.DeserializeObject<ProfilesData>(jsonString);

                ProfilesListBox.ItemsSource = profilesData.Profiles;
                ProfilesListBox.DisplayMemberPath = "ProfileName";
                CollectionView view = (CollectionView)CollectionViewSource.GetDefaultView(ProfilesListBox.ItemsSource);
                view.SortDescriptions.Add(new SortDescription("ProfileName", ListSortDirection.Ascending));
            }
        }

        private void CloseAndReturn_Click(object sender, RoutedEventArgs e)
        {
            if (ProfilesListBox.SelectedItem is Profile selectedProfile)
            {
                SelectedFileName = selectedProfile.FileName;
                DialogResult = true;
            }
            else
            {
                MessageBox.Show("Please select a profile.");
            }
        }

        private async void ProfilesListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ProfilesListBox.SelectedItem is Profile selectedProfile)
            {
                Textbox_Online_Profile_Description.Text = "Author: "+selectedProfile.Author+"\nVersion: "+selectedProfile.Version+"\n"+selectedProfile.Description+"\n"; ;
                //Label_Online_Profile_Description.Content = "\n URL:" + selectedProfile.FileName;
                try
                {
                    string jsonUrl = "https://raw.githubusercontent.com/tcfshcrw/FFB_PEDAL_PROFILE/master/Profiles/" + selectedProfile.FileName;
                    tmp_config = await GetProfileDataAsync(jsonUrl);
                    Update_ForceCurve();
                    Textbox_Online_Profile_Description.Text += "\nPreview:\n";
                    Textbox_Online_Profile_Description.Text += "DAP Version: "+tmp_config.payloadHeader_.version+"\n";
                    Textbox_Online_Profile_Description.Text += "Max force: " + tmp_config.payloadPedalConfig_.maxForce + "\n";
                    Textbox_Online_Profile_Description.Text += "Preload: " + tmp_config.payloadPedalConfig_.preloadForce + "\n";
                    //Textbox_Online_Profile_Description.Text += "Max force: " + tmp_config.payloadPedalConfig_.maxForce + "\n";
                    Textbox_Online_Profile_Description.Text += "Travel: " + ((float)(tmp_config.payloadPedalConfig_.pedalEndPosition-tmp_config.payloadPedalConfig_.pedalStartPosition)/100.0f*tmp_config.payloadPedalConfig_.lengthPedal_travel) + "\n";
                    Textbox_Online_Profile_Description.Text += "Damping: " + tmp_config.payloadPedalConfig_.dampingPress + "\n";
                    switch (tmp_config.payloadPedalConfig_.kf_modelOrder)
                    {
                        case 0:
                            Textbox_Online_Profile_Description.Text += "KF Model: Const. Vel\n";
                            break;
                        case 1:
                            Textbox_Online_Profile_Description.Text += "KF Model: Const. Acc\n";
                            break;
                        case 2:
                            Textbox_Online_Profile_Description.Text += "KF Model: EXP.\n";
                            break;
                        case 4:
                            Textbox_Online_Profile_Description.Text += "KF Model: None\n";
                            break;
                    }
                    Textbox_Online_Profile_Description.Text += "KF :" + tmp_config.payloadPedalConfig_.kf_modelNoise + "\n";
                    switch (tmp_config.payloadPedalConfig_.control_strategy_b)
                    {
                        case 0:
                            Textbox_Online_Profile_Description.Text += "Control: Static PID\n";
                            Textbox_Online_Profile_Description.Text += "P Gain:"+tmp_config.payloadPedalConfig_.PID_p_gain+"\n";
                            Textbox_Online_Profile_Description.Text += "I Gain:" + tmp_config.payloadPedalConfig_.PID_i_gain + "\n";
                            Textbox_Online_Profile_Description.Text += "D Gain:" + tmp_config.payloadPedalConfig_.PID_d_gain + "\n";
                            Textbox_Online_Profile_Description.Text += "Feed Forward Gain:" + tmp_config.payloadPedalConfig_.PID_velocity_feedforward_gain + "\n";
                            break;
                        case 1:
                            Textbox_Online_Profile_Description.Text += "Control: Dynamic PID\n";
                            Textbox_Online_Profile_Description.Text += "P Gain:" + tmp_config.payloadPedalConfig_.PID_p_gain + "\n";
                            Textbox_Online_Profile_Description.Text += "I Gain:" + tmp_config.payloadPedalConfig_.PID_i_gain + "\n";
                            Textbox_Online_Profile_Description.Text += "D Gain:" + tmp_config.payloadPedalConfig_.PID_d_gain + "\n";
                            Textbox_Online_Profile_Description.Text += "Feed Forward Gain:" + tmp_config.payloadPedalConfig_.PID_velocity_feedforward_gain + "\n";
                            break;
                        case 2:
                            Textbox_Online_Profile_Description.Text += "Control: MPC Control\n";
                            Textbox_Online_Profile_Description.Text += "MPC 0th Gain:" + tmp_config.payloadPedalConfig_.MPC_0th_order_gain + "\n";
                            break;
                    }




                }
                catch (Exception ex)
                {
                    System.Windows.MessageBox.Show($"Error loading JSON: {ex.Message}", "Error", MessageBoxButton.OK, MessageBoxImage.Warning);

                }
                

            }
            
        }


        private void Btn_Online_profile_leave_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        private async Task<DAP_config_st> GetProfileDataAsync(string url)
        {
            using (HttpClient client = new HttpClient())
            {
                string jsonString = await client.GetStringAsync(url);
                //return JsonConvert.DeserializeObject<Profile_Online>(jsonString);
                return JsonConvert.DeserializeObject<DAP_config_st>(jsonString);
            }
        }
        private void Update_ForceCurve()
        {

            double[] x = new double[6];
            double[] y = new double[6];
            double x_quantity = 100;
            double y_max = 100;
            double dx = canvas_Online_Config_curve.Width / x_quantity;
            double dy = canvas_Online_Config_curve.Height / y_max;
            //draw pedal force-travel curve
            x[0] = 0;
            x[1] = 20;
            x[2] = 40;
            x[3] = 60;
            x[4] = 80;
            x[5] = 100;

            y[0] = tmp_config.payloadPedalConfig_.relativeForce_p000;
            y[1] = tmp_config.payloadPedalConfig_.relativeForce_p020;
            y[2] = tmp_config.payloadPedalConfig_.relativeForce_p040;
            y[3] = tmp_config.payloadPedalConfig_.relativeForce_p060;
            y[4] = tmp_config.payloadPedalConfig_.relativeForce_p080;
            y[5] = tmp_config.payloadPedalConfig_.relativeForce_p100;

            // Use cubic interpolation to smooth the original data
            (double[] xs2, double[] ys2, double[] a, double[] b) = Cubic.Interpolate1D(x, y, 100);
            System.Windows.Media.PointCollection myPointCollection2 = new System.Windows.Media.PointCollection();


            for (int pointIdx = 0; pointIdx < 100; pointIdx++)
            {
                System.Windows.Point Pointlcl = new System.Windows.Point(dx * xs2[pointIdx], dy * ys2[pointIdx]);
                myPointCollection2.Add(Pointlcl);
                
            }
            this.Polyline_Online_Config_ForceCurve.Points = myPointCollection2;
            //set the rect
            double control_rect_value_max = 100;
            double dyy = canvas_Online_Config_curve.Height / control_rect_value_max;
            Canvas.SetTop(rect0_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p000 - rect0_Online_Config.Height / 2);
            Canvas.SetLeft(rect0_Online_Config, 0 * canvas_Online_Config_curve.Width / 5 - rect0_Online_Config.Width / 2);
            Canvas.SetTop(rect1_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p020 - rect1_Online_Config.Height / 2);
            Canvas.SetLeft(rect1_Online_Config, 1 * canvas_Online_Config_curve.Width / 5 - rect1_Online_Config.Width / 2);
            Canvas.SetTop(rect2_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p040 - rect2_Online_Config.Height / 2);
            Canvas.SetLeft(rect2_Online_Config, 2 * canvas_Online_Config_curve.Width / 5 - rect2_Online_Config.Width / 2);
            Canvas.SetTop(rect3_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p060 - rect3_Online_Config.Height / 2);
            Canvas.SetLeft(rect3_Online_Config, 3 * canvas_Online_Config_curve.Width / 5 - rect3_Online_Config.Width / 2);
            Canvas.SetTop(rect4_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p080 - rect4_Online_Config.Height / 2);
            Canvas.SetLeft(rect4_Online_Config, 4 * canvas_Online_Config_curve.Width / 5 - rect4_Online_Config.Width / 2);
            Canvas.SetTop(rect5_Online_Config, canvas_Online_Config_curve.Height - dyy * tmp_config.payloadPedalConfig_.relativeForce_p100 - rect5_Online_Config.Height / 2);
            Canvas.SetLeft(rect5_Online_Config, 5 * canvas_Online_Config_curve.Width / 5 - rect5_Online_Config.Width / 2);
        }
        }

    public class ProfilesData
    {
        public List<Profile> Profiles { get; set; }
    }

    public class Profile
    {
        public string Author { get; set; }
        public string ProfileName { get; set; }
        public string Version { get; set; }
        public string FileName { get; set; }
        public string Description { get; set; }
    }
}
