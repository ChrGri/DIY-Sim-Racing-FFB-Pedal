#pragma once
#include "Arduino.h"

// Effect pipeline: offset samples computed on the PC (plugin scripts) and played
// back by the pedal on its own clock. One packet carries one channel. The plugin
// sends overlapping sample sets; a newer set overwrites the samples of an older
// one with the same index. The whole packet fits into a single 64 byte HID report.
#define EFFECT_PIPELINE_CHANNELS_U8 2U
#define EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8 21U
#define EFFECT_PIPELINE_SAMPLE_PERIOD_US_U32 2000UL // 500 Hz plugin sample clock

// offsetType_u8
#define EFFECT_PIPELINE_OFFSET_OFF_U8 0U
#define EFFECT_PIPELINE_OFFSET_FORCE_U8 1U          // 0.01 N per LSB, + = pedal pushes back harder
#define EFFECT_PIPELINE_OFFSET_ANGLE_U8 2U          // 0.001 deg pedal angle per LSB, + = towards the driver
#define EFFECT_PIPELINE_OFFSET_FORCE_RELATIVE_U8 3U // 0.0001 per LSB of the measured pedal force, + = heavier

typedef struct __attribute__((packed)) PayloadEffectPipeline
{
  uint32_t startSampleIndex_u32; // plugin sample clock index of samples_ai16[0]
  uint8_t channel_u8;            // 0 .. EFFECT_PIPELINE_CHANNELS_U8 - 1
  uint8_t offsetType_u8;
  uint8_t sampleCount_u8;        // valid samples, <= EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8
  int16_t samples_ai16[EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8];
} PayloadEffectPipeline_t;
