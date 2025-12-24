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
        //public UInt32 timeInMs_u32;
        public UInt32 timeInUs_u32;
        public UInt32 cycleCount_u32;
        //public UInt32 timeInUsFromSerialTask_u32;
        public float pedalForce_raw_fl32;
        public float pedalForce_filtered_fl32;
        public float forceVel_est_fl32;

        // register values from servo
        public Int32 servoPosition_i32;
        public Int32 servoPositionTarget_i32;
        public Int16 servoPositionEstimated_i16;
        public Int32 targetPosition_i32;
        public Int32 currentSpeedInMilliHz_i32;
        //public Int16 servoPositionEstimated_stepperPos_i16;
        public Int16 servo_position_error_i16;
        public UInt16 angleSensorOutput_ui16;
        public Int16 servo_voltage_0p1V_i16;
        public Int16 servo_current_percent_i16;
        public byte brakeResistorState_b;
    };
}
