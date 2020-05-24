
#define AMOUNT_VOLTAGES 5

#include <Arduino.h>

class LedHandler;
class BatteryHandler {

 private:
  Tonuino* tonuino;
  LedHandler* ledHandler;
  float batteryVoltages[AMOUNT_VOLTAGES];
  uint8_t currentArrayIndex;
  long batteryMeasureTimestamp;

  float readBatteryVoltage();
  void checkBatteryVoltage(bool skipReading);

 public:
  BatteryHandler(Tonuino* tonuino, LedHandler* ledHandler);
  ~BatteryHandler();

  void setup();
  void loop();
};