using System;
using System.Runtime.InteropServices;

namespace DiyFfbPedal
{
    // Mirrors PayloadEffectPipeline_t (Common_Libs/DiyActivePedal_types/src/PayloadEffectPipeline.h)
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadEffectPipeline
    {
        public UInt32 startSampleIndex_u32;   // plugin sample clock index of samples_ai16[0]
        public byte channel_u8;
        public byte offsetType_u8;            // see EffectPipelineOffsetType
        public byte sampleCount_u8;
        public fixed short samples_ai16[21];  // Constants.effectPipelineSamplesPerPacket
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct DAP_effect_pipeline_st
    {
        public payloadHeader payloadHeader_;
        public payloadEffectPipeline payloadEffectPipeline_;
        public payloadFooter payloadFooter_;
    }

    public enum EffectPipelineOffsetType : byte
    {
        Off = 0,
        Force_N = 1,          // 0.01 N per LSB, + = pedal pushes back harder
        Position_mm = 2,      // 0.001 mm sled travel per LSB, + = towards the driver
        ForceRelative = 3,    // 0.0001 per LSB of the measured pedal force, + = heavier
    }
}
