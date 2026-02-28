#ifndef SIGNAL_FILTER_2ND_ORDER_H
#define SIGNAL_FILTER_2ND_ORDER_H

#include <stdint.h>
#include <Arduino.h> // Assuming this is for `micros()` and `fabsf()`

// Kalman Filter class declaration
class KalmanFilter2ndOrder {
public:
    KalmanFilter2ndOrder(float varianceEstimate);
    float filteredValue(float measurement, float command, uint8_t modelNoiseScaling_u8);
    float changeVelocity();
    float changeAccel();

private:
    // State
    float _x[3]; // position, velocity, acceleration
    float _P_cov[3][3]; // 3x3 error covariance matrix

    // Matrices
    float _F[3][3]; // State transition matrix
    float _H[1][3]; // Measurement matrix
    float _Q[3][3]; // Process noise covariance
    float _R;       // Measurement noise covariance (scalar)
    float _K[3];    // Kalman Gain vector

    // Time
    unsigned long _timeLastObservation;

    // Helper functions
    void multiplyMatrices(float mat1[3][3], float mat2[3][3], float result[3][3]);
    void multiplyMatrices_3x3_3x1(float mat1[3][3], float vec1[], float result[]);
};

#endif // SIGNAL_FILTER_2ND_ORDER_H
