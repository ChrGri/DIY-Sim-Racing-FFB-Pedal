#include "DiyActivePedal_types.h"


#include "PedalGeometry.h"
#include "StepperWithLimits.h"

#include <EEPROM.h>

static const float s_absScaling_fl32 = 50.0f;

#define WAIT_TIME_IN_MS_TO_ACQUIRE_GLOBAL_STRUCT_U32 500U

static const uint32_t s_eepromOffset_u32 = ASSIGNMENT_EEPROM_OFFSET_U32 + sizeof(DapAssignmentReg_t) + ASSIGNMENT_EEPROM_OFFSET_U32/*+(DAP_VERSION_CONFIG_U8-128) * sizeof(DapConfig_t) % (2048-sizeof(DapConfig_t))*/;

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
  /*
  payloadPedalConfig_st.relativeForce_p000 = 0;
  payloadPedalConfig_st.relativeForce_p020 = 20;
  payloadPedalConfig_st.relativeForce_p040 = 40;
  payloadPedalConfig_st.relativeForce_p060 = 60;
  payloadPedalConfig_st.relativeForce_p080 = 80;
  payloadPedalConfig_st.relativeForce_p100 = 100;
  */
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

  payloadPedalConfig_st.simulateAbsTrigger_u8 = 0;// add for abs trigger
  payloadPedalConfig_st.simulateAbsValue_u8 = 80;// add for abs trigger
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
  payloadPedalConfig_st.configHash_u32 = (uint32_t)175245064 ;
  payloadPedalConfig_st.endstopDetectionThreshold_u8 = 30;

  payloadPedalConfig_st.virtualPedalDampingInPercent_u8 = 100;
  payloadPedalConfig_st.virtualPedalMassInPercent_u8 = 30;

  payloadPedalConfig_st.endstopStiffness_kg_mm_u8 = 50;
  payloadPedalConfig_st.endstopTravelRange_mm_u8 = 10;

}




void DapConfig_t::storeConfigToEeprom(DapConfig_t& config_st)
{

  EEPROM.put(s_eepromOffset_u32, config_st); 
  EEPROM.commit();
  ActiveSerial->println("Successfully stored config in EPROM");
  
  /*if (true == config_st.payloadHeader_st.storeToEeprom_u8)
  {
    config_st.payloadHeader_st.storeToEeprom_u8 = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
    EEPROM.put(0, config_st); 
    EEPROM.commit();
    ActiveSerial->println("Successfully stored config in EPROM");
  }*/
}

void DapConfig_t::loadConfigFromEeprom(DapConfig_t& config_st)
{
  DapConfig_t local_config_st;

  EEPROM.get(s_eepromOffset_u32, local_config_st);
  //EEPROM.commit();

  config_st = local_config_st;

  // check if version matches revision, in case, update the default config
  /*if (local_config_st.payloadHeader_st.version_u8 == DAP_VERSION_CONFIG_U8)
  {
    config_st = local_config_st;
    ActiveSerial->println("Successfully loaded config from EPROM");
  }
  else
  { 
    ActiveSerial->println("Couldn't load config from EPROM due to version mismatch");
    ActiveSerial->print("Target version: ");
    ActiveSerial->println(DAP_VERSION_CONFIG_U8);
    ActiveSerial->print("Source version: ");
    ActiveSerial->println(local_config_st.payloadHeader_st.version_u8);

  }*/

}





void DapCalculationVariables_t::updateFromConfig(DapConfig_t& config_st)
{
  startPosRel_fl32 = ((float)config_st.payloadPedalConfig_st.pedalStartPosition_u8) / 100.0f;
  endPosRel_fl32 = ((float)config_st.payloadPedalConfig_st.pedalEndPosition_u8) / 100.0f;
  
  //read force and trave linto calculaiton Variables
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
  // cubic interpolator
  float travel_x_afl32[config_st.payloadPedalConfig_st.quantityOfControl_u8];
  float force_y_afl32[config_st.payloadPedalConfig_st.quantityOfControl_u8];
  
  for (int index_i32 = 0; index_i32 < config_st.payloadPedalConfig_st.quantityOfControl_u8; index_i32++)
  {
    travel_x_afl32[index_i32] = travel_afl32[index_i32];
    force_y_afl32[index_i32] = force_afl32[index_i32];
  }
  
  cubic_st.interpolate1D(travel_x_afl32, force_y_afl32, config_st.payloadPedalConfig_st.quantityOfControl_u8, config_st.payloadPedalConfig_st.quantityOfControl_u8);
  interpolatorA_pfl32 = cubic_st.result_st.a_afl32;
  interpolatorB_pfl32 = cubic_st.result_st.b_afl32;
  /*
  for (int i = 0; i < config_st.payloadPedalConfig_st.quantityOfControl_u8 - 1; ++i)
  {
    //ActiveSerial->printf("original a=%.3f, b=%.3f\n", config_st.payloadPedalConfig_st.cubic_spline_param_a_array[i], config_st.payloadPedalConfig_st.cubic_spline_param_b_array[i]);
    ActiveSerial->printf("ESP calculated a=%.3f, b=%.3f\n", interpolatorA_pfl32[i], interpolatorB_pfl32[i]);
  }
  */
  
  //testing code
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
  
  float joystick_x_afl32[numOfJoystickControl_u8] = {0};
  float joystick_y_afl32[numOfJoystickControl_u8] = {0};
  for (int index_i32 = 0; index_i32 < numOfJoystickControl_u8; index_i32++)
  {
    joystick_x_afl32[index_i32] = joystickOrig_afl32[index_i32] - joystickOrig_afl32[0];
    joystick_y_afl32[index_i32] = joystickMapping_afl32[index_i32];
  }
  joystickInterpolator_st.interpolate1D(joystick_x_afl32, joystick_y_afl32, numOfJoystickControl_u8, 100);
  /*
  for (int i = 0; i < 5; ++i)
  {
    //ActiveSerial->printf("original a=%.3f, b=%.3f\n", config_st.payloadPedalConfig_st.cubic_spline_param_a_array[i], config_st.payloadPedalConfig_st.cubic_spline_param_b_array[i]);
    ActiveSerial->printf("joystick calculated a=%.3f, b=%.3f\n", joystickInterpolator._result.a[i], joystickInterpolator._result.b[i]);
  }
  
  for (int i = 0; i < 100; i++)
  {
    ActiveSerial->printf("joystick value:y= %.3f\n", joystickInterpolator._result.yInterp[i]);
  }
  */



  if (startPosRel_fl32 == endPosRel_fl32)
  {
    endPosRel_fl32 = startPosRel_fl32 + 0.01f;
  }

  absFrequency_fl32 = ((float)config_st.payloadPedalConfig_st.absFrequency_u8);
  absAmplitude_fl32 = ((float)config_st.payloadPedalConfig_st.absAmplitude_u8)  *0.001f;//in percent, max 20% of force range

  rpmMaxFreq_fl32 = ((float)config_st.payloadPedalConfig_st.rpmMaxFreq_u8);
  rpmMinFreq_fl32 = ((float)config_st.payloadPedalConfig_st.rpmMinFreq_u8);
  rpmAmp_fl32 = ((float)config_st.payloadPedalConfig_st.rpmAmp_u8)  * 0.0002f;//in kg, max 4% of force range
  // Bite point effect;

  bpTriggerValue_fl32 = (float)config_st.payloadPedalConfig_st.bpTriggerValue_u8;
  bpAmp_fl32 = ((float)config_st.payloadPedalConfig_st.bpAmp_u8)   *0.001f;//in kg, max 20% of force range
  bpFreq_fl32 = (float)config_st.payloadPedalConfig_st.bpFreq_u8;
  wsAmp_fl32 = ((float)config_st.payloadPedalConfig_st.wsAmp_u8)  *0.001f;//in kg, max 20% of force range
  wsFreq_fl32 = (float)config_st.payloadPedalConfig_st.wsFreq_u8;
  // update force variables
  forceMin_fl32 = ((float)config_st.payloadPedalConfig_st.preloadForce_fl32);
  forceMax_fl32 = ((float)config_st.payloadPedalConfig_st.maxForce_fl32);
  forceRange_fl32 = forceMax_fl32 - forceMin_fl32;
  forceMaxDefault_fl32 = ((float)config_st.payloadPedalConfig_st.maxForce_fl32);
  pedalType_u8 = config_st.payloadPedalConfig_st.pedalType_u8;

  // calculate steps per motor revolution
  float helper_fl32 = MAXIMUM_STEPPER_SPEED_U32 / (s_maximumStepperRpm_u32 / s_secondsPerMinute_u32);
  helper_fl32 = floor(helper_fl32 / 10.0f) * 10.0f;
  helper_fl32 = constrain(helper_fl32, 2000.0f, 10000.0f);
  stepsPerMotorRevolution_u32 = helper_fl32;

    // // when spindle pitch is smaller than 8, choose coarse microstepping
    // if ( 8 > config_st.payloadPedalConfig_st.spindlePitch_mmPerRev_u8)
    // {stepsPerMotorRevolution_u32 = 3200;}
    // else{stepsPerMotorRevolution_u32 = 6400;}

    // stepsPerMotorRevolution_u32 = 3750;
}

void IRAM_ATTR_FLAG DapCalculationVariables_t::dynamicUpdate()
{
  forceRange_fl32 = forceMax_fl32 - forceMin_fl32;
}

void IRAM_ATTR_FLAG DapCalculationVariables_t::resetMaxForce()
{
  forceMax_fl32 = forceMaxDefault_fl32;
}

void IRAM_ATTR_FLAG DapCalculationVariables_t::updateEndstops(int32_t newMinEndstop_i32, int32_t newMaxEndstop_i32)
{
 
  if (newMinEndstop_i32 == newMaxEndstop_i32)
  {
    newMaxEndstop_i32 = newMinEndstop_i32 + 10;
  }
  
  stepperPosMinEndstop_i32 = newMinEndstop_i32;
  stepperPosMaxEndstop_i32 = newMaxEndstop_i32;
  stepperPosEndstopRange_i32 = stepperPosMaxEndstop_i32 - stepperPosMinEndstop_i32;
  
  softEndstopMinStepperPos_i32 = stepperPosEndstopRange_i32 * startPosRel_fl32;
  softEndstopMaxStepperPos_i32 = stepperPosEndstopRange_i32 * endPosRel_fl32;
  stepperPosMinDefault_i32 = softEndstopMinStepperPos_i32;
  stepperPosRange_fl32 = softEndstopMaxStepperPos_i32 - softEndstopMinStepperPos_i32;
  //currentPedalPositionRatio_fl32=((float)(currentPedalPosition_u32-stepperPosMinDefault_i32))/((float)stepperPosRangeDefault_fl32);

}

void IRAM_ATTR_FLAG DapCalculationVariables_t::updateStiffness()
{
  springStiffnesss_fl32 = forceRange_fl32 / stepperPosRange_fl32;
  if ( fabsf(springStiffnesss_fl32) > 0.0001f )
  {
    springStiffnesssInv_fl32 = 1.0f / springStiffnesss_fl32;
  }
  else
  {
    springStiffnesssInv_fl32 = 1000000.0f;
  }
}

void DapCalculationVariables_t::stepperPosSetback()
{
  softEndstopMinStepperPos_i32 = stepperPosMinDefault_i32;
  softEndstopMaxStepperPos_i32 = stepperPosMaxDefault_i32;
  stepperPosRange_fl32 = stepperPosRangeDefault_fl32;
}

void DapCalculationVariables_t::updateStepperMinPos(int32_t newMinstop_i32)
{
  softEndstopMinStepperPos_i32 = newMinstop_i32;
  
  stepperPosRange_fl32 = softEndstopMaxStepperPos_i32 - softEndstopMinStepperPos_i32;
}

