#ifndef _REPEAT_SINGE_MODIFIER_H
#define _REPEAT_SINGE_MODIFIER_H

class Tonuino;
class RepeatSingleModifier : public Modifier {
 public:
  virtual bool handleNext() {
    DEBUG_PRINTLN(F("== RepeatSingleModifier::handleNext() -> REPEAT CURRENT TRACK"));
    delay(50);
    if (Tonuino::isPlaying()) return true;
    Tonuino::mp3.playFolderTrack(Tonuino::myFolder->folder, Tonuino::currentTrack);
    Tonuino::lastTrackFinished = 0;
    return true;
  }
  RepeatSingleModifier() {
    DEBUG_PRINTLN(F("=== RepeatSingleModifier()"));
  }
  uint8_t getActive() {
    DEBUG_PRINTLN(F("== RepeatSingleModifier::getActive()"));
    return 6;
  }
};

#endif // _REPEAT_SINGE_MODIFIER_H