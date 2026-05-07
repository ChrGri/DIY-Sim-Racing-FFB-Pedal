#include "DiyActivePedal_types.h"

void DapConfig_t::initializeDefaults()
{
  payloadHeader_st.startOfFrame0_u8 = SOF_BYTE_0_U8;
  payloadHeader_st.startOfFrame1_u8 = SOF_BYTE_1_U8;
  payloadHeader_st.payloadType_u8 = DAP_PAYLOAD_TYPE_CONFIG_U8;
  payloadHeader_st.version_u8 = DAP_VERSION_CONFIG_U8;
  payloadHeader_st.storeToEeprom_u8 = false;

  payloadFooter_st.enfOfFrame0_u8 = EOF_BYTE_0_U8;
  payloadFooter_st.enfOfFrame1_u8 = EOF_BYTE_1_U8;

  payloadPedalConfig_st.pedalStartPosition_u8 = 10;
  payloadPedalConfig_st.pedalEndPosition_u8 = 85;

  payloadPedalConfig_st.maxForce_fl32 = 60;
  payloadPedalConfig_st.preloadForce_fl32 = 2;
  payloadPedalConfig_st.quantityOfControl_u8 = 6;
  payloadPedalConfig_st.relativeForce00_u8 = 0;
  payloadPedalConfig_st.relativeForce01_u8 = 20;
  payloadPedalConfig_st.relativeForce02_u8 = 40;
  payloadPedalConfig_st.relativeForce03_u8 = 60;
  payloadPedalConfig_st.relativeForce04_u8 = 80;
  payloadPedalConfig_st.relativeForce05_u8 = 100;
  payloadPedalConfig_st.relativeForce06_u8 = 0;
  payloadPedalConfig_st.relativeForce07_u8 = 0;
  payloadPedalConfig_st.relativeForce08_u8 = 0;
  payloadPedalConfig_st.relativeForce09_u8 = 0;
  payloadPedalConfig_st.relativeForce10_u8 = 0;
  payloadPedalConfig_st.relativeTravel00_u8 = 0;
  payloadPedalConfig_st.relativeTravel01_u8 = 20;
  payloadPedalConfig_st.relativeTravel02_u8 = 40;
  payloadPedalConfig_st.relativeTravel03_u8 = 60;
  payloadPedalConfig_st.relativeTravel04_u8 = 80;
  payloadPedalConfig_st.relativeTravel05_u8 = 100;
  payloadPedalConfig_st.relativeTravel06_u8 = 0;
  payloadPedalConfig_st.relativeTravel07_u8 = 0;
  payloadPedalConfig_st.relativeTravel08_u8 = 0;
  payloadPedalConfig_st.relativeTravel09_u8 = 0;
  payloadPedalConfig_st.relativeTravel10_u8 = 0;

  payloadPedalConfig_st.numOfJoystickMapControl_u8 = 6;
  payloadPedalConfig_st.joystickMapOrig00_u8 = 0;
  payloadPedalConfig_st.joystickMapOrig01_u8 = 20;
  payloadPedalConfig_st.joystickMapOrig02_u8 = 40;
  payloadPedalConfig_st.joystickMapOrig03_u8 = 60;
  payloadPedalConfig_st.joystickMapOrig04_u8 = 80;
  payloadPedalConfig_st.joystickMapOrig05_u8 = 100;
  payloadPedalConfig_st.joystickMapOrig06_u8 = 0;
  payloadPedalConfig_st.joystickMapOrig07_u8 = 0;
  payloadPedalConfig_st.joystickMapOrig08_u8 = 0;
  payloadPedalConfig_st.joystickMapOrig09_u8 = 0;
  payloadPedalConfig_st.joystickMapOrig10_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped00_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped01_u8 = 20;
  payloadPedalConfig_st.joystickMapMapped02_u8 = 40;
  payloadPedalConfig_st.joystickMapMapped03_u8 = 60;
  payloadPedalConfig_st.joystickMapMapped04_u8 = 80;
  payloadPedalConfig_st.joystickMapMapped05_u8 = 100;
  payloadPedalConfig_st.joystickMapMapped06_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped07_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped08_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped09_u8 = 0;
  payloadPedalConfig_st.joystickMapMapped10_u8 = 0;

  payloadPedalConfig_st.absFrequency_u8 = 15;
  payloadPedalConfig_st.absAmplitude_u8 = 0;
  payloadPedalConfig_st.absPattern_u8 = 0;
  payloadPedalConfig_st.absForceOrTarvelBit_u8 = 0;

  payloadPedalConfig_st.lengthPedalA_i16 = 205;
  payloadPedalConfig_st.lengthPedalB_i16 = 220;
  payloadPedalConfig_st.lengthPedalD_i16 = 60;
  payloadPedalConfig_st.lengthPedalCHorizontal_i16 = 215;
  payloadPedalConfig_st.lengthPedalCVertical_i16 = 60;
  payloadPedalConfig_st.lengthPedalTravel_i16 = 100;
  payloadPedalConfig_st.spindlePitch_mmPerRev_u8 = 5;

  payloadPedalConfig_st.simulateAbsTrigger_u8 = 0;
  payloadPedalConfig_st.simulateAbsValue_u8 = 80;
  payloadPedalConfig_st.rpmMaxFreq_u8 = 40;
  payloadPedalConfig_st.rpmMinFreq_u8 = 10;
  payloadPedalConfig_st.rpmAmp_u8 = 5;
  payloadPedalConfig_st.bpTriggerValue_u8 = 50;
  payloadPedalConfig_st.bpAmp_u8 = 1;
  payloadPedalConfig_st.bpFreq_u8 = 15;
  payloadPedalConfig_st.bpTrigger_u8 = 0;
  payloadPedalConfig_st.gMulti_u8 = 50;
  payloadPedalConfig_st.gWindow_u8 = 60;
  payloadPedalConfig_st.wsAmp_u8 = 1;
  payloadPedalConfig_st.wsFreq_u8 = 15;
  payloadPedalConfig_st.roadMulti_u8 = 50;
  payloadPedalConfig_st.roadWindow_u8 = 60;
  payloadPedalConfig_st.maxGameOutput_u8 = 100;
  payloadPedalConfig_st.kfModelNoise_u8 = 128;
  payloadPedalConfig_st.kfModelOrder_u8 = 1;
  payloadPedalConfig_st.debugFlags0_u8 = 0;
  payloadPedalConfig_st.loadcellRating_u8 = 150;
  payloadPedalConfig_st.travelAsJoystickOutput_u8 = 0;
  payloadPedalConfig_st.invertLoadcellReading_u8 = 0;
  payloadPedalConfig_st.invertMotorDirection_u8 = 0;
  payloadPedalConfig_st.pedalType_u8 = 4;
  payloadPedalConfig_st.stepLossFunctionFlags_u8 = 0b11;
  payloadPedalConfig_st.kfModelNoiseJoystick_u8 = 1;
  payloadPedalConfig_st.kfJoystick_u8 = 0;
  payloadPedalConfig_st.servoIdleTimeout_u8 = 0;
  payloadPedalConfig_st.minForceForEffects_u8 = 0;
  payloadPedalConfig_st.configHash_u32 = 175245064U;
  payloadPedalConfig_st.endstopDetectionThreshold_u8 = 30;
  payloadPedalConfig_st.virtualPedalDampingInPercent_u8 = 100;
  payloadPedalConfig_st.virtualPedalMassInPercent_u8 = 30;
  payloadPedalConfig_st.endstopStiffness_kg_mm_u8 = 50;
  payloadPedalConfig_st.endstopTravelRange_mm_u8 = 10;
}

