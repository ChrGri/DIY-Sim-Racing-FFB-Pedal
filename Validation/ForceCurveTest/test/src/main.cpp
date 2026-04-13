#include <Arduino.h>
#include "DiyActivePedal_types.h"
#include "ForceCurve.h"

// Instantiate the necessary structs and classes
DapConfig_t testConfig;
DapCalculationVariables_t testCalc;
ForceCurveInterpolated forceCurve; 

void setup() {
  // Initialize standard Serial for the Arduino Serial Monitor/Plotter
  Serial.begin(115200);
  while (!Serial) {;} // Wait for native USB connection if applicable
  delay(2000);

  Serial.println("--- DAP Spline & Gradient Verification Test ---");

  // 1. Initialize default values to avoid garbage memory
  testConfig.initializeDefaults();

  // 2. Inject our specific "Whip Effect" test data
  testConfig.payloadPedalConfig_st.quantityOfControl_u8 = 6;

  // X-Axis: Travel (0 to 100%)
  testConfig.payloadPedalConfig_st.relativeTravel00_u8 = 0;
  testConfig.payloadPedalConfig_st.relativeTravel01_u8 = 20;
  testConfig.payloadPedalConfig_st.relativeTravel02_u8 = 40;
  testConfig.payloadPedalConfig_st.relativeTravel03_u8 = 60;
  testConfig.payloadPedalConfig_st.relativeTravel04_u8 = 80;
  testConfig.payloadPedalConfig_st.relativeTravel05_u8 = 100;

  // Y-Axis: Force Mapping (Linear at first, then sharp jump at end)
  testConfig.payloadPedalConfig_st.relativeForce00_u8 = 0;
  testConfig.payloadPedalConfig_st.relativeForce01_u8 = 10;
  testConfig.payloadPedalConfig_st.relativeForce02_u8 = 20;
  testConfig.payloadPedalConfig_st.relativeForce03_u8 = 30;
  testConfig.payloadPedalConfig_st.relativeForce04_u8 = 50;
  testConfig.payloadPedalConfig_st.relativeForce05_u8 = 100;

  // Setup ranges so dimensional normalization in EvalForceGradientCubicSpline works
  testConfig.payloadPedalConfig_st.preloadForce_fl32 = 0.0f;
  testConfig.payloadPedalConfig_st.maxForce_fl32 = 100.0f; // Force Range = 100
  testCalc.stepperPosRange_fl32 = 100.0f;                 // Pos Range = 100

  // 3. Trigger the CubicInterpolator matrix solver
  testCalc.updateFromConfig(testConfig);

  // 4. Optional: Print the resulting A and B coefficients to Serial Monitor
  // (Disable this block if you only want to use the Serial Plotter)
  /*
  Serial.println("\n--- Solved Coefficients ---");
  for (int i = 0; i < testConfig.payloadPedalConfig_st.quantityOfControl_u8 - 1; i++) {
    Serial.print("Segment "); Serial.print(i);
    Serial.print(": a = "); Serial.print(testCalc.interpolatorA_pfl32[i], 4);
    Serial.print(" | b = "); Serial.println(testCalc.interpolatorB_pfl32[i], 4);
  }
  */

  // 5. Output CSV Headers for the Serial Plotter
  Serial.println("Travel,Force_Curve,Gradient");

  // 6. Sweep through fractional position (0.0 to 1.0) and evaluate
  int numTestPoints = 100; // 100 points for a smooth graph
  for (int i = 0; i <= numTestPoints; i++) {
    float fractionalPos = (float)i / (float)numTestPoints;

    // Calculate Y (Force)
    float forceY = forceCurve.EvalForceCubicSpline(&testConfig, &testCalc, fractionalPos);

    // Calculate dy/dx (Gradient). false = un-normalized (physical scaling)
    float gradient = forceCurve.EvalForceGradientCubicSpline(&testConfig, &testCalc, fractionalPos, true);

    // Print in CSV format: X, Y1, Y2
    Serial.print(fractionalPos * 100.0f); // X-axis (Travel %)
    Serial.print(",");
    Serial.print(forceY, 3);              // Y1 (Force output)
    Serial.print(",");
    Serial.println(gradient, 3);          // Y2 (Gradient output)
  }
}

void loop() {
  // Test runs once in setup. 
  delay(1000);
}