#include "LoadCell.h"
#include "Main.h"

#ifndef USES_ADS1220

#include <SPI.h>
#include <ADS1256.h>


static const float s_adcClockMHz_fl32 = 7.68f;  // crystal frequency used on ADS1256
static const float s_adcVref_fl32 = 2.5f;        // voltage reference

static const int s_numberOfSamplesForLoadcellOffsetEstimation_i32 = 1000;
static const float s_defaultVarianceEstimate_fl32 = 0.2f * 0.2f;
static const float s_loadcellVarianceMin_fl32 = 7.0f * 1e-5f; // on 8th april 2025, approx. 50g fluctuation were observed --> 6 * sigma = 50g --> sigma = 50g / 6 = 8g --> sigma^2 = (8g)^2 = 0.00064
//static const float CONVERSION_FACTOR = LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000));

float s_updatedConversionFactor_fl32 = 1.0f;
#define CONVERSION_FACTOR LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000.0f))



uint8_t s_globalChannel0_u8, s_globalChannel1_u8, s_globalChannel2_u8;


// --- Semaphore Handle ---
// Moved to global scope to be accessible by the ISR and the class
static SemaphoreHandle_t drdySemaphore = NULL;

// --- Interrupt Service Routine (ISR) ---
// This function is called every time the DRDY pin goes low
void IRAM_ATTR drdyInterrupt()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // Give the semaphore to unblock the reading task.
    if (drdySemaphore != NULL)
    {
        xSemaphoreGiveFromISR(drdySemaphore, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);  // request context switch if needed
    }
}


ADS1256& ADC()
{
  static ADS1256 adc(s_adcClockMHz_fl32, s_adcVref_fl32, /*useresetpin=*/false
  , PIN_DRDY_U8, PIN_SCK_U8, PIN_MISO_U8, PIN_MOSI_U8, PIN_CS_U8);    // RESETPIN is permanently tied to 3.3v


  static bool firstTime = true;
  if (firstTime)
  {
    ActiveSerial->println("Starting ADC");  
    adc.initSpi(s_adcClockMHz_fl32);
    delay(1000);
    
    ActiveSerial->println("ADS: send SDATAC command");
    //adc.sendCommand(ADS1256_CMD_SDATAC);
    
    // start the ADS1256 with a certain data rate and gain       
    adc.begin(ADC_SAMPLE_RATE_U32, ADS1256_GAIN_64, false);  
    
    
    ActiveSerial->println("ADC Started");
    
    adc.waitDRDY(); // wait for DRDY to go low before changing multiplexer register
    if ( fabs(CONVERSION_FACTOR) > 0.01f)
    {
        adc.setConversionFactor(CONVERSION_FACTOR);
    }
    else
    {
        adc.setConversionFactor(1);
    }
    firstTime = false;
  }
  return adc;
}


void LoadCellAds1256::setLoadcellRating(uint8_t loadcellRating_u8) const
{
  ADS1256& adc = ADC();
  float originalConversionFactor_fl32 = CONVERSION_FACTOR;
  
  s_updatedConversionFactor_fl32 = 1.0f;
  if (LOADCELL_WEIGHT_RATING_KG>0)
  {
      s_updatedConversionFactor_fl32 = 2.0f * ((float)loadcellRating_u8) * (CONVERSION_FACTOR/LOADCELL_WEIGHT_RATING_KG);
  }
  // ActiveSerial->print("OrigConversionFactor: ");
  // ActiveSerial->print(originalConversionFactor_fl32);
  // ActiveSerial->print(",     NewConversionFactor:");
  // ActiveSerial->println(s_updatedConversionFactor_fl32);

  // adc.setConversionFactor( updatedConversionFactor_f64 );
  adc.setConversionFactor( 1 );
}



// --- Class Constructor ---
// This is where we will now handle the one-time setup for the semaphore and interrupt.
LoadCellAds1256::LoadCellAds1256(uint8_t channel0, uint8_t channel1)
    : _zeroPoint(0.0f), _varianceEstimate(s_defaultVarianceEstimate_fl32)
{
    s_globalChannel0_u8 = channel0;
    s_globalChannel1_u8 = channel1;
    // Get the ADC instance to ensure it's initialized
    ADS1256& adc = ADC();
    delay(100);
    // --- ONE-TIME SETUP ---
    // Create the binary semaphore ONCE.
    if (drdySemaphore == NULL)
    {
        drdySemaphore = xSemaphoreCreateBinary();

        if (drdySemaphore != NULL)
        {
            ActiveSerial->println("DRDY Semaphore created successfully.");
            ActiveSerial->println("starting attach.....");
            // Attach the interrupt ONCE, after the semaphore is created.
            attachInterrupt(digitalPinToInterrupt(PIN_DRDY_U8), drdyInterrupt, FALLING);
            ActiveSerial->println("DRDY interrupt attached.");
        } else {
            ActiveSerial->println("Error: Failed to create DRDY semaphore!");
        }
    }
    

    // Set the initial MUX channels for differential reading
    adc.setChannel(channel0, channel1);
}

float LoadCellAds1256::getReadingKg() const
{
  ADS1256& adc = ADC();

  float weight_kg = 0.0f;
  static float voltage_mV;
  
  // Check if the semaphore is valid before trying to take it.
  if (drdySemaphore != NULL)
  {
      // Wait for the ISR to give the semaphore.
      // This blocks indefinitely until the DRDY interrupt occurs.
      if (xSemaphoreTake(drdySemaphore, portMAX_DELAY) == pdTRUE)
      {

          // additional DRDY check for better smoothness
          // adc.waitDRDY();

          // final check if DRDY is low. If nor, just discard the measurement.
          if (digitalRead(PIN_DRDY_U8) == LOW)
          {
            voltage_mV = adc.readCurrentChannel();
          }
      }
  }

  // Read the value and apply corrections
  // NOTE: The ADC channel is set in the constructor and doesn't need to be set again here
  // unless you are switching between multiple channels in your application.
  weight_kg = voltage_mV * s_updatedConversionFactor_fl32 - (_zeroPoint + 3.0f * _standardDeviationEstimate);

  return weight_kg;
}


void LoadCellAds1256::estimateBiasAndVariance()
{
  ADS1256& adc = ADC();
  
  ActiveSerial->println("Identify loadcell bias and variance");
  float varEstimate;
  float mean = 0.0f;
  float M2 = 0.0f;
  long n = 0;

  // capturer N measurements on do regressive mean and variance estimate
  // Use Welford-algorithm
  for (long i = 0; i < s_numberOfSamplesForLoadcellOffsetEstimation_i32; i++)
  {
    float loadcellReading = getReadingKg();
    n++;
    float delta = loadcellReading - mean;
    mean += delta / n;
    M2 += delta * (loadcellReading - mean);
  }

  varEstimate = M2 / ((float)n - 1.0f); // empirical variance 
  // make sure estimate is nonzero
  if (varEstimate < s_loadcellVarianceMin_fl32)
  {
    varEstimate = s_loadcellVarianceMin_fl32;
  }

  _zeroPoint = mean;
  _standardDeviationEstimate = sqrt(varEstimate);
  _varianceEstimate = varEstimate;

  ActiveSerial->print("Offset ");
  ActiveSerial->print(_zeroPoint, 5);
  ActiveSerial->println("kg");

  // ActiveSerial->print("Variance est.: ");
  // ActiveSerial->print(varEstimate, 5);
  // ActiveSerial->println("kg");

  ActiveSerial->print("Stddev. est.: ");
  ActiveSerial->print(_standardDeviationEstimate, 5);
  ActiveSerial->println("kg");
}





#endif
