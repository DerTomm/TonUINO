#include "NfcHandler.h"
#include "Tonuino.h"
#include "FreezeDance.h"
#include "KindergardenMode.h"
#include "RepeatSingleModifier.h"
#include "SleepTimer.h"
#include "Locked.h"
#include "ToddlerMode.h"

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::readCard(NfcTagObject *nfcTag) {
  NfcTagObject tempCard;
  // Show some details of the PICC (that is: the tag/card)
  DEBUG_PRINT(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  DEBUG_PRINTLN();
  DEBUG_PRINT(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  DEBUG_PRINTLN(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using key A
  if ((piccType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    DEBUG_PRINTLN(F("Authenticating Classic using key A..."));
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  } else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte pACK[] = {0, 0};  //16 bit PassWord ACK returned by the tempCard

    // Authenticate using key A
    DEBUG_PRINTLN(F("Authenticating MIFARE UL..."));
    status = mfrc522.PCD_NTAG216_AUTH(key.keyByte, pACK);
  }

  if (status != MFRC522::STATUS_OK) {
    DEBUG_PRINT(F("PCD_Authenticate() failed: "));
    DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Show the whole sector as it currently is
  // DEBUG_PRINTLN(F("Current data in sector:"));
  // mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  // DEBUG_PRINTLN();

  // Read data from the block
  if ((piccType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    DEBUG_PRINT(F("Reading data from block "));
    DEBUG_PRINT(blockAddr);
    DEBUG_PRINTLN(F(" ..."));
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      DEBUG_PRINT(F("MIFARE_Read() failed: "));
      DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
  } else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte buffer2[18];
    byte size2 = sizeof(buffer2);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(8, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      DEBUG_PRINT(F("MIFARE_Read_1() failed: "));
      DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(9, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      DEBUG_PRINT(F("MIFARE_Read_2() failed: "));
      DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 4, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(10, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      DEBUG_PRINT(F("MIFARE_Read_3() failed: "));
      DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 8, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(11, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      DEBUG_PRINT(F("MIFARE_Read_4() failed: "));
      DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 12, buffer2, 4);
  }

  DEBUG_PRINT(F("Data on Card "));
  DEBUG_PRINTLN(F(":"));
  dump_byte_array(buffer, 16);
  DEBUG_PRINTLN();
  DEBUG_PRINTLN();

  uint32_t tempCookie;
  tempCookie = (uint32_t)buffer[0] << 24;
  tempCookie += (uint32_t)buffer[1] << 16;
  tempCookie += (uint32_t)buffer[2] << 8;
  tempCookie += (uint32_t)buffer[3];

  tempCard.cookie = tempCookie;
  tempCard.version = buffer[4];
  tempCard.nfcFolderSettings.folder = buffer[5];
  tempCard.nfcFolderSettings.mode = buffer[6];
  tempCard.nfcFolderSettings.special = buffer[7];
  tempCard.nfcFolderSettings.special2 = buffer[8];

  if (tempCard.cookie == cardCookie) {

    if (Tonuino::activeModifier != NULL && tempCard.nfcFolderSettings.folder != 0) {
      if (Tonuino::activeModifier->handleRFID(&tempCard) == true) {
        return false;
      }
    }

    if (tempCard.nfcFolderSettings.folder == 0) {
      if (Tonuino::activeModifier != NULL) {
        if (Tonuino::activeModifier->getActive() == tempCard.nfcFolderSettings.mode) {
          Tonuino::removeActiveModifier(true);
          DEBUG_PRINTLN(F("modifier removed"));
          if (Tonuino::isPlaying()) {
            Tonuino::mp3.playAdvertisement(261);
          } else {
            Tonuino::mp3.start();
            delay(100);
            Tonuino::mp3.playAdvertisement(261);
            delay(100);
            Tonuino::mp3.pause();
          }
          delay(2000);
          return false;
        }
      }
      if (tempCard.nfcFolderSettings.mode != 0 && tempCard.nfcFolderSettings.mode != 255) {
        if (Tonuino::isPlaying()) {
          Tonuino::mp3.playAdvertisement(260);
        } else {
          Tonuino::mp3.start();
          delay(100);
          Tonuino::mp3.playAdvertisement(260);
          delay(100);
          Tonuino::mp3.pause();
        }
      }
      switch (tempCard.nfcFolderSettings.mode) {
        case 0:
        case 255:
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          Tonuino::adminMenu(true);
          break;
        case 1:
          Tonuino::activeModifier = new SleepTimer(tempCard.nfcFolderSettings.special);
          break;
        case 2:
          Tonuino::activeModifier = new FreezeDance();
          break;
        case 3:
          Tonuino::activeModifier = new Locked();
          break;
        case 4:
          Tonuino::activeModifier = new ToddlerMode();
          break;
        case 5:
          Tonuino::activeModifier = new KindergardenMode();
          break;
        case 6:
          Tonuino::activeModifier = new RepeatSingleModifier();
          break;
      }
      delay(2000);
      return false;
    } else {
      memcpy(nfcTag, &tempCard, sizeof(NfcTagObject));
      DEBUG_PRINTLN(nfcTag->nfcFolderSettings.folder);
      Tonuino::myFolder = &nfcTag->nfcFolderSettings;
      DEBUG_PRINTLN(Tonuino::myFolder->folder);
    }
    return true;
  } else {
    memcpy(nfcTag, &tempCard, sizeof(NfcTagObject));
    return true;
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::writeCard(NfcTagObject nfcTag) {
  MFRC522::PICC_Type mifareType;
  byte buffer[16] = {0x13, 0x37, 0xb3, 0x47,  // 0x1337 0xb347 magic cookie to
                     // identify our nfc tags
                     0x02,                              // version 1
                     nfcTag.nfcFolderSettings.folder,   // the folder picked by the user
                     nfcTag.nfcFolderSettings.mode,     // the playback mode picked by the user
                     nfcTag.nfcFolderSettings.special,  // track or function for admin cards
                     nfcTag.nfcFolderSettings.special2,
                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);

  // Authenticate using key B
  //authentificate with the card and set card specific parameters
  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    DEBUG_PRINTLN(F("Authenticating again using key A..."));
    status = mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  } else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte pACK[] = {0, 0};  //16 bit PassWord ACK returned by the NFCtag

    // Authenticate using key A
    DEBUG_PRINTLN(F("Authenticating UL..."));
    status = mfrc522.PCD_NTAG216_AUTH(key.keyByte, pACK);
  }

  if (status != MFRC522::STATUS_OK) {
    DEBUG_PRINT(F("PCD_Authenticate() failed: "));
    DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
    Tonuino::mp3.playMp3FolderTrack(401);
    return;
  }

  // Write data to the block
  DEBUG_PRINT(F("Writing data into block "));
  DEBUG_PRINT(blockAddr);
  DEBUG_PRINTLN(F(" ..."));
  dump_byte_array(buffer, 16);
  DEBUG_PRINTLN();

  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, buffer, 16);
  } else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte buffer2[16];
    byte size2 = sizeof(buffer2);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(8, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 4, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(9, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 8, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(10, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 12, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(11, buffer2, 16);
  }

  if (status != MFRC522::STATUS_OK) {
    DEBUG_PRINT(F("MIFARE_Write() failed: "));
    DEBUG_PRINTLN(mfrc522.GetStatusCodeName(status));
    Tonuino::mp3.playMp3FolderTrack(401);
  } else
    Tonuino::mp3.playMp3FolderTrack(400);
  DEBUG_PRINTLN();
  delay(2000);
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::sleep() {
  DEBUG_PRINTLN("NfcHandler.sleep()");
  mfrc522.PCD_AntennaOff();
  mfrc522.PCD_SoftPowerDown();
}

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::isNewCardPresent() {
  return mfrc522.PICC_IsNewCardPresent();
}

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::readCardSerial() {
  return mfrc522.PICC_ReadCardSerial();
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::halt() {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::init() {
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_38dB);
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    DEBUG_PRINT(buffer[i] < 0x10 ? " 0" : " ");
    DEBUG_EPRINT(buffer[i], HEX);
  }
}