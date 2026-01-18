using log4net.Plugin;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : SimHub.Plugins.IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public void ConfigSendingQueue()
        {
            for (int i = 0; i < 3; i++)
            {
                if (IsGetConfigSendRequest[i])
                {
                    TimeSpan diff = DateTime.Now - ConfigBufferGet_lastTime[i];
                    if (diff.TotalMilliseconds > ConfigSendInterval_ms)
                    {
                        IsGetConfigSendRequest[i] = false;
                        SendConfigWithoutSaveToEEPROM(BufferConfig_st[i], (byte)i);
                    }
                }
                
            }
            
        }
    }
}
