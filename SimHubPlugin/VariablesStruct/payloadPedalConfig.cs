using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace User.PluginSdkDemo
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    unsafe public struct payloadPedalConfig
    {
        // configure pedal start and endpoint
        // In percent
        public byte pedalStartPosition;
        public byte pedalEndPosition;

        // configure pedal forces
        public float maxForce;
        public float preloadForce;

        // design force vs travel curve
        // In percent
        /*
        public byte relativeForce_p000;
        public byte relativeForce_p020;
        public byte relativeForce_p040;
        public byte relativeForce_p060;
        public byte relativeForce_p080;
        public byte relativeForce_p100;
        */
        public byte quantityOfControl;
        public byte relativeForce00;
        public byte relativeForce01;
        public byte relativeForce02;
        public byte relativeForce03;
        public byte relativeForce04;
        public byte relativeForce05;
        public byte relativeForce06;
        public byte relativeForce07;
        public byte relativeForce08;
        public byte relativeForce09;
        public byte relativeForce10;
        
        public byte relativeTravel00;
        public byte relativeTravel01;
        public byte relativeTravel02;
        public byte relativeTravel03;
        public byte relativeTravel04;
        public byte relativeTravel05;
        public byte relativeTravel06;
        public byte relativeTravel07;
        public byte relativeTravel08;
        public byte relativeTravel09;
        public byte relativeTravel10;

        public byte numOfJoystickMapControl;
        public byte joystickMapOrig00;
        public byte joystickMapOrig01;
        public byte joystickMapOrig02;
        public byte joystickMapOrig03;
        public byte joystickMapOrig04;
        public byte joystickMapOrig05;
        public byte joystickMapOrig06;
        public byte joystickMapOrig07;
        public byte joystickMapOrig08;
        public byte joystickMapOrig09;
        public byte joystickMapOrig10;
        public byte joystickMapMapped00;
        public byte joystickMapMapped01;
        public byte joystickMapMapped02;
        public byte joystickMapMapped03;
        public byte joystickMapMapped04;
        public byte joystickMapMapped05;
        public byte joystickMapMapped06;
        public byte joystickMapMapped07;
        public byte joystickMapMapped08;
        public byte joystickMapMapped09;
        public byte joystickMapMapped10;

        // parameter to configure damping
        public byte dampingPress;
        public byte dampingPull;

        // configure ABS effect 
        public byte absFrequency; // In Hz
        public byte absAmplitude; // In kg/20
        public byte absPattern; // 0: sinewave, 1: sawtooth
        public byte absForceOrTarvelBit;


        // geometric properties of the pedal
        // in mm
        public Int16 lengthPedal_a;
        public Int16 lengthPedal_b;
        public Int16 lengthPedal_d;
        public Int16 lengthPedal_c_horizontal;
        public Int16 lengthPedal_c_vertical;
        public Int16 lengthPedal_travel;


        public byte Simulate_ABS_trigger; //simulateABS
        public byte Simulate_ABS_value; //simulated ABS value
        public byte RPM_max_freq;
        public byte RPM_min_freq;
        public byte RPM_AMP;
        public byte BP_trigger_value;
        public byte BP_amp;
        public byte BP_freq;
        public byte BP_trigger;
        public byte G_multi;
        public byte G_window;
        public byte WS_amp;
        public byte WS_freq;
        public byte Impact_multi;
        public byte Impact_window;
        //Custom Vibration 1
        public byte CV_amp_1;
        public byte CV_freq_1;
        //Custom Vibration 2
        public byte CV_amp_2;
        public byte CV_freq_2;

        public byte maxGameOutput;

        // Kalman filter model noise
        public byte kf_modelNoise;
        public byte kf_modelOrder;

        // debug flags, sued to enable debug output
        public byte debug_flags_0;

        // loadcell rating in kg / 2 --> to get value in kg, muiltiply by 2
        public byte loadcell_rating;

        // use loadcell or travel as joystick output
        public byte travelAsJoystickOutput_u8;

        // invert loadcell sign
        public byte invertLoadcellReading_u8;

        // invert motor direction
        public byte invertMotorDirection_u8;

        // spindle pitch in mm/rev
        public byte spindlePitch_mmPerRev_u8;

        // pedal type
        public byte pedal_type;

        // OTA update flag
        //public byte OTA_flag;

        // Misc flags
        public byte stepLossFunctionFlags_u8;
        // Kalman filter model noise
        public byte kf_Joystick_u8;
        public byte kf_modelNoise_joystick;
        public byte servoIdleTimeout;

        public byte positionSmoothingFactor_u8;
        public byte minForceForEffects;

        public byte servoRatioOfInertia_u8;
        public UInt32 configHash_u32;

    }
}
