#include "LoadCell_ads1220.h"
#include "Main.h"
#include "Arduino.h"

#ifdef USES_ADS1220

#include <SPI.h>
#include <ADS1220_WE.h>


static const int s_numberOfSamplesForLoadcellOfffsetEstimation_i32 = 1000;
static const float s_defaultVarianceEstimate_fl32 = 0.2f * 0.2f;
static const float s_loadcellVarianceMin_fl32 = 7.0f * 1e-5f; // on 8th april 2025, approx. 50g fluctuation were observed --> 6 * sigma = 50g --> sigma = 50g / 6 = 8g --> sigma^2 = (8g)^2 = 0.00064

static float s_updatedConversionFactor_fl32 = 1.0f;

#define TIMEOUT_FOR_DRDY_TO_BECOME_LOW_U32 (uint32_t)2000
#define DELAY_IN_US_FOR_DRDY_TO_BECOME_LOW_U32 (uint32_t)20

// reference voltage in milli-volts
static float s_refVoltageInMv_fl32 = 5000.0f;

static SemaphoreHandle_t g_timerFireLoadcellReadingReady_sh;
// --- Semaphore Handle ---
// Moved to global scope to be accessible by the ISR and the class


// This is our Interrupt Service Routine
void IRAM_ATTR drdyInterrupt() 
{
  BaseType_t higherPriorityTaskWoken_bt = pdFALSE;
    // Give the semaphore to unblock the reading task.
    if (g_timerFireLoadcellReadingReady_sh != NULL) 
    {
      xSemaphoreGiveFromISR(g_timerFireLoadcellReadingReady_sh, &higherPriorityTaskWoken_bt);
        portYIELD_FROM_ISR(higherPriorityTaskWoken_bt);  // request context switch if needed
    }
}

/* Provides a singleton instance of the ADS1220 ADC driver. */
ADS1220_WE& getADC() 
{
  
  static SPIClass s_adsSPI_sc(FSPI);  // Or use VSPI or HSPI for ESP32
  static ADS1220_WE s_adc_awe(&s_adsSPI_sc, FFB_ADS1220_CS, FFB_ADS1220_DRDY, true);
  
  //static ADS1220_WE adc(FFB_ADS1220_CS, FFB_ADS1220_DRDY);

  static bool s_firstTime_b = true;
  if (s_firstTime_b) 
  {
    ActiveSerial->println("Initializing ADS1220 ADC...");

    // Initialize custom SPI bus. This should be done only once.
    s_adsSPI_sc.begin(FFB_ADS1220_SCLK, FFB_ADS1220_DOUT, FFB_ADS1220_DIN, FFB_ADS1220_CS);

    // Initialize ADS1220
    if (!s_adc_awe.init()) 
    {
      ActiveSerial->println("ADS1220 not found!");
      while (1);
    }

    // ADS1220 Configuration
    s_adc_awe.setDataRate(ADS1220_DR_LVL_6);     // 2000SPS

    // PGA
    s_adc_awe.setGain(ADS1220_GAIN_128);            // Gain for load cell

    // reference voltage
    s_adc_awe.setVRefSource(ADS1220_VREF_AVDD_AVSS);
    //ads.setVRefValue_V(4.7f);    // set reference voltage in volts
    s_adc_awe.setAvddAvssAsVrefAndCalibrate();

    float refVolt_fl32 = s_adc_awe.getVRef_V();
    s_refVoltageInMv_fl32 = refVolt_fl32 * 1000.0f; // convert to mV
    ActiveSerial->print("Reference voltage: ");
    ActiveSerial->print(refVolt_fl32);
    ActiveSerial->println("V");

    // differential channels
    s_adc_awe.setCompareChannels(ADS1220_MUX_0_1);              // Differential AIN0 - AIN1

    // set modulalar frequency
    s_adc_awe.setOperatingMode(ADS1220_TURBO_MODE);

    // continous reading mode
    s_adc_awe.setConversionMode(ADS1220_CONTINUOUS);  // Add this line in setup

    // set 50HZ and 60Hz FIR filter
    s_adc_awe.setFIRFilter(ADS1220_50HZ_60HZ);

    // set 
    //adc.setDrdyMode(ADS1220_DOUT_DRDY);
    s_adc_awe.setDrdyMode(ADS1220_DRDY);

    // needs to wait fir DRDY come from low to high --> do not use
    s_adc_awe.setNonBlockingMode(true); // switch ton non-blocking mode
    
    // assign interrupt to DRDY falling edge to make waiting more efficient
    attachInterrupt(digitalPinToInterrupt(FFB_ADS1220_DRDY), drdyInterrupt, FALLING);


    ActiveSerial->println("ADC Started");
    
    s_firstTime_b = false;
  }

  return s_adc_awe;
}






