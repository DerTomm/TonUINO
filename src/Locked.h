#ifndef _LOCKED_H
#define _LOCKED_H

class Tonuino;
class Locked : public Modifier {
 public:
  bool handlePause() {
    DEBUG_PRINTLN(F("== Locked::handlePause() -> LOCKED!"));
    return true;
  }
  bool handleNextButton() {
    DEBUG_PRINTLN(F("== Locked::handleNextButton() -> LOCKED!"));
    return true;
  }
  bool handlePreviousButton() {
    DEBUG_PRINTLN(F("== Locked::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  bool handleVolumeUp() {
    DEBUG_PRINTLN(F("== Locked::handleVolumeUp() -> LOCKED!"));
    return true;
  }
  bool handleVolumeDown() {
    DEBUG_PRINTLN(F("== Locked::handleVolumeDown() -> LOCKED!"));
    return true;
  }
  bool handleRFID(NfcTagObject *newCard) {
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

#endif  // _LOCKED_H