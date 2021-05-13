#ifndef _KINDERGARDEN_MODE_H
#define _KINDERGARDEN_MODE_H

class Tonuino;
class KindergardenMode : public Modifier {
 private:
  NfcTagObject nextCard;
  bool cardQueued = false;

 public:
  virtual bool handleNext() {
    DEBUG_PRINTLN(F("== KindergardenMode::handleNext() -> NEXT"));
    //if (this->nextCard.cookie == cardCookie && this->nextCard.nfcFolderSettings.folder != 0 && this->nextCard.nfcFolderSettings.mode != 0) {
    //myFolder = &this->nextCard.nfcFolderSettings;
    if (this->cardQueued == true) {
      this->cardQueued = false;

      Tonuino::myCard = nextCard;
      Tonuino::myFolder = &Tonuino::myCard.nfcFolderSettings;
      DEBUG_PRINTLN(Tonuino::myFolder->folder);
      DEBUG_PRINTLN(Tonuino::myFolder->mode);
      Tonuino::playFolder();
      return true;
    }
    return false;
  }
  //    virtual bool handlePause()     {
  //      DEBUG_PRINTLN(F("== KindergardenMode::handlePause() -> LOCKED!"));
  //      return true;
  //    }
  bool handleNextButton() {
    DEBUG_PRINTLN(F("== KindergardenMode::handleNextButton() -> LOCKED!"));
    return true;
  }
  bool handlePreviousButton() {
    DEBUG_PRINTLN(F("== KindergardenMode::handlePreviousButton() -> LOCKED!"));
    return true;
  }
  bool handleRFID(NfcTagObject *newCard) {  // lot of work to do!
    DEBUG_PRINTLN(F("== KindergardenMode::handleRFID() -> queued!"));
    this->nextCard = *newCard;
    this->cardQueued = true;
    if (!Tonuino::isPlaying()) {
      handleNext();
    }
    return true;
  }
  KindergardenMode() {
    DEBUG_PRINTLN(F("=== KindergardenMode()"));
    //      if (isPlaying())
    //        mp3.playAdvertisement(305);
    //      delay(500);
  }
  uint8_t getActive() {
    DEBUG_PRINTLN(F("== KindergardenMode::getActive()"));
    return 5;
  }
};

#endif  // _KINDERGARDEN_MODE_H