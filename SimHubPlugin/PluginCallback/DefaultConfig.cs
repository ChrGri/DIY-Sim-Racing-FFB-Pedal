using SimHub.Plugins;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    public partial class DIY_FFB_Pedal : IPlugin, IDataPlugin, IWPFSettingsV2
    {
        public DAP_config_st DefaultConfig;
        public DAP_system_profile_cls DefaultProfile;
        unsafe public void DefaultConfigInitializing()
        {
            DefaultConfig.payloadHeader_.storeToEeprom = 0;
            DefaultConfig.payloadHeader_.PedalTag = 99;
            DefaultConfig.payloadFooter_.enfOfFrame0_u8 = ENDOFFRAMCHAR[0];
            DefaultConfig.payloadFooter_.enfOfFrame1_u8 = ENDOFFRAMCHAR[1];
            DefaultConfig.payloadHeader_.startOfFrame0_u8 = STARTOFFRAMCHAR[0];
            DefaultConfig.payloadHeader_.startOfFrame1_u8 = STARTOFFRAMCHAR[1];
            DefaultConfig.payloadHeader_.payloadType = (byte)Constants.pedalConfigPayload_type;
            DefaultConfig.payloadHeader_.version = (byte)Constants.pedalConfigPayload_version;
            DefaultConfig.payloadPedalConfig_.pedalStartPosition = 35;
            DefaultConfig.payloadPedalConfig_.pedalEndPosition = 80;
            DefaultConfig.payloadPedalConfig_.maxForce = 50;
            DefaultConfig.payloadPedalConfig_.preloadForce = 0;
            DefaultConfig.payloadPedalConfig_.quantityOfControl = 6;
            DefaultConfig.payloadPedalConfig_.relativeForce00 = 0;
            DefaultConfig.payloadPedalConfig_.relativeForce01 = 20;
            DefaultConfig.payloadPedalConfig_.relativeForce02 = 40;
            DefaultConfig.payloadPedalConfig_.relativeForce03 = 60;
            DefaultConfig.payloadPedalConfig_.relativeForce04 = 80;
            DefaultConfig.payloadPedalConfig_.relativeForce05 = 100;
            DefaultConfig.payloadPedalConfig_.relativeForce06 = 0;
            DefaultConfig.payloadPedalConfig_.relativeForce07 = 0;
            DefaultConfig.payloadPedalConfig_.relativeForce08 = 0;
            DefaultConfig.payloadPedalConfig_.relativeForce09 = 0;
            DefaultConfig.payloadPedalConfig_.relativeForce10 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel00 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel01 = 20;
            DefaultConfig.payloadPedalConfig_.relativeTravel02 = 40;
            DefaultConfig.payloadPedalConfig_.relativeTravel03 = 60;
            DefaultConfig.payloadPedalConfig_.relativeTravel04 = 80;
            DefaultConfig.payloadPedalConfig_.relativeTravel05 = 100;
            DefaultConfig.payloadPedalConfig_.relativeTravel06 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel07 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel08 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel09 = 0;
            DefaultConfig.payloadPedalConfig_.relativeTravel10 = 0;
            DefaultConfig.payloadPedalConfig_.numOfJoystickMapControl = 6;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped00 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped01 = 20;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped02 = 40;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped03 = 60;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped04 = 80;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped05 = 100;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped06 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped07 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped08 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped09 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapMapped10 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig00 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig01 = 20;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig02 = 40;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig03 = 60;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig04 = 80;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig05 = 100;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig06 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig07 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig08 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig09 = 0;
            DefaultConfig.payloadPedalConfig_.joystickMapOrig10 = 0;
            DefaultConfig.payloadPedalConfig_.dampingPress = 0;
            DefaultConfig.payloadPedalConfig_.dampingPull = 0;
            DefaultConfig.payloadPedalConfig_.absFrequency = 5;
            DefaultConfig.payloadPedalConfig_.absAmplitude = 20;
            DefaultConfig.payloadPedalConfig_.absPattern = 0;
            DefaultConfig.payloadPedalConfig_.absForceOrTarvelBit = 0;

            DefaultConfig.payloadPedalConfig_.lengthPedal_a = 205;
            DefaultConfig.payloadPedalConfig_.lengthPedal_b = 220;
            DefaultConfig.payloadPedalConfig_.lengthPedal_d = 60;
            DefaultConfig.payloadPedalConfig_.lengthPedal_c_horizontal = 215;
            DefaultConfig.payloadPedalConfig_.lengthPedal_c_vertical = 60;
            DefaultConfig.payloadPedalConfig_.lengthPedal_travel = 100;

            DefaultConfig.payloadPedalConfig_.Simulate_ABS_trigger = 0;
            DefaultConfig.payloadPedalConfig_.Simulate_ABS_value = 80;
            DefaultConfig.payloadPedalConfig_.RPM_max_freq = 40;
            DefaultConfig.payloadPedalConfig_.RPM_min_freq = 10;
            DefaultConfig.payloadPedalConfig_.RPM_AMP = 30;
            DefaultConfig.payloadPedalConfig_.BP_trigger_value = 50;
            DefaultConfig.payloadPedalConfig_.BP_amp = 1;
            DefaultConfig.payloadPedalConfig_.BP_freq = 15;
            DefaultConfig.payloadPedalConfig_.BP_trigger = 0;
            DefaultConfig.payloadPedalConfig_.G_multi = 50;
            DefaultConfig.payloadPedalConfig_.G_window = 10;
            DefaultConfig.payloadPedalConfig_.WS_amp = 1;
            DefaultConfig.payloadPedalConfig_.WS_freq = 15;
            DefaultConfig.payloadPedalConfig_.Impact_multi = 50;
            DefaultConfig.payloadPedalConfig_.Impact_window = 60;
            DefaultConfig.payloadPedalConfig_.CV_amp_1 = 0;
            DefaultConfig.payloadPedalConfig_.CV_freq_1 = 10;
            DefaultConfig.payloadPedalConfig_.CV_amp_2 = 0;
            DefaultConfig.payloadPedalConfig_.CV_freq_2 = 10;
            DefaultConfig.payloadPedalConfig_.maxGameOutput = 100;
            DefaultConfig.payloadPedalConfig_.kf_modelNoise = 90;
            DefaultConfig.payloadPedalConfig_.kf_modelOrder = 0;


            DefaultConfig.payloadPedalConfig_.loadcell_rating = 150;

            DefaultConfig.payloadPedalConfig_.travelAsJoystickOutput_u8 = 0;

            DefaultConfig.payloadPedalConfig_.invertLoadcellReading_u8 = 0;
            DefaultConfig.payloadPedalConfig_.invertMotorDirection_u8 = 0;

            DefaultConfig.payloadPedalConfig_.spindlePitch_mmPerRev_u8 = 5;
            DefaultConfig.payloadPedalConfig_.pedal_type = 99;
            DefaultConfig.payloadPedalConfig_.stepLossFunctionFlags_u8 = 0b11;
            DefaultConfig.payloadPedalConfig_.kf_modelNoise_joystick = 1;
            DefaultConfig.payloadPedalConfig_.kf_Joystick_u8 = 0;
            DefaultConfig.payloadPedalConfig_.positionSmoothingFactor_u8 = 0;
            DefaultConfig.payloadPedalConfig_.minForceForEffects = 0;
            DefaultConfig.payloadPedalConfig_.servoRatioOfInertia_u8 = 1;
            DefaultConfig.payloadPedalConfig_.configHash_u32 = 0;
        }

    }
}
