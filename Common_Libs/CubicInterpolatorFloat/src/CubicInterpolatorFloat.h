#pragma once
#include <Arduino.h> // For malloc, free, sqrtf, etc.
#define MAX_POINTS_U32 100U
// source https://swharden.com/blog/2022-01-22-spline-interpolation/
typedef struct Result
{
    int32_t count_i32;
    float evenDistances_afl32[MAX_POINTS_U32];
    float yInterp_afl32[MAX_POINTS_U32];
    float a_afl32[MAX_POINTS_U32];
    float b_afl32[MAX_POINTS_U32];
} Result_t;

class Cubic
{
public:
    Result_t result_st;
    void interpolate1D(const float *xs_pfl32, const float *ys_pfl32, int32_t inputCount_i32, int32_t outputCount_i32);
    
private:
    void fitMatrix(const float *x_pfl32, const float *y_pfl32, int32_t numPoints_i32, float *a_pfl32, float *b_pfl32);
    void interpolate(const float *xOrig_pfl32, const float *yOrig_pfl32, int32_t nOrig_i32,
                            const float *xInterp_pfl32, int32_t nInterp_i32,
                            const float *a_pfl32, const float *b_pfl32,
                            float *yInterp_pfl32);
};
//Cubic _cubicInterpolator;