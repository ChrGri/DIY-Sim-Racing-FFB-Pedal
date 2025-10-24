using SimHub.Plugins.OutputPlugins.GraphicalDash.Behaviors.DoubleText.Imp;
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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// AssignmentConfigurationWindow.xaml 的互動邏輯
    /// </summary>
    public partial class AssignmentConfigurationWindow : Window
    {
        public class PedalItem
        {
            public string Name { get; set; }
            public int Id { get; set; }
        }
        private List<PedalItem> _pedalList;
        private List<PedalItem> _unassignedPedalList;
        private DIY_FFB_Pedal _plugin;
        private int _pedalSelect;
        private string[] _pedalName= new string[3] {"Clutch", "Brake", "Throttle"};
        private int[] _pedalActionId = new int[3] {(int)PedalSystemAction.SET_ASSIGNMENT_0, (int)PedalSystemAction.SET_ASSIGNMENT_1, (int)PedalSystemAction.SET_ASSIGNMENT_2 };
        private string[] _unassignedPedalName = new string[4] {"Selected a Unassigned Pedal", "#1", "#2", "#3" };
        private int[] _unassignedPedalId = new int[4] { (int)PedalIdEnum.PEDAL_ID_UNKNOWN, (int)PedalIdEnum.PEDAL_ID_TEMP_1, (int)PedalIdEnum.PEDAL_ID_TEMP_2, (int)PedalIdEnum.PEDAL_ID_TEMP_3 };
        double clientWidth;
        double clientHeight;
        public AssignmentConfigurationWindow(DIY_FFB_Pedal Plugin)
        {
            InitializeComponent();
            //this.Loaded += Window_Loaded;
            _plugin = Plugin;
            
            _unassignedPedalList = new List<PedalItem>();
            for (int i = 0; i < Plugin._calculations.unassignedPedalCount+1;i++)
            {
                PedalItem item = new PedalItem();
                item.Name = _unassignedPedalName[i];
                if (i > 0)
                {
                    item.Name = item.Name + "-";
                    for (int j = 0; j < 6; j++)
                    {
                        item.Name = item.Name  + Plugin._calculations.unassignedPedalMacaddress[i-1][j].ToString("X2");
                        if (j < 5)
                        {
                            item.Name = item.Name + ":";
                        }
                    }
                }

                item.Id = _unassignedPedalId[i];
                _unassignedPedalList.Add(item);
            }
            if (_plugin._calculations.pedalWirelessStatus[Plugin.Settings.table_selected]==WirelessConnectStateEnum.PEDAL_WIRELESS_IS_READY)
            {
                Btn_Assign.IsEnabled = false;
                Btn_Beep.IsEnabled = false;
                Btn_VibrationOff.IsEnabled = false;
                Btn_VibrationOn.IsEnabled = false;
                Btn_AssignmentClear.IsEnabled = true;
                Label_Role.Content =  _pedalName[_plugin.Settings.table_selected]+" is connected, click Clear to remove Assignment";
                ComboBoxUnassignedPedal.Visibility=Visibility.Hidden;
                Label_DetectedPedal.Visibility=Visibility.Hidden;
            }
            else
            {
                Btn_Assign.IsEnabled = true;
                Btn_Beep.IsEnabled = true;
                Btn_VibrationOff.IsEnabled = true;
                Btn_VibrationOn.IsEnabled = true;
                Btn_Assign.IsEnabled = true;
                Btn_AssignmentClear.IsEnabled = false;
                Label_Role.Content = "Set as " + _pedalName[_plugin.Settings.table_selected];
                ComboBoxUnassignedPedal.Visibility = Visibility.Visible;
                Label_DetectedPedal.Visibility = Visibility.Visible;
            }
            _pedalSelect = -1;
            ComboBoxUnassignedPedal.ItemsSource = _unassignedPedalList;
            ComboBoxUnassignedPedal.DisplayMemberPath = "Name";
            ComboBoxUnassignedPedal.SelectedValuePath = "Id";
            ComboBoxUnassignedPedal.SelectedIndex = 0;
            


        }

        unsafe private void Btn_Beep_Click(object sender, RoutedEventArgs e)
        {
            DAP_action_st tmp_action = default;
            tmp_action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp_action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp_action.payloadHeader_.PedalTag = (byte)_pedalSelect;
            //tmp_action.payloadHeader_.PedalTag = (byte)PedalIdEnum.PEDAL_ID_TEMP_1;
            tmp_action.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.ASSIGNMENT_CHECK_BEEP;
            _plugin.SendPedalActionWireless(tmp_action, (byte)_pedalSelect);
            //_plugin.SendPedalActionWireless(tmp_action, (byte)PedalIdEnum.PEDAL_ID_TEMP_1);

        }

        private void Btn_Assign_Click(object sender, RoutedEventArgs e)
        {
            if (_pedalSelect != (int)PedalIdEnum.PEDAL_ID_UNKNOWN && _pedalActionId[_plugin.Settings.table_selected] != (int)PedalSystemAction.NONE)
            {
                DAP_action_st tmp_action = default;
                tmp_action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp_action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
                tmp_action.payloadHeader_.PedalTag = (byte)_pedalSelect;
                //tmp_action.payloadHeader_.PedalTag = (byte)PedalIdEnum.PEDAL_ID_TEMP_1;
                tmp_action.payloadPedalAction_.system_action_u8 = (byte)_pedalActionId[_plugin.Settings.table_selected];
                _plugin.SendPedalActionWireless(tmp_action, (byte)_pedalSelect);
            }

            this.Close();

        }

        private void ComboBoxUnassignedPedal_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ComboBox cbo = (ComboBox)sender;
            if (cbo.SelectedValue != null)
            {
                _pedalSelect = (int)cbo.SelectedValue;
            }
            else
            {
                _pedalSelect = -2;
            }

            //Label_debug.Content = "Unassigned:" + _pedalSelect + " To:" + _pedalActionId[_plugin.Settings.table_selected];
        }

        private void Btn_VibrationOn_Click(object sender, RoutedEventArgs e)
        {
            DAP_action_st tmp_action = default;
            tmp_action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp_action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp_action.payloadHeader_.PedalTag = (byte)_pedalSelect;
            tmp_action.payloadPedalAction_.RPM_u8 = (byte)20;
            _plugin.SendPedalActionWireless(tmp_action, (byte)_pedalSelect);
        }

        private void Btn_VibrationOff_Click(object sender, RoutedEventArgs e)
        {
            DAP_action_st tmp_action = default;
            tmp_action.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp_action.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp_action.payloadHeader_.PedalTag = (byte)_pedalSelect;
            tmp_action.payloadPedalAction_.RPM_u8 = (byte)0;
            _plugin.SendPedalActionWireless(tmp_action, (byte)_pedalSelect);
        }

        private void Btn_AssignmentClear_Click(object sender, RoutedEventArgs e)
        {
            DAP_action_st tmp = default;
            tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.CLEAR_ASSIGNMENT;
            tmp.payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
            /*
            tmp.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            tmp.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            tmp.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            tmp.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            */
            _plugin.SendPedalAction(tmp, (byte)_plugin.Settings.table_selected);
            this.Close();
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {

            clientWidth = MainContentGrid.ActualWidth;
            clientHeight = MainContentGrid.ActualHeight;
            Label_debug2.Content = "Width: " + clientWidth + ", Height:" + clientHeight;
        }
    }
}
