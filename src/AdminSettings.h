#ifndef _ADMIN_SETTINGS_H
#define _ADMIN_SETTINGS_H

#include <Arduino.h>
#include "FolderSettings.h"

class Tonuinoo;
class AdminSettings {

 public:
  uint32_t cookie;
  byte version;
  uint8_t maxVolume;
  uint8_t minVolume;
  uint8_t initVolume;
  uint8_t eq;
  bool locked;
  long standbyTimer;
  bool invertVolumeButtons;
  FolderSettings shortCuts[4];
  uint8_t adminMenuLocked;
  uint8_t adminMenuPin[4];

};

#endif // _ADMIN_SETTINGS_H