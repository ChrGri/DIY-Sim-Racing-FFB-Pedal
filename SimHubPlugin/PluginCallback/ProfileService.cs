using Newtonsoft.Json;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public class ProfileService
        {
            public ObservableCollection<ProfileListItem> ProfileList { get; set; } = new ObservableCollection<ProfileListItem>();

            private Dictionary<string, ProfileDictionaryStruct> _bindNameLookup;
            private Dictionary<string, ProfileDictionaryStruct> _bindGameLookup;
            private DIY_FFB_Pedal _plugin;
            private string lastCarName = string.Empty;
            private string lastGameName = string.Empty;
            public bool[] GamePofileConfigChange_b = new bool[3] { false, false, false };
            public string[] GameConfigPathBuffer = new string[3] { string.Empty, string.Empty , string.Empty };
            public DAP_config_st[] ConfigBuffer = new DAP_config_st[3];
            public string CurrentGameProfile = string.Empty;
            public struct ProfileDictionaryStruct
            {
                public string FullPath;
                public ProfileListItem ProfileListItem;
            }
            public ProfileService(DIY_FFB_Pedal Plugin)
            {
                ProfileList.CollectionChanged += ProfileList_CollectionChanged;
                RebuildLookupDictionary();
                _plugin = Plugin;
            }
            private void ProfileList_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
            {
                RebuildLookupDictionary();
            }

            private void RebuildLookupDictionary()
            {
                _bindNameLookup = new Dictionary<string, ProfileDictionaryStruct>(StringComparer.OrdinalIgnoreCase);
                _bindGameLookup = new Dictionary<string, ProfileDictionaryStruct>(StringComparer.OrdinalIgnoreCase);
                foreach (var item in ProfileList)
                {
                    try
                    {
                        DAP_system_profile_cls tempProfile = LoadProfileFromJsonFile(item.FullPath);
                        ProfileDictionaryStruct tempStruct = new ProfileDictionaryStruct
                        {
                            FullPath = item.FullPath,
                            ProfileListItem = item
                        };
                        string rawKeys = tempProfile.BindGameOrCar;

                        if (!string.IsNullOrEmpty(rawKeys))
                        {
                            string[] keyParts = rawKeys.Split(',');

                            foreach (var part in keyParts)
                            {
                                string keyName = part.Trim();
                                if (!string.IsNullOrEmpty(keyName))
                                {
                                    if (PedalConstStrings.AutoProfileSwitchGameList.Contains(keyName))
                                    {
                                        if (!_bindGameLookup.ContainsKey(keyName))
                                        {
                                            _bindGameLookup.Add(keyName, tempStruct);
                                        }
                                    }
                                    else
                                    {
                                        if (!_bindNameLookup.ContainsKey(keyName))
                                        {
                                            _bindNameLookup.Add(keyName, tempStruct);
                                        }
                                    }

                                }
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        SimHub.Logging.Current.Error($"Profile load error: {ex.Message}");
                        throw;
                    }
                }
            }

            public ProfileDictionaryStruct GetProfileByNameFast(string key)
            {
                if (_bindNameLookup.TryGetValue(key, out ProfileDictionaryStruct profileItem))
                {
                    return profileItem;
                }
                return new ProfileDictionaryStruct { FullPath = string.Empty, ProfileListItem = new ProfileListItem() };
            }

            public ProfileDictionaryStruct GetProfileByGameFast(string key)
            {
                if (_bindGameLookup.TryGetValue(key, out ProfileDictionaryStruct profileItem))
                {
                    return profileItem;
                }
                return new ProfileDictionaryStruct { FullPath = string.Empty, ProfileListItem = new ProfileListItem() };
            }
            public void ApplyProfileAutoForCar(string carName)
            {
                bool excuteChecker = true;
                if (carName == "") excuteChecker = false;
                if (carName == lastCarName) excuteChecker = false;



                if (excuteChecker)
                {
                    lastCarName = carName;
                    ProfileDictionaryStruct profileItem = GetProfileByNameFast(carName);
                    string ProfilePath = profileItem.FullPath;
                    if (ProfilePath != string.Empty) //if carmodel matched
                    {
                        if (File.Exists(ProfilePath))
                        {
                            var item = ProfileList.FirstOrDefault(founditem => founditem.FullPath == ProfilePath);
                            if (item != null)
                            {
                                _plugin._calculations.ProfileEditing = item.FileName;
                            }
                            ApplyProfile(ProfilePath);
                            UpdateProfileLabelDefaultAndEditing();
                            _plugin.wpfHandle.ToastNotification("Profile Applied", $"Profile:{item.ListNameOrig} for Car:{carName} applied.");
                        }
                    }

                }

            }
            public void ApplyProfileAutoForGame(string gameName)
            {
                bool excuteChecker = true;
                if (gameName == "") excuteChecker = false;
                if (gameName == lastGameName) excuteChecker = false;



                if (excuteChecker)
                {
                    lastGameName = gameName;
                    ProfileDictionaryStruct profileItem = GetProfileByGameFast(gameName);
                    string ProfilePath = profileItem.FullPath;
                    if (ProfilePath != string.Empty) //if carmodel matched
                    {
                        if (File.Exists(ProfilePath))
                        {
                            var item = ProfileList.FirstOrDefault(founditem => founditem.FullPath == ProfilePath);
                            if (item != null)
                            {
                                _plugin._calculations.ProfileEditing = item.FileName;
                            }
                            ApplyProfileForGame(ProfilePath);
                            CurrentGameProfile= item.FileName;
                            UpdateProfileLabelDefaultAndEditing();
                            //_plugin.wpfHandle.ToastNotification("Profile Applied", $"Profile:{item.ListNameOrig} for Game:{gameName} applied.");
                        }
                    }

                }

            }

            public DAP_system_profile_cls LoadProfileFromJsonFile(string filePath)
            {
                if (!File.Exists(filePath))
                {
                    //return new Profile();
                }

                try
                {
                    string jsonString = File.ReadAllText(filePath, Encoding.UTF8);
                    DAP_system_profile_cls data = (DAP_system_profile_cls)JsonConvert.DeserializeObject(jsonString, typeof(DAP_system_profile_cls));
                    return data != null ? data : new DAP_system_profile_cls();

                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error($"json read error, return new Profile: {ex.Message}");
                    return new DAP_system_profile_cls();
                }
            }
            public void ApplyProfile(string profilePath)
            {
                DAP_system_profile_cls tmpProfile = LoadProfileFromJsonFile(profilePath);
                for (int i = 0; i < 3; i++)
                {
                    if (tmpProfile.ConfigPath[i] != "" && File.Exists(tmpProfile.ConfigPath[i]))
                    {
                        DAP_config_st tmpConfig = _plugin.ConfigService.ReadConfig(tmpProfile.ConfigPath[i]);
                        _plugin.wpfHandle.dap_config_st[i] = tmpConfig;
                        _plugin.SendConfigWithoutSaveToEEPROM(tmpConfig, (byte)i);
                        _plugin._calculations.ConfigEditing[i] = _plugin.ConfigService.ConfigList.FirstOrDefault(item => item.FullPath == tmpProfile.ConfigPath[i]).FileName;

                        //System.Threading.Thread.Sleep(100);
                    }
                    //write the effect setting
                    _plugin.Settings.ABS_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][0]);
                    _plugin.Settings.RPM_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][1]);
                    if (i == 1) _plugin.Settings.G_force_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][3]);
                    _plugin.Settings.WS_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][4]);
                    _plugin.Settings.Road_impact_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][5]);
                    _plugin.Settings.CV1_enable_flag[i] = tmpProfile.Effects[i][6];
                    _plugin.Settings.CV2_enable_flag[i] = tmpProfile.Effects[i][7];
                }
                _plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                //wpfHandle.updateTheGuiFromConfig();
            }
            public void ApplyProfileForGame(string profilePath)
            {
                DAP_system_profile_cls tmpProfile = LoadProfileFromJsonFile(profilePath);
                for (int i = 0; i < 3; i++)
                {
                    if (tmpProfile.ConfigPath[i] != "" && File.Exists(tmpProfile.ConfigPath[i]))
                    {
                        DAP_config_st tmpConfig = _plugin.ConfigService.ReadConfig(tmpProfile.ConfigPath[i]);
                        ConfigBuffer[i] = tmpConfig;
                        GameConfigPathBuffer[i] = tmpProfile.ConfigPath[i];
                        GamePofileConfigChange_b[i] = true;
                        
                        /*
                        _plugin.wpfHandle.dap_config_st[i] = tmpConfig;
                        _plugin.SendConfigWithoutSaveToEEPROM(tmpConfig, (byte)i);
                        _plugin._calculations.ConfigEditing[i] = _plugin.ConfigService.ConfigList.FirstOrDefault(item => item.FullPath == tmpProfile.ConfigPath[i]).FileName;
                        */

                        //System.Threading.Thread.Sleep(100);
                    }
                    //write the effect setting
                    _plugin.Settings.ABS_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][0]);
                    _plugin.Settings.RPM_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][1]);
                    if (i == 1) _plugin.Settings.G_force_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][3]);
                    _plugin.Settings.WS_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][4]);
                    _plugin.Settings.Road_impact_enable_flag[i] = _plugin.BoolToInt(tmpProfile.Effects[i][5]);
                    _plugin.Settings.CV1_enable_flag[i] = tmpProfile.Effects[i][6];
                    _plugin.Settings.CV2_enable_flag[i] = tmpProfile.Effects[i][7];
                }
                _plugin.ConfigService.UpdateConfigLabelDefaultAndEditing();
                //wpfHandle.updateTheGuiFromConfig();
            }
            public void RefreshProfileList()
            {

                try
                {

                    if (ProfileList == null) ProfileList = new ObservableCollection<ProfileListItem> { };
                    //if (ProfileList.Count > 0) { ProfileList.Clear(); }


                    if (string.IsNullOrEmpty(_plugin.profileFolderPath) || !Directory.Exists(_plugin.profileFolderPath))
                    {
                        return;
                    }

                    try
                    {

                        string[] fullPaths = Directory.GetFiles(_plugin.profileFolderPath, "*.json");
                        var itemsToRemove = ProfileList
                            .Where(item => !fullPaths.Any(path => Path.GetFileName(path) == item.FileName))
                            .ToList();

                        foreach (var item in itemsToRemove)
                        {
                            ProfileList.Remove(item);
                        }

                        foreach (var path in fullPaths)
                        {
                            string fileName = Path.GetFileName(path);
                            if (!ProfileList.Any(item => item.FileName == fileName))
                            {
                                ProfileListItem item = new ProfileListItem();
                                item.FileName = fileName;
                                item.ListName = Path.GetFileNameWithoutExtension(path);
                                item.ListNameOrig = item.ListName;
                                item.FullPath = Path.GetFullPath(path);
                                item.IsDefault = false;
                                item.IsCurrent = false;

                                ProfileList.Add(item);
                            }
                        }
                        RebuildLookupDictionary();
                        UpdateProfileLabelDefaultAndEditing();
                    }
                    catch (Exception ex)
                    {

                    }

                }
                catch (Exception ex)
                {
                }
            }
            public void UpdateProfileLabelDefaultAndEditing()
            {
                if (ProfileList.Count > 0)
                {
                    //reset all config listname
                    foreach (ProfileListItem item in this.ProfileList)
                    {
                        item.ListName = item.ListNameOrig;
                    }
                    var foundItem = this.ProfileList.FirstOrDefault(item => item.FileName == _plugin._calculations.ProfileEditing);
                    if (foundItem != null) foundItem.ListName += "(Loaded)";
                }

            }
            public static void SaveProfileToJsonFile(DAP_system_profile_cls profileData, string filePath)
            {
                if (profileData == null)
                {
                    SimHub.Logging.Current.Error("Profile is null, cancel action");
                    return;
                }

                try
                {
                    string jsonString = JsonConvert.SerializeObject(profileData, Formatting.Indented);
                    File.WriteAllText(filePath, jsonString, Encoding.UTF8);

                    SimHub.Logging.Current.Info($"Profile svaed: {filePath}");
                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error($"Profile save error: {ex.Message}");
                    throw;
                }
            }

            public void ClearAutoSwitchStatus()
            {
                lastCarName = string.Empty;
            }
        }
    }
}
