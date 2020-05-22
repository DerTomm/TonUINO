#ifndef _TODDLER_MODE_H
#define _TODDLER_MODE_H

class Tonuino;
class ToddlerMode : public Modifier {
 public:
  virtual bool handlePause() {
    DEBUG_PRINTLN(F("== ToddlerMode::handlePause() -> LOCKED!"));
    return true;
  }
  virtual bool handleNextButton() {
    DEBUG_PRINTLN(F("== ToddlerMode::handleNextButton() -> LOCKED!"));
    return true;
  }
  virtual bool handlePreviousButton() {
    DEBUG_PRINTLN(F("== ToddlerMode::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeUp() {
    DEBUG_PRINTLN(F("== ToddlerMode::handleVolumeUp() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeDown() {
    DEBUG_PRINTLN(F("== ToddlerMode::handleVolumeDown() -> LOCKED!"));
    return true;
  }
  ToddlerMode(void) {
    DEBUG_PRINTLN(F("=== ToddlerMode()"));
    //      if (isPlaying())
    //        mp3.playAdvertisement(304);
  }
  uint8_t getActive() {
    DEBUG_PRINTLN(F("== ToddlerMode::getActive()"));
    return 4;
  }
};

#endif // _TODDLER_MODE_H