#ifndef _FOLDER_SETTINGS_H
#define _FOLDER_SETTINGS_H

#include <Arduino.h>

class FolderSettings {
 public:
  uint8_t folder;
  uint8_t mode;
  uint8_t special;
  uint8_t special2;
};

#endif // _FOLDER_SETTINGS_H