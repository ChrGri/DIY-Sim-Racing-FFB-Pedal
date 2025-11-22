using Newtonsoft.Json;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Documents;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public ObservableCollection<ConfigListItem> ConfigList { get; set; }
        public ObservableCollection<ProfileListItem> ProfileList { get; set; }
        private const string configFolderName = "configs";
        private const string profileFolderName = "profiles";
        private const string logFolderName = "log";
        private const string baseFolderName = "DiyFfbPedal";
        private const string simhubPluginDataFolderName = "PluginData";
        private const string simhubPluginDataCommonFolderName = "Common";
        public string configFolderPath = string.Empty;
        public string profileFolderPath = string.Empty;
        public string logFolderPath = string.Empty;
        public void EnsureFolderExistsAndProcess()
        {
            
            string currentDirectory = Directory.GetCurrentDirectory();
            string simhubCommonFolder = currentDirectory + "\\PluginsData\\Common";
            string baseFolderPath = Path.Combine(simhubCommonFolder, baseFolderName);
            configFolderPath= Path.Combine(baseFolderPath, configFolderName);
            profileFolderPath = Path.Combine(baseFolderPath, profileFolderName);
            logFolderPath = Path.Combine(baseFolderPath, logFolderName);
            if (!Directory.Exists(baseFolderPath))
            {
                try
                {
                    Directory.CreateDirectory(baseFolderPath);
                }
                catch (Exception ex)
                {
                    return;
                }
            }
            if (!Directory.Exists(configFolderPath))
            {
                try
                {
                    Directory.CreateDirectory(configFolderPath);
                }
                catch (Exception ex)
                {
                    return;
                }
            }
            if (!Directory.Exists(profileFolderPath))
            {
                try
                {
                    Directory.CreateDirectory(profileFolderPath);
                }
                catch (Exception ex)
                {
                    return;
                }
            }
            if (!Directory.Exists(logFolderPath))
            {
                try
                {
                    Directory.CreateDirectory(logFolderPath);
                }
                catch (Exception ex)
                {
                    return;
                }
            }
            RefreshConfigList();
            RefreshProfileList();
            
        }

        public void UpdateConfigLabelDefaultAndEditing()
        {
            if (ConfigList.Count>0)
            {
                //reset all config listname
                foreach (ConfigListItem item in ConfigList)
                {
                    item.ListName = item.ListNameOrig;
                }
                var foundItem= ConfigList.FirstOrDefault(item => item.FileName == Settings.DefaultConfig[Settings.table_selected]);
                if(foundItem!=null) foundItem.ListName = foundItem.ListNameOrig+ "(default)";
                foundItem = ConfigList.FirstOrDefault(item => item.FileName == _calculations.ConfigEditing[Settings.table_selected]);
                if (foundItem != null) foundItem.ListName += "(editing)";
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
                var foundItem= this.ProfileList.FirstOrDefault(item => item.FileName == _calculations.ProfileEditing);
                if (foundItem != null) foundItem.ListName += "(editing)";
            }

        }
        public void RefreshConfigList()
        {
            string currentDirectory = Directory.GetCurrentDirectory();
            string simhubCommonFolder = currentDirectory + "\\PluginsData\\Common";
            string baseFolderPath = Path.Combine(simhubCommonFolder, baseFolderName);
            string configFolderPath = Path.Combine(baseFolderPath, configFolderName);
            string profileFolderPath = Path.Combine(baseFolderPath, profileFolderName);
            try
            {

                if (ConfigList == null) ConfigList = new ObservableCollection<ConfigListItem> { };
                if (ConfigList.Count > 0) { ConfigList.Clear(); }


                if (string.IsNullOrEmpty(configFolderPath) || !Directory.Exists(configFolderPath))
                {
                    return;
                }

                try
                {

                    string[] fullPaths = Directory.GetFiles(configFolderPath, "*.json");


                    foreach (var path in fullPaths)
                    {
                        ConfigListItem item = new ConfigListItem();
                        item.FileName = Path.GetFileName(path);
                        item.ListName = Path.GetFileNameWithoutExtension(path);
                        item.ListNameOrig = item.ListName;
                        item.FullPath = Path.GetFullPath(path);
                        item.IsDefault = false;
                        item.IsCurrent = false;

                        ConfigList.Add(item);
                    }
                    UpdateConfigLabelDefaultAndEditing();
                }
                catch (Exception ex)
                {

                }

            }
            catch (Exception ex)
            {
            }
        }

        public void RefreshProfileList()
        {

            try
            {

                if (ProfileList == null) ProfileList = new ObservableCollection<ProfileListItem> { };
                if (ProfileList.Count > 0) { ProfileList.Clear(); }


                if (string.IsNullOrEmpty(profileFolderPath) || !Directory.Exists(profileFolderPath))
                {
                    return;
                }

                try
                {

                    string[] fullPaths = Directory.GetFiles(profileFolderPath, "*.json");


                    foreach (var path in fullPaths)
                    {
                        ProfileListItem item = new ProfileListItem();
                        item.FileName = Path.GetFileName(path);
                        item.ListName = Path.GetFileNameWithoutExtension(path);
                        item.ListNameOrig = item.ListName;
                        item.FullPath = Path.GetFullPath(path);
                        item.IsDefault = false;
                        item.IsCurrent = false;

                        ProfileList.Add(item);
                    }
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

        public DAP_config_st ReadConfig(string filePath)
        {
            
            DAP_config_st config = new DAP_config_st();
            config = DefaultConfig;
            if (!File.Exists(filePath)) return config;
            // Read the entire JSON file
            try
            {
                string jsonString = File.ReadAllText(filePath);

                // Parse all of the JSON.
                //JsonNode forecastNode = JsonNode.Parse(jsonString);
                dynamic data = JsonConvert.DeserializeObject(jsonString);

                int version = 0;
                byte[] compatibleForce = new byte[6];
                bool compatibleMode = false;
                try
                {
                    version = (int)data["payloadHeader_"]["version"];

                    if (version < 150)
                    {
                        compatibleMode = true;
                        compatibleForce[0] = (byte)data["payloadPedalConfig_"]["relativeForce_p000"];
                        compatibleForce[1] = (byte)data["payloadPedalConfig_"]["relativeForce_p020"];
                        compatibleForce[2] = (byte)data["payloadPedalConfig_"]["relativeForce_p040"];
                        compatibleForce[3] = (byte)data["payloadPedalConfig_"]["relativeForce_p060"];
                        compatibleForce[4] = (byte)data["payloadPedalConfig_"]["relativeForce_p080"];
                        compatibleForce[5] = (byte)data["payloadPedalConfig_"]["relativeForce_p100"];
                    }
                }
                catch (Exception ex)
                {

                }
                config = JsonConvert.DeserializeObject<DAP_config_st>(jsonString);
                if (compatibleMode)
                {
                    config.payloadPedalConfig_.quantityOfControl = 6;
                    config.payloadPedalConfig_.relativeForce00 = compatibleForce[0];
                    config.payloadPedalConfig_.relativeForce01 = compatibleForce[1];
                    config.payloadPedalConfig_.relativeForce02 = compatibleForce[2];
                    config.payloadPedalConfig_.relativeForce03 = compatibleForce[3];
                    config.payloadPedalConfig_.relativeForce04 = compatibleForce[4];
                    config.payloadPedalConfig_.relativeForce05 = compatibleForce[5];
                    config.payloadPedalConfig_.relativeTravel00 = 0;
                    config.payloadPedalConfig_.relativeTravel01 = 20;
                    config.payloadPedalConfig_.relativeTravel02 = 40;
                    config.payloadPedalConfig_.relativeTravel03 = 60;
                    config.payloadPedalConfig_.relativeTravel04 = 80;
                    config.payloadPedalConfig_.relativeTravel05 = 100;
                    config.payloadPedalConfig_.numOfJoystickMapControl = 6;
                    config.payloadPedalConfig_.joystickMapMapped00 = 0;
                    config.payloadPedalConfig_.joystickMapMapped01 = 20;
                    config.payloadPedalConfig_.joystickMapMapped02 = 40;
                    config.payloadPedalConfig_.joystickMapMapped03 = 60;
                    config.payloadPedalConfig_.joystickMapMapped04 = 80;
                    config.payloadPedalConfig_.joystickMapMapped05 = 100;
                    config.payloadPedalConfig_.joystickMapOrig00 = 0;
                    config.payloadPedalConfig_.joystickMapOrig01 = 20;
                    config.payloadPedalConfig_.joystickMapOrig02 = 40;
                    config.payloadPedalConfig_.joystickMapOrig03 = 60;
                    config.payloadPedalConfig_.joystickMapOrig04 = 80;
                    config.payloadPedalConfig_.joystickMapOrig05 = 100;

                }

                if (config.payloadPedalConfig_.spindlePitch_mmPerRev_u8 == 0)
                {
                    config.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
                }
                if (config.payloadPedalConfig_.kf_modelNoise == 0)
                {
                    config.payloadPedalConfig_.kf_modelNoise = 5;
                }
                if (config.payloadPedalConfig_.pedal_type != Settings.table_selected)
                {
                    config.payloadPedalConfig_.pedal_type = (byte)Settings.table_selected;

                }
                if (config.payloadPedalConfig_.lengthPedal_a == 0)
                {
                    config.payloadPedalConfig_.lengthPedal_a = 205;
                }
                if (config.payloadPedalConfig_.lengthPedal_b == 0)
                {
                    config.payloadPedalConfig_.lengthPedal_b = 220;
                }
                if (config.payloadPedalConfig_.lengthPedal_d < 0)
                {
                    config.payloadPedalConfig_.lengthPedal_d = 60;
                }
                if (config.payloadPedalConfig_.lengthPedal_c_horizontal == 0)
                {
                    config.payloadPedalConfig_.lengthPedal_c_horizontal = 215;
                }
                if (config.payloadPedalConfig_.lengthPedal_c_vertical == 0)
                {
                    config.payloadPedalConfig_.lengthPedal_c_vertical = 60;
                }
                if (config.payloadPedalConfig_.lengthPedal_travel == 0)
                {
                    config.payloadPedalConfig_.lengthPedal_travel = 100;
                }
                if (config.payloadPedalConfig_.pedalStartPosition < 5)
                {
                    config.payloadPedalConfig_.pedalStartPosition = 5;
                }
                if (config.payloadPedalConfig_.pedalEndPosition > 95)
                {
                    config.payloadPedalConfig_.pedalEndPosition = 95;
                }

                
            }
            catch (Exception ex)
            {
                SimHub.Logging.Current.Error($"Profile save error: {ex.Message}");
                throw;
            }
            return config;

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

        public void ApplyProfile(string profilePath)
        { 
            DAP_system_profile_cls tmpProfile= LoadProfileFromJsonFile(profilePath);
            for (int i = 0; i < 3; i++)
            {
                if (tmpProfile.ConfigPath[i] != "" && File.Exists(tmpProfile.ConfigPath[i]))
                {
                    DAP_config_st tmpConfig = ReadConfig(tmpProfile.ConfigPath[i]);
                    wpfHandle.dap_config_st[i]=tmpConfig;
                    SendConfigWithoutSaveToEEPROM(tmpConfig,(byte)i);
                    _calculations.ConfigEditing[i]= ConfigList.FirstOrDefault(item => item.FullPath == tmpProfile.ConfigPath[i]).FileName;
                    //write the effect setting
                    if (tmpProfile.Effects[i][0])
                    {
                        Settings.ABS_enable_flag[i] = 1;
                    }
                    else
                    {
                        Settings.ABS_enable_flag[i] = 0;
                    }

                    if (tmpProfile.Effects[i][1])
                    {
                        Settings.RPM_enable_flag[i] = 1;
                    }
                    else
                    {
                        Settings.RPM_enable_flag[i] = 0;
                    }

                    if (tmpProfile.Effects[i][3])
                    {
                        Settings.G_force_enable_flag[i] = 1;
                    }
                    else
                    {
                        Settings.G_force_enable_flag[i] = 0;
                    }

                    if (tmpProfile.Effects[i][4])
                    {
                        Settings.WS_enable_flag[i] = 1;
                    }
                    else
                    {
                        Settings.WS_enable_flag[i] = 0;
                    }

                    if (tmpProfile.Effects[i][5])
                    {
                        Settings.Road_impact_enable_flag[i] = 1;
                    }
                    else
                    {
                        Settings.Road_impact_enable_flag[i] = 0;
                    }
                    Settings.CV1_enable_flag[i] = tmpProfile.Effects[i][6];
                    Settings.CV2_enable_flag[i] = tmpProfile.Effects[i][7];
                    System.Threading.Thread.Sleep(100);
                }
            }
            UpdateConfigLabelDefaultAndEditing();
            //wpfHandle.updateTheGuiFromConfig();
        }
    }
}
