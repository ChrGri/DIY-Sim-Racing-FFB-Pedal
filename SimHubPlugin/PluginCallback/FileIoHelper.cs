using Newtonsoft.Json;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Documents;
using static System.Net.Mime.MediaTypeNames;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        //public ObservableCollection<ConfigListItem> ConfigList { get; set; }
        //public ObservableCollection<ProfileListItem> ProfileList { get; set; }
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
            configFolderPath = Path.Combine(baseFolderPath, configFolderName);
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

            ConfigService.RefreshConfigList();
            ProfileServicePlugin.RefreshProfileList();

        }
    }
        
}
