#ifndef _TODDLER_MODE_H
#define _TODDLER_MODE_H

class Tonuinoo;
class ToddlerMode : public Modifier {
 public:
  virtual bool handlePause() {
    Serial.println(F("== ToddlerMode::handlePause() -> LOCKED!"));
    return true;
  }
  virtual bool handleNextButton() {
    Serial.println(F("== ToddlerMode::handleNextButton() -> LOCKED!"));
    return true;
  }
  virtual bool handlePreviousButton() {
    Serial.println(F("== ToddlerMode::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeUp() {
    Serial.println(F("== ToddlerMode::handleVolumeUp() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeDown() {
    Serial.println(F("== ToddlerMode::handleVolumeDown() -> LOCKED!"));
    return true;
  }
  ToddlerMode(void) {
    Serial.println(F("=== ToddlerMode()"));
    //      if (isPlaying())
    //        mp3.playAdvertisement(304);
  }
  uint8_t getActive() {
    Serial.println(F("== ToddlerMode::getActive()"));
    return 4;
  }
};

#endif // _TODDLER_MODE_H