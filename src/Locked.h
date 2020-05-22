#ifndef _LOCKED_H
#define _LOCKED_H

class Tonuino;
class Locked : public Modifier {
 public:
  virtual bool handlePause() {
    DEBUG_PRINTLN(F("== Locked::handlePause() -> LOCKED!"));
    return true;
  }
  virtual bool handleNextButton() {
    DEBUG_PRINTLN(F("== Locked::handleNextButton() -> LOCKED!"));
    return true;
  }
  virtual bool handlePreviousButton() {
    DEBUG_PRINTLN(F("== Locked::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeUp() {
    DEBUG_PRINTLN(F("== Locked::handleVolumeUp() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeDown() {
    DEBUG_PRINTLN(F("== Locked::handleVolumeDown() -> LOCKED!"));
    return true;
  }
  virtual bool handleRFID(NfcTagObject *newCard) {
    DEBUG_PRINTLN(F("== Locked::handleRFID() -> LOCKED!"));
    return true;
  }
  Locked(void) {
    DEBUG_PRINTLN(F("=== Locked()"));
    //      if (isPlaying())
    //        mp3.playAdvertisement(303);
  }
  uint8_t getActive() {
    return 3;
  }
};

#endif // _LOCKED_H