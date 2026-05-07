#pragma once
#define MAX_DATA_POINTS_U32 100U
// moving average filter:https://github.com/sebnil/Moving-Avarage-Filter--Arduino-Library-/tree/master
class MovingAverageFilter
{
public:
  //construct without coefs
  MovingAverageFilter(uint32_t dataPointsCount_u32);
  int32_t dataPointsCount_i32;
  float process(float input_fl32);
  

private:
  float values_afl32[MAX_DATA_POINTS_U32];
  int32_t currentIndex_i32; // stores the current array index to create a circular buffer through the array
  
  float output_fl32;
  int32_t loopIndex_i32; // loop counter
};

MovingAverageFilter::MovingAverageFilter(uint32_t dataPointsCount_u32)
{
  currentIndex_i32 = 0; //initialize so that we start to write at index 0
  if (dataPointsCount_u32 < MAX_DATA_POINTS_U32)
  {
    dataPointsCount_i32 = dataPointsCount_u32;
  }
  else
  {
    dataPointsCount_i32 = MAX_DATA_POINTS_U32;
  }

  for (loopIndex_i32 = 0; loopIndex_i32 < dataPointsCount_i32; loopIndex_i32++)
  {
    values_afl32[loopIndex_i32] = 0.0f; // fill the array with 0's
  }
}

float MovingAverageFilter::process(float input_fl32)
{
  output_fl32 = 0.0f;

  values_afl32[currentIndex_i32] = input_fl32;
  currentIndex_i32 = (currentIndex_i32 + 1) % dataPointsCount_i32;

  for (loopIndex_i32 = 0; loopIndex_i32 < dataPointsCount_i32; loopIndex_i32++)
  {
    output_fl32 += values_afl32[loopIndex_i32];
  }

  float retValue_fl32 = 0.0f;
  if (dataPointsCount_i32 > 0)
  {
    retValue_fl32 = output_fl32 / dataPointsCount_i32;
  }
  
  return retValue_fl32;
}