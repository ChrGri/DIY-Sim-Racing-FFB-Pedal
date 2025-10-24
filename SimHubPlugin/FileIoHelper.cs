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
        private const string configFolderName = "configs";
        private const string profileFolderName = "profiles";
        private const string baseFolderName = "DiyFfbPedal";
        private const string simhubPluginDataFolderName = "PluginData";
        private const string simhubPluginDataCommonFolderName = "Common";
        public void EnsureFolderExistsAndProcess()
        {
            
            string currentDirectory = Directory.GetCurrentDirectory();
            string simhubCommonFolder = currentDirectory + "\\PluginsData\\Common";
            string baseFolderPath = Path.Combine(simhubCommonFolder, baseFolderName);
            string configFolderPath= Path.Combine(baseFolderPath, configFolderName);
            string profileFolderPath = Path.Combine(baseFolderPath, profileFolderName);
            
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
            RefreshConfigList();
            
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
                        item.FullPath = Path.GetFullPath(path);
                        item.IsDefault = false;
                        item.IsCurrent = false;

                        ConfigList.Add(item);
                    }
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
            // Read the entire JSON file
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


            payloadPedalConfig payloadPedalConfig_fromJson_st = config.payloadPedalConfig_;
            //var s = default(payloadPedalConfig);
            Object obj = payloadPedalConfig_fromJson_st;// s;



            FieldInfo[] fi = payloadPedalConfig_fromJson_st.GetType().GetFields(BindingFlags.Public | BindingFlags.Instance);

            // Iterate over each field and print its name and value
            foreach (var field in fi)
            {

                if (data["payloadPedalConfig_"][field.Name] != null)
                //if (forecastNode["payloadPedalConfig_"][field.Name] != null)
                {
                    try
                    {
                        if (field.FieldType == typeof(float))
                        {
                            //float value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<float>();
                            float value = (float)data["payloadPedalConfig_"][field.Name];
                            field.SetValue(obj, value);
                        }

                        if (field.FieldType == typeof(byte))
                        {
                            //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                            byte value = (byte)data["payloadPedalConfig_"][field.Name];
                            field.SetValue(obj, value);
                        }

                        if (field.FieldType == typeof(Int16))
                        {
                            //byte value = forecastNode["payloadPedalConfig_"][field.Name].GetValue<byte>();
                            Int16 value = (Int16)data["payloadPedalConfig_"][field.Name];
                            field.SetValue(obj, value);
                        }


                    }
                    catch (Exception)
                    {

                    }

                }
            }

            // set values in global structure
            config.payloadPedalConfig_ = (payloadPedalConfig)obj;// payloadPedalConfig_fromJson_st;
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

            if (compatibleMode)
            {
                //get old verison file, auto convert to new config
                compatibleMode = false;
                config.payloadPedalConfig_.quantityOfControl = 6;
                /*
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce00 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p000;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce01 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p020;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce02 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p040;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce03 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p060;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce04 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p080;
                dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce05 = dap_config_st[indexOfSelectedPedal_u].payloadPedalConfig_.relativeForce_p100;
                */
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
            }
        
            return config;
        }
    }
}
