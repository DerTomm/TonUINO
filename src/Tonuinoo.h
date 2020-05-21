#ifndef _TONUINOO_H
#define _TONUINOO_H

#include <Arduino.h>
#include <DFMiniMp3.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <FastLED.h>
#include <avr/sleep.h>
#include "NfcHandler.h"
#include "Modifier.h"
#include "AdminSettings.h"

// Uncomment the below line to enable five button support
//#define FIVEBUTTONS

// Uncomment the below line to stop playback when card is removed
//#define PAUSEONCARDREMOVAL

// delay for volume buttons
#define LONG_PRESS_DELAY 300

class Tonuinoo {

 private:
  NfcHandler *nfcHandler;

 public:
  Tonuinoo();
  ~Tonuinoo();

  class Mp3NotificationCallback {
   public:
    static void OnError(uint16_t errorCode) {
      // see DfMp3_Error for code meaning
      Serial.println();
      Serial.print("Com Error ");
      Serial.println(errorCode);
    }
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char *action) {
      if (source & DfMp3_PlaySources_Sd) Serial.print("SD Karte ");
      if (source & DfMp3_PlaySources_Usb) Serial.print("USB ");
      if (source & DfMp3_PlaySources_Flash) Serial.print("Flash ");
      Serial.println(action);
    }
    static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track) {
      //      Serial.print("Track beendet");
      //      Serial.println(track);
      //      delay(100);
      nextTrack(track);
    }
    static void OnPlaySourceOnline(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "online");
    }
    static void OnPlaySourceInserted(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "bereit");
      if (source & DfMp3_PlaySources_Flash) {
        disableStandbyTimer();
      }
    }
    static void OnPlaySourceRemoved(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "entfernt");
      if (source & DfMp3_PlaySources_Flash) {
        setStandbyTimer();
      }
    }
  };

  static void nextTrack(uint16_t track);
  static void disableStandbyTimer();
  static void setStandbyTimer();
  static bool isPlaying();
  static void playFolder();
  static void adminMenu(bool fromCard = false);

  static Modifier *activeModifier;
  static NfcTagObject myCard;
  static FolderSettings *myFolder;

  // DFPlayer Mini
  static DFMiniMp3<SoftwareSerial, Mp3NotificationCallback> *mp3;
  static uint16_t lastTrackFinished;
  static uint16_t currentTrack;
  uint16_t numTracksInFolder;
  uint16_t firstTrack;
  uint8_t queue[255];
  uint8_t volume;

  AdminSettings mySettings;  // admin settings stored in eeprom
  unsigned long sleepAtMillis = 0;
};

#endif  // _TONUINOO_H