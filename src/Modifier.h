#ifndef _MODIFIER_H
#define _MODIFIER_H

#include "NfcTagObject.h"

class Modifier {

 public:
  virtual void loop() {}
  virtual bool handlePause() {
    return false;
  }
  virtual bool handleNext() {
    return false;
  }
  virtual bool handlePrevious() {
    return false;
  }
  virtual bool handleNextButton() {
    return false;
  }
  virtual bool handlePreviousButton() {
    return false;
  }
  virtual bool handleVolumeUp() {
    return false;
  }
  virtual bool handleVolumeDown() {
    return false;
  }
  virtual bool handleRFID(NfcTagObject *newCard) {
    return false;
  }
  virtual uint8_t getActive() {
    return 0;
  }

  Modifier() {}
  virtual ~Modifier() {};
};

#endif  // _MODIFIER_H