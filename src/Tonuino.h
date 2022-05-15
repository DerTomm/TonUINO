#ifndef _TONUINOO_H
#define _TONUINOO_H

#include <Arduino.h>
#include <DFMiniMp3.h>
#include <SoftwareSerial.h>
#include "Modifier.h"
#include "AdminSettings.h"

/* BASED ON:
   _____         _____ _____ _____ _____
  |_   _|___ ___|  |  |     |   | |     |
    | | | . |   |  |  |-   -| | | |  |  |
    |_| |___|_|_|_____|_____|_|___|_____|
    TonUINO

    created by Thorsten Voß and licensed under GNU/GPL.
    Information and contribution at https://tonuino.de.
*/

//#define MY_DEBUG

#ifndef MY_DEBUG
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_EPRINT(x, y)
#define DEBUG_EPRINTLN(x, y)
#else
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_EPRINT(x, y) Serial.print(x, y)
#define DEBUG_EPRINTLN(x, y) Serial.print(x, y)
#endif

// Uncomment the below line to enable five button support
//#define FIVEBUTTONS

// Uncomment the below line to stop playback when card is removed
//#define PAUSEONCARDREMOVAL

// delay for volume buttons
#define LONG_PRESS 1000
#define LONG_PRESS_DELAY 300

#define busyPin 4
#define shutdownPin 7
#define amplifierStandbyPin 8
#define openAnalogPin A7

#define BUTTON_DOWN A0
#define BUTTON_PAUSE A1
#define BUTTON_UP A2

#ifdef FIVEBUTTONS
#define buttonFourPin A3
#define buttonFivePin A4
#endif

class NfcHandler;
class LedHandler;
class BatteryHandler;
class Button;
class Tonuino {

 private:
  void playShortCut(uint8_t shortCut);
  static void setupCard();
  static bool askCode(uint8_t *code);
  static void resetCard();
  static bool setupFolder(FolderSettings *theFolder);

  static Button buttonPause;
  static Button buttonUp;
  static Button buttonDown;

  static NfcHandler nfcHandler;

  LedHandler *ledHandler;
  BatteryHandler *batteryHandler;

#ifdef FIVEBUTTONS
  Button buttonFour(buttonFourPin);
  Button buttonFive(buttonFivePin);
#endif
  static bool ignorePauseButton;
  static bool ignoreUpButton;
  static bool ignoreDownButton;
#ifdef FIVEBUTTONS
  bool ignoreButtonFour = false;
  bool ignoreButtonFive = false;
#endif

 public:
  Tonuino();
  ~Tonuino();

  class Mp3NotificationCallback {
   public:
    static void OnError(uint16_t errorCode) {
      // see DfMp3_Error for code meaning
      DEBUG_PRINTLN();
      DEBUG_PRINT("Com Error ");
      DEBUG_PRINTLN(errorCode);
    }
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char *action) {
      if (source & DfMp3_PlaySources_Sd) DEBUG_PRINT("SD Karte ");
      if (source & DfMp3_PlaySources_Usb) DEBUG_PRINT("USB ");
      if (source & DfMp3_PlaySources_Flash) DEBUG_PRINT("Flash ");
      DEBUG_PRINTLN(action);
    }
    static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track) {
      //      DEBUG_PRINT("Track beendet");
      //      DEBUG_PRINTLN(track);
      //      delay(100);
      ptrTonuino->nextTrack(track);
    }
    static void OnPlaySourceOnline(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "online");
      if (source & DfMp3_PlaySources_Sd) {
        ptrTonuino->setStandbyTimer();
      }
    }
    static void OnPlaySourceInserted(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "bereit");
      if (source & DfMp3_PlaySources_Flash) {
        ptrTonuino->disableStandbyTimer();
      }
    }
    static void OnPlaySourceRemoved(DfMp3_PlaySources source) {
      PrintlnSourceAction(source, "entfernt");
      if (source & DfMp3_PlaySources_Flash) {
        ptrTonuino->setStandbyTimer();
      }
    }
  };

  static Tonuino *ptrTonuino;
 
  static void poweroff();
  static void nextTrack(uint16_t track);
  static void disableStandbyTimer();
  static void setStandbyTimer();
  static bool isPlaying();
  static void playFolder();
  static void adminMenu(bool fromCard = false);
  static uint8_t voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
                           bool preview = false, int previewFromFolder = 0, int defaultValue = 0, bool exitWithLongPress = false);
  static bool checkTwo(uint8_t a[], uint8_t b[]);
  static void shuffleQueue();
  static void writeSettingsToFlash();
  static void resetSettings();
  void migrateSettings(int oldVersion);
  void loadSettingsFromFlash();
  void previousTrack();
  static void checkStandbyAtMillis();
  static void waitForTrackToFinish();
  static void readButtons();
  void volumeUpButton();
  void volumeDownButton();
  void nextButton();
  void previousButton();
  static void disableDfplayerAmplifier();
  void enableDfplayerAmplifier();
  void setup();
  void loop();
  static void removeActiveModifier(bool byUser = false);

  static Modifier *activeModifier;
  static NfcTagObject myCard;
  static FolderSettings *myFolder;

  // DFPlayer Mini
  static DFMiniMp3<SoftwareSerial, Mp3NotificationCallback> mp3;
  static uint16_t lastTrackFinished;
  static uint16_t currentTrack;
  static uint16_t numTracksInFolder;
  static uint16_t firstTrack;
  static uint8_t queue[255];
  static uint8_t volume;

  static bool cardKnown;
  static AdminSettings mySettings;  // admin settings stored in eeprom
  static unsigned long sleepAtMillis;
};

#endif  // _TONUINOO_H