void DapCalculationVariables_t::updateStepperMaxPos(int32_t newMaxstop_i32)
{
  
  softEndstopMaxStepperPos_i32 = newMaxstop_i32;
  stepperPosRange_fl32 = softEndstopMaxStepperPos_i32 - softEndstopMinStepperPos_i32;
}

void DapCalculationVariables_t::setDefaultPos()
{
  stepperPosMinDefault_i32 = softEndstopMinStepperPos_i32;
  stepperPosMaxDefault_i32 = softEndstopMaxStepperPos_i32;
  stepperPosRangeDefault_fl32 = stepperPosRange_fl32;
}



/**********************************************************************************************/
/*                                                                                            */
/*                         DAP_config_class                                                   */
/*                                                                                            */
/**********************************************************************************************/
// constructor
DapConfigClass::DapConfigClass()
{

  // create the mutex
  mutex_sh = xSemaphoreCreateMutex();
  if (mutex_sh == NULL)
  {
    ActiveSerial->println("Error: Mutex could not be created!");
    ESP.restart();
  }

  // initialize the default config
  config_st.initializeDefaults();
}


// method to safely get the config variable
bool DapConfigClass::getConfig(DapConfig_t* dapConfigIn_pst, uint16_t timeoutInMs_u16)
{
  bool configUpdated_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
  if (xSemaphoreTake(mutex_sh, pdMS_TO_TICKS(timeoutInMs_u16)) == pdTRUE)
  {
    *dapConfigIn_pst = config_st;
    // gives back the mutex
    xSemaphoreGive(mutex_sh);
    configUpdated_b = true;
  }

  return configUpdated_b;
}

// method to safely set the config variable
void DapConfigClass::setConfig(DapConfig_t config_st)
{
  // boolean returnV_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex_sh, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_ACQUIRE_GLOBAL_STRUCT_U32)) == pdTRUE)
  {
    this->config_st = config_st;
    // returnV_b = true;
    // gives back the mutex
    xSemaphoreGive(mutex_sh);
  }
  else
  {
    ActiveSerial->println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}

uint16_t DapConfigClass::checksumCalculator_u16(uint8_t *data_pu8, uint16_t length_u16)
{
  uint16_t currentCrc_u16 = 0x0000U;
  uint8_t sum1_u8 = (uint8_t)currentCrc_u16;
  uint8_t sum2_u8 = (uint8_t)(currentCrc_u16 >> 8U);
  int index_i32;

  for (index_i32 = 0; index_i32 < length_u16; index_i32 = index_i32 + 1)
  {
    sum1_u8 = (sum1_u8 + data_pu8[index_i32]) % 255U;
    sum2_u8 = (sum2_u8 + sum1_u8) % 255U;
  }

  return (uint16_t)((sum2_u8 << 8U) | sum1_u8);
}

void DapConfigClass::loadConfigFromEeprom()
{
  if (xSemaphoreTake(mutex_sh, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_ACQUIRE_GLOBAL_STRUCT_U32)) == pdTRUE)
  {
    config_st.loadConfigFromEeprom(config_st);
    xSemaphoreGive(mutex_sh);
  }
}

void DapConfigClass::storeConfigToEeprom()
{
  if (xSemaphoreTake(mutex_sh, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_ACQUIRE_GLOBAL_STRUCT_U32)) == pdTRUE)
  {
    config_st.payloadHeader_st.storeToEeprom_u8 = 0;
    uint16_t crc_u16 = checksumCalculator_u16((uint8_t*)(&(config_st.payloadHeader_st)), sizeof(config_st.payloadHeader_st) + sizeof(config_st.payloadPedalConfig_st));
    config_st.payloadFooter_st.checkSum_u16 = crc_u16;
    config_st.storeConfigToEeprom(config_st);
    xSemaphoreGive(mutex_sh);
  }
}

void DapConfigClass::initializedConfig()
{
  // boolean returnV_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex_sh, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_ACQUIRE_GLOBAL_STRUCT_U32)) == pdTRUE)
  {
    config_st.initializeDefaults();
    // returnV_b = true;
    // gives back the mutex
    xSemaphoreGive(mutex_sh);
  }
  else
  {
    ActiveSerial->println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}
