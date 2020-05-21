#ifndef _FEEDBACK_MODIFIER_H
#define _FEEDBACK_MODIFIER_H

#include "Tonuino.h"

// An modifier can also do somethings in addition to the modified action
// by returning false (not handled) at the end
// This simple FeedbackModifier will tell the volume before changing it and
// give some feedback once a RFID card is detected.
class FeedbackModifier : public Modifier {
 public:
  virtual bool handleVolumeDown() {
    if (volume > mySettings.minVolume) {
      mp3.playAdvertisement(volume - 1);
    } else {
      mp3.playAdvertisement(volume);
    }
    delay(500);
    Serial.println(F("== FeedbackModifier::handleVolumeDown()!"));
    return false;
  }
  virtual bool handleVolumeUp() {
    if (volume < mySettings.maxVolume) {
      mp3.playAdvertisement(volume + 1);
    } else {
      mp3.playAdvertisement(volume);
    }
    delay(500);
    Serial.println(F("== FeedbackModifier::handleVolumeUp()!"));
    return false;
  }
  virtual bool handleRFID(NfcTagObject *newCard) {
    Serial.println(F("== FeedbackModifier::handleRFID()"));
    return false;
  }
};

#endif // _FEEDBACK_MODIFIER_H