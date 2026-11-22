#pragma once

// Effect pipelines: offset samples computed on the PC (plugin effect scripts)
// and played back on the pedal.
//
// The plugin numbers its samples with its own 500 Hz sample clock and sends
// overlapping sets of EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8 samples per channel
// (~42 ms) with every telemetry update (~60 Hz). The pedal does not need a
// synchronized clock: it estimates the offset between the plugin sample clock
// and its own timer from the packet arrivals (the earliest arrival, i.e. the
// lowest transport latency, wins) and plays the samples back with a fixed
// playout delay, interpolating linearly between them. Jitter and single lost
// packets are absorbed by the overlap, a missing sample fades the channel out.

#include <Arduino.h>
#include "DiyActivePedal_types.h"

// playback runs this far behind the newest sample time estimate
#define EFFECT_PIPELINE_PLAYOUT_DELAY_US_I64 ((int64_t)25000)
// the clock offset estimate relaxes by this much per second, so a drifting
// clock or a permanently higher latency is followed again
#define EFFECT_PIPELINE_OFFSET_RELAX_US_PER_S_F32 200.0f
// re-sync the clock offset after this long without packets
#define EFFECT_PIPELINE_RESYNC_AFTER_US_I64 ((int64_t)500000)
// fade-out time constant when samples are missing
#define EFFECT_PIPELINE_FADE_TAU_S_F32 0.03f
#define EFFECT_PIPELINE_RING_SIZE_U32 64UL // power of two, ~128 ms at 500 Hz

// Safety limits of the summed pipeline offsets (the plugin limits as well)
#define EFFECT_PIPELINE_MAX_FORCE_N_F32 60.0f
#define EFFECT_PIPELINE_MAX_ANGLE_DEG_F32 2.0f
// the angle offset is converted to sled travel; this is the hard limit of the result
#define EFFECT_PIPELINE_MAX_SLED_MM_F32 5.0f
#define EFFECT_PIPELINE_MAX_FORCE_RELATIVE_F32 0.8f

class EffectPipeline_t {
public:
  // true if a channel carries angle offsets (the geometry is only computed then)
  bool hasAngleChannel() const {
    for (uint8_t c = 0; c < EFFECT_PIPELINE_CHANNELS_U8; c++) {
      if (channels_[c].offsetType_u8 == EFFECT_PIPELINE_OFFSET_ANGLE_U8) return true;
    }
    return false;
  }

  // Called from the communication tasks (serial / ESP-NOW RX).
  void receive(const PayloadEffectPipeline_t &pkt) {
    if (pkt.channel_u8 >= EFFECT_PIPELINE_CHANNELS_U8) return;
    uint8_t count = pkt.sampleCount_u8;
    if (count > EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8)
      count = EFFECT_PIPELINE_SAMPLES_PER_PACKET_U8;

    int64_t nowUs = esp_timer_get_time();
    int64_t packetStartUs =
        (int64_t)pkt.startSampleIndex_u32 * (int64_t)EFFECT_PIPELINE_SAMPLE_PERIOD_US_U32;
    // plugin time minus local time; a higher value means a lower transport latency
    int64_t offsetCandidateUs = packetStartUs - nowUs;

    portENTER_CRITICAL(&mux_);
    if (!clockValid_b_ || (nowUs - lastPacketUs_) > EFFECT_PIPELINE_RESYNC_AFTER_US_I64 ||
        offsetCandidateUs > clockOffsetUs_) {
      clockOffsetUs_ = offsetCandidateUs;
      clockValid_b_ = true;
    }
    lastPacketUs_ = nowUs;

    Channel_t &ch = channels_[pkt.channel_u8];
    ch.offsetType_u8 = pkt.offsetType_u8;
    for (uint8_t i = 0; i < count; i++) {
      uint32_t idx = pkt.startSampleIndex_u32 + i;
      uint32_t slot = idx & (EFFECT_PIPELINE_RING_SIZE_U32 - 1UL);
      ch.sampleIndex_au32[slot] = idx;
      ch.samples_ai16[slot] = pkt.samples_ai16[i];
      ch.slotValid_ab[slot] = true;
    }
    portEXIT_CRITICAL(&mux_);
  }

