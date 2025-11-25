using SimHub.Plugins.ProfilesCommon;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
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

namespace User.PluginSdkDemo.UIFunction
{
    /// <summary>
    /// SystemSetting_ProfilesNew.xaml 的互動邏輯
    /// </summary>
    public partial class SystemSetting_ProfilesNew : UserControl, INotifyPropertyChanged
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
                }
            }
        }
        private DAP_system_profile_cls _tmpProfile;
        public DAP_system_profile_cls tmpProfile
        {
            get { return _tmpProfile; }
            set
            {
                if (_tmpProfile != value)
                {
                    _tmpProfile = value;
                    OnPropertyChanged();
                }
            }
        }
        private ObservableCollection<ProfileListItem> _itemList;
        public ObservableCollection<ProfileListItem> ItemList
        {
            get { return _itemList; }
            set { _itemList = value; OnPropertyChanged(); }
        }

        private ObservableCollection<ConfigListItem> _configList;
        public ObservableCollection<ConfigListItem> ConfigList
        {
            get { return _configList; }
            set { _configList = value; OnPropertyChanged(); }
        }
        public event PropertyChangedEventHandler PropertyChanged;
        protected virtual void OnPropertyChanged([CallerMemberName] string propertyName = null)
        {
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        }

        public ICommand ReadProfileCommand { get; }
        public ICommand ApplyProfileCommand { get; }
        public ICommand AddNewProfileCommand { get; }
        public ICommand RefreshListCommand { get; }
        public ICommand OverwriteProfileCommand { get; }
        public ICommand SaveAsNewProfileCommand { get; }
        public ICommand DeleteProfileCommand { get; }
        public ICommand RenameProfileCommand { get; }
        private string _currentProfileName = string.Empty;
        public string CurrentProfileName
        {
            get { return _currentProfileName; }
            set
            {
                if (_currentProfileName != value)
                {
                    _currentProfileName = value;
                    OnPropertyChanged();
                }
            }
        }

        private void InitializePlugin(DIY_FFB_Pedal data)
        {
            if (data != null)
            {

                //this.DataContext = data;
                ItemList = data.ProfileServicePlugin.ProfileList;
                ConfigList = data.ConfigService.ConfigList;
            }
        }
        public SystemSetting_ProfilesNew()
        {
            InitializeComponent();
            tmpProfile = new DAP_system_profile_cls();
            if (_plugin != null)
            {
                ItemList = _plugin.ProfileServicePlugin.ProfileList;
                ConfigList = _plugin.ConfigService.ConfigList;
            }
            AddNewProfileCommand = new RelayCommand(AddNewProfile);
            ReadProfileCommand = new RelayCommand(ReadProfile);
            ApplyProfileCommand = new RelayCommand(ApplyProfile);
            RefreshListCommand = new RelayCommand(RefreshProfileList);
            OverwriteProfileCommand = new RelayCommand(OverwriteProfile);
            SaveAsNewProfileCommand = new RelayCommand(SaveAsNewProfile);
            DeleteProfileCommand = new RelayCommand(DeleteProfile);
            RenameProfileCommand = new RelayCommand(RenameProfile);
            this.DataContext = this;
        }
        private void ReadProfile(object parameter)
        {
            if (parameter is ProfileListItem item)
            {
                string fileName = item.FullPath;
                CurrentProfileName = item.ListNameOrig;
                _plugin._calculations.ProfileEditing = item.FileName;
                _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
                DAP_system_profile_cls _newTmp = _plugin.ProfileServicePlugin.LoadProfileFromJsonFile(fileName);
                if (_newTmp != null)
                {
                    this.tmpProfile = _newTmp;
                }


            }

        }

        private void RefreshProfileList(object parameter)
        {
            if (_plugin != null) _plugin.ProfileServicePlugin.RefreshProfileList();
        }

        unsafe private void OverwriteProfile(object parameter)
        {
            //ItemList.Add(new ConfigListItem { ListName = $"New Item {ItemList.Count + 1}" });
            //_plugin.wpfHandle.ToastNotification("Test", "OverWrtie Config");
            if (parameter is ProfileListItem item)
            {
                string fileName = item.FullPath;
                _plugin._calculations.ProfileEditing = item.FileName;
                _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
                CurrentProfileName = item.ListNameOrig;
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_system_profile_cls));
                serializer.WriteObject(writer, tmpProfile);
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
                _plugin.ProfileServicePlugin.RefreshProfileList();

            }
        }
        unsafe private void SaveAsNewProfile(object parameter)
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
                _plugin._calculations.ProfileEditing = nameGet + ".json";


                string fileName = System.IO.Path.Combine(_plugin.profileFolderPath, nameGet);
                fileName += ".json";
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_system_profile_cls));
                serializer.WriteObject(writer, tmpProfile);
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
                _plugin.ProfileServicePlugin.RefreshProfileList();
                _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
                var foundItem = _plugin.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _plugin._calculations.ProfileEditing);
                CurrentProfileName = foundItem.ListNameOrig;
            }
        }
        unsafe private void AddNewProfile(object parameter)
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
                _plugin._calculations.ProfileEditing = nameGet + ".json";
                tmpProfile = new DAP_system_profile_cls();
                string fileName = System.IO.Path.Combine(_plugin.profileFolderPath, nameGet);
                fileName += ".json";
                var stream1 = new MemoryStream();
                var writer = JsonReaderWriterFactory.CreateJsonWriter(stream1, Encoding.UTF8, true, true, "  ");
                var serializer = new DataContractJsonSerializer(typeof(DAP_system_profile_cls));
                serializer.WriteObject(writer, tmpProfile);
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
                _plugin.ProfileServicePlugin.RefreshProfileList();
                _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
                var foundItem = _plugin.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FileName == _plugin._calculations.ProfileEditing);
                CurrentProfileName = foundItem.ListNameOrig;
            }


        }
        public void ApplyProfileOnUiWithPath(string profilePath)
        {
            if (profilePath != null) this.tmpProfile = _plugin.ProfileServicePlugin.LoadProfileFromJsonFile(profilePath);
            _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
            var foundItem = _plugin.ProfileServicePlugin.ProfileList.FirstOrDefault(item => item.FullPath == profilePath);
            _plugin._calculations.ProfileEditing = foundItem.FileName;
            CurrentProfileName = foundItem.ListNameOrig;
        }

        private void ApplyProfile(object parameter)
        {
            if (parameter is ProfileListItem item)
            {
                string fileName = item.FullPath;
                CurrentProfileName = item.ListNameOrig;
                _plugin._calculations.ProfileEditing = item.FileName;
                _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();
                DAP_system_profile_cls _newTmp = _plugin.ProfileServicePlugin.LoadProfileFromJsonFile(fileName);
                if (_newTmp != null)
                {
                    this.tmpProfile = _newTmp;
                }
                //write to system
                _plugin.ProfileServicePlugin.ApplyProfile(item.FullPath);
            }
        }

        private void Btn_ClearClutch_Click(object sender, RoutedEventArgs e)
        {
            tmpProfile.ConfigPath[0] = string.Empty;
            for (int i = 0; i < 8; i++)
            {
                tmpProfile.Effects[0][i] = false;
            }
            this.tmpProfile = this.tmpProfile;
            OnPropertyChanged(nameof(tmpProfile));
        }

        private void Btn_ClearBrake_Click(object sender, RoutedEventArgs e)
        {
            tmpProfile.ConfigPath[1] = string.Empty;
            for (int i = 0; i < 8; i++)
            {
                tmpProfile.Effects[1][i] = false;
            }
            this.tmpProfile = this.tmpProfile;
            OnPropertyChanged(nameof(tmpProfile));
        }

        private void Btn_ClearThrottle_Click(object sender, RoutedEventArgs e)
        {
            DAP_system_profile_cls tmp = tmpProfile;
            tmp.ConfigPath[2] = string.Empty;
            for (int i = 0; i < 8; i++)
            {
                tmp.Effects[2][i] = false;
            }
            this.tmpProfile = tmp;
            OnPropertyChanged(nameof(tmpProfile));
        }
        private void DeleteProfile(object parameter)
        {
            if (parameter is ProfileListItem item)
            {
                try
                {
                    string fullPathToDelete = item.FullPath;
                    string MSG_tmp = "Please confirm whether you want to proceed with the profile delete:" + item.FileName + ".";
                    var result = System.Windows.MessageBox.Show(MSG_tmp, "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                    if (result == MessageBoxResult.OK)
                    {
                        if (File.Exists(fullPathToDelete))
                        {
                            File.Delete(fullPathToDelete);
                            _plugin.ProfileServicePlugin.RefreshProfileList();

                        }
                        else
                        {
                            System.Windows.MessageBox.Show("Error, couldn't find profile json file", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                            return;
                        }

                    }
                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error($"Profile delete error: {ex.Message}");
                    throw;
                }
            }
        }

        private void RenameProfile(object parameter)
        {
            if (parameter is ProfileListItem item)
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
                    string oldPath = System.IO.Path.Combine(_plugin.profileFolderPath, oldFileName);
                    string newPath = System.IO.Path.Combine(_plugin.profileFolderPath, newFileName);

                    try
                    {
                        if (File.Exists(newPath))
                        {
                            System.Windows.MessageBox.Show("The file is already exist.", "Warning", MessageBoxButton.OKCancel, MessageBoxImage.Question);
                            return;
                        }
                        File.Move(oldPath, newPath);
                        if (_plugin._calculations.ProfileEditing.Equals(item.FileName, StringComparison.Ordinal))
                        {
                            _plugin._calculations.ProfileEditing = newFileName;
                        }
                        //_plugin._calculations.ProfileEditing = newFileName;
                        _plugin.ProfileServicePlugin.RefreshProfileList();
                        _plugin.ProfileServicePlugin.UpdateProfileLabelDefaultAndEditing();

                    }
                    catch (Exception ex)
                    {
                        
                    }
                }
            }
        }

        private void Btn_ClearCarName_Click(object sender, RoutedEventArgs e)
        {
            tmpProfile.BindGameOrCar = string.Empty;
            OnPropertyChanged(nameof(tmpProfile));
        }
    }
}