void DapCalculationVariables_t::updateFromConfig(DapConfig_t& config_st)
{
  startPosRel_fl32 = ((float)config_st.payloadPedalConfig_st.pedalStartPosition_u8) / 100.0f;
  endPosRel_fl32 = ((float)config_st.payloadPedalConfig_st.pedalEndPosition_u8) / 100.0f;

  force_afl32[0] = config_st.payloadPedalConfig_st.relativeForce00_u8;
  force_afl32[1] = config_st.payloadPedalConfig_st.relativeForce01_u8;
  force_afl32[2] = config_st.payloadPedalConfig_st.relativeForce02_u8;
  force_afl32[3] = config_st.payloadPedalConfig_st.relativeForce03_u8;
  force_afl32[4] = config_st.payloadPedalConfig_st.relativeForce04_u8;
  force_afl32[5] = config_st.payloadPedalConfig_st.relativeForce05_u8;
  force_afl32[6] = config_st.payloadPedalConfig_st.relativeForce06_u8;
  force_afl32[7] = config_st.payloadPedalConfig_st.relativeForce07_u8;
  force_afl32[8] = config_st.payloadPedalConfig_st.relativeForce08_u8;
  force_afl32[9] = config_st.payloadPedalConfig_st.relativeForce09_u8;
  force_afl32[10] = config_st.payloadPedalConfig_st.relativeForce10_u8;

  travel_afl32[0] = config_st.payloadPedalConfig_st.relativeTravel00_u8;
  travel_afl32[1] = config_st.payloadPedalConfig_st.relativeTravel01_u8;
  travel_afl32[2] = config_st.payloadPedalConfig_st.relativeTravel02_u8;
  travel_afl32[3] = config_st.payloadPedalConfig_st.relativeTravel03_u8;
  travel_afl32[4] = config_st.payloadPedalConfig_st.relativeTravel04_u8;
  travel_afl32[5] = config_st.payloadPedalConfig_st.relativeTravel05_u8;
  travel_afl32[6] = config_st.payloadPedalConfig_st.relativeTravel06_u8;
  travel_afl32[7] = config_st.payloadPedalConfig_st.relativeTravel07_u8;
  travel_afl32[8] = config_st.payloadPedalConfig_st.relativeTravel08_u8;
  travel_afl32[9] = config_st.payloadPedalConfig_st.relativeTravel09_u8;
  travel_afl32[10] = config_st.payloadPedalConfig_st.relativeTravel10_u8;

  float travel_x_afl32[11] = {0};
  float force_y_afl32[11] = {0};
  for (uint8_t index_u8 = 0; index_u8 < config_st.payloadPedalConfig_st.quantityOfControl_u8; index_u8++)
  {
    travel_x_afl32[index_u8] = travel_afl32[index_u8];
    force_y_afl32[index_u8] = force_afl32[index_u8];
  }

  cubic_st.interpolate1D(
      travel_x_afl32,
      force_y_afl32,
      config_st.payloadPedalConfig_st.quantityOfControl_u8,
      config_st.payloadPedalConfig_st.quantityOfControl_u8);
  interpolatorA_pfl32 = cubic_st.result_st.a_afl32;
  interpolatorB_pfl32 = cubic_st.result_st.b_afl32;

  numOfJoystickControl_u8 = config_st.payloadPedalConfig_st.numOfJoystickMapControl_u8;
  joystickOrig_afl32[0] = config_st.payloadPedalConfig_st.joystickMapOrig00_u8;
  joystickOrig_afl32[1] = config_st.payloadPedalConfig_st.joystickMapOrig01_u8;
  joystickOrig_afl32[2] = config_st.payloadPedalConfig_st.joystickMapOrig02_u8;
  joystickOrig_afl32[3] = config_st.payloadPedalConfig_st.joystickMapOrig03_u8;
  joystickOrig_afl32[4] = config_st.payloadPedalConfig_st.joystickMapOrig04_u8;
  joystickOrig_afl32[5] = config_st.payloadPedalConfig_st.joystickMapOrig05_u8;
  joystickOrig_afl32[6] = config_st.payloadPedalConfig_st.joystickMapOrig06_u8;
  joystickOrig_afl32[7] = config_st.payloadPedalConfig_st.joystickMapOrig07_u8;
  joystickOrig_afl32[8] = config_st.payloadPedalConfig_st.joystickMapOrig08_u8;
  joystickOrig_afl32[9] = config_st.payloadPedalConfig_st.joystickMapOrig09_u8;
  joystickOrig_afl32[10] = config_st.payloadPedalConfig_st.joystickMapOrig10_u8;
  joystickMapping_afl32[0] = config_st.payloadPedalConfig_st.joystickMapMapped00_u8;
  joystickMapping_afl32[1] = config_st.payloadPedalConfig_st.joystickMapMapped01_u8;
  joystickMapping_afl32[2] = config_st.payloadPedalConfig_st.joystickMapMapped02_u8;
  joystickMapping_afl32[3] = config_st.payloadPedalConfig_st.joystickMapMapped03_u8;
  joystickMapping_afl32[4] = config_st.payloadPedalConfig_st.joystickMapMapped04_u8;
  joystickMapping_afl32[5] = config_st.payloadPedalConfig_st.joystickMapMapped05_u8;
  joystickMapping_afl32[6] = config_st.payloadPedalConfig_st.joystickMapMapped06_u8;
  joystickMapping_afl32[7] = config_st.payloadPedalConfig_st.joystickMapMapped07_u8;
  joystickMapping_afl32[8] = config_st.payloadPedalConfig_st.joystickMapMapped08_u8;
  joystickMapping_afl32[9] = config_st.payloadPedalConfig_st.joystickMapMapped09_u8;
  joystickMapping_afl32[10] = config_st.payloadPedalConfig_st.joystickMapMapped10_u8;

  float joystick_x_afl32[11] = {0};
  float joystick_y_afl32[11] = {0};
  for (uint8_t index_u8 = 0; index_u8 < numOfJoystickControl_u8; index_u8++)
  {
    joystick_x_afl32[index_u8] = joystickOrig_afl32[index_u8] - joystickOrig_afl32[0];
    joystick_y_afl32[index_u8] = joystickMapping_afl32[index_u8];
  }
  joystickInterpolator_st.interpolate1D(joystick_x_afl32, joystick_y_afl32, numOfJoystickControl_u8, 100);

  if (startPosRel_fl32 == endPosRel_fl32)
  {
    endPosRel_fl32 = startPosRel_fl32 + 0.01f;
  }

  absFrequency_fl32 = (float)config_st.payloadPedalConfig_st.absFrequency_u8;
  absAmplitude_fl32 = (float)config_st.payloadPedalConfig_st.absAmplitude_u8 * 0.001f;
  rpmMaxFreq_fl32 = (float)config_st.payloadPedalConfig_st.rpmMaxFreq_u8;
  rpmMinFreq_fl32 = (float)config_st.payloadPedalConfig_st.rpmMinFreq_u8;
  rpmAmp_fl32 = (float)config_st.payloadPedalConfig_st.rpmAmp_u8 * 0.0002f;
  bpTriggerValue_fl32 = (float)config_st.payloadPedalConfig_st.bpTriggerValue_u8;
  bpAmp_fl32 = (float)config_st.payloadPedalConfig_st.bpAmp_u8 * 0.001f;
  bpFreq_fl32 = (float)config_st.payloadPedalConfig_st.bpFreq_u8;
  wsAmp_fl32 = (float)config_st.payloadPedalConfig_st.wsAmp_u8 * 0.001f;
  wsFreq_fl32 = (float)config_st.payloadPedalConfig_st.wsFreq_u8;
  forceMin_fl32 = (float)config_st.payloadPedalConfig_st.preloadForce_fl32;
  forceMax_fl32 = (float)config_st.payloadPedalConfig_st.maxForce_fl32;
  forceRange_fl32 = forceMax_fl32 - forceMin_fl32;
  forceMaxDefault_fl32 = (float)config_st.payloadPedalConfig_st.maxForce_fl32;
  pedalType_u8 = config_st.payloadPedalConfig_st.pedalType_u8;

  stepsPerMotorRevolution_u32 = 3200U;
}