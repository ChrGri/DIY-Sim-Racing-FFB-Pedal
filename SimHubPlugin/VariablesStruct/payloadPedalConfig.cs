using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace User.PluginSdkDemo
{
    //[StructLayout(LayoutKind.Sequential, Pack = 1)]
    
    public struct payloadPedalConfig
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
        public byte relativeForce_p000;
        public byte relativeForce_p020;
        public byte relativeForce_p040;
        public byte relativeForce_p060;
        public byte relativeForce_p080;
        public byte relativeForce_p100;

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
        // cubic spline params
        public float cubic_spline_param_a_0;
        public float cubic_spline_param_a_1;
        public float cubic_spline_param_a_2;
        public float cubic_spline_param_a_3;
        public float cubic_spline_param_a_4;

        public float cubic_spline_param_b_0;
        public float cubic_spline_param_b_1;
        public float cubic_spline_param_b_2;
        public float cubic_spline_param_b_3;
        public float cubic_spline_param_b_4;

        // PID settings
        public float PID_p_gain;
        public float PID_i_gain;
        public float PID_d_gain;
        public float PID_velocity_feedforward_gain;

        // MPC settings
        public float MPC_0th_order_gain;
        public float MPC_1st_order_gain;
        public float MPC_2nd_order_gain;



        public byte control_strategy_b;

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

    }
}
