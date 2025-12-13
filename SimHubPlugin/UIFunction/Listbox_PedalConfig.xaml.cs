using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.Serialization.Json;
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
using Path = System.IO.Path;

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// Listbox_PedalConfig.xaml 的互動邏輯
    /// </summary>
    public partial class Listbox_PedalConfig : UserControl
    {
        public ObservableCollection<ConfigListItem> ItemList { get; set; }
        private DIY_FFB_Pedal _plugin;
        private const string configFolderName = "configs";
        private const string profileFolderName = "profiles";
        private const string baseFolderName = "DiyFfbPedal";
        private const string simhubPluginDataFolderName = "PluginData";
        private const string simhubPluginDataCommonFolderName = "Common";
        private DAP_config_st tmp_config;
        public ICommand ApplyConfigCommand { get; }
        public ICommand SetDefaultConfigCommand { get; }
        public ICommand AddNewConfigCommand { get; }
        public ICommand RefreshListCommand { get; }
        public ICommand OverwriteConfigCommand { get; }
        public ICommand SaveAsNewConfigCommand { get; }
        public ICommand ResetEditingConfigCommand { get; }
        public ICommand PreviewConfigCommand { get; }
        public ICommand DeleteConfigCommand { get; }
        public ICommand RenameConfigCommand { get; }
        public Listbox_PedalConfig()
        {
            InitializeComponent();
            //_plugin = Plugin;
            if(_plugin!=null) ItemList = _plugin.ConfigService.ConfigList;
            AddNewConfigCommand = new RelayCommand(AddNewItem);
            ApplyConfigCommand = new RelayCommand(ApplyConfig);
            RefreshListCommand = new RelayCommand(RefreshConfigList);
            SetDefaultConfigCommand = new RelayCommand(SetDefaultItem);
            OverwriteConfigCommand = new RelayCommand(OverwriteConfig);
            SaveAsNewConfigCommand = new RelayCommand(SaveAsNewConfig);
            ResetEditingConfigCommand = new RelayCommand(ResetConfig);
            PreviewConfigCommand = new RelayCommand(PreviewConfig);
            DeleteConfigCommand = new RelayCommand(DeleteConfig);
            RenameConfigCommand = new RelayCommand(RenameConfig);
            this.DataContext = this;
        }
        public DIY_FFB_Pedal Plugin
        {
            get { return _plugin; }
            set
            {
                if (_plugin != value)
                {
                    _plugin = value;
                    InitializePlugin(_plugin);
                }
            }
        }
        private void InitializePlugin(DIY_FFB_Pedal data)
        {
            if (data != null)
            {
                
                //this.DataContext = data;
                ItemList= data.ConfigService.ConfigList;
            }
        }
        unsafe private void AddNewItem(object parameter)
        {
            NameInputWindow sideWindow = new NameInputWindow();
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {
                string nameGet = sideWindow.input;
                Plugin._calculations.ConfigEditing[Plugin.Settings.table_selected] = nameGet+".json";
                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                //ItemList.Add(new ConfigListItem { ListName = nameGet });
                //_plugin.wpfHandle.ToastNotification("Test", "New Config:" + nameGet);
                DAP_config_st tmp = _plugin.DefaultConfig;
                tmp.payloadFooter_.enfOfFrame0_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[0];
                tmp.payloadFooter_.enfOfFrame1_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[1];
                tmp.payloadHeader_.startOfFrame0_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[0];
                tmp.payloadHeader_.startOfFrame1_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[1];
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.configHash_u32 = Plugin.ConfigService.ConfigHashMap.Fnv1aHash(nameGet+".json");
                DAP_config_st* v = &tmp;
                byte* p = (byte*)v;
                tmp.payloadFooter_.checkSum = _plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));

                string currentDirectory = Directory.GetCurrentDirectory();
                string simhubCommonFolder = currentDirectory + "\\PluginsData\\Common";
                string baseFolderPath = System.IO.Path.Combine(simhubCommonFolder, baseFolderName);
                string configFolderPath = Path.Combine(baseFolderPath, configFolderName);
                string fileName = Path.Combine(configFolderPath, nameGet);
                fileName += ".json";
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                serializer.WriteObject(writer, tmp);
                writer.Flush();

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                // Check if file already exists. If yes, delete it.     
                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }
                System.IO.File.WriteAllText(fileName, jsonString);
                _plugin.ConfigService.RefreshConfigList();
            }


        }
        private void ApplyConfig(object parameter)
        {
            //ItemList.Add(new ConfigListItem { ListName = $"New Item {ItemList.Count + 1}" });
            if (parameter is ConfigListItem item)
            {


                tmp_config = _plugin.ConfigService.ReadConfig(item.FullPath);
                tmp_config.payloadPedalConfig_.configHash_u32 = Plugin.ConfigService.ConfigHashMap.Fnv1aHash(item.FileName);
                Plugin._calculations.ConfigEditing[Plugin.Settings.table_selected] = item.FileName;
                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                tmp_config.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;
                _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected] = tmp_config;
                //_plugin.SendConfigWithoutSaveToEEPROM(_plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected], (byte)_plugin.Settings.table_selected);
                _plugin.wpfHandle.updateTheGuiFromConfig();

            }
            //_plugin.wpfHandle.ToastNotification("Test", "Apply Config");
        }

        private void RefreshConfigList(object parameter)
        {
            //ItemList.Add(new ConfigListItem { ListName = $"New Item {ItemList.Count + 1}" });
            //_plugin.wpfHandle.ToastNotification("Test", "Refresh Config");
            if (_plugin != null) _plugin.ConfigService.RefreshConfigList();
        }

        unsafe private void OverwriteConfig(object parameter)
        {
            //ItemList.Add(new ConfigListItem { ListName = $"New Item {ItemList.Count + 1}" });
            //_plugin.wpfHandle.ToastNotification("Test", "OverWrtie Config");
            if (parameter is ConfigListItem item)
            {
                string fileName = item.FullPath;
                Plugin._calculations.ConfigEditing[Plugin.Settings.table_selected] = item.FileName;
                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                DAP_config_st tmp = _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected];
                tmp.payloadFooter_.enfOfFrame0_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[0];
                tmp.payloadFooter_.enfOfFrame1_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[1];
                tmp.payloadHeader_.startOfFrame0_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[0];
                tmp.payloadHeader_.startOfFrame1_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[1];
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.configHash_u32 = Plugin.ConfigService.ConfigHashMap.Fnv1aHash(item.FileName);
                DAP_config_st* v = &tmp;
                byte* p = (byte*)v;
                tmp.payloadFooter_.checkSum = _plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                serializer.WriteObject(writer, tmp);
                writer.Flush();

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                // Check if file already exists. If yes, delete it.     
                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }
                System.IO.File.WriteAllText(fileName, jsonString);
                _plugin.ConfigService.RefreshConfigList();

            }
        }
        unsafe private void SaveAsNewConfig(object parameter)
        {
            NameInputWindow sideWindow = new NameInputWindow();
            double screenWidth = SystemParameters.PrimaryScreenWidth;
            double screenHeight = SystemParameters.PrimaryScreenHeight;
            sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
            sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
            if (sideWindow.ShowDialog() == true)
            {
                string nameGet = sideWindow.input;
                //ItemList.Add(new ConfigListItem { ListName = nameGet });
                //_plugin.wpfHandle.ToastNotification("Test", "New Config:" + nameGet);
                Plugin._calculations.ConfigEditing[Plugin.Settings.table_selected] = nameGet + ".json";

                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                DAP_config_st tmp = _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected];

                tmp.payloadFooter_.enfOfFrame0_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[0];
                tmp.payloadFooter_.enfOfFrame1_u8 = _plugin.wpfHandle.ENDOFFRAMCHAR[1];
                tmp.payloadHeader_.startOfFrame0_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[0];
                tmp.payloadHeader_.startOfFrame1_u8 = _plugin.wpfHandle.STARTOFFRAMCHAR[1];
                tmp.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
                tmp.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
                tmp.payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;
                tmp.payloadPedalConfig_.configHash_u32 = Plugin.ConfigService.ConfigHashMap.Fnv1aHash(nameGet + ".json");
                DAP_config_st* v = &tmp;
                byte* p = (byte*)v;
                tmp.payloadFooter_.checkSum = _plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));

                string currentDirectory = Directory.GetCurrentDirectory();
                string simhubCommonFolder = currentDirectory + "\\PluginsData\\Common";
                string baseFolderPath = System.IO.Path.Combine(simhubCommonFolder, baseFolderName);
                string configFolderPath = Path.Combine(baseFolderPath, configFolderName);
                string fileName = Path.Combine(configFolderPath, nameGet);
                fileName += ".json";
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_config_st));
                serializer.WriteObject(writer, tmp);
                writer.Flush();

                stream1.Position = 0;
                StreamReader sr = new StreamReader(stream1);
                string jsonString = sr.ReadToEnd();

                // Check if file already exists. If yes, delete it.     
                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }
                System.IO.File.WriteAllText(fileName, jsonString);
                _plugin.ConfigService.RefreshConfigList();
            }
        }
        unsafe private void SetDefaultItem(object parameter)
        {
            if (_plugin != null)
            {
                if (parameter is ConfigListItem item)
                {
                    Plugin.Settings.DefaultConfig[Plugin.Settings.table_selected] = item.FileName;
                    Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                    DAP_config_st tmp = _plugin.ConfigService.ReadConfig(item.FullPath);
                    tmp.payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
                    tmp.payloadPedalConfig_.configHash_u32 = Plugin.ConfigService.ConfigHashMap.Fnv1aHash(item.FileName);
                    tmp.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;
                    tmp.payloadHeader_.storeToEeprom = (byte)1;
                    DAP_config_st* v = &tmp;
                    byte* p = (byte*)v;
                    tmp.payloadFooter_.checkSum = _plugin.checksumCalc(p, sizeof(payloadHeader) + sizeof(payloadPedalConfig));
                    _plugin.SendConfig(tmp, (byte)_plugin.Settings.table_selected);
                }

            }
        }

        private void ResetConfig(object parameter)
        {
            if (_plugin != null)
            {
                _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected] = _plugin.DefaultConfig;
                _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected].payloadHeader_.PedalTag = (byte)_plugin.Settings.table_selected;
                _plugin.wpfHandle.dap_config_st[_plugin.Settings.table_selected].payloadPedalConfig_.pedal_type= (byte)_plugin.Settings.table_selected;
                _plugin.wpfHandle.updateTheGuiFromConfig();
                Plugin._calculations.ConfigEditing[Plugin.Settings.table_selected] = string.Empty;
                Plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
            }
        }

        private void PreviewConfig(object parameter)
        {
            if (parameter is ConfigListItem item && _plugin != null)
            {
                
                ConfigPreviewWindow sideWindow = new ConfigPreviewWindow(_plugin,item);
                double screenWidth = SystemParameters.PrimaryScreenWidth;
                double screenHeight = SystemParameters.PrimaryScreenHeight;
                sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
                sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;
                sideWindow.Show();
            }

        }
        private void DeleteConfig(object parameter)
        {
            if (parameter is ConfigListItem item)
            {
                try
                {
                    string fullPathToDelete = item.FullPath;
                    string MSG_tmp = "Please confirm whether you want to proceed with the config delete:" + item.FileName + ".";
                    var result = System.Windows.MessageBox.Show(MSG_tmp, "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                    if (result == MessageBoxResult.OK)
                    {
                        if (File.Exists(fullPathToDelete))
                        {
                            File.Delete(fullPathToDelete);
                            _plugin.ConfigService.RefreshConfigList();

                        }
                        else
                        {
                            System.Windows.MessageBox.Show("Error, couldn't find config json file", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                            return;
                        }

                    }
                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error($"Config delete error: {ex.Message}");
                    throw;
                }
            }
        }
        private void RenameConfig(object parameter)
        {
            if (parameter is ConfigListItem item)
            {
                NameInputWindow sideWindow = new NameInputWindow();
                double screenWidth = SystemParameters.PrimaryScreenWidth;
                double screenHeight = SystemParameters.PrimaryScreenHeight;
                sideWindow.Left = screenWidth / 2 - sideWindow.Width / 2;
                sideWindow.Top = screenHeight / 2 - sideWindow.Height / 2;

                sideWindow.input = item.ListNameOrig;

                if (sideWindow.ShowDialog() == true)
                {
                    string newName = sideWindow.input;

                    if (string.IsNullOrEmpty(newName) || newName.Equals(item.ListNameOrig, StringComparison.Ordinal))
                    {
                        return;
                    }

                    string oldFileName = item.FileName;
                    string newFileName = newName + ".json";
                    string oldPath = System.IO.Path.Combine(_plugin.configFolderPath, oldFileName);
                    string newPath = System.IO.Path.Combine(_plugin.configFolderPath, newFileName);

                    try
                    {
                        if (File.Exists(newPath))
                        {
                            System.Windows.MessageBox.Show("The file is already exist.", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                            return;
                        }
                        File.Move(oldPath, newPath);

                        for (int i = 0; i < 3; i++)
                        {
                            if (_plugin._calculations.ConfigEditing[i].Equals(item.FileName, StringComparison.Ordinal))
                            {
                                _plugin._calculations.ConfigEditing[i] = newFileName;
                            }
                            if (_plugin.Settings.DefaultConfig[i].Equals(item.FileName, StringComparison.Ordinal))
                            {
                                _plugin.Settings.DefaultConfig[i] = newFileName;
                            }
                        }

                        //_plugin._calculations.ProfileEditing = newFileName;
                        _plugin.ConfigService.RefreshConfigList();
                        _plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                        System.Windows.MessageBox.Show("If the config is already set in porfile, please also rebind the config into profile", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);

                    }
                    catch (Exception ex)
                    {

                    }
                }
            }
        }
    }
}
