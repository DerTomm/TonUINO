#ifndef _NFC_TAG_OBJECT_H
#define _NFC_TAG_OBJECT_H

#include "FolderSettings.h"

class NfcTagObject {
 public:
  uint32_t cookie;
  uint8_t version;
  FolderSettings nfcFolderSettings;
  //  uint8_t folder;
  //  uint8_t mode;
  //  uint8_t special;
  //  uint8_t special2;
};

#endif // _NFC_TAG_OBJECT_H