#ifndef _FREEZE_DANCE_H
#define _FREEZE_DANCE_H

class Tonuino;
class FreezeDance : public Modifier {
 private:
  unsigned long nextStopAtMillis = 0;
  const uint8_t minSecondsBetweenStops = 5;
  const uint8_t maxSecondsBetweenStops = 30;

  void setNextStopAtMillis() {
    uint16_t seconds = random(this->minSecondsBetweenStops, this->maxSecondsBetweenStops + 1);
    DEBUG_PRINTLN(F("=== FreezeDance::setNextStopAtMillis()"));
    DEBUG_PRINTLN(seconds);
    this->nextStopAtMillis = millis() + seconds * 1000;
  }

 public:
  void loop() {
    if (this->nextStopAtMillis != 0 && millis() > this->nextStopAtMillis) {
      DEBUG_PRINTLN(F("== FreezeDance::loop() -> FREEZE!"));
      if (Tonuino::isPlaying()) {
        Tonuino::mp3.playAdvertisement(301);
        delay(500);
      }
      setNextStopAtMillis();
    }
  }
  FreezeDance(void) {
    DEBUG_PRINTLN(F("=== FreezeDance()"));
    if (Tonuino::isPlaying()) {
      delay(1000);
      Tonuino::mp3.playAdvertisement(300);
      delay(500);
    }
    setNextStopAtMillis();
  }
  uint8_t getActive() {
    DEBUG_PRINTLN(F("== FreezeDance::getActive()"));
    return 2;
  }
};

#endif // _FREEZE_DANCE_H