// ---------------------------------------------------------------------------
// DapAttributeProtocol.cpp
//
// Apply/Read helpers that translate DapAttrPacket_t <-> config/action structs.
// ---------------------------------------------------------------------------

#include "DapAttributeProtocol.h"
#include "DiyActivePedal_types.h"
#include <string.h>   // memcpy

// Helper: bit-exact float <-> int64_t conversion
static inline int64_t floatToI64(float f) {
    int64_t v = 0;
    memcpy(&v, &f, sizeof(f));
    return v;
}
static inline float i64ToFloat(int64_t v) {
    float f = 0.0f;
    memcpy(&f, &v, sizeof(f));
    return f;
}

// ---------------------------------------------------------------------------
// DapAttr_ApplyToConfig
// Writes one attribute value into PayloadPedalConfig_t.
// ---------------------------------------------------------------------------
void DapAttr_ApplyToConfig(PayloadPedalConfig_t* cfg, const DapAttrPacket_t* pkt)
{
    if (!cfg || !pkt) return;
    int64_t v = pkt->value_i64;
    switch ((DapConfigAttrId_e)pkt->attrId_u16) {
        case DAP_ATTR_CFG_PEDAL_START_POS_U8:                  cfg->pedalStartPosition_u8                  = (uint8_t)v;  break;
        case DAP_ATTR_CFG_PEDAL_END_POS_U8:                    cfg->pedalEndPosition_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_MAX_FORCE_FL32:                      cfg->maxForce_fl32                          = i64ToFloat(v); break;
        case DAP_ATTR_CFG_PRELOAD_FORCE_FL32:                  cfg->preloadForce_fl32                      = i64ToFloat(v); break;
        case DAP_ATTR_CFG_QUANTITY_OF_CONTROL_U8:              cfg->quantityOfControl_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_00_U8:               cfg->relativeForce00_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_01_U8:               cfg->relativeForce01_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_02_U8:               cfg->relativeForce02_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_03_U8:               cfg->relativeForce03_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_04_U8:               cfg->relativeForce04_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_05_U8:               cfg->relativeForce05_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_06_U8:               cfg->relativeForce06_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_07_U8:               cfg->relativeForce07_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_08_U8:               cfg->relativeForce08_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_09_U8:               cfg->relativeForce09_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_10_U8:               cfg->relativeForce10_u8                     = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_00_U8:              cfg->relativeTravel00_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_01_U8:              cfg->relativeTravel01_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_02_U8:              cfg->relativeTravel02_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_03_U8:              cfg->relativeTravel03_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_04_U8:              cfg->relativeTravel04_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_05_U8:              cfg->relativeTravel05_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_06_U8:              cfg->relativeTravel06_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_07_U8:              cfg->relativeTravel07_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_08_U8:              cfg->relativeTravel08_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_09_U8:              cfg->relativeTravel09_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_10_U8:              cfg->relativeTravel10_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_NUM_OF_JOYSTICK_MAP_CONTROL_U8:     cfg->numOfJoystickMapControl_u8             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_00_U8:            cfg->joystickMapOrig00_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_01_U8:            cfg->joystickMapOrig01_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_02_U8:            cfg->joystickMapOrig02_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_03_U8:            cfg->joystickMapOrig03_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_04_U8:            cfg->joystickMapOrig04_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_05_U8:            cfg->joystickMapOrig05_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_06_U8:            cfg->joystickMapOrig06_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_07_U8:            cfg->joystickMapOrig07_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_08_U8:            cfg->joystickMapOrig08_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_09_U8:            cfg->joystickMapOrig09_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_10_U8:            cfg->joystickMapOrig10_u8                   = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_00_U8:          cfg->joystickMapMapped00_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_01_U8:          cfg->joystickMapMapped01_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_02_U8:          cfg->joystickMapMapped02_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_03_U8:          cfg->joystickMapMapped03_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_04_U8:          cfg->joystickMapMapped04_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_05_U8:          cfg->joystickMapMapped05_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_06_U8:          cfg->joystickMapMapped06_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_07_U8:          cfg->joystickMapMapped07_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_08_U8:          cfg->joystickMapMapped08_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_09_U8:          cfg->joystickMapMapped09_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_10_U8:          cfg->joystickMapMapped10_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ABS_FREQUENCY_U8:                   cfg->absFrequency_u8                        = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ABS_AMPLITUDE_U8:                   cfg->absAmplitude_u8                        = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ABS_PATTERN_U8:                     cfg->absPattern_u8                          = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ABS_FORCE_OR_TRAVEL_BIT_U8:         cfg->absForceOrTarvelBit_u8                 = (uint8_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_A_I16:                 cfg->lengthPedalA_i16                       = (int16_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_B_I16:                 cfg->lengthPedalB_i16                       = (int16_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_D_I16:                 cfg->lengthPedalD_i16                       = (int16_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_C_HORIZONTAL_I16:      cfg->lengthPedalCHorizontal_i16             = (int16_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_C_VERTICAL_I16:        cfg->lengthPedalCVertical_i16               = (int16_t)v;  break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_TRAVEL_I16:            cfg->lengthPedalTravel_i16                  = (int16_t)v;  break;
        case DAP_ATTR_CFG_SIMULATE_ABS_TRIGGER_U8:            cfg->simulateAbsTrigger_u8                  = (uint8_t)v;  break;
        case DAP_ATTR_CFG_SIMULATE_ABS_VALUE_U8:              cfg->simulateAbsValue_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RPM_MAX_FREQ_U8:                    cfg->rpmMaxFreq_u8                          = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RPM_MIN_FREQ_U8:                    cfg->rpmMinFreq_u8                          = (uint8_t)v;  break;
        case DAP_ATTR_CFG_RPM_AMP_U8:                         cfg->rpmAmp_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_BP_TRIGGER_VALUE_U8:                cfg->bpTriggerValue_u8                      = (uint8_t)v;  break;
        case DAP_ATTR_CFG_BP_AMP_U8:                          cfg->bpAmp_u8                               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_BP_FREQ_U8:                         cfg->bpFreq_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_BP_TRIGGER_U8:                      cfg->bpTrigger_u8                           = (uint8_t)v;  break;
        case DAP_ATTR_CFG_G_MULTI_U8:                         cfg->gMulti_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_G_WINDOW_U8:                        cfg->gWindow_u8                             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_WS_AMP_U8:                          cfg->wsAmp_u8                               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_WS_FREQ_U8:                         cfg->wsFreq_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ROAD_MULTI_U8:                      cfg->roadMulti_u8                           = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ROAD_WINDOW_U8:                     cfg->roadWindow_u8                          = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_AMP1_U8:                         cfg->cvAmp1_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_FREQ1_U8:                        cfg->cvFreq1_u8                             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_AMP2_U8:                         cfg->cvAmp2_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_FREQ2_U8:                        cfg->cvFreq2_u8                             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_AMP3_U8:                         cfg->cvAmp3_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_FREQ3_U8:                        cfg->cvFreq3_u8                             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_AMP4_U8:                         cfg->cvAmp4_u8                              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CV_FREQ4_U8:                        cfg->cvFreq4_u8                             = (uint8_t)v;  break;
        case DAP_ATTR_CFG_MAX_GAME_OUTPUT_U8:                 cfg->maxGameOutput_u8                       = (uint8_t)v;  break;
        case DAP_ATTR_CFG_KF_MODEL_NOISE_U8:                  cfg->kfModelNoise_u8                        = (uint8_t)v;  break;
        case DAP_ATTR_CFG_KF_MODEL_ORDER_U8:                  cfg->kfModelOrder_u8                        = (uint8_t)v;  break;
        case DAP_ATTR_CFG_DEBUG_FLAGS0_U8:                    cfg->debugFlags0_u8                         = (uint8_t)v;  break;
        case DAP_ATTR_CFG_LOADCELL_RATING_U8:                 cfg->loadcellRating_u8                      = (uint8_t)v;  break;
        case DAP_ATTR_CFG_TRAVEL_AS_JOYSTICK_OUTPUT_U8:       cfg->travelAsJoystickOutput_u8              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_INVERT_LOADCELL_READING_U8:         cfg->invertLoadcellReading_u8               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_INVERT_MOTOR_DIRECTION_U8:          cfg->invertMotorDirection_u8                = (uint8_t)v;  break;
        case DAP_ATTR_CFG_SPINDLE_PITCH_MM_PER_REV_U8:        cfg->spindlePitch_mmPerRev_u8               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_PEDAL_TYPE_U8:                      cfg->pedalType_u8                           = (uint8_t)v;  break;
        case DAP_ATTR_CFG_STEP_LOSS_FUNCTION_FLAGS_U8:        cfg->stepLossFunctionFlags_u8               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_KF_JOYSTICK_U8:                     cfg->kfJoystick_u8                          = (uint8_t)v;  break;
        case DAP_ATTR_CFG_KF_MODEL_NOISE_JOYSTICK_U8:         cfg->kfModelNoiseJoystick_u8                = (uint8_t)v;  break;
        case DAP_ATTR_CFG_SERVO_IDLE_TIMEOUT_U8:              cfg->servoIdleTimeout_u8                    = (uint8_t)v;  break;
        case DAP_ATTR_CFG_MIN_FORCE_FOR_EFFECTS_U8:           cfg->minForceForEffects_u8                  = (uint8_t)v;  break;
        case DAP_ATTR_CFG_CONFIG_HASH_U32:                    cfg->configHash_u32                         = (uint32_t)v; break;
        case DAP_ATTR_CFG_ENDSTOP_DETECTION_THRESHOLD_U8:     cfg->endstopDetectionThreshold_u8           = (uint8_t)v;  break;
        case DAP_ATTR_CFG_VIRTUAL_PEDAL_MASS_IN_PERCENT_U8:   cfg->virtualPedalMassInPercent_u8           = (uint8_t)v;  break;
        case DAP_ATTR_CFG_VIRTUAL_PEDAL_DAMPING_IN_PERCENT_U8:cfg->virtualPedalDampingInPercent_u8        = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ENDSTOP_STIFFNESS_KG_MM_U8:        cfg->endstopStiffness_kg_mm_u8              = (uint8_t)v;  break;
        case DAP_ATTR_CFG_ENDSTOP_TRAVEL_RANGE_MM_U8:         cfg->endstopTravelRange_mm_u8               = (uint8_t)v;  break;
        case DAP_ATTR_CFG_DAMPING_PROGRESSION_U8:             cfg->dampingProgression_u8                  = (uint8_t)v;  break;
        case DAP_ATTR_CFG_COULOMB_FRICTION_IN_0P1N_U8:        cfg->coulombFrictionIn0p1N_u8               = (uint8_t)v;  break;
        default: break;
    }
}

// ---------------------------------------------------------------------------
// DapAttr_ReadFromConfig
// Fills pkt->value_i64 from the corresponding PayloadPedalConfig_t field.
// ---------------------------------------------------------------------------
void DapAttr_ReadFromConfig(const PayloadPedalConfig_t* cfg, DapAttrPacket_t* pkt)
{
    if (!cfg || !pkt) return;
    switch ((DapConfigAttrId_e)pkt->attrId_u16) {
        case DAP_ATTR_CFG_PEDAL_START_POS_U8:                  pkt->value_i64 = cfg->pedalStartPosition_u8;                  break;
        case DAP_ATTR_CFG_PEDAL_END_POS_U8:                    pkt->value_i64 = cfg->pedalEndPosition_u8;                    break;
        case DAP_ATTR_CFG_MAX_FORCE_FL32:                      pkt->value_i64 = floatToI64(cfg->maxForce_fl32);               break;
        case DAP_ATTR_CFG_PRELOAD_FORCE_FL32:                  pkt->value_i64 = floatToI64(cfg->preloadForce_fl32);           break;
        case DAP_ATTR_CFG_QUANTITY_OF_CONTROL_U8:              pkt->value_i64 = cfg->quantityOfControl_u8;                   break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_00_U8:               pkt->value_i64 = cfg->relativeForce00_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_01_U8:               pkt->value_i64 = cfg->relativeForce01_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_02_U8:               pkt->value_i64 = cfg->relativeForce02_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_03_U8:               pkt->value_i64 = cfg->relativeForce03_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_04_U8:               pkt->value_i64 = cfg->relativeForce04_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_05_U8:               pkt->value_i64 = cfg->relativeForce05_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_06_U8:               pkt->value_i64 = cfg->relativeForce06_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_07_U8:               pkt->value_i64 = cfg->relativeForce07_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_08_U8:               pkt->value_i64 = cfg->relativeForce08_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_09_U8:               pkt->value_i64 = cfg->relativeForce09_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_FORCE_10_U8:               pkt->value_i64 = cfg->relativeForce10_u8;                     break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_00_U8:              pkt->value_i64 = cfg->relativeTravel00_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_01_U8:              pkt->value_i64 = cfg->relativeTravel01_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_02_U8:              pkt->value_i64 = cfg->relativeTravel02_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_03_U8:              pkt->value_i64 = cfg->relativeTravel03_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_04_U8:              pkt->value_i64 = cfg->relativeTravel04_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_05_U8:              pkt->value_i64 = cfg->relativeTravel05_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_06_U8:              pkt->value_i64 = cfg->relativeTravel06_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_07_U8:              pkt->value_i64 = cfg->relativeTravel07_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_08_U8:              pkt->value_i64 = cfg->relativeTravel08_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_09_U8:              pkt->value_i64 = cfg->relativeTravel09_u8;                    break;
        case DAP_ATTR_CFG_RELATIVE_TRAVEL_10_U8:              pkt->value_i64 = cfg->relativeTravel10_u8;                    break;
        case DAP_ATTR_CFG_NUM_OF_JOYSTICK_MAP_CONTROL_U8:     pkt->value_i64 = cfg->numOfJoystickMapControl_u8;             break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_00_U8:            pkt->value_i64 = cfg->joystickMapOrig00_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_01_U8:            pkt->value_i64 = cfg->joystickMapOrig01_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_02_U8:            pkt->value_i64 = cfg->joystickMapOrig02_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_03_U8:            pkt->value_i64 = cfg->joystickMapOrig03_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_04_U8:            pkt->value_i64 = cfg->joystickMapOrig04_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_05_U8:            pkt->value_i64 = cfg->joystickMapOrig05_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_06_U8:            pkt->value_i64 = cfg->joystickMapOrig06_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_07_U8:            pkt->value_i64 = cfg->joystickMapOrig07_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_08_U8:            pkt->value_i64 = cfg->joystickMapOrig08_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_09_U8:            pkt->value_i64 = cfg->joystickMapOrig09_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_10_U8:            pkt->value_i64 = cfg->joystickMapOrig10_u8;                   break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_00_U8:          pkt->value_i64 = cfg->joystickMapMapped00_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_01_U8:          pkt->value_i64 = cfg->joystickMapMapped01_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_02_U8:          pkt->value_i64 = cfg->joystickMapMapped02_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_03_U8:          pkt->value_i64 = cfg->joystickMapMapped03_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_04_U8:          pkt->value_i64 = cfg->joystickMapMapped04_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_05_U8:          pkt->value_i64 = cfg->joystickMapMapped05_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_06_U8:          pkt->value_i64 = cfg->joystickMapMapped06_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_07_U8:          pkt->value_i64 = cfg->joystickMapMapped07_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_08_U8:          pkt->value_i64 = cfg->joystickMapMapped08_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_09_U8:          pkt->value_i64 = cfg->joystickMapMapped09_u8;                 break;
        case DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_10_U8:          pkt->value_i64 = cfg->joystickMapMapped10_u8;                 break;
        case DAP_ATTR_CFG_ABS_FREQUENCY_U8:                   pkt->value_i64 = cfg->absFrequency_u8;                        break;
        case DAP_ATTR_CFG_ABS_AMPLITUDE_U8:                   pkt->value_i64 = cfg->absAmplitude_u8;                        break;
        case DAP_ATTR_CFG_ABS_PATTERN_U8:                     pkt->value_i64 = cfg->absPattern_u8;                          break;
        case DAP_ATTR_CFG_ABS_FORCE_OR_TRAVEL_BIT_U8:         pkt->value_i64 = cfg->absForceOrTarvelBit_u8;                 break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_A_I16:                 pkt->value_i64 = cfg->lengthPedalA_i16;                       break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_B_I16:                 pkt->value_i64 = cfg->lengthPedalB_i16;                       break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_D_I16:                 pkt->value_i64 = cfg->lengthPedalD_i16;                       break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_C_HORIZONTAL_I16:      pkt->value_i64 = cfg->lengthPedalCHorizontal_i16;             break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_C_VERTICAL_I16:        pkt->value_i64 = cfg->lengthPedalCVertical_i16;               break;
        case DAP_ATTR_CFG_LENGTH_PEDAL_TRAVEL_I16:            pkt->value_i64 = cfg->lengthPedalTravel_i16;                  break;
        case DAP_ATTR_CFG_SIMULATE_ABS_TRIGGER_U8:            pkt->value_i64 = cfg->simulateAbsTrigger_u8;                  break;
        case DAP_ATTR_CFG_SIMULATE_ABS_VALUE_U8:              pkt->value_i64 = cfg->simulateAbsValue_u8;                    break;
        case DAP_ATTR_CFG_RPM_MAX_FREQ_U8:                    pkt->value_i64 = cfg->rpmMaxFreq_u8;                          break;
        case DAP_ATTR_CFG_RPM_MIN_FREQ_U8:                    pkt->value_i64 = cfg->rpmMinFreq_u8;                          break;
        case DAP_ATTR_CFG_RPM_AMP_U8:                         pkt->value_i64 = cfg->rpmAmp_u8;                              break;
        case DAP_ATTR_CFG_BP_TRIGGER_VALUE_U8:                pkt->value_i64 = cfg->bpTriggerValue_u8;                      break;
        case DAP_ATTR_CFG_BP_AMP_U8:                          pkt->value_i64 = cfg->bpAmp_u8;                               break;
        case DAP_ATTR_CFG_BP_FREQ_U8:                         pkt->value_i64 = cfg->bpFreq_u8;                              break;
        case DAP_ATTR_CFG_BP_TRIGGER_U8:                      pkt->value_i64 = cfg->bpTrigger_u8;                           break;
        case DAP_ATTR_CFG_G_MULTI_U8:                         pkt->value_i64 = cfg->gMulti_u8;                              break;
        case DAP_ATTR_CFG_G_WINDOW_U8:                        pkt->value_i64 = cfg->gWindow_u8;                             break;
        case DAP_ATTR_CFG_WS_AMP_U8:                          pkt->value_i64 = cfg->wsAmp_u8;                               break;
        case DAP_ATTR_CFG_WS_FREQ_U8:                         pkt->value_i64 = cfg->wsFreq_u8;                              break;
        case DAP_ATTR_CFG_ROAD_MULTI_U8:                      pkt->value_i64 = cfg->roadMulti_u8;                           break;
        case DAP_ATTR_CFG_ROAD_WINDOW_U8:                     pkt->value_i64 = cfg->roadWindow_u8;                          break;
        case DAP_ATTR_CFG_CV_AMP1_U8:                         pkt->value_i64 = cfg->cvAmp1_u8;                              break;
        case DAP_ATTR_CFG_CV_FREQ1_U8:                        pkt->value_i64 = cfg->cvFreq1_u8;                             break;
        case DAP_ATTR_CFG_CV_AMP2_U8:                         pkt->value_i64 = cfg->cvAmp2_u8;                              break;
        case DAP_ATTR_CFG_CV_FREQ2_U8:                        pkt->value_i64 = cfg->cvFreq2_u8;                             break;
        case DAP_ATTR_CFG_CV_AMP3_U8:                         pkt->value_i64 = cfg->cvAmp3_u8;                              break;
        case DAP_ATTR_CFG_CV_FREQ3_U8:                        pkt->value_i64 = cfg->cvFreq3_u8;                             break;
        case DAP_ATTR_CFG_CV_AMP4_U8:                         pkt->value_i64 = cfg->cvAmp4_u8;                              break;
        case DAP_ATTR_CFG_CV_FREQ4_U8:                        pkt->value_i64 = cfg->cvFreq4_u8;                             break;
        case DAP_ATTR_CFG_MAX_GAME_OUTPUT_U8:                 pkt->value_i64 = cfg->maxGameOutput_u8;                       break;
        case DAP_ATTR_CFG_KF_MODEL_NOISE_U8:                  pkt->value_i64 = cfg->kfModelNoise_u8;                        break;
        case DAP_ATTR_CFG_KF_MODEL_ORDER_U8:                  pkt->value_i64 = cfg->kfModelOrder_u8;                        break;
        case DAP_ATTR_CFG_DEBUG_FLAGS0_U8:                    pkt->value_i64 = cfg->debugFlags0_u8;                         break;
        case DAP_ATTR_CFG_LOADCELL_RATING_U8:                 pkt->value_i64 = cfg->loadcellRating_u8;                      break;
        case DAP_ATTR_CFG_TRAVEL_AS_JOYSTICK_OUTPUT_U8:       pkt->value_i64 = cfg->travelAsJoystickOutput_u8;              break;
        case DAP_ATTR_CFG_INVERT_LOADCELL_READING_U8:         pkt->value_i64 = cfg->invertLoadcellReading_u8;               break;
        case DAP_ATTR_CFG_INVERT_MOTOR_DIRECTION_U8:          pkt->value_i64 = cfg->invertMotorDirection_u8;                break;
        case DAP_ATTR_CFG_SPINDLE_PITCH_MM_PER_REV_U8:        pkt->value_i64 = cfg->spindlePitch_mmPerRev_u8;               break;
        case DAP_ATTR_CFG_PEDAL_TYPE_U8:                      pkt->value_i64 = cfg->pedalType_u8;                           break;
        case DAP_ATTR_CFG_STEP_LOSS_FUNCTION_FLAGS_U8:        pkt->value_i64 = cfg->stepLossFunctionFlags_u8;               break;
        case DAP_ATTR_CFG_KF_JOYSTICK_U8:                     pkt->value_i64 = cfg->kfJoystick_u8;                          break;
        case DAP_ATTR_CFG_KF_MODEL_NOISE_JOYSTICK_U8:         pkt->value_i64 = cfg->kfModelNoiseJoystick_u8;                break;
        case DAP_ATTR_CFG_SERVO_IDLE_TIMEOUT_U8:              pkt->value_i64 = cfg->servoIdleTimeout_u8;                    break;
        case DAP_ATTR_CFG_MIN_FORCE_FOR_EFFECTS_U8:           pkt->value_i64 = cfg->minForceForEffects_u8;                  break;
        case DAP_ATTR_CFG_CONFIG_HASH_U32:                    pkt->value_i64 = cfg->configHash_u32;                         break;
        case DAP_ATTR_CFG_ENDSTOP_DETECTION_THRESHOLD_U8:     pkt->value_i64 = cfg->endstopDetectionThreshold_u8;           break;
        case DAP_ATTR_CFG_VIRTUAL_PEDAL_MASS_IN_PERCENT_U8:   pkt->value_i64 = cfg->virtualPedalMassInPercent_u8;           break;
        case DAP_ATTR_CFG_VIRTUAL_PEDAL_DAMPING_IN_PERCENT_U8:pkt->value_i64 = cfg->virtualPedalDampingInPercent_u8;        break;
        case DAP_ATTR_CFG_ENDSTOP_STIFFNESS_KG_MM_U8:        pkt->value_i64 = cfg->endstopStiffness_kg_mm_u8;              break;
        case DAP_ATTR_CFG_ENDSTOP_TRAVEL_RANGE_MM_U8:         pkt->value_i64 = cfg->endstopTravelRange_mm_u8;               break;
        case DAP_ATTR_CFG_DAMPING_PROGRESSION_U8:             pkt->value_i64 = cfg->dampingProgression_u8;                  break;
        case DAP_ATTR_CFG_COULOMB_FRICTION_IN_0P1N_U8:        pkt->value_i64 = cfg->coulombFrictionIn0p1N_u8;               break;
        default: pkt->value_i64 = 0; break;
    }
}

// ---------------------------------------------------------------------------
// DapAttr_ApplyToAction
// Writes one attribute value into PayloadPedalAction_t.
// ---------------------------------------------------------------------------
void DapAttr_ApplyToAction(DapActions_t* act, const DapAttrPacket_t* pkt)
{
    if (!act || !pkt) return;
    int64_t v = pkt->value_i64;
    switch ((DapActionAttrId_e)pkt->attrId_u16) {
        case DAP_ATTR_ACT_SYSTEM_ACTION_U8:               act->payloadPedalAction_st.systemAction_u8                = (uint8_t)v; break;
        case DAP_ATTR_ACT_TRIGGER_ABS_U8:                 act->payloadPedalAction_st.triggerAbs_u8                  = (uint8_t)v; break;
        case DAP_ATTR_ACT_RPM_U8:                         act->payloadPedalAction_st.rpm_u8                         = (uint8_t)v; break;
        case DAP_ATTR_ACT_G_VALUE_U8:                     act->payloadPedalAction_st.gValue_u8                      = (uint8_t)v; break;
        case DAP_ATTR_ACT_WHEEL_SLIP_U8:                  act->payloadPedalAction_st.wheelSlip_u8                   = (uint8_t)v; break;
        case DAP_ATTR_ACT_IMPACT_VALUE_U8:                act->payloadPedalAction_st.impactValue_u8                 = (uint8_t)v; break;
        case DAP_ATTR_ACT_START_SYSTEM_IDENTIFICATION_U8: act->payloadPedalAction_st.startSystemIdentification_u8   = (uint8_t)v; break;
        case DAP_ATTR_ACT_TRIGGER_CV1_U8:                 act->payloadPedalAction_st.triggerCv1_u8                  = (uint8_t)v; break;
        case DAP_ATTR_ACT_TRIGGER_CV2_U8:                 act->payloadPedalAction_st.triggerCv2_u8                  = (uint8_t)v; break;
        case DAP_ATTR_ACT_TRIGGER_CV3_U8:                 act->payloadPedalAction_st.triggerCv3_u8                  = (uint8_t)v; break;
        case DAP_ATTR_ACT_TRIGGER_CV4_U8:                 act->payloadPedalAction_st.triggerCv4_u8                  = (uint8_t)v; break;
        case DAP_ATTR_ACT_RETURN_PEDAL_CONFIG_U8:         act->payloadPedalAction_st.returnPedalConfig_u8           = (uint8_t)v; break;
        case DAP_ATTR_ACT_RUDDER_ACTION_U8:               act->payloadPedalAction_st.rudderAction_u8                = (uint8_t)v; break;
        case DAP_ATTR_ACT_RUDDER_BRAKE_ACTION_U8:         act->payloadPedalAction_st.rudderBrakeAction_u8           = (uint8_t)v; break;
        default: break;
    }
}
