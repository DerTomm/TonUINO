#ifndef _LOCKED_H
#define _LOCKED_H

#include "Tonuino.h"

class Locked : public Modifier {
 public:
  virtual bool handlePause() {
    Serial.println(F("== Locked::handlePause() -> LOCKED!"));
    return true;
  }
  virtual bool handleNextButton() {
    Serial.println(F("== Locked::handleNextButton() -> LOCKED!"));
    return true;
  }
  virtual bool handlePreviousButton() {
    Serial.println(F("== Locked::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeUp() {
    Serial.println(F("== Locked::handleVolumeUp() -> LOCKED!"));
    return true;
  }
  virtual bool handleVolumeDown() {
    Serial.println(F("== Locked::handleVolumeDown() -> LOCKED!"));
    return true;
  }
  virtual bool handleRFID(NfcTagObject *newCard) {
    Serial.println(F("== Locked::handleRFID() -> LOCKED!"));
    return true;
  }
  Locked(void) {
    Serial.println(F("=== Locked()"));
    //      if (isPlaying())
    //        mp3.playAdvertisement(303);
  }
  uint8_t getActive() {
    return 3;
  }
};

#endif // _LOCKED_H