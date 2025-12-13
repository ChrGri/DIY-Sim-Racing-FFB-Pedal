#include "DiyActivePedal_types.h"


#include "PedalGeometry.h"
#include "StepperWithLimits.h"

#include <EEPROM.h>

static const float ABS_SCALING = 50.0f;

#define WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT 500

const uint32_t EEPROM_OFFSET = ASSIGNMENT_EEPROM_OFFSET+ sizeof(DAP_Assignement_reg)+ASSIGNMENT_EEPROM_OFFSET/*+(DAP_VERSION_CONFIG-128) * sizeof(DAP_config_st) % (2048-sizeof(DAP_config_st))*/;

void DAP_config_st::initialiseDefaults() {

  payLoadHeader_.startOfFrame0_u8 = SOF_BYTE_0;
  payLoadHeader_.startOfFrame1_u8 = SOF_BYTE_1;
  payLoadHeader_.payloadType = DAP_PAYLOAD_TYPE_CONFIG;
  payLoadHeader_.version = DAP_VERSION_CONFIG;
  payLoadHeader_.storeToEeprom = false;

  payloadFooter_.enfOfFrame0_u8 = EOF_BYTE_0;
  payloadFooter_.enfOfFrame1_u8 = EOF_BYTE_1;

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
  payLoadPedalConfig_.minForceForEffects_u8=0;
  payLoadPedalConfig_.servoRatioOfInertia_u8=1;
  payLoadPedalConfig_.configHash = (uint32_t)175245064 ;
}




void DAP_config_st::storeConfigToEprom(DAP_config_st& config_st)
{

  EEPROM.put(EEPROM_OFFSET, config_st); 
  EEPROM.commit();
  ActiveSerial->println("Successfully stored config in EPROM");
  
  /*if (true == config_st.payLoadHeader_.storeToEeprom)
  {
    config_st.payLoadHeader_.storeToEeprom = false; // set to false, thus at restart existing EEPROM config isn't restored to EEPROM
    EEPROM.put(0, config_st); 
    EEPROM.commit();
    ActiveSerial->println("Successfully stored config in EPROM");
  }*/
}

void DAP_config_st::loadConfigFromEprom(DAP_config_st& config_st)
{
  DAP_config_st local_config_st;

  EEPROM.get(EEPROM_OFFSET, local_config_st);
  //EEPROM.commit();

  config_st = local_config_st;

  // check if version matches revision, in case, update the default config
  /*if (local_config_st.payLoadHeader_.version == DAP_VERSION_CONFIG)
  {
    config_st = local_config_st;
    ActiveSerial->println("Successfully loaded config from EPROM");
  }
  else
  { 
    ActiveSerial->println("Couldn't load config from EPROM due to version mismatch");
    ActiveSerial->print("Target version: ");
    ActiveSerial->println(DAP_VERSION_CONFIG);
    ActiveSerial->print("Source version: ");
    ActiveSerial->println(local_config_st.payLoadHeader_.version);

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
    //ActiveSerial->printf("original a=%.3f, b=%.3f\n", config_st.payLoadPedalConfig_.cubic_spline_param_a_array[i], config_st.payLoadPedalConfig_.cubic_spline_param_b_array[i]);
    ActiveSerial->printf("ESP calculated a=%.3f, b=%.3f\n", interpolatorA[i], interpolatorB[i]);
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
    //ActiveSerial->printf("original a=%.3f, b=%.3f\n", config_st.payLoadPedalConfig_.cubic_spline_param_a_array[i], config_st.payLoadPedalConfig_.cubic_spline_param_b_array[i]);
    ActiveSerial->printf("joystick calculated a=%.3f, b=%.3f\n", joystickInterpolarter._result.a[i], joystickInterpolarter._result.b[i]);
  }
  
  for(int i=0;i<100;i++)
  {
    ActiveSerial->printf("joystick value:y= %.3f\n",joystickInterpolarter._result.yInterp[i]);
  }
  */



  if (startPosRel == endPosRel)
  {
    endPosRel = startPosRel + 0.01f;
  }

  absFrequency = ((float)config_st.payLoadPedalConfig_.absFrequency);
  absAmplitude = ((float)config_st.payLoadPedalConfig_.absAmplitude)  *0.001f;//in percent, max 20% of force range

  dampingPress = ((float)config_st.payLoadPedalConfig_.dampingPress) * 0.00015f;
  RPM_max_freq = ((float)config_st.payLoadPedalConfig_.RPM_max_freq);
  RPM_min_freq = ((float)config_st.payLoadPedalConfig_.RPM_min_freq);
  RPM_AMP = ((float)config_st.payLoadPedalConfig_.RPM_AMP)  * 0.0002f;//in kg, max 4% of force range
  // Bite point effect;

  BP_trigger_value = (float)config_st.payLoadPedalConfig_.BP_trigger_value;
  BP_amp = ((float)config_st.payLoadPedalConfig_.BP_amp)   *0.001f;//in kg, max 20% of force range
  BP_freq = (float)config_st.payLoadPedalConfig_.BP_freq;
  WS_amp = ((float)config_st.payLoadPedalConfig_.WS_amp)  *0.001f;//in kg, max 20% of force range
  WS_freq = (float)config_st.payLoadPedalConfig_.WS_freq;
  // update force variables
  Force_Min = ((float)config_st.payLoadPedalConfig_.preloadForce);
  Force_Max = ((float)config_st.payLoadPedalConfig_.maxForce);
  Force_Range = Force_Max - Force_Min;
  Force_Max_default = ((float)config_st.payLoadPedalConfig_.maxForce);
  pedal_type = config_st.payLoadPedalConfig_.pedal_type;

  // calculate steps per motor revolution
  float helper = MAXIMUM_STEPPER_SPEED / (MAXIMUM_STEPPER_RPM / SECONDS_PER_MINUTE);
  helper = floor(helper / 10.0f) * 10.0f;
  helper = constrain(helper, 2000.0f, 10000.0f);
  stepsPerMotorRevolution = helper;

    // // when spindle pitch is smaller than 8, choose coarse microstepping
    // if ( 8 > config_st.payLoadPedalConfig_.spindlePitch_mmPerRev_u8)
    // {stepsPerMotorRevolution = 3200;}
    // else{stepsPerMotorRevolution = 6400;}

    // stepsPerMotorRevolution = 3750;
}

void IRAM_ATTR_FLAG DAP_calculationVariables_st::dynamic_update()
{
  Force_Range = Force_Max - Force_Min;
}

void IRAM_ATTR_FLAG DAP_calculationVariables_st::reset_maxforce()
{
  Force_Max = Force_Max_default;
}

void IRAM_ATTR_FLAG DAP_calculationVariables_st::updateEndstops(long newMinEndstop, long newMaxEndstop) {
 
  if ( newMinEndstop == newMaxEndstop )
  {
    newMaxEndstop = newMinEndstop  + 10;
  }
  
  stepperPosMinEndstop = newMinEndstop;
  stepperPosMaxEndstop = newMaxEndstop;
  stepperPosEndstopRange = stepperPosMaxEndstop - stepperPosMinEndstop;
  
  stepperPosMin = stepperPosEndstopRange * startPosRel;
  stepperPosMax = stepperPosEndstopRange * endPosRel;
  stepperPosMin_default = stepperPosMin;
  stepperPosRange = stepperPosMax - stepperPosMin;
  //current_pedal_position_ratio=((float)(current_pedal_position-stepperPosMin_default))/((float)stepperPosRange_default);
}

void IRAM_ATTR_FLAG DAP_calculationVariables_st::updateStiffness() {
  springStiffnesss = Force_Range / stepperPosRange;
  if ( fabsf(springStiffnesss) > 0.0001f )
  {
      springStiffnesssInv = 1.0f / springStiffnesss;
  }
  else
  {
    springStiffnesssInv = 1000000.0f;
  }
  
  }

void DAP_calculationVariables_st::StepperPos_setback()
{
  stepperPosMin=stepperPosMin_default;
  stepperPosMax=stepperPosMax_default;
  stepperPosRange = stepperPosRange_default;
}

void DAP_calculationVariables_st::update_stepperMinpos(long newMinstop)
{
  stepperPosMin=newMinstop;
  
  stepperPosRange = stepperPosMax - stepperPosMin;
}
void DAP_calculationVariables_st::update_stepperMaxpos( long newMaxstop)
{
  
  stepperPosMax=newMaxstop;
  stepperPosRange = stepperPosMax - stepperPosMin;
}

void DAP_calculationVariables_st::Default_pos()
{
  stepperPosMin_default = stepperPosMin;
  stepperPosMax_default = stepperPosMax;
  stepperPosRange_default=stepperPosRange;
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
  if (mutex == NULL) {
    ActiveSerial->println("Error: Mutex could not be created!");
    ESP.restart();
  }

  // initialize the default config
  _config_st.initialiseDefaults();
}


// method to safely get the config variable
bool DAP_config_class::getConfig(DAP_config_st * dapConfigIn_pst, uint16_t timeoutInMs_u16) {
  bool configUpdated_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  // if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(timeoutInMs_u16)) == pdTRUE) {
    *dapConfigIn_pst = _config_st;
    // gives back the mutex
    xSemaphoreGive(mutex);
    configUpdated_b = true;
  }

  return configUpdated_b;
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
    ActiveSerial->println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}

uint16_t  DAP_config_class::checksumCalculator(uint8_t * data, uint16_t length)
{
   uint16_t curr_crc = 0x0000;
   uint8_t sum1 = (uint8_t) curr_crc;
   uint8_t sum2 = (uint8_t) (curr_crc >> 8);
   int index;
   for(index = 0; index < length; index = index+1)
   {
      sum1 = (sum1 + data[index]) % 255;
      sum2 = (sum2 + sum1) % 255;
   }
   return (sum2 << 8) | sum1;
}

void DAP_config_class::loadConfigFromEprom() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    _config_st.loadConfigFromEprom(_config_st);
    xSemaphoreGive(mutex);
  }
}

void DAP_config_class::storeConfigToEprom() {
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE) {
    _config_st.payLoadHeader_.storeToEeprom = 0;
    uint16_t crc = checksumCalculator((uint8_t*)(&(_config_st.payLoadHeader_)), sizeof(_config_st.payLoadHeader_) + sizeof(_config_st.payLoadPedalConfig_));
    _config_st.payloadFooter_.checkSum = crc;
    _config_st.storeConfigToEprom(_config_st);
    xSemaphoreGive(mutex);
  }
}

void DAP_config_class::initializedConfig()
{
  // boolean returnV_b = false;
  // requests the mutex, waits N milliseconds if not available immediately
  if (xSemaphoreTake(mutex, pdMS_TO_TICKS(WAIT_TIME_IN_MS_TO_AQUIRE_GLOBAL_STRUCT)) == pdTRUE)
  {
    _config_st.initialiseDefaults();
    // returnV_b = true;
    // gives back the mutex
    xSemaphoreGive(mutex);
  }
  else
  {
    ActiveSerial->println("Error: Coul not aquire mutex!");
  }

  // return returnV_b;
}