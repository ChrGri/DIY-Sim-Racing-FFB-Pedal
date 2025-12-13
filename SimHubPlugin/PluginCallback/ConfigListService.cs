using Newtonsoft.Json;
using SimHub.Plugins;
using SimHub.Plugins.ProfilesCommon;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public class ConfigFileHashMap
        {
            private Dictionary<string, uint> _nameToHash = new Dictionary<string, uint>(StringComparer.OrdinalIgnoreCase);
            private Dictionary<uint, string> _hashToName = new Dictionary<uint, string>();
            public void Add(string fileName)
            {
                uint hash = Fnv1aHash(fileName);
                if (_hashToName.ContainsKey(hash))
                {
                    string existingFile = _hashToName[hash];
                    if (!existingFile.Equals(fileName, StringComparison.OrdinalIgnoreCase))
                    {
                        throw new InvalidOperationException($"Same Hash founded! '{fileName}' and '{existingFile}'  Hash was the same: {hash}");
                    }
                    return;
                }
                if (_nameToHash.ContainsKey(fileName))
                {
                    uint oldHash = _nameToHash[fileName];
                    _hashToName.Remove(oldHash);
                }
                _nameToHash[fileName] = hash;
                _hashToName[hash] = fileName;
            }
            public uint Fnv1aHash(string text)
            {
                const uint FnvPrime = 16777619;
                const uint OffsetBasis = 2166136261;

                uint hash = OffsetBasis;
                byte[] bytes = System.Text.Encoding.UTF8.GetBytes(text);

                foreach (byte b in bytes)
                {
                    hash ^= b;
                    hash *= FnvPrime;
                }

                return hash;
            }
            public uint? GetHash(string fileName)
            {
                if (_nameToHash.TryGetValue(fileName, out uint hash))
                {
                    return hash;
                }
                return null;
            }

            public string GetFileName(uint hash)
            {
                if (_hashToName.TryGetValue(hash, out string name))
                {
                    return name;
                }
                return string.Empty;
            }
            public void Clear()
            {
                _nameToHash.Clear();
                _hashToName.Clear();
            }
            public int Count()
            {
                return _nameToHash.Count;
            }

        }




        public class ConfigListService
        {
            public ConfigFileHashMap ConfigHashMap = new ConfigFileHashMap();
            public ObservableCollection<ConfigListItem> ConfigList { get; set; }
            private DIY_FFB_Pedal _plugin;
            public ConfigListService(DIY_FFB_Pedal Plugin)
            {
                _plugin = Plugin;
            }
            public void UpdateConfigLabelDefaultAndEditing()
            {
                if (ConfigList.Count > 0)
                {
                    //reset all config listname
                    foreach (ConfigListItem item in ConfigList)
                    {
                        item.ListName = item.ListNameOrig;
                    }
                    var foundItem = ConfigList.FirstOrDefault(item => item.FileName == _plugin.Settings.DefaultConfig[_plugin.Settings.table_selected]);
                    if (foundItem != null) foundItem.ListName = foundItem.ListNameOrig + "(Default)";
                    foundItem = ConfigList.FirstOrDefault(item => item.FileName == _plugin._calculations.ConfigEditing[_plugin.Settings.table_selected]);
                    if (foundItem != null) foundItem.ListName += "(Loaded)";
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
                    if (ConfigHashMap.Count()>0) ConfigHashMap.Clear();

                    if (string.IsNullOrEmpty(configFolderPath) || !Directory.Exists(configFolderPath))
                    {
                        return;
                    }

                    try
                    {

                        string[] fullPaths = Directory.GetFiles(configFolderPath, "*.json");
                        var itemsToRemove = ConfigList
                        .Where(item => !fullPaths.Any(path => Path.GetFileName(path) == item.FileName))
                        .ToList();

                        foreach (var item in itemsToRemove)
                        {
                            ConfigList.Remove(item);
                        }

                        foreach (var path in fullPaths)
                        {
                            string fileName = Path.GetFileName(path);
                            if (!ConfigList.Any(item => item.FileName == fileName))
                            {
                                ConfigListItem item = new ConfigListItem();
                                item.FileName = Path.GetFileName(path);
                                item.ListName = Path.GetFileNameWithoutExtension(path);
                                item.ListNameOrig = item.ListName;
                                item.FullPath = Path.GetFullPath(path);
                                item.IsDefault = false;
                                item.IsCurrent = false;

                                ConfigList.Add(item);
                                
                                //string fileName = Path.GetFileName(path);
                                
                            }
                            ConfigHashMap.Add(fileName);
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

            public DAP_config_st ReadConfig(string filePath)
            {

                DAP_config_st config = new DAP_config_st();
                config = _plugin.DefaultConfig;
                if (!File.Exists(filePath)) return config;
                // Read the entire JSON file
                try
                {   
                    string filename = Path.GetFileName(filePath);
                    string jsonString = File.ReadAllText(filePath);

                    // Parse all of the JSON.
                    //JsonNode forecastNode = JsonNode.Parse(jsonString);
                    dynamic data = JsonConvert.DeserializeObject(jsonString);
                    uint hash = ConfigHashMap.Fnv1aHash(filename);
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
                    if (config.payloadPedalConfig_.pedal_type != _plugin.Settings.table_selected)
                    {
                        config.payloadPedalConfig_.pedal_type = (byte)_plugin.Settings.table_selected;

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
                    config.payloadPedalConfig_.configHash_u32 = hash;

                }
                catch (Exception ex)
                {
                    SimHub.Logging.Current.Error($"Profile save error: {ex.Message}");
                    throw;
                }
                
                return config;

            }
        }
    }
}

