#ifndef _TONUINO_H
#define _TONUINO_H

#include <Arduino.h>
#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>
#include <FastLED.h>
#include "Modifier.h"
#include "NfcTagObject.h"
#include "Locked.h"
#include "ToddlerMode.h"
#include "FolderSettings.h"
#include "AdminSettings.h"
#include "NfcHandler.h"

// uncomment the below line to enable five button support
//#define FIVEBUTTONS

// uncomment the below line to stop playback when card is removed
//#define PAUSEONCARDREMOVAL

// delay for volume buttons
#define LONG_PRESS_DELAY 300

static const uint32_t cardCookie = 322417479;

// DFPlayer Mini

uint16_t numTracksInFolder;
uint16_t currentTrack;
uint16_t firstTrack;
uint8_t queue[255];
uint8_t volume;

AdminSettings mySettings;  // admin settings stored in eeprom
NfcTagObject myCard;
FolderSettings *myFolder;
unsigned long sleepAtMillis = 0;
static uint16_t _lastTrackFinished;

static void nextTrack(uint16_t track);

uint8_t voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
                  bool preview = false, int previewFromFolder = 0, int defaultValue = 0, bool exitWithLongPress = false);
bool isPlaying();
bool checkTwo(uint8_t a[], uint8_t b[]);
void dump_byte_array(byte *buffer, byte bufferSize);
void adminMenu(bool fromCard = false);
bool knownCard = false;

// Additional members and prototypes

void disablestandbyTimer();
void setstandbyTimer();

void playFolder();

long batteryMeasureTimestamp;

Modifier *activeModifier = NULL;

#define buttonPause A0
#define buttonUp A1
#define buttonDown A2
#define busyPin 4
#define shutdownPin 7
#define amplifierStandbyPin 8
#define openAnalogPin A7

// FastLED
#define DATA_PIN 6
#define NUM_LEDS 1
CRGB leds[NUM_LEDS];

#ifdef FIVEBUTTONS
#define buttonFourPin A3
#define buttonFivePin A4
#endif

#define LONG_PRESS 1000

Button pauseButton(buttonPause);
Button upButton(buttonUp);
Button downButton(buttonDown);
#ifdef FIVEBUTTONS
Button buttonFour(buttonFourPin);
Button buttonFive(buttonFivePin);
#endif
bool ignorePauseButton = false;
bool ignoreUpButton = false;
bool ignoreDownButton = false;
#ifdef FIVEBUTTONS
bool ignoreButtonFour = false;
bool ignoreButtonFive = false;
#endif

void disableDfplayerAmplifier();
void enableDfplayerAmplifier();
void poweroff();
void setStatusLedColor(CRGB color);
void playShortCut(uint8_t shortCut);
void checkBatteryVoltage();
void setupCard();
bool askCode(uint8_t *code);
void resetCard();
bool setupFolder(FolderSettings *theFolder);

class Mp3NotifyCallback {
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
      disablestandbyTimer();
    }
  }
  static void OnPlaySourceRemoved(DfMp3_PlaySources source) {
    PrintlnSourceAction(source, "entfernt");
    if (source & DfMp3_PlaySources_Flash) {
      setstandbyTimer();
    }
  }
};

DFMiniMp3<SoftwareSerial, Mp3NotifyCallback> *mp3;
NfcHandler *nfcHandler;

#endif  // _TONUINO_H