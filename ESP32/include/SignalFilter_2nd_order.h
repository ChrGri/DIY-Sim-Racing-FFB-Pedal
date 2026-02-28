#ifndef SIGNAL_FILTER_2ND_ORDER_H
#define SIGNAL_FILTER_2ND_ORDER_H

#include <stdint.h>
#include <Arduino.h> // Assuming this is for `micros()` and `fabsf()`

// Kalman Filter class declaration
class KalmanFilter2ndOrder {
public:
    KalmanFilter2ndOrder(float varianceEstimate_fl32);
    float filteredValue(float measurement_fl32, float command_fl32, uint8_t modelNoiseScaling_u8);
    float changeVelocity();
    float changeAccel();

private:
    // State
    float stateX_afl32[3]; // position, velocity, acceleration
    float stateCovarianceP_aafl32[3][3]; // 3x3 error covariance matrix

    // Matrices
    float stateTransitionF_aafl32[3][3]; // State transition matrix
    float measurementMatrixH_aafl32[1][3]; // Measurement matrix
    float processNoiseQ_aafl32[3][3]; // Process noise covariance
    float measurementNoiseR_fl32;       // Measurement noise covariance (scalar)
    float kalmanGainK_afl32[3];    // Kalman Gain vector

    // Time
    unsigned long timeLastObservation_u32;

    // Helper functions
    void multiplyMatrices(float matrix1_aafl32[3][3], float matrix2_aafl32[3][3], float result_aafl32[3][3]);
    void multiplyMatrices_3x3_3x1(float matrix1_aafl32[3][3], float vector1_afl32[], float result_afl32[]);
};

#endif // SIGNAL_FILTER_2ND_ORDER_H
