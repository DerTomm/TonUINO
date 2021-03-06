#ifndef _NFC_HANDLER_H
#define _NFC_HANDLER_H

#define RST_PIN 9
#define SS_PIN 10

#include <MFRC522.h>
#include "NfcTagObject.h"

class Tonuino;
class NfcHandler {

 public:
  NfcHandler() : mfrc522(SS_PIN, RST_PIN) {};
  ~NfcHandler() {};

  bool readCard(NfcTagObject *nfcTag);
  void writeCard(NfcTagObject nfcTag);
  void sleep();
  bool isNewCardPresent();
  bool readCardSerial();
  void halt();
  void init();

  static const uint32_t cardCookie = 322417479;

 private:
  void dump_byte_array(byte *buffer, byte bufferSize);

  MFRC522 mfrc522;
  MFRC522::MIFARE_Key key;
  bool successRead;
  byte sector = 1;
  byte blockAddr = 4;
  byte trailerBlock = 7;
  MFRC522::StatusCode status;
  bool card_present = false;
  bool card_present_prev = false;
};

#endif  // _NFC_HANDLER_H