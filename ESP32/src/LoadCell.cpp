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



uint8_t global_channel0_u8, global_channel1_u8, global_channel2_u8;


ADS1256& ADC() {
  static ADS1256 adc(ADC_CLOCK_MHZ, ADC_VREF, /*useresetpin=*/false
  , PIN_DRDY, PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);    // RESETPIN is permanently tied to 3.3v


  static bool firstTime = true;
  if (firstTime) {
    Serial.println("Starting ADC");  
    adc.initSpi(ADC_CLOCK_MHZ);
    delay(1000);
    
    Serial.println("ADS: send SDATAC command");
    //adc.sendCommand(ADS1256_CMD_SDATAC);
    
    // start the ADS1256 with a certain data rate and gain       
    adc.begin(ADC_SAMPLE_RATE, ADS1256_GAIN_64, false);  
    
    
    Serial.println("ADC Started");
    
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


void LoadCell_ADS1256::setLoadcellRating(uint8_t loadcellRating_u8) const {
  ADS1256& adc = ADC();
  float originalConversionFactor_f64 = CONVERSION_FACTOR;
  
  updatedConversionFactor_f64 = 1.0f;
  if (LOADCELL_WEIGHT_RATING_KG>0)
  {
      updatedConversionFactor_f64 = 2.0f * ((float)loadcellRating_u8) * (CONVERSION_FACTOR/LOADCELL_WEIGHT_RATING_KG);
  }
  Serial.print("OrigConversionFactor: ");
  Serial.print(originalConversionFactor_f64);
  Serial.print(",     NewConversionFactor:");
  Serial.println(updatedConversionFactor_f64);


  // adc.setConversionFactor( updatedConversionFactor_f64 );
  adc.setConversionFactor( 1 );
}




LoadCell_ADS1256::LoadCell_ADS1256(uint8_t channel0, uint8_t channel1)
  : _zeroPoint(0.0f), _varianceEstimate(DEFAULT_VARIANCE_ESTIMATE)
{

  global_channel0_u8 = channel0;
  global_channel1_u8 = channel1;
  ADC().setChannel(channel0,channel1);   // Set the MUX for differential between ch0 and ch1 
  //ADC().setChannel(channel1, channel0);   // Set the MUX for differential between ch0 and ch1 
}

float LoadCell_ADS1256::getReadingKg() const {
  ADS1256& adc = ADC();
  adc.waitDRDY();        // wait for DRDY to go low before next register read
  // adc.setGain(ADS1256_GAIN_64);
  // adc.setChannel(global_channel0_u8, global_channel1_u8);   // Set the MUX for differential between ch0 and ch1
  // correct bias, assume AWGN --> 3 * sigma is 99.9 %
  return adc.readCurrentChannel()*updatedConversionFactor_f64 - ( _zeroPoint + 3.0f * _standardDeviationEstimate );
}

// float LoadCell_ADS1256::getAngleMeasurement() const {
//   ADS1256& adc = ADC();
//   adc.waitDRDY();        // wait for DRDY to go low before next register read
//   adc.setGain(ADS1256_GAIN_1);
//   adc.setChannel(global_channel2_u8);  
//   return adc.readCurrentChannel();
// }

void LoadCell_ADS1256::setZeroPoint() {
  Serial.println("ADC: Identify loadcell offset");
  
  // Due to construction and gravity, the loadcell measures an initial voltage difference.
  // To compensate this difference, the difference is estimated by moving average filter.
  float loadcellOffset = 0.0f;
  for (long i = 0; i < NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION; i++) {
    loadcellOffset += getReadingKg(); // DOUT arriving here are from MUX AIN0 and 
  }
  loadcellOffset /= NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION;

  Serial.print("Offset ");
  Serial.println(loadcellOffset,10);

  _zeroPoint = loadcellOffset;
}

void LoadCell_ADS1256::estimateVariance() {
  ADS1256& adc = ADC();
  

  Serial.println("ADC: Identify loadcell variance");
  float varNormalizer = 1. / (float)(NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION - 1);
  float varEstimate = 0.0f;
  for (long i = 0; i < NUMBER_OF_SAMPLES_FOR_LOADCELL_OFFFSET_ESTIMATION; i++){
    float loadcellReading = getReadingKg();
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
