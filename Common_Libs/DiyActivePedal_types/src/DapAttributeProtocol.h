#pragma once
// ---------------------------------------------------------------------------
// DapAttributeProtocol.h
//
// Attribute-based HID protocol for PC→ESP32 configuration updates.
// Inspired by OpenFFBoard field-by-field addressing.
//
// HID OUT report format (64 bytes):
//   buf[0]    = DAP_ATTR_HID_MARKER (0xA1) — identifies attribute packet
//   buf[1..12]= DapAttrPacket_t     (12 bytes, see below)
//   buf[13..63]= reserved / zero
//
// HID INPUT reply format (64 bytes, same layout):
//   buf[0]    = DAP_ATTR_HID_MARKER (0xA1)
//   buf[1..12]= DapAttrPacket_t with cmdType = DAP_ATTR_CMD_ACK or _ERR
//
// This header is shared between the ESP32 firmware and (via a C# mirror)
// the SimHub plugin.
// ---------------------------------------------------------------------------

#ifdef __cplusplus
#include <stdint.h>

// ---------------------------------------------------------------------------
// HID marker byte – distinguishes attribute packets from legacy chunked structs
// ---------------------------------------------------------------------------
#define DAP_ATTR_HID_MARKER  0xA1u
#define DAP_ATTR_PACKET_BYTES 12u   // size of DapAttrPacket_t on the wire

// ---------------------------------------------------------------------------
// Command type (cmdType_u8)
// ---------------------------------------------------------------------------
typedef enum : uint8_t
{
    DAP_ATTR_CMD_WRITE = 0x00u,  // PC→ESP: write one attribute
    DAP_ATTR_CMD_READ  = 0x01u,  // PC→ESP: request one attribute value
    DAP_ATTR_CMD_ACK   = 0x0Au,  // ESP→PC: acknowledgement for a WRITE/READ
    DAP_ATTR_CMD_ERR   = 0x07u,  // ESP→PC: error (unknown attrId etc.)
} DapAttrCmdType_e;

// ---------------------------------------------------------------------------
// Class ID (classId_u8)
// ---------------------------------------------------------------------------
typedef enum : uint8_t
{
    DAP_ATTR_CLASS_CONFIG       = 0x00u,  // targets PayloadPedalConfig_t fields
    DAP_ATTR_CLASS_ACTION       = 0x01u,  // targets PayloadPedalAction_t fields
    DAP_ATTR_CLASS_SERVO_MODBUS = 0x02u,  // direct iSV57 register access
        // attrId_u16 = absolute Modbus holding-register address
        //              (already resolved with pr_X_00 offsets on the PC side)
        // value_i64  = register value (int16 range) for WRITE, ignored for READ
} DapAttrClassId_e;

