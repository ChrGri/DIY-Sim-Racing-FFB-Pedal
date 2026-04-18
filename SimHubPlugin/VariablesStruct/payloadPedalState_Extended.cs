using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct payloadPedalState_Extended
    {
        // register values from servo
        public UInt32 servoStateCycleCount_u32;
        public Int32 servoPositionTarget_i32;
        public Int32 servoPositionFeedback_i32;
        public Int16 servoPositionError_i16;
        public Int16 servoVoltage0p1V_i16;
        public Int16 servoCurrentPercent_i16;

        // values from ESP
        public UInt32 timeInUs_u32;
        public UInt32 cycleCount_u32;
        public float pedalForceRaw_fl32;
        public float pedalForceFiltered_fl32;
        public float forceVelEst_fl32;
        public Int32 targetPosition_i32;
        public Int32 currentSpeedInHz_i32;
        public byte brakeResistorState_b;
        public byte oscillationMonitorValue_u8;

        public float admittance_expectedForce_N;
        public byte admittance_isOscillating;
        public float admittance_admittancePsi_N;
        public float admittance_virtualMass_kg;
        public float admittance_virtualDamping_Ns_m;

        public float admittance_virtualPosition_m;
        public float admittance_virtualVelocity_mps;
        public float admittance_virtualAcceleration_mps2;



    };
}
