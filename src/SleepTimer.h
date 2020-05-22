#ifndef _SLEEP_TIMER_H
#define _SLEEP_TIMER_H

class Tonuino;
class SleepTimer : public Modifier {
 private:
  unsigned long sleepAtMillis = 0;

 public:
  void loop() {
    if (this->sleepAtMillis != 0 && millis() > this->sleepAtMillis) {
      DEBUG_PRINTLN(F("=== SleepTimer::loop() -> SLEEP!"));
      Tonuino::mp3.pause();
      Tonuino::setStandbyTimer();
      Tonuino::activeModifier = NULL;
      delete this;
    }
  }

  SleepTimer(uint8_t minutes) {
    DEBUG_PRINTLN(F("=== SleepTimer()"));
    DEBUG_PRINTLN(minutes);
    this->sleepAtMillis = millis() + minutes * 60000;
    //      if (isPlaying())
    //        mp3.playAdvertisement(302);
    //      delay(500);
  }
  uint8_t getActive() {
    DEBUG_PRINTLN(F("== SleepTimer::getActive()"));
    return 1;
  }

  virtual ~SleepTimer() {}
};

#endif  // _SLEEP_TIMER_H