using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    // https://stackoverflow.com/questions/14344305/best-way-to-structure-class-struct-in-c-sharp
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    //[Serializable]
    public struct payloadPedalAction
    {
        public byte triggerAbs_u8;
        public byte system_action_u8; //1=reset position, 2=restart ESP
        public byte startSystemIdentification_u8;
        public byte returnPedalConfig_u8;
        public byte RPM_u8;
        public byte G_value;
        public byte WS_u8;
        public byte impact_value;
        public byte Trigger_CV_1;
        public byte Trigger_CV_2;
        public byte Rudder_action;
        public byte Rudder_brake_action;
    };
}
