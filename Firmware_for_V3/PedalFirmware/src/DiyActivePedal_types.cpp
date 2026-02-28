#include "DiyActivePedal_types.h"


#include "PedalGeometry.h"
#include "StepperWithLimits.h"

#include <EEPROM.h>

static const float ABS_SCALING = 50;

#define WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT 500

const uint32_t EEPROM_OFFSET = (DAP_VERSION_CONFIG-128) * sizeof(DAP_config_st) % (2048-sizeof(DAP_config_st));

void DAP_config_st::initializeDefaults() {
  payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_CONFIG;
  payLoadHeader_.version = DAP_VERSION_CONFIG;
  payLoadHeader_.storeToEeprom = false;

  payLoadPedalConfig_.pedalStartPosition = 10;
  payLoadPedalConfig_.pedalEndPosition = 85;

  payLoadPedalConfig_.maxForce = 60;
  payLoadPedalConfig_.preloadForce = 2;
  /*
  payLoadPedalConfig_.relativeForce_p000 = 0;
  payLoadPedalConfig_.relativeForce_p020 = 20;
  payLoadPedalConfig_.relativeForce_p040 = 40;
  payLoadPedalConfig_.relativeForce_p060 = 60;
  payLoadPedalConfig_.relativeForce_p080 = 80;
  payLoadPedalConfig_.relativeForce_p100 = 100;
  */
  payLoadPedalConfig_.quantityOfControl=6;
  payLoadPedalConfig_.relativeForce00 = 0;
  payLoadPedalConfig_.relativeForce01 = 20;
  payLoadPedalConfig_.relativeForce02 = 40;
  payLoadPedalConfig_.relativeForce03 = 60;
  payLoadPedalConfig_.relativeForce04 = 80;
  payLoadPedalConfig_.relativeForce05 = 100;
  payLoadPedalConfig_.relativeForce06 = 0;
  payLoadPedalConfig_.relativeForce07 = 0;
  payLoadPedalConfig_.relativeForce08 = 0;
  payLoadPedalConfig_.relativeForce09 = 0;
  payLoadPedalConfig_.relativeForce10 = 0;
  payLoadPedalConfig_.relativeTravel00 = 0;
  payLoadPedalConfig_.relativeTravel01 = 20;
  payLoadPedalConfig_.relativeTravel02 = 40;
  payLoadPedalConfig_.relativeTravel03 = 60;
  payLoadPedalConfig_.relativeTravel04 = 80;
  payLoadPedalConfig_.relativeTravel05 = 100;
  payLoadPedalConfig_.relativeTravel06 = 0;
  payLoadPedalConfig_.relativeTravel07 = 0;
  payLoadPedalConfig_.relativeTravel08 = 0;
  payLoadPedalConfig_.relativeTravel09 = 0;
  payLoadPedalConfig_.relativeTravel10 = 0;

  payLoadPedalConfig_.numOfJoystickMapControl=6;
  payLoadPedalConfig_.joystickMapOrig00=0;
  payLoadPedalConfig_.joystickMapOrig01=20;
  payLoadPedalConfig_.joystickMapOrig02=40;
  payLoadPedalConfig_.joystickMapOrig03=60;
  payLoadPedalConfig_.joystickMapOrig04=80;
  payLoadPedalConfig_.joystickMapOrig05=100;
  payLoadPedalConfig_.joystickMapOrig06=0;
  payLoadPedalConfig_.joystickMapOrig07=0;
  payLoadPedalConfig_.joystickMapOrig08=0;
  payLoadPedalConfig_.joystickMapOrig09=0;
  payLoadPedalConfig_.joystickMapOrig10=0;
  payLoadPedalConfig_.joystickMapMapped00=0;
  payLoadPedalConfig_.joystickMapMapped01=20;
  payLoadPedalConfig_.joystickMapMapped02=40;
  payLoadPedalConfig_.joystickMapMapped03=60;
  payLoadPedalConfig_.joystickMapMapped04=80;
  payLoadPedalConfig_.joystickMapMapped05=100;
  payLoadPedalConfig_.joystickMapMapped06=0;
  payLoadPedalConfig_.joystickMapMapped07=0;
  payLoadPedalConfig_.joystickMapMapped08=0;
  payLoadPedalConfig_.joystickMapMapped09=0;
  payLoadPedalConfig_.joystickMapMapped10=0;

  payLoadPedalConfig_.dampingPress = 0;
  payLoadPedalConfig_.dampingPull = 0;

  payLoadPedalConfig_.absFrequency = 15;
  payLoadPedalConfig_.absAmplitude = 0;
  payLoadPedalConfig_.absPattern = 0;
  payLoadPedalConfig_.absForceOrTarvelBit = 0;

  payLoadPedalConfig_.lengthPedal_a = 205;
  payLoadPedalConfig_.lengthPedal_b = 220; 
  payLoadPedalConfig_.lengthPedal_d = 60; 
  payLoadPedalConfig_.lengthPedal_c_horizontal = 215;
  payLoadPedalConfig_.lengthPedal_c_vertical = 60;
  payLoadPedalConfig_.lengthPedal_travel = 100;
  payLoadPedalConfig_.spindlePitch_mmPerRev_u8=5;

  payLoadPedalConfig_.Simulate_ABS_trigger = 0;// add for abs trigger
  payLoadPedalConfig_.Simulate_ABS_value = 80;// add for abs trigger
  payLoadPedalConfig_.RPM_max_freq = 40;
  payLoadPedalConfig_.RPM_min_freq = 10;
  payLoadPedalConfig_.RPM_AMP = 5;
  payLoadPedalConfig_.BP_trigger_value =50;
  payLoadPedalConfig_.BP_amp=1;
  payLoadPedalConfig_.BP_freq=15;
  payLoadPedalConfig_.BP_trigger=0;
  payLoadPedalConfig_.G_multi = 50;
  payLoadPedalConfig_.G_window=60;
  payLoadPedalConfig_.WS_amp=1;
  payLoadPedalConfig_.WS_freq=15;
  payLoadPedalConfig_.Road_multi = 50;
  payLoadPedalConfig_.Road_window=60;
  /*
  payLoadPedalConfig_.cubic_spline_param_a_array[0] = 0;
  payLoadPedalConfig_.cubic_spline_param_a_array[1] = 0;
  payLoadPedalConfig_.cubic_spline_param_a_array[2] = 0;
  payLoadPedalConfig_.cubic_spline_param_a_array[3] = 0;
  payLoadPedalConfig_.cubic_spline_param_a_array[4] = 0;

  payLoadPedalConfig_.cubic_spline_param_b_array[0] = 0;
  payLoadPedalConfig_.cubic_spline_param_b_array[1] = 0;
  payLoadPedalConfig_.cubic_spline_param_b_array[2] = 0;
  payLoadPedalConfig_.cubic_spline_param_b_array[3] = 0;
  payLoadPedalConfig_.cubic_spline_param_b_array[4] = 0;
  */

  payLoadPedalConfig_.PID_p_gain = 0.3f;
  payLoadPedalConfig_.PID_i_gain = 50.0f;
  payLoadPedalConfig_.PID_d_gain = 0.0f;
  payLoadPedalConfig_.PID_velocity_feedforward_gain = 0.0f;


  payLoadPedalConfig_.MPC_0th_order_gain = 10.0f;
  payLoadPedalConfig_.MPC_1st_order_gain = 0.0f;
  payLoadPedalConfig_.MPC_2nd_order_gain = 0.0f;

  payLoadPedalConfig_.control_strategy_b = 0;

  payLoadPedalConfig_.maxGameOutput = 100;

  payLoadPedalConfig_.kf_modelNoise = 128;
  payLoadPedalConfig_.kf_modelOrder = 1;

  payLoadPedalConfig_.debug_flags_0 = 0;

  payLoadPedalConfig_.loadcell_rating = 150;

  payLoadPedalConfig_.travelAsJoystickOutput_u8 = 0;

  payLoadPedalConfig_.invertLoadcellReading_u8 = 0;

  payLoadPedalConfig_.invertMotorDirection_u8 = 0;
  payLoadPedalConfig_.pedal_type=4;
  payLoadPedalConfig_.stepLossFunctionFlags_u8=0b11;
  payLoadPedalConfig_.kf_modelNoise_joystick=1;
  payLoadPedalConfig_.kf_Joystick_u8=0;
  payLoadPedalConfig_.servoIdleTimeout=0;
}




void DAP_config_st::storeConfigToEeprom(DAP_config_st& config_st)
{

  EEPROM.put(EEPROM_OFFSET, config_st); 
  EEPROM.commit();
  Serial.println("Successfully stored config in EPROM");
  
  /*if (true == config_st.payLoadHeader_.storeToEeprom)
  {
    config_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
    EEPROM.put(0, config_st); 
    EEPROM.commit();
    Serial.println("Successfully stored config in EPROM");
  }*/
}

void DAP_config_st::loadConfigFromEeprom(DAP_config_st& config_st)
{
  DAP_config_st local_config_st;

  EEPROM.get(EEPROM_OFFSET, local_config_st);
  //EEPROM.commit();

  config_st = local_config_st;

  // check if version matches revision, in case, update the default config
  /*if (local_config_st.payLoadHeader_.version == DAP_VERSION_CONFIG)
  {
    config_st = local_config_st;
    Serial.println("Successfully loaded config from EPROM");
  }
  else
  { 
    Serial.println("Couldn't load config from EPROM due to version mismatch");
    Serial.print("Target version: ");
    Serial.println(DAP_VERSION_CONFIG);
    Serial.print("Source version: ");
    Serial.println(local_config_st.payLoadHeader_.version);

  }*/

}





void DAP_calculationVariables_st::updateFromConfig(DAP_config_st& config_st) 
{
  startPosRel = ((float)config_st.payLoadPedalConfig_.pedalStartPosition) / 100.0f;
  endPosRel = ((float)config_st.payLoadPedalConfig_.pedalEndPosition) / 100.0f;
  
  //read force and trave linto calculaiton Variables
  force[0] = config_st.payLoadPedalConfig_.relativeForce00;
  force[1] = config_st.payLoadPedalConfig_.relativeForce01;
  force[2] = config_st.payLoadPedalConfig_.relativeForce02;
  force[3] = config_st.payLoadPedalConfig_.relativeForce03;
  force[4] = config_st.payLoadPedalConfig_.relativeForce04;
  force[5] = config_st.payLoadPedalConfig_.relativeForce05;
  force[6] = config_st.payLoadPedalConfig_.relativeForce06;
  force[7] = config_st.payLoadPedalConfig_.relativeForce07;
  force[8] = config_st.payLoadPedalConfig_.relativeForce08;
  force[9] = config_st.payLoadPedalConfig_.relativeForce09;
  force[10] = config_st.payLoadPedalConfig_.relativeForce10;

  travel[0] = config_st.payLoadPedalConfig_.relativeTravel00;
  travel[1] = config_st.payLoadPedalConfig_.relativeTravel01;
  travel[2] = config_st.payLoadPedalConfig_.relativeTravel02;
  travel[3] = config_st.payLoadPedalConfig_.relativeTravel03;
  travel[4] = config_st.payLoadPedalConfig_.relativeTravel04;
  travel[5] = config_st.payLoadPedalConfig_.relativeTravel05;
  travel[6] = config_st.payLoadPedalConfig_.relativeTravel06;
  travel[7] = config_st.payLoadPedalConfig_.relativeTravel07;
  travel[8] = config_st.payLoadPedalConfig_.relativeTravel08;
  travel[9] = config_st.payLoadPedalConfig_.relativeTravel09;
  travel[10] = config_st.payLoadPedalConfig_.relativeTravel10;
  // cubic interpolator
  float travel_x[config_st.payLoadPedalConfig_.quantityOfControl];
  float force_y[config_st.payLoadPedalConfig_.quantityOfControl];
  
  for (int i = 0; i < config_st.payLoadPedalConfig_.quantityOfControl;i++)
  {
    travel_x[i]=travel[i];
    force_y[i]=force[i];
  }
  
  _cubic.Interpolate1D(travel_x, force_y, config_st.payLoadPedalConfig_.quantityOfControl - 1, config_st.payLoadPedalConfig_.quantityOfControl-1);
  interpolatorA = _cubic._result.a;
  interpolatorB = _cubic._result.b;
  /*
  for (int i = 0; i < config_st.payLoadPedalConfig_.quantityOfControl - 1; ++i)
  {
    //Serial.printf("original a=%.3f, b=%.3f\n", config_st.payLoadPedalConfig_.cubic_spline_param_a_array[i], config_st.payLoadPedalConfig_.cubic_spline_param_b_array[i]);
    Serial.printf("ESP calculated a=%.3f, b=%.3f\n", interpolatorA[i], interpolatorB[i]);
  }
  */
  
  //testing code
  numOfJoystickControl=config_st.payLoadPedalConfig_.numOfJoystickMapControl;
  joystickOrig[0]=config_st.payLoadPedalConfig_.joystickMapOrig00;
  joystickOrig[1]=config_st.payLoadPedalConfig_.joystickMapOrig01;
  joystickOrig[2]=config_st.payLoadPedalConfig_.joystickMapOrig02;
  joystickOrig[3]=config_st.payLoadPedalConfig_.joystickMapOrig03;
  joystickOrig[4]=config_st.payLoadPedalConfig_.joystickMapOrig04;
  joystickOrig[5]=config_st.payLoadPedalConfig_.joystickMapOrig05;
  joystickOrig[6]=config_st.payLoadPedalConfig_.joystickMapOrig06;
  joystickOrig[7]=config_st.payLoadPedalConfig_.joystickMapOrig07;
  joystickOrig[8]=config_st.payLoadPedalConfig_.joystickMapOrig08;
  joystickOrig[9]=config_st.payLoadPedalConfig_.joystickMapOrig09;
  joystickOrig[10]=config_st.payLoadPedalConfig_.joystickMapOrig10;
  joystickMapping[0]=config_st.payLoadPedalConfig_.joystickMapMapped00;
  joystickMapping[1]=config_st.payLoadPedalConfig_.joystickMapMapped01;
  joystickMapping[2]=config_st.payLoadPedalConfig_.joystickMapMapped02;
  joystickMapping[3]=config_st.payLoadPedalConfig_.joystickMapMapped03;
  joystickMapping[4]=config_st.payLoadPedalConfig_.joystickMapMapped04;
  joystickMapping[5]=config_st.payLoadPedalConfig_.joystickMapMapped05;
  joystickMapping[6]=config_st.payLoadPedalConfig_.joystickMapMapped06;
  joystickMapping[7]=config_st.payLoadPedalConfig_.joystickMapMapped07;
  joystickMapping[8]=config_st.payLoadPedalConfig_.joystickMapMapped08;
  joystickMapping[9]=config_st.payLoadPedalConfig_.joystickMapMapped09;
  joystickMapping[10]=config_st.payLoadPedalConfig_.joystickMapMapped10;
  
  float joystick_x[numOfJoystickControl]={0};
  float joystick_y[numOfJoystickControl]={0};
  for(int i=0;i<numOfJoystickControl;i++)
  {
    joystick_x[i]=joystickOrig[i]-joystickOrig[0];
    joystick_y[i]=joystickMapping[i];
  }
  joystickInterpolarter.Interpolate1D(joystick_x,joystick_y,numOfJoystickControl,100);
  /*
  for (int i = 0; i < 5; ++i)
  {
    //Serial.printf("original a=%.3f, b=%.3f\n", config_st.payLoadPedalConfig_.cubic_spline_param_a_array[i], config_st.payLoadPedalConfig_.cubic_spline_param_b_array[i]);
    Serial.printf("joystick calculated a=%.3f, b=%.3f\n", joystickInterpolarter._result.a[i], joystickInterpolarter._result.b[i]);
  }
  
  for(int i=0;i<100;i++)
  {
    Serial.printf("joystick value:y= %.3f\n",joystickInterpolarter._result.yInterp[i]);
  }
  */



  if (startPosRel == endPosRel)
  {
    endPosRel = startPosRel + 1 / 100;
  }

  absFrequency = ((float)config_st.payLoadPedalConfig_.absFrequency);
  absAmplitude = ((float)config_st.payLoadPedalConfig_.absAmplitude) / 20.0f; // in kg or percent

  dampingPress = ((float)config_st.payLoadPedalConfig_.dampingPress) * 0.00015f;
  rpmMaxFreq_f = ((float)config_st.payLoadPedalConfig_.RPM_max_freq);
  rpmMinFreq_f = ((float)config_st.payLoadPedalConfig_.RPM_min_freq);
  rpmAmp_f = ((float)config_st.payLoadPedalConfig_.RPM_AMP) / 100.0f;
  // Bite point effect;

  bpTriggerValue_f = (float)config_st.payLoadPedalConfig_.BP_trigger_value;
  bpAmp_f = ((float)config_st.payLoadPedalConfig_.BP_amp) / 100.0f;
  bpFreq_f = (float)config_st.payLoadPedalConfig_.BP_freq;
  wsAmp_f = ((float)config_st.payLoadPedalConfig_.WS_amp) / 20.0f;
  wsFreq_f = (float)config_st.payLoadPedalConfig_.WS_freq;
  // update force variables
  forceMin_f = ((float)config_st.payLoadPedalConfig_.preloadForce);
  forceMax_f = ((float)config_st.payLoadPedalConfig_.maxForce);
  forceRange_f = forceMax_f - forceMin_f;
  forceMaxDefault_f = ((float)config_st.payLoadPedalConfig_.maxForce);
  pedalType_u8 = config_st.payLoadPedalConfig_.pedal_type;

  // calculate steps per motor revolution
  float helper = MAXIMUM_STEPPER_SPEED / (MAXIMUM_STEPPER_RPM / SECONDS_PER_MINUTE);
  helper = floor(helper / 10) * 10;
  helper = constrain(helper, 2000, 10000);
  stepsPerMotorRevolution = helper;

    // // when spindle pitch is smaller than 8, choose coarse microstepping
    // if ( 8 > config_st.payLoadPedalConfig_.spindlePitch_mmPerRev_u8)
    // {stepsPerMotorRevolution = 3200;}
    // else{stepsPerMotorRevolution = 6400;}

    // stepsPerMotorRevolution = 3750;
}

void DAP_calculationVariables_st::dynamicUpdate()
{
  forceRange_f = forceMax_f - forceMin_f;
}

void DAP_calculationVariables_st::resetMaxForce()
{
  forceMax_f = forceMaxDefault_f;
}

void DAP_calculationVariables_st::updateEndstops(long newMinEndstop, long newMaxEndstop) {
 
  if ( newMinEndstop == newMaxEndstop )
  {
    newMaxEndstop = newMinEndstop  + 10;
  }
  
  stepperPosMinEndstop = newMinEndstop;
  stepperPosMaxEndstop = newMaxEndstop;
  stepperPosEndstopRange = stepperPosMaxEndstop - stepperPosMinEndstop;
  
  stepperPosMin = stepperPosEndstopRange * startPosRel;
  stepperPosMax = stepperPosEndstopRange * endPosRel;
  stepperPosMinDefault = stepperPosMin;
  stepperPosRange = stepperPosMax - stepperPosMin;
  //currentPedalPositionRatio_f=((float)(currentPedalPosition_u32-stepperPosMinDefault))/((float)stepperPosRangeDefault);
}

void DAP_calculationVariables_st::updateStiffness() {
  springStiffnesss = forceRange_f / stepperPosRange;
  if ( fabs(springStiffnesss) > 0.0001 )
  {
      springStiffnesssInv = 1.0 / springStiffnesss;
  }
  else
  {
    springStiffnesssInv = 1000000;
  }
  
  }

void DAP_calculationVariables_st::stepperPosSetback()
{
  stepperPosMin=stepperPosMinDefault;
  stepperPosMax=stepperPosMaxDefault;
  stepperPosRange = stepperPosRangeDefault;
}

void DAP_calculationVariables_st::updateStepperMinPos(long newMinstop)
{
  stepperPosMin=newMinstop;
  
  stepperPosRange = stepperPosMax - stepperPosMin;
}
void DAP_calculationVariables_st::updateStepperMaxPos(long newMaxstop)
{
  
  stepperPosMax=newMaxstop;
  stepperPosRange = stepperPosMax - stepperPosMin;
}

void DAP_calculationVariables_st::setDefaultPos()
{
  stepperPosMinDefault = stepperPosMin;
  stepperPosMaxDefault = stepperPosMax;
  stepperPosRangeDefault=stepperPosRange;
}



/**********************************************************************************************/
/*                                                                                            */
/*                         DAP_config_class                                                   */
/*                                                                                            */
/**********************************************************************************************/
// constructor
DAP_config_class::DAP_config_class() {

  // create the mutex
  mutex = xSemaphoreCreateMutex();
  if (mutex == NULL)
  {
    Serial.println("Error: Mutex could not be created!");
    ESP.restart();
  }

  // initialize the default config
  _config_st.initializeDefaults();
}


// method to safely get the config variable
DAP_config_st DAP_config_class::getConfig() {
  DAP_config_st tmp;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    tmp = _config_st;
    // gives back the mutex
    xSemaphoreGive(mutex);
  }

  return tmp;
}

// method to safely set the config variable
void DAP_config_class::setConfig(DAP_config_st tmp) {
  // boolean returnV_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    _config_st = tmp;
    // returnV_b = true;
    // gives back the mutex
    xSemaphoreGive(mutex);
  }
  else
  {
    Serial.println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}



void DAP_config_class::loadConfigFromEeprom() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    _config_st.loadConfigFromEeprom(_config_st);
    xSemaphoreGive(mutex);
  }
}

void DAP_config_class::storeConfigToEeprom() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    _config_st.storeConfigToEeprom(_config_st);
    xSemaphoreGive(mutex);
  }
}

void DAP_config_class::initializedConfig()
{
  // boolean returnV_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE)
  {
    _config_st.initializeDefaults();
    // returnV_b = true;
    // gives back the mutex
    xSemaphoreGive(mutex);
  }
  else
  {
    Serial.println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}