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

static SPIClass s_adsSPI_sc(FSPI);  // Or use VSPI or HSPI for ESP32
static const SPISettings s_fastAdsSPISettings(4000000, MSBFIRST, SPI_MODE1);
static float s_rawToKgConversionFactor_fl32 = 0.0f;

static inline void updateRawToKgConversionFactor() {
    // 24-bit full scale is 2^23 = 8388608. Gain is 128.
    // result_mV = (raw24 / 8388608.0) * s_refVoltageInMv_fl32 / 128.0
    // weight_kg = result_mV * s_updatedConversionFactor_fl32 * 0.001
    // => weight_kg = raw24 * [ (s_refVoltageInMv_fl32 / (8388608.0f * 128.0f)) * s_updatedConversionFactor_fl32 * 0.001f ]
    s_rawToKgConversionFactor_fl32 = (s_refVoltageInMv_fl32 / (8388608.0f * 128.0f)) * s_updatedConversionFactor_fl32 * 0.001f;
}

static inline int32_t IRAM_ATTR fastReadADS1220Raw() {
    uint8_t rxBuf[3] = {0, 0, 0};
    uint8_t txBuf[3] = {0, 0, 0};

    s_adsSPI_sc.beginTransaction(s_fastAdsSPISettings);
    digitalWrite(FFB_ADS1220_CS, LOW);
    s_adsSPI_sc.transferBytes(txBuf, rxBuf, 3);
    digitalWrite(FFB_ADS1220_CS, HIGH);
    s_adsSPI_sc.endTransaction();

    int32_t rawResult = ((int32_t)rxBuf[0] << 24) | ((int32_t)rxBuf[1] << 16) | ((int32_t)rxBuf[2] << 8);
    return rawResult >> 8; // sign-extend 24-bit to signed 32-bit int
}

/* Provides a singleton instance of the ADS1220 ADC driver. */
ADS1220_WE& getADC() 
{
  static ADS1220_WE s_adc_awe(&s_adsSPI_sc, FFB_ADS1220_CS, FFB_ADS1220_DRDY, true);
  
  //static ADS1220_WE adc(FFB_ADS1220_CS, FFB_ADS1220_DRDY);

  static bool s_firstTime_b = true;
  if (s_firstTime_b) 
  {
    ActiveSerial->println("Initializing ADS1220 ADC...");

    // Initialize custom SPI bus. This should be done only once.
    s_adsSPI_sc.begin(FFB_ADS1220_SCLK, FFB_ADS1220_DOUT, FFB_ADS1220_DIN, FFB_ADS1220_CS);
    pinMode(FFB_ADS1220_CS, OUTPUT);
    digitalWrite(FFB_ADS1220_CS, HIGH);

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
    updateRawToKgConversionFactor();

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
    // Workaround for ESP32 IPC1 stack overflow when attaching interrupt from Core 0
    xTaskCreatePinnedToCore(
        [](void* pvParameters) {
            attachInterrupt(digitalPinToInterrupt(FFB_ADS1220_DRDY), drdyInterrupt, FALLING);
            vTaskDelete(NULL);
        },
        "attachInt", 2048, NULL, configMAX_PRIORITIES - 1, NULL, 1);
    delay(50); // give the task time to execute


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
  updateRawToKgConversionFactor();
}


// #define LOADCELL_RADING_INTERVALL_IN_US (uint32_t)500
float IRAM_ATTR LoadCellAds1220::readLoadcellWeightInKg() const 
{
  static float s_lastWeightKg_fl32 = 0.0f;

  // wait for the timer to fire
  // This will block until the timer callback gives the semaphore. It won't consume CPU time while waiting.
  if(g_timerFireLoadcellReadingReady_sh != NULL)
  {
    if (xSemaphoreTake(g_timerFireLoadcellReadingReady_sh, portMAX_DELAY) == pdTRUE) 
    {
      // final check if DRDY is low. If not, just retain previous measurement.
      if (digitalRead(FFB_ADS1220_DRDY) == LOW)
      {
        int32_t raw24 = fastReadADS1220Raw();
        s_lastWeightKg_fl32 = (float)raw24 * s_rawToKgConversionFactor_fl32;
      }
    }
  }

  // correct bias, assume AWGN --> 3 * sigma is 99.9 %
  return s_lastWeightKg_fl32 - ( zeroPoint_fl32 + 3.0f * standardDeviationEstimate_fl32 );
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
  standardDeviationEstimate_fl32 = sqrtf(varianceEstimateLocal_fl32);
  varianceEstimate_fl32 = varianceEstimateLocal_fl32;

  ActiveSerial->print("Offset ");
  ActiveSerial->print(zeroPoint_fl32, 5);
  ActiveSerial->println("kg");

  ActiveSerial->print("Stddev. est.: ");
  ActiveSerial->print(standardDeviationEstimate_fl32, 5);
  ActiveSerial->println("kg");
}

#endif

