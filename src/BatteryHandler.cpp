#include "Tonuino.h"
#include "BatteryHandler.h"
#include "LedHandler.h"

/************************************************************************************************************************************************************
 * 
 */
BatteryHandler::BatteryHandler(Tonuino* tonuino, LedHandler* ledHandler) {
  this->tonuino = tonuino;
  this->ledHandler = ledHandler;
  batteryMeasureTimestamp = 0;
}

/************************************************************************************************************************************************************
 * 
 */
BatteryHandler::~BatteryHandler() {}

/************************************************************************************************************************************************************
 * 
 */
void BatteryHandler::setup() {
  pinMode(A5, INPUT);  // Battery voltage sense pin
  delay(20);

  // Pre-populate battery voltages array with current voltage for first average
  float currentVoltage = readBatteryVoltage();
  currentArrayIndex = 0;
  for (int i = 0; i < AMOUNT_VOLTAGES; i++) {
    batteryVoltages[currentArrayIndex] = currentVoltage;
    currentArrayIndex = (currentArrayIndex + 1) % AMOUNT_VOLTAGES;
  }

  // This initially sets status LED color
  checkBatteryVoltage(true);
}

/************************************************************************************************************************************************************
 * 
 */
void BatteryHandler::loop() {
  if (millis() - batteryMeasureTimestamp >= 30000) {
    checkBatteryVoltage(false);
    batteryMeasureTimestamp = millis();
  }
}

/************************************************************************************************************************************************************
 * 
 */
float BatteryHandler::readBatteryVoltage() {
  int batteryVoltageReading = analogRead(A5);
  float batteryVoltage = batteryVoltageReading * (4.2 / 1023.0);
  DEBUG_PRINT("Battery voltage: ");
  DEBUG_PRINTLN(batteryVoltage);
  return batteryVoltage;
}

/************************************************************************************************************************************************************
 * 
 */
void BatteryHandler::checkBatteryVoltage(bool skipReading) {
  if (!skipReading) {
    batteryVoltages[currentArrayIndex] = readBatteryVoltage();
    currentArrayIndex = (currentArrayIndex + 1) % AMOUNT_VOLTAGES;
  }
  float avgBatteryVoltage = 0.0;
  for (int i = 0; i < AMOUNT_VOLTAGES; i++) {
    avgBatteryVoltage += batteryVoltages[i];
  }
  avgBatteryVoltage /= AMOUNT_VOLTAGES;
  DEBUG_PRINT("Battery voltage average: ");
  DEBUG_PRINTLN(avgBatteryVoltage);
  if (avgBatteryVoltage < 3.2) {
    // Shutdown Tonuino if battery voltages goes below 3.2V
    DEBUG_PRINTLN("Shutting down Tonuino due to low battery voltage (<3.2V)");
    ledHandler->indicateErrorState();
    tonuino->poweroff();
  } else if (avgBatteryVoltage < 3.4) {
    ledHandler->setStatusLedColor(CRGB::Red);
  } else if (avgBatteryVoltage < 3.7) {
    ledHandler->setStatusLedColor(CRGB::Yellow);
  } else {
    ledHandler->setStatusLedColor(CRGB::Green);
  }
}