LoadCellAds1220::LoadCellAds1220()
  : zeroPoint_fl32(0.0f), varianceEstimate_fl32(s_defaultVarianceEstimate_fl32)
{
  // differential channels
  getADC().setCompareChannels(ADS1220_MUX_0_1);              // Differential AIN0 - AIN1
  g_timerFireLoadcellReadingReady_sh = xSemaphoreCreateBinary();
}



void LoadCellAds1220::setLoadcellRating(uint8_t loadcellRating_u8) const 
{
  getADC(); // Ensure ADC is initialized
  
  s_updatedConversionFactor_fl32 = 1.0f;
  if (LOADCELL_WEIGHT_RATING_KG>0)
  {
      float excitationVoltage_fl32 = s_refVoltageInMv_fl32 / 1000.0f;
      float fullScaleMv_fl32 = LOADCELL_SENSITIVITY_MV_V * excitationVoltage_fl32; // 2 mV/V * Vexc
      float loadcellRatingInGram_fl32 = (((float)loadcellRating_u8) * 1000.0f); // convert kg to gram
      float gramsPerMillivolt_fl32 =  loadcellRatingInGram_fl32  / fullScaleMv_fl32;  // g per mV
      s_updatedConversionFactor_fl32 = gramsPerMillivolt_fl32;
      s_updatedConversionFactor_fl32 *= 2.0f; // empirically identified
  }
}


// #define LOADCELL_RADING_INTERVALL_IN_US (uint32_t)500
float IRAM_ATTR LoadCellAds1220::readLoadcellWeightInKg() const 
{
  ADS1220_WE& adc_awe = getADC();
  static float s_voltageMv_fl32;

  // wait for the timer to fire
  // This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
  if(g_timerFireLoadcellReadingReady_sh != NULL)
  {
    if (xSemaphoreTake(g_timerFireLoadcellReadingReady_sh, portMAX_DELAY) == pdTRUE) 
    {
      
      // final check if DRDY is low. If nor, just discard the measurement.
      if (digitalRead(FFB_ADS1220_DRDY) == LOW)
      {
        // Read the voltage from the ADS1220
        s_voltageMv_fl32 = adc_awe.getVoltage_mV();
      }
      
    }
  }

  float weightGrams_fl32 = s_voltageMv_fl32 * s_updatedConversionFactor_fl32;
  float weightKg_fl32 = weightGrams_fl32 * 0.001f; // convert grams to kg
  
  // correct bias, assume AWGN --> 3 * sigma is 99.9 %
  return weightKg_fl32 - ( zeroPoint_fl32 + 3.0f * standardDeviationEstimate_fl32 );
}



void LoadCellAds1220::estimateBiasAndVariance() 
{
  getADC(); // Ensure ADC is initialized
  
  ActiveSerial->println("Identify loadcell bias and variance");
  float varianceEstimateLocal_fl32;
  float mean_fl32 = 0.0f;
  float sumOfSquaresOfDifferences_fl32 = 0.0f;
  long sampleCount_i64 = 0;
  

  // capturer N measurements on do regressive mean and variance estimate
  // Use Welford-algorithm
  for (long i_i64 = 0; i_i64 < s_numberOfSamplesForLoadcellOfffsetEstimation_i32; i_i64++)
  {
    float loadcellReading_fl32 = readLoadcellWeightInKg();
    sampleCount_i64++;
    float delta_fl32 = loadcellReading_fl32 - mean_fl32;
    mean_fl32 += delta_fl32 / (float)sampleCount_i64;
    sumOfSquaresOfDifferences_fl32 += delta_fl32 * (loadcellReading_fl32 - mean_fl32);
  }

  varianceEstimateLocal_fl32 = sumOfSquaresOfDifferences_fl32 / ((float)sampleCount_i64 - 1.0f); // empirical variance 
  // make sure estimate is nonzero
  if (varianceEstimateLocal_fl32 < s_loadcellVarianceMin_fl32) 
  { 
    varianceEstimateLocal_fl32 = s_loadcellVarianceMin_fl32;
  }

  zeroPoint_fl32 = mean_fl32;
  standardDeviationEstimate_fl32 = sqrt(varianceEstimateLocal_fl32);
  varianceEstimate_fl32 = varianceEstimateLocal_fl32;

  ActiveSerial->print("Offset ");
  ActiveSerial->print(zeroPoint_fl32, 5);
  ActiveSerial->println("kg");

  ActiveSerial->print("Stddev. est.: ");
  ActiveSerial->print(standardDeviationEstimate_fl32, 5);
  ActiveSerial->println("kg");
}

#endif

