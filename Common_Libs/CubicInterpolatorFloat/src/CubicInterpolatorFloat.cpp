#include "CubicInterpolatorFloat.h"
#include <math.h>

void Cubic::interpolate1D(const float *xs_pfl32, const float *ys_pfl32, int32_t inputCount_i32, int32_t outputCount_i32)
{
    //Result result;
    float inputDistances_afl32[MAX_POINTS_U32] = {0};

    for (int32_t inputSampleIndex_i32 = 1; inputSampleIndex_i32 < inputCount_i32; inputSampleIndex_i32++)
    {
        float dx_fl32 = xs_pfl32[inputSampleIndex_i32] - xs_pfl32[inputSampleIndex_i32 - 1];
        inputDistances_afl32[inputSampleIndex_i32] = inputDistances_afl32[inputSampleIndex_i32 - 1] + fabsf(dx_fl32);
    }

    float meanDistance_fl32 = inputDistances_afl32[inputCount_i32 - 1] / (outputCount_i32 - 1);
    for (int32_t outputSampleIndex_i32 = 0; outputSampleIndex_i32 < outputCount_i32; outputSampleIndex_i32++)
    {
        result_st.evenDistances_afl32[outputSampleIndex_i32] = outputSampleIndex_i32 * meanDistance_fl32;
    }

    fitMatrix(xs_pfl32, ys_pfl32, inputCount_i32, result_st.a_afl32, result_st.b_afl32);
    interpolate(xs_pfl32, ys_pfl32, inputCount_i32, result_st.evenDistances_afl32, outputCount_i32, result_st.a_afl32, result_st.b_afl32, result_st.yInterp_afl32);
    //result_st=result;
    result_st.count_i32 = outputCount_i32;
    //return result;
}

void Cubic::fitMatrix(const float *x_pfl32, const float *y_pfl32, int32_t numPoints_i32, float *a_pfl32, float *b_pfl32)
{
    float r_afl32[MAX_POINTS_U32] = {0}, matrixA_afl32[MAX_POINTS_U32] = {0}, matrixB_afl32[MAX_POINTS_U32] = {0}, matrixC_afl32[MAX_POINTS_U32] = {0};
    float dx1_fl32, dx2_fl32, dy1_fl32, dy2_fl32;

    dx1_fl32 = x_pfl32[1] - x_pfl32[0];
    matrixC_afl32[0] = 1.0f / dx1_fl32;
    matrixB_afl32[0] = 2.0f * matrixC_afl32[0];
    r_afl32[0] = 3.0f * (y_pfl32[1] - y_pfl32[0]) / (dx1_fl32 * dx1_fl32);

    for (int32_t matrixIndex_i32 = 1; matrixIndex_i32 < numPoints_i32 - 1; matrixIndex_i32++)
    {
        dx1_fl32 = x_pfl32[matrixIndex_i32] - x_pfl32[matrixIndex_i32 - 1];
        dx2_fl32 = x_pfl32[matrixIndex_i32 + 1] - x_pfl32[matrixIndex_i32];
        matrixA_afl32[matrixIndex_i32] = 1.0f / dx1_fl32;
        matrixC_afl32[matrixIndex_i32] = 1.0f / dx2_fl32;
        matrixB_afl32[matrixIndex_i32] = 2.0f * (matrixA_afl32[matrixIndex_i32] + matrixC_afl32[matrixIndex_i32]);
        dy1_fl32 = y_pfl32[matrixIndex_i32] - y_pfl32[matrixIndex_i32 - 1];
        dy2_fl32 = y_pfl32[matrixIndex_i32 + 1] - y_pfl32[matrixIndex_i32];
        r_afl32[matrixIndex_i32] = 3.0f * (dy1_fl32 / (dx1_fl32 * dx1_fl32) + dy2_fl32 / (dx2_fl32 * dx2_fl32));
    }

    dx1_fl32 = x_pfl32[numPoints_i32 - 1] - x_pfl32[numPoints_i32 - 2];
    dy1_fl32 = y_pfl32[numPoints_i32 - 1] - y_pfl32[numPoints_i32 - 2];
    matrixA_afl32[numPoints_i32 - 1] = 1.0f / dx1_fl32;
    matrixB_afl32[numPoints_i32 - 1] = 2.0f * matrixA_afl32[numPoints_i32 - 1];
    r_afl32[numPoints_i32 - 1] = 3.0f * (dy1_fl32 / (dx1_fl32 * dx1_fl32));

    float cPrime_afl32[MAX_POINTS_U32] = {0};
    float dPrime_afl32[MAX_POINTS_U32] = {0};
    float k_afl32[MAX_POINTS_U32] = {0};

    cPrime_afl32[0] = matrixC_afl32[0] / matrixB_afl32[0];
    for (int32_t forwardIndex_i32 = 1; forwardIndex_i32 < numPoints_i32; forwardIndex_i32++)
    {
        cPrime_afl32[forwardIndex_i32] = matrixC_afl32[forwardIndex_i32] / (matrixB_afl32[forwardIndex_i32] - cPrime_afl32[forwardIndex_i32 - 1] * matrixA_afl32[forwardIndex_i32]);
    }

    dPrime_afl32[0] = r_afl32[0] / matrixB_afl32[0];
    for (int32_t forwardIndex_i32 = 1; forwardIndex_i32 < numPoints_i32; forwardIndex_i32++)
    {
        dPrime_afl32[forwardIndex_i32] = (r_afl32[forwardIndex_i32] - dPrime_afl32[forwardIndex_i32 - 1] * matrixA_afl32[forwardIndex_i32]) / (matrixB_afl32[forwardIndex_i32] - cPrime_afl32[forwardIndex_i32 - 1] * matrixA_afl32[forwardIndex_i32]);
    }

    k_afl32[numPoints_i32 - 1] = dPrime_afl32[numPoints_i32 - 1];
    for (int32_t backSubIndex_i32 = numPoints_i32 - 2; backSubIndex_i32 >= 0; backSubIndex_i32--)
    {
        k_afl32[backSubIndex_i32] = dPrime_afl32[backSubIndex_i32] - cPrime_afl32[backSubIndex_i32] * k_afl32[backSubIndex_i32 + 1];
    }

    for (int32_t segmentIndex_i32 = 1; segmentIndex_i32 < numPoints_i32; segmentIndex_i32++)
    {
        dx1_fl32 = x_pfl32[segmentIndex_i32] - x_pfl32[segmentIndex_i32 - 1];
        dy1_fl32 = y_pfl32[segmentIndex_i32] - y_pfl32[segmentIndex_i32 - 1];
        a_pfl32[segmentIndex_i32 - 1] = k_afl32[segmentIndex_i32 - 1] * dx1_fl32 - dy1_fl32;
        b_pfl32[segmentIndex_i32 - 1] = -k_afl32[segmentIndex_i32] * dx1_fl32 + dy1_fl32;
    }
}

void Cubic::interpolate(const float *xOrig_pfl32, const float *yOrig_pfl32, int32_t nOrig_i32,
                        const float *xInterp_pfl32, int32_t nInterp_i32,
                        const float *a_pfl32, const float *b_pfl32,
                        float *yInterp_pfl32)
{
    for (int32_t interpIndex_i32 = 0; interpIndex_i32 < nInterp_i32; interpIndex_i32++)
    {
        int32_t segmentIndex_i32;
        for (segmentIndex_i32 = 0; segmentIndex_i32 < nOrig_i32 - 2; segmentIndex_i32++)
        {
            if (xInterp_pfl32[interpIndex_i32] <= xOrig_pfl32[segmentIndex_i32 + 1])
            {
                break;
            }
        }

        float dx_fl32 = xOrig_pfl32[segmentIndex_i32 + 1] - xOrig_pfl32[segmentIndex_i32];
        float t_fl32 = (xInterp_pfl32[interpIndex_i32] - xOrig_pfl32[segmentIndex_i32]) / dx_fl32;
        yInterp_pfl32[interpIndex_i32] = (1.0f - t_fl32) * yOrig_pfl32[segmentIndex_i32] + t_fl32 * yOrig_pfl32[segmentIndex_i32 + 1] +
                     t_fl32 * (1.0f - t_fl32) * (a_pfl32[segmentIndex_i32] * (1.0f - t_fl32) + b_pfl32[segmentIndex_i32] * t_fl32);
    }
}