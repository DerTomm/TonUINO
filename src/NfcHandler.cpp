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
NfcHandler::NfcHandler() {
  mfrc522 = new MFRC522(SS_PIN, RST_PIN);
}

/**************************************************************************************************************************************************************
 * 
 */
NfcHandler::~NfcHandler() {}

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::readCard(NfcTagObject *nfcTag) {
  NfcTagObject tempCard;
  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522->uid.uidByte, mfrc522->uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522->PICC_GetType(mfrc522->uid.sak);
  Serial.println(mfrc522->PICC_GetTypeName(piccType));

  byte buffer[18];
  byte size = sizeof(buffer);

  // Authenticate using key A
  if ((piccType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    Serial.println(F("Authenticating Classic using key A..."));
    status = mfrc522->PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522->uid));
  } else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte pACK[] = {0, 0};  //16 bit PassWord ACK returned by the tempCard

    // Authenticate using key A
    Serial.println(F("Authenticating MIFARE UL..."));
    status = mfrc522->PCD_NTAG216_AUTH(key.keyByte, pACK);
  }

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522->GetStatusCodeName(status));
    return false;
  }

  // Show the whole sector as it currently is
  // Serial.println(F("Current data in sector:"));
  // mfrc522->PICC_DumpMifareClassicSectorToSerial(&(mfrc522->uid), &key, sector);
  // Serial.println();

  // Read data from the block
  if ((piccType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    Serial.print(F("Reading data from block "));
    Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522->GetStatusCodeName(status));
      return false;
    }
  } else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte buffer2[18];
    byte size2 = sizeof(buffer2);

    status = (MFRC522::StatusCode)mfrc522->MIFARE_Read(8, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_1() failed: "));
      Serial.println(mfrc522->GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522->MIFARE_Read(9, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_2() failed: "));
      Serial.println(mfrc522->GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 4, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522->MIFARE_Read(10, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_3() failed: "));
      Serial.println(mfrc522->GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 8, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522->MIFARE_Read(11, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_4() failed: "));
      Serial.println(mfrc522->GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 12, buffer2, 4);
  }

  Serial.print(F("Data on Card "));
  Serial.println(F(":"));
  dump_byte_array(buffer, 16);
  Serial.println();
  Serial.println();

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
          Tonuino::activeModifier = NULL;
          Serial.println(F("modifier removed"));
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
          mfrc522->PICC_HaltA();
          mfrc522->PCD_StopCrypto1();
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
      Serial.println(nfcTag->nfcFolderSettings.folder);
      Tonuino::myFolder = &nfcTag->nfcFolderSettings;
      Serial.println(Tonuino::myFolder->folder);
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

  mifareType = mfrc522->PICC_GetType(mfrc522->uid.sak);

  // Authenticate using key B
  //authentificate with the card and set card specific parameters
  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    Serial.println(F("Authenticating again using key A..."));
    status = mfrc522->PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522->uid));
  } else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte pACK[] = {0, 0};  //16 bit PassWord ACK returned by the NFCtag

    // Authenticate using key A
    Serial.println(F("Authenticating UL..."));
    status = mfrc522->PCD_NTAG216_AUTH(key.keyByte, pACK);
  }

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522->GetStatusCodeName(status));
    Tonuino::mp3.playMp3FolderTrack(401);
    return;
  }

  // Write data to the block
  Serial.print(F("Writing data into block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dump_byte_array(buffer, 16);
  Serial.println();

  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K)) {
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Write(blockAddr, buffer, 16);
  } else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL) {
    byte buffer2[16];
    byte size2 = sizeof(buffer2);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer, 4);
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Write(8, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 4, 4);
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Write(9, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 8, 4);
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Write(10, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 12, 4);
    status = (MFRC522::StatusCode)mfrc522->MIFARE_Write(11, buffer2, 16);
  }

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522->GetStatusCodeName(status));
    Tonuino::mp3.playMp3FolderTrack(401);
  } else
    Tonuino::mp3.playMp3FolderTrack(400);
  Serial.println();
  delay(2000);
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::sleep() {
  Serial.println("NfcHandler.sleep()");
  mfrc522->PCD_AntennaOff();
  mfrc522->PCD_SoftPowerDown();
}

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::isNewCardPresent() {
  return mfrc522->PICC_IsNewCardPresent();
}

/**************************************************************************************************************************************************************
 * 
 */
bool NfcHandler::readCardSerial() {
  return mfrc522->PICC_ReadCardSerial();
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::halt() {
  mfrc522->PICC_HaltA();
  mfrc522->PCD_StopCrypto1();
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::init() {
  SPI.begin();                         // Init SPI bus
  mfrc522->PCD_Init();                 // Init MFRC522
  mfrc522->PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void NfcHandler::dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}