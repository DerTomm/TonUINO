#ifndef _REPEAT_SINGE_MODIFIER_H
#define _REPEAT_SINGE_MODIFIER_H

class Tonuinoo;
class RepeatSingleModifier : public Modifier {
 public:
  virtual bool handleNext() {
    Serial.println(F("== RepeatSingleModifier::handleNext() -> REPEAT CURRENT TRACK"));
    delay(50);
    if (Tonuinoo::isPlaying()) return true;
    Tonuinoo::mp3->playFolderTrack(Tonuinoo::myFolder->folder, Tonuinoo::currentTrack);
    Tonuinoo::lastTrackFinished = 0;
    return true;
  }
  RepeatSingleModifier() {
    Serial.println(F("=== RepeatSingleModifier()"));
  }
  uint8_t getActive() {
    Serial.println(F("== RepeatSingleModifier::getActive()"));
    return 6;
  }
};

#endif // _REPEAT_SINGE_MODIFIER_H