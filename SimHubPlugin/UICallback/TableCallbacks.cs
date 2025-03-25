using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace User.PluginSdkDemo
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        private void TabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            // update the sliders & serial port selection accordingly
            if (Plugin != null)
            {
                indexOfSelectedPedal_u = (uint)MyTab.SelectedIndex;
                Plugin.Settings.table_selected = (uint)MyTab.SelectedIndex;                
                Plugin._calculations.Update_CV1_textbox = true;
                Plugin._calculations.Update_CV2_textbox = true;
                Plugin._calculations.OTASettingUpdate_b = true;
                PedalTabChange = true;
                PedalTabChange_last = DateTime.Now;
                updateTheGuiFromConfig();
            }
        }

        private void Tab_main_1_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            updateTheGuiFromConfig();
        }

        private void Function_Tab_seleciton_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (Function_Tab_seleciton.SelectedIndex == 2)
            {
                //Update_Profile_Checkbox_b = true;
                Plugin._calculations.Update_Profile_Checkbox_b = true;
                updateTheGuiFromConfig();
            }
        }


        private void TabControl_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            if (Plugin != null)
            {
                //Update_CV_textbox = true;
                Plugin._calculations.Update_CV1_textbox = true;
                Plugin._calculations.Update_CV2_textbox = true;
                Plugin._calculations.OTASettingUpdate_b = true;
                updateTheGuiFromConfig();
            }

        }
    }
}
