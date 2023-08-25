#include <ArduinoJson.h>
#include "DiyActivePedal_types.h"

void sendConfig(DAP_config_st data)
{
  //Serial.println(data.payLoadHeader_.payloadType);
  StaticJsonDocument<560> doc;
  doc ["payloadType"] = data.payLoadHeader_.payloadType;
  doc ["version"] = data.payLoadHeader_.version;
  doc ["storeToEeprom"] = data.payLoadHeader_.storeToEeprom;

  doc ["pedalStartPosition"] = data.payLoadPedalConfig_.pedalStartPosition;
  doc ["pedalEndPosition"] = data.payLoadPedalConfig_.pedalEndPosition;

  doc ["maxForce"] = data.payLoadPedalConfig_.maxForce;
  doc ["preloadForce"] = data.payLoadPedalConfig_.preloadForce;

  doc ["relativeForce_p000"] = data.payLoadPedalConfig_.relativeForce_p000;
  doc ["relativeForce_p020"] = data.payLoadPedalConfig_.relativeForce_p020;
  doc ["relativeForce_p040"] = data.payLoadPedalConfig_.relativeForce_p040;
  doc ["relativeForce_p060"] = data.payLoadPedalConfig_.relativeForce_p060;
  doc ["relativeForce_p080"] = data.payLoadPedalConfig_.relativeForce_p080;
  doc ["relativeForce_p100"] = data.payLoadPedalConfig_.relativeForce_p100;

  doc ["dampingPress"] = data.payLoadPedalConfig_.dampingPress;
  doc ["dampingPull"] = data.payLoadPedalConfig_.dampingPull;

  doc ["absFrequency"] = data.payLoadPedalConfig_.absFrequency;
  doc ["absAmplitude"] = data.payLoadPedalConfig_.absAmplitude;

  doc ["lengthPedal_AC"] = data.payLoadPedalConfig_.lengthPedal_AC;
  doc ["horPos_AB"] = data.payLoadPedalConfig_.horPos_AB;
  doc ["verPos_AB"] = data.payLoadPedalConfig_.verPos_AB;
  doc ["lengthPedal_CB"] = data.payLoadPedalConfig_.lengthPedal_CB;

  doc ["cubic_spline_param_a_array[0]"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[0];
  doc ["cubic_spline_param_a_array[1]"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[1];
  doc ["cubic_spline_param_a_array[2]"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[2];
  doc ["cubic_spline_param_a_array[3]"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[3];
  doc ["cubic_spline_param_a_array[4]"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[4];

  doc ["cubic_spline_param_b_array[0]"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[0];
  doc ["cubic_spline_param_b_array[1]"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[1];
  doc ["cubic_spline_param_b_array[2]"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[2];
  doc ["cubic_spline_param_b_array[3]"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[3];
  doc ["cubic_spline_param_b_array[4]"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[4];

  doc ["PID_p_gain"] = data.payLoadPedalConfig_.PID_p_gain;
  doc ["PID_i_gain"] = data.payLoadPedalConfig_.PID_i_gain;
  doc ["PID_d_gain"] = data.payLoadPedalConfig_.PID_d_gain;

  doc ["maxGameOutput"] = data.payLoadPedalConfig_.maxGameOutput;

  String jsonString;
    serializeJson(doc, jsonString);

    Serial.println(jsonString);
    delay(1000);
}