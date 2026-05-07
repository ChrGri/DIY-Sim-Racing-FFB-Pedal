using log4net.Plugin;
using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using IPlugin = SimHub.Plugins.IPlugin;

namespace DiyFfbPedal
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public int BoolToInt(bool val)
        {
            return val ? 1 : 0;
        }

        unsafe private void PedalRestartAction()
        {
            DAP_action_st tmp2;
            tmp2.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            tmp2.payloadHeader_.payloadType = (byte)Constants.pedalActionPayload_type;
            tmp2.payloadPedalAction_.triggerAbs_u8 = 0;
            tmp2.payloadPedalAction_.RPM_u8 = 0;
            tmp2.payloadPedalAction_.G_value = 128;
            tmp2.payloadPedalAction_.WS_u8 = 0;
            tmp2.payloadPedalAction_.impact_value = 0;
            tmp2.payloadPedalAction_.Trigger_CV_1 = 0;
            tmp2.payloadPedalAction_.Trigger_CV_2 = 0;
            tmp2.payloadPedalAction_.Rudder_action = 0;
            tmp2.payloadPedalAction_.system_action_u8 = (byte)PedalSystemAction.PEDAL_RESTART;
            for (uint i = 0; i < 3; i++)
            {
                tmp2.payloadHeader_.PedalTag = (byte)i;
                DAP_action_st* vv = &tmp2;
                tmp2.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
                tmp2.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
                tmp2.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
                tmp2.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
                byte* p2 = (byte*)vv;
                tmp2.payloadFooter_.checkSum = checksumCalc(p2, sizeof(payloadHeader) + sizeof(payloadPedalAction));
                SendPedalAction(tmp2, (byte)i);
                Task.Delay(50).Wait();
            }
        }
    }
}
