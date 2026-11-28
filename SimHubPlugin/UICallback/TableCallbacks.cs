using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;

namespace DiyFfbPedal
{
    public partial class DIYFFBPedalControlUI : System.Windows.Controls.UserControl
    {
        // Pedal tabs are shown as Throttle, Brake, Clutch; each TabItem's Tag holds its pedal index
        // (0 = clutch, 1 = brake, 2 = throttle), so never use the tab position as the pedal index.
        private uint GetSelectedPedalIndex()
        {
            TabItem tab = MyTab.SelectedItem as TabItem;
            return tab == null ? 0 : Convert.ToUInt32(tab.Tag);
        }

        internal void SelectPedalTab(uint pedalIndex)
        {
            foreach (object item in MyTab.Items)
            {
                TabItem tab = item as TabItem;
                if (tab != null && Convert.ToUInt32(tab.Tag) == pedalIndex)
                {
                    MyTab.SelectedItem = tab;
                    return;
                }
            }
        }

        internal void SelectMainTab(int tabIndex)
        {
            if (Function_Tab_seleciton != null && tabIndex >= 0 && tabIndex < Function_Tab_seleciton.Items.Count)
            {
                Function_Tab_seleciton.SelectedIndex = tabIndex;
            }
        }

        private void TabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            // update the sliders & serial port selection accordingly
            if (Plugin != null)
            {
                indexOfSelectedPedal_u = GetSelectedPedalIndex();
                Plugin.Settings.table_selected = indexOfSelectedPedal_u;
                Plugin._calculations.Update_CV1_textbox = true;
                Plugin._calculations.Update_CV2_textbox = true;
                Plugin._calculations.Update_CV3_textbox = true;
                Plugin._calculations.Update_CV4_textbox = true;
                Plugin._calculations.OTASettingUpdate_b = true;
                PedalTabChange = true;
                PedalTabChange_last = DateTime.Now;
                updateTheGuiFromConfig();
                Plugin.ConfigService.RefreshConfigList();
            }
        }

        private void Tab_main_1_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            updateTheGuiFromConfig();
        }

        private void Function_Tab_seleciton_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (ReferenceEquals(e.Source, Function_Tab_seleciton))
            {
                if (Plugin?.Settings != null && Function_Tab_seleciton.SelectedIndex >= 0)
                {
                    if (Plugin.Settings.main_tab_selected != Function_Tab_seleciton.SelectedIndex)
                    {
                        Plugin.Settings.main_tab_selected = Function_Tab_seleciton.SelectedIndex;
                        Plugin.SavePluginSettings();
                    }
                }

                bool isLivePlot = Tab_LivePlot != null && Tab_LivePlot.IsSelected;
                bool isPoppedOut = _livePlotWindow != null && _livePlotWindow.IsLoaded;
                if (!isPoppedOut)
                {
                    LivePlotSection?.OnTabSelected(isLivePlot);
                }
            }
        }

        private void TabControl_SelectionChanged_1(object sender, SelectionChangedEventArgs e)
        {
            if (Plugin != null)
            {
                //Update_CV_textbox = true;
                Plugin._calculations.Update_CV1_textbox = true;
                Plugin._calculations.Update_CV2_textbox = true;
                Plugin._calculations.Update_CV3_textbox = true;
                Plugin._calculations.Update_CV4_textbox = true;
                Plugin._calculations.OTASettingUpdate_b = true;
                updateTheGuiFromConfig();
            }

        }
    }
}