  // Called from the pedal task. Adds the pipeline offsets to the effect sums:
  // effectForce_kg (same sign as the foot force: + makes the pedal lighter) and
  // effectPos_steps (+ = deeper into the travel, like the built-in effects).
  // sledMmPerDeg: local sled travel per degree pedal angle at the current position
  // (from the pedal geometry), converts the angle offsets into sled travel.
  void addOffsets(float measuredForce_kg, float stepsPerMm, float sledMmPerDeg,
                  float &effectForce_kg, float &effectPos_steps) {
    int64_t nowUs = esp_timer_get_time();
    float dt_s = (lastUpdateUs_ > 0) ? (float)(nowUs - lastUpdateUs_) * 1e-6f : 0.0f;
    lastUpdateUs_ = nowUs;
    if (dt_s > 0.1f) dt_s = 0.1f;

    float force_N = 0.0f;
    float angle_deg = 0.0f;
    float forceRelative = 0.0f;

    portENTER_CRITICAL(&mux_);
    bool clockValid_b = clockValid_b_;
    // follow clock drift / latency changes slowly
    if (clockValid_b && dt_s > 0.0f) {
      clockOffsetRelax_f32_ += EFFECT_PIPELINE_OFFSET_RELAX_US_PER_S_F32 * dt_s;
      int64_t relaxUs = (int64_t)clockOffsetRelax_f32_;
      clockOffsetUs_ -= relaxUs;
      clockOffsetRelax_f32_ -= (float)relaxUs;
    }
    int64_t playUs = nowUs + clockOffsetUs_ - EFFECT_PIPELINE_PLAYOUT_DELAY_US_I64;

    for (uint8_t c = 0; c < EFFECT_PIPELINE_CHANNELS_U8; c++) {
      Channel_t &ch = channels_[c];
      float value = 0.0f;
      bool valid_b = false;
      if (clockValid_b && playUs >= 0) {
        uint32_t idx0 = (uint32_t)(playUs / (int64_t)EFFECT_PIPELINE_SAMPLE_PERIOD_US_U32);
        float frac = (float)(playUs % (int64_t)EFFECT_PIPELINE_SAMPLE_PERIOD_US_U32) /
                     (float)EFFECT_PIPELINE_SAMPLE_PERIOD_US_U32;
        uint32_t s0 = idx0 & (EFFECT_PIPELINE_RING_SIZE_U32 - 1UL);
        uint32_t s1 = (idx0 + 1UL) & (EFFECT_PIPELINE_RING_SIZE_U32 - 1UL);
        bool has0 = ch.slotValid_ab[s0] && ch.sampleIndex_au32[s0] == idx0;
        bool has1 = ch.slotValid_ab[s1] && ch.sampleIndex_au32[s1] == idx0 + 1UL;
        if (has0 && has1) {
          value = (float)ch.samples_ai16[s0] +
                  frac * (float)(ch.samples_ai16[s1] - ch.samples_ai16[s0]);
          valid_b = true;
        } else if (has0) {
          value = (float)ch.samples_ai16[s0];
          valid_b = true;
        }
      }
      uint8_t type = ch.offsetType_u8;

      // missing data: fade the last value out instead of freezing it
      if (valid_b) {
        ch.output_f32 = value;
      } else if (dt_s > 0.0f) {
        ch.output_f32 *= expf(-dt_s / EFFECT_PIPELINE_FADE_TAU_S_F32);
      }

      switch (type) {
      case EFFECT_PIPELINE_OFFSET_FORCE_U8:
        force_N += ch.output_f32 * 0.01f;
        break;
      case EFFECT_PIPELINE_OFFSET_ANGLE_U8:
        angle_deg += ch.output_f32 * 0.001f;
        break;
      case EFFECT_PIPELINE_OFFSET_FORCE_RELATIVE_U8:
        forceRelative += ch.output_f32 * 0.0001f;
        break;
      default:
        ch.output_f32 = 0.0f;
        break;
      }
    }
    portEXIT_CRITICAL(&mux_);

    force_N = constrain(force_N, -EFFECT_PIPELINE_MAX_FORCE_N_F32, EFFECT_PIPELINE_MAX_FORCE_N_F32);
    angle_deg = constrain(angle_deg, -EFFECT_PIPELINE_MAX_ANGLE_DEG_F32,
                          EFFECT_PIPELINE_MAX_ANGLE_DEG_F32);
    float sled_mm = (sledMmPerDeg > 0.0f && isfinite(sledMmPerDeg)) ? angle_deg * sledMmPerDeg : 0.0f;
    sled_mm = constrain(sled_mm, -EFFECT_PIPELINE_MAX_SLED_MM_F32, EFFECT_PIPELINE_MAX_SLED_MM_F32);
    forceRelative = constrain(forceRelative, -EFFECT_PIPELINE_MAX_FORCE_RELATIVE_F32,
                              EFFECT_PIPELINE_MAX_FORCE_RELATIVE_F32);

    // API sign convention -> internal convention (see the header comment)
    effectForce_kg -= force_N / 9.80665f;
    effectForce_kg -= forceRelative * measuredForce_kg;
    effectPos_steps -= sled_mm * stepsPerMm;

    lastForce_N_f32 = force_N;
    lastAngle_deg_f32 = angle_deg;
    lastSled_mm_f32 = sled_mm;
    lastForceRelative_f32 = forceRelative;
  }

  // for debug output
  float lastForce_N_f32 = 0.0f;
  float lastAngle_deg_f32 = 0.0f;
  float lastSled_mm_f32 = 0.0f;
  float lastForceRelative_f32 = 0.0f;

private:
  struct Channel_t {
    uint8_t offsetType_u8 = EFFECT_PIPELINE_OFFSET_OFF_U8;
    uint32_t sampleIndex_au32[EFFECT_PIPELINE_RING_SIZE_U32] = {};
    int16_t samples_ai16[EFFECT_PIPELINE_RING_SIZE_U32] = {};
    bool slotValid_ab[EFFECT_PIPELINE_RING_SIZE_U32] = {};
    float output_f32 = 0.0f;
  };

  portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
  Channel_t channels_[EFFECT_PIPELINE_CHANNELS_U8];
  int64_t clockOffsetUs_ = 0;
  float clockOffsetRelax_f32_ = 0.0f;
  bool clockValid_b_ = false;
  int64_t lastPacketUs_ = 0;
  int64_t lastUpdateUs_ = 0;
};
