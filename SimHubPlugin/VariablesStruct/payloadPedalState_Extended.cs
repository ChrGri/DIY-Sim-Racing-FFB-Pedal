using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{

    public struct payloadPedalState_Extended
    {
        public UInt32 timeInMs_u32;
        public float pedalForce_raw_fl32;
        public float pedalForce_filtered_fl32;
        public float forceVel_est_fl32;

        // register values from servo
        public Int16 servoPosition_i16;
        public Int16 servoPositionTarget_i16;
        public UInt16 angleSensorOutput_ui16;
        public Int16 servo_voltage_0p1V_i16;
        public Int16 servo_current_percent_i16;
    };
}
