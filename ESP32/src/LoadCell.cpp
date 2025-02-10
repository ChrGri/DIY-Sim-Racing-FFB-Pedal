#include "LoadCell.h"

#include <SPI.h>
#include <ADS1256.h>
#include "Main.h"

static const float ADC_CLOCK_MHZ = 7.68f;  // crystal frequency used on ADS1256
static const float ADC_VREF = 2.5f;        // voltage reference

static const int NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION = 1000;
static const float DEFAULT_VARIANCE_ESTIMATE = 0.2f * 0.2f;
static const float LOADCELL_VARIANCE_MIN = 0.001f;
//static const float CONVERSION_FACTOR = LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000));

float updatedConversionFactor_f64 = 1.0f;
#define CONVERSION_FACTOR LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000.0f))


ADS1256& ADC() {
  static ADS1256 adc(ADC_VREF, PIN_DRDY, 
    PIN_CS, PIN_SCK, PIN_MISO, PIN_MOSI);    // RESETPIN is permanently tied to 3.3v

  static bool firstTime = true;
  if (firstTime) {
    Serial.println("Starting ADC");  
    adc.InitSPI(ADC_CLOCK_MHZ);
    adc.InitializeADC(DRATE_2000SPS, ADS1256_GAIN_64, false);
    delay(500);
    
    Serial.println("ADC Started");
    
    adc.waitForDRDY(); // wait for DRDY to go low before changing multiplexer register
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


void LoadCell_ADS1256::setLoadcellRating(uint8_t loadcellRating_u8) {
  ADS1256& adc = ADC();
  float originalConversionFactor_f64 = CONVERSION_FACTOR;
  
  float updatedConversionFactor_f64 = 1.0f;
  if (LOADCELL_WEIGHT_RATING_KG>0)
  {
      updatedConversionFactor_f64 = 2.0f * ((float)loadcellRating_u8) * (CONVERSION_FACTOR/LOADCELL_WEIGHT_RATING_KG);
  }

  _conversionFactor = updatedConversionFactor_f64;
  
  Serial.print("OrigConversionFactor: ");
  Serial.print(originalConversionFactor_f64);
  Serial.print(",     NewConversionFactor:");
  Serial.println(updatedConversionFactor_f64);

  adc.setConversionFactor( updatedConversionFactor_f64 );
}




LoadCell_ADS1256::LoadCell_ADS1256(uint8_t pdType, uint8_t differential)
  : _zeroPoint(0.0f), _varianceEstimate(DEFAULT_VARIANCE_ESTIMATE)
{
  ADC().setMUX(differential);

  if (pdType != PEDAL_TYPE_EXT) {
    _conversionFactor = (LOADCELL_WEIGHT_RATING_KG / (LOADCELL_EXCITATION_V * (LOADCELL_SENSITIVITY_MV_V/1000)));
  }
}

float LoadCell_ADS1256::getReadingKg(uint8_t differential) const {
  ADS1256& adc = ADC();
  
  adc.setConversionFactor(_conversionFactor);

  // correct bias, assume AWGN --> 3 * sigma is 99.9 %
  return adc.readCurrentChannel(differential) - ( _zeroPoint + 3.0f * _standardDeviationEstimate );
}

void LoadCell_ADS1256::setZeroPoint(uint8_t differential) {
  Serial.println("ADC: Identify loadcell offset");
  
  // Due to construction and gravity, the loadcell measures an initial voltage difference.
  // To compensate this difference, the difference is estimated by moving average filter.
  float loadcellOffset = 0.0f;
  for (long i = 0; i < NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION; i++) {
    loadcellOffset += getReadingKg(differential); // DOUT arriving here are from MUX AIN0 and 
  }
  loadcellOffset /= NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION;

  Serial.print("Offset ");
  Serial.println(loadcellOffset,10);

  _zeroPoint = loadcellOffset;
}

void LoadCell_ADS1256::estimateVariance(uint8_t differential) {
  ADS1256& adc = ADC();
  

  Serial.println("ADC: Identify loadcell variance");
  float varNormalizer = 1. / (float)(NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION - 1);
  float varEstimate = 0.0f;
  for (long i = 0; i < NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION; i++){
    float loadcellReading = getReadingKg(differential);
    //Serial.println(loadcellReading);
    varEstimate += sq(loadcellReading) * varNormalizer;
  }

  _standardDeviationEstimate = sqrt(varEstimate);

  Serial.println("Variance est.:");
  Serial.println(varEstimate);

  Serial.println("Stddev est.:");
  Serial.println(_standardDeviationEstimate);

  // make sure estimate is nonzero
  if (varEstimate < LOADCELL_VARIANCE_MIN) { 
    varEstimate = LOADCELL_VARIANCE_MIN;
  }
  varEstimate *= 9.0f; // The variance is 1*sigma --> to make it 3*sigma, we have to multiply by 3*3

  _varianceEstimate = varEstimate;

  
  
}

void LoadCell_ADS1256::clearADCBuffer(uint8_t differential) {
  ADS1256& adc = ADC();
  adc.readCurrentChannel(differential);
}