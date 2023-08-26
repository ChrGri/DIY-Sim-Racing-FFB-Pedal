#include <ArduinoJson.h>
#include "DiyActivePedal_types.h"

void sendConfig(DAP_config_st data)
{
  //Serial.println(data.payLoadHeader_.payloadType);
  StaticJsonDocument<610> doc;


    JsonObject payloadHeader = doc.createNestedObject("payloadHeader_");
    payloadHeader["checkSum"] =data.payLoadHeader_.checkSum;
    payloadHeader["payloadType"] =data.payLoadHeader_.payloadType;
    payloadHeader["storeToEeprom"] =data.payLoadHeader_.storeToEeprom;
    payloadHeader["version"] =data.payLoadHeader_.version;

    JsonObject payloadPedalConfig = doc.createNestedObject("payloadPedalConfig_");
    payloadPedalConfig["PID_d_gain"] = data.payLoadPedalConfig_.PID_d_gain;
    payloadPedalConfig["PID_i_gain"] = data.payLoadPedalConfig_.PID_i_gain;
    payloadPedalConfig["PID_p_gain"] = data.payLoadPedalConfig_.PID_p_gain;
    payloadPedalConfig["absAmplitude"] = data.payLoadPedalConfig_.absAmplitude;
    payloadPedalConfig["absFrequency"] = data.payLoadPedalConfig_.absFrequency;
    payloadPedalConfig["cubic_spline_param_a_0"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[0];
    payloadPedalConfig["cubic_spline_param_a_1"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[1];
    payloadPedalConfig["cubic_spline_param_a_2"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[2];
    payloadPedalConfig["cubic_spline_param_a_3"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[3];
    payloadPedalConfig["cubic_spline_param_a_4"] = data.payLoadPedalConfig_.cubic_spline_param_a_array[4];
    payloadPedalConfig["cubic_spline_param_b_0"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[0];
    payloadPedalConfig["cubic_spline_param_b_1"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[1];
    payloadPedalConfig["cubic_spline_param_b_2"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[2];
    payloadPedalConfig["cubic_spline_param_b_3"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[3];
    payloadPedalConfig["cubic_spline_param_b_4"] = data.payLoadPedalConfig_.cubic_spline_param_b_array[4];
    payloadPedalConfig["dampingPress"] = data.payLoadPedalConfig_.dampingPress;
    payloadPedalConfig["dampingPull"] = data.payLoadPedalConfig_.dampingPull;
    payloadPedalConfig["horPos_AB"] = data.payLoadPedalConfig_.horPos_AB;
    payloadPedalConfig["lengthPedal_AC"] = data.payLoadPedalConfig_.lengthPedal_AC;
    payloadPedalConfig["lengthPedal_CB"] = data.payLoadPedalConfig_.lengthPedal_CB;
    payloadPedalConfig["maxForce"] = data.payLoadPedalConfig_.maxForce;
    payloadPedalConfig["maxGameOutput"] = data.payLoadPedalConfig_.maxGameOutput;
    payloadPedalConfig["pedalEndPosition"] = data.payLoadPedalConfig_.pedalEndPosition;
    payloadPedalConfig["pedalStartPosition"] = data.payLoadPedalConfig_.pedalStartPosition;
    payloadPedalConfig["preloadForce"] = data.payLoadPedalConfig_.preloadForce;
    payloadPedalConfig["relativeForce_p000"] = data.payLoadPedalConfig_.relativeForce_p000;
    payloadPedalConfig["relativeForce_p020"] = data.payLoadPedalConfig_.relativeForce_p020;
    payloadPedalConfig["relativeForce_p040"] = data.payLoadPedalConfig_.relativeForce_p040;
    payloadPedalConfig["relativeForce_p060"] = data.payLoadPedalConfig_.relativeForce_p060;
    payloadPedalConfig["relativeForce_p080"] = data.payLoadPedalConfig_.relativeForce_p080;
    payloadPedalConfig["relativeForce_p100"] = data.payLoadPedalConfig_.relativeForce_p100;
    payloadPedalConfig["verPos_AB"] = data.payLoadPedalConfig_.verPos_AB;

  String jsonString;
    serializeJson(doc, jsonString);

    Serial.println(jsonString);
    
}