// ---------------------------------------------------------------------------
// Attribute IDs for DAP_ATTR_CLASS_CONFIG
// Each enum value maps one-to-one to a field in PayloadPedalConfig_t.
// ---------------------------------------------------------------------------
typedef enum : uint16_t
{
    // Pedal travel limits
    DAP_ATTR_CFG_PEDAL_START_POS_U8                  = 0x0000u,
    DAP_ATTR_CFG_PEDAL_END_POS_U8                    = 0x0001u,

    // Forces
    DAP_ATTR_CFG_MAX_FORCE_FL32                      = 0x0002u,
    DAP_ATTR_CFG_PRELOAD_FORCE_FL32                  = 0x0003u,

    // Force-vs-travel curve (11 control points each)
    DAP_ATTR_CFG_QUANTITY_OF_CONTROL_U8              = 0x0010u,
    DAP_ATTR_CFG_RELATIVE_FORCE_00_U8                = 0x0011u,
    DAP_ATTR_CFG_RELATIVE_FORCE_01_U8                = 0x0012u,
    DAP_ATTR_CFG_RELATIVE_FORCE_02_U8                = 0x0013u,
    DAP_ATTR_CFG_RELATIVE_FORCE_03_U8                = 0x0014u,
    DAP_ATTR_CFG_RELATIVE_FORCE_04_U8                = 0x0015u,
    DAP_ATTR_CFG_RELATIVE_FORCE_05_U8                = 0x0016u,
    DAP_ATTR_CFG_RELATIVE_FORCE_06_U8                = 0x0017u,
    DAP_ATTR_CFG_RELATIVE_FORCE_07_U8                = 0x0018u,
    DAP_ATTR_CFG_RELATIVE_FORCE_08_U8                = 0x0019u,
    DAP_ATTR_CFG_RELATIVE_FORCE_09_U8                = 0x001Au,
    DAP_ATTR_CFG_RELATIVE_FORCE_10_U8                = 0x001Bu,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_00_U8               = 0x001Cu,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_01_U8               = 0x001Du,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_02_U8               = 0x001Eu,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_03_U8               = 0x001Fu,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_04_U8               = 0x0020u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_05_U8               = 0x0021u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_06_U8               = 0x0022u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_07_U8               = 0x0023u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_08_U8               = 0x0024u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_09_U8               = 0x0025u,
    DAP_ATTR_CFG_RELATIVE_TRAVEL_10_U8               = 0x0026u,

    // Joystick mapping (11 orig + 11 mapped)
    DAP_ATTR_CFG_NUM_OF_JOYSTICK_MAP_CONTROL_U8      = 0x0030u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_00_U8             = 0x0031u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_01_U8             = 0x0032u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_02_U8             = 0x0033u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_03_U8             = 0x0034u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_04_U8             = 0x0035u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_05_U8             = 0x0036u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_06_U8             = 0x0037u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_07_U8             = 0x0038u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_08_U8             = 0x0039u,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_09_U8             = 0x003Au,
    DAP_ATTR_CFG_JOYSTICK_MAP_ORIG_10_U8             = 0x003Bu,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_00_U8           = 0x003Cu,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_01_U8           = 0x003Du,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_02_U8           = 0x003Eu,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_03_U8           = 0x003Fu,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_04_U8           = 0x0040u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_05_U8           = 0x0041u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_06_U8           = 0x0042u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_07_U8           = 0x0043u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_08_U8           = 0x0044u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_09_U8           = 0x0045u,
    DAP_ATTR_CFG_JOYSTICK_MAP_MAPPED_10_U8           = 0x0046u,

    // ABS effect
    DAP_ATTR_CFG_ABS_FREQUENCY_U8                    = 0x0050u,
    DAP_ATTR_CFG_ABS_AMPLITUDE_U8                    = 0x0051u,
    DAP_ATTR_CFG_ABS_PATTERN_U8                      = 0x0052u,
    DAP_ATTR_CFG_ABS_FORCE_OR_TRAVEL_BIT_U8          = 0x0053u,

    // Pedal geometry (mm, int16)
    DAP_ATTR_CFG_LENGTH_PEDAL_A_I16                  = 0x0060u,
    DAP_ATTR_CFG_LENGTH_PEDAL_B_I16                  = 0x0061u,
    DAP_ATTR_CFG_LENGTH_PEDAL_D_I16                  = 0x0062u,
    DAP_ATTR_CFG_LENGTH_PEDAL_C_HORIZONTAL_I16       = 0x0063u,
    DAP_ATTR_CFG_LENGTH_PEDAL_C_VERTICAL_I16         = 0x0064u,
    DAP_ATTR_CFG_LENGTH_PEDAL_TRAVEL_I16             = 0x0065u,

    // ABS simulation
    DAP_ATTR_CFG_SIMULATE_ABS_TRIGGER_U8             = 0x0070u,
    DAP_ATTR_CFG_SIMULATE_ABS_VALUE_U8               = 0x0071u,

    // RPM effect
    DAP_ATTR_CFG_RPM_MAX_FREQ_U8                     = 0x0080u,
    DAP_ATTR_CFG_RPM_MIN_FREQ_U8                     = 0x0081u,
    DAP_ATTR_CFG_RPM_AMP_U8                          = 0x0082u,

    // Bite point effect
    DAP_ATTR_CFG_BP_TRIGGER_VALUE_U8                 = 0x0090u,
    DAP_ATTR_CFG_BP_AMP_U8                           = 0x0091u,
    DAP_ATTR_CFG_BP_FREQ_U8                          = 0x0092u,
    DAP_ATTR_CFG_BP_TRIGGER_U8                       = 0x0093u,

    // G-force effect
    DAP_ATTR_CFG_G_MULTI_U8                          = 0x00A0u,
    DAP_ATTR_CFG_G_WINDOW_U8                         = 0x00A1u,

    // Wheel slip effect
    DAP_ATTR_CFG_WS_AMP_U8                           = 0x00B0u,
    DAP_ATTR_CFG_WS_FREQ_U8                          = 0x00B1u,

    // Road impact effect
    DAP_ATTR_CFG_ROAD_MULTI_U8                       = 0x00C0u,
    DAP_ATTR_CFG_ROAD_WINDOW_U8                      = 0x00C1u,

    // Custom vibrations 1-4
    DAP_ATTR_CFG_CV_AMP1_U8                          = 0x00D0u,
    DAP_ATTR_CFG_CV_FREQ1_U8                         = 0x00D1u,
    DAP_ATTR_CFG_CV_AMP2_U8                          = 0x00D2u,
    DAP_ATTR_CFG_CV_FREQ2_U8                         = 0x00D3u,
    DAP_ATTR_CFG_CV_AMP3_U8                          = 0x00D4u,
    DAP_ATTR_CFG_CV_FREQ3_U8                         = 0x00D5u,
    DAP_ATTR_CFG_CV_AMP4_U8                          = 0x00D6u,
    DAP_ATTR_CFG_CV_FREQ4_U8                         = 0x00D7u,

    // Controller / output
    DAP_ATTR_CFG_MAX_GAME_OUTPUT_U8                  = 0x00E0u,

    // Kalman filter
    DAP_ATTR_CFG_KF_MODEL_NOISE_U8                   = 0x00F0u,
    DAP_ATTR_CFG_KF_MODEL_ORDER_U8                   = 0x00F1u,

    // Debug & hardware config
    DAP_ATTR_CFG_DEBUG_FLAGS0_U8                     = 0x0100u,
    DAP_ATTR_CFG_LOADCELL_RATING_U8                  = 0x0101u,
    DAP_ATTR_CFG_TRAVEL_AS_JOYSTICK_OUTPUT_U8        = 0x0102u,
    DAP_ATTR_CFG_INVERT_LOADCELL_READING_U8          = 0x0103u,
    DAP_ATTR_CFG_INVERT_MOTOR_DIRECTION_U8           = 0x0104u,
    DAP_ATTR_CFG_SPINDLE_PITCH_MM_PER_REV_U8         = 0x0105u,
    DAP_ATTR_CFG_PEDAL_TYPE_U8                       = 0x0106u,
    DAP_ATTR_CFG_STEP_LOSS_FUNCTION_FLAGS_U8         = 0x0107u,
    DAP_ATTR_CFG_KF_JOYSTICK_U8                      = 0x0108u,
    DAP_ATTR_CFG_KF_MODEL_NOISE_JOYSTICK_U8          = 0x0109u,
    DAP_ATTR_CFG_SERVO_IDLE_TIMEOUT_U8               = 0x010Au,
    DAP_ATTR_CFG_MIN_FORCE_FOR_EFFECTS_U8            = 0x010Bu,
    DAP_ATTR_CFG_CONFIG_HASH_U32                     = 0x010Cu,
    DAP_ATTR_CFG_ENDSTOP_DETECTION_THRESHOLD_U8      = 0x010Du,

    // Virtual pedal dynamics
    DAP_ATTR_CFG_VIRTUAL_PEDAL_MASS_IN_PERCENT_U8    = 0x0110u,
    DAP_ATTR_CFG_VIRTUAL_PEDAL_DAMPING_IN_PERCENT_U8 = 0x0111u,

    // Endstop parameters
    DAP_ATTR_CFG_ENDSTOP_STIFFNESS_KG_MM_U8          = 0x0120u,
    DAP_ATTR_CFG_ENDSTOP_TRAVEL_RANGE_MM_U8          = 0x0121u,

    // Elastomere / spring behavior
    DAP_ATTR_CFG_DAMPING_PROGRESSION_U8              = 0x0130u,

    // Friction
    DAP_ATTR_CFG_COULOMB_FRICTION_IN_0P1N_U8         = 0x0140u,

    // Special control attributes
    DAP_ATTR_CFG_STORE_TO_EEPROM                     = 0xFFFEu, // set storeToEeprom flag before commit
    DAP_ATTR_CFG_COMMIT                              = 0xFFFFu, // push shadow → s_configUpdateAvailableQueue
} DapConfigAttrId_e;

// ---------------------------------------------------------------------------
// Attribute IDs for DAP_ATTR_CLASS_ACTION
// Each enum value maps one-to-one to a field in the PayloadPedalAction struct.
// ---------------------------------------------------------------------------
typedef enum : uint16_t
{
    DAP_ATTR_ACT_SYSTEM_ACTION_U8                = 0x0000u,
    DAP_ATTR_ACT_TRIGGER_ABS_U8                  = 0x0001u,
    DAP_ATTR_ACT_RPM_U8                          = 0x0002u,
    DAP_ATTR_ACT_G_VALUE_U8                      = 0x0003u,
    DAP_ATTR_ACT_WHEEL_SLIP_U8                   = 0x0004u,
    DAP_ATTR_ACT_IMPACT_VALUE_U8                 = 0x0005u,
    DAP_ATTR_ACT_START_SYSTEM_IDENTIFICATION_U8  = 0x0006u,
    DAP_ATTR_ACT_TRIGGER_CV1_U8                  = 0x0007u,
    DAP_ATTR_ACT_TRIGGER_CV2_U8                  = 0x0008u,
    DAP_ATTR_ACT_TRIGGER_CV3_U8                  = 0x0009u,
    DAP_ATTR_ACT_TRIGGER_CV4_U8                  = 0x000Au,
    DAP_ATTR_ACT_RETURN_PEDAL_CONFIG_U8          = 0x000Bu,
    DAP_ATTR_ACT_RUDDER_ACTION_U8                = 0x000Cu,
    DAP_ATTR_ACT_RUDDER_BRAKE_ACTION_U8          = 0x000Du,
    // Execute all accumulated action fields immediately (like COMMIT for config)
    DAP_ATTR_ACT_EXECUTE                         = 0xFFFFu,
} DapActionAttrId_e;

// ---------------------------------------------------------------------------
// Wire packet (12 bytes)
// ---------------------------------------------------------------------------
typedef struct __attribute__((packed)) DapAttrPacket
{
    uint8_t  cmdType_u8;   // DapAttrCmdType_e
    uint8_t  classId_u8;   // DapAttrClassId_e
    uint16_t attrId_u16;   // DapConfigAttrId_e or DapActionAttrId_e
    int64_t  value_i64;    // payload; float fields: bit-exact via memcpy
} DapAttrPacket_t;

static_assert(sizeof(DapAttrPacket_t) == DAP_ATTR_PACKET_BYTES,
              "DapAttrPacket_t size mismatch");

#endif // __cplusplus
