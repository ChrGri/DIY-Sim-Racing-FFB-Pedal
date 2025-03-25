using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    static class Constants
    {
        // payload revisiom
        public const uint pedalConfigPayload_version = 145;


        // pyload types
        public const uint pedalConfigPayload_type = 100;
        public const uint pedalActionPayload_type = 110;
        public const uint pedalStateBasicPayload_type = 120;
        public const uint pedalStateExtendedPayload_type = 130;
        public const uint bridgeStatePayloadType = 210;
        public const uint Basic_Wifi_info_type = 220;
        public const string pluginVersion = "0.89.99";
    }
}
