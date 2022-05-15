#include <EEPROM.h>
#include <JC_Button.h>
#include <avr/sleep.h>
#include "Tonuino.h"
#include "NfcHandler.h"
#include "LedHandler.h"
#include "BatteryHandler.h"

/*
 * Declaration of static members
 */
SoftwareSerial mySoftwareSerial(2, 3);  // RX, TX
DFMiniMp3<SoftwareSerial, Tonuino::Mp3NotificationCallback> Tonuino::mp3(mySoftwareSerial);
NfcTagObject Tonuino::myCard;
FolderSettings* Tonuino::myFolder;
Modifier* Tonuino::activeModifier;
bool Tonuino::cardKnown = false;
unsigned long Tonuino::sleepAtMillis = 0;
NfcHandler Tonuino::nfcHandler;
Button Tonuino::buttonPause(BUTTON_PAUSE);
Button Tonuino::buttonUp(BUTTON_UP);
Button Tonuino::buttonDown(BUTTON_DOWN);
bool Tonuino::ignorePauseButton = false;
bool Tonuino::ignoreUpButton = false;
bool Tonuino::ignoreDownButton = false;
uint16_t Tonuino::lastTrackFinished;
uint16_t Tonuino::currentTrack;
uint16_t Tonuino::numTracksInFolder;
uint16_t Tonuino::firstTrack;
uint8_t Tonuino::queue[255];
uint8_t Tonuino::volume;
AdminSettings Tonuino::mySettings;
Tonuino* Tonuino::ptrTonuino;

/**************************************************************************************************************************************************************
 * 
 */
Tonuino::Tonuino() {
  ptrTonuino = this;
  ledHandler = new LedHandler();
  batteryHandler = new BatteryHandler(this, ledHandler);
}

/**************************************************************************************************************************************************************
 * 
 */
Tonuino::~Tonuino() {
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::removeActiveModifier(bool byUser) {
  if (byUser) {
    LedHandler::resetBrightness();  // reset LED brightness if modifier (e.g. SleepTimer) is removed on demand by user
  }
  delete Tonuino::activeModifier;
  Tonuino::activeModifier = NULL;
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::shuffleQueue() {
  // Queue für die Zufallswiedergabe erstellen
  for (uint8_t x = 0; x < numTracksInFolder - firstTrack + 1; x++)
    queue[x] = x + firstTrack;
  // Rest mit 0 auffüllen
  for (uint8_t x = numTracksInFolder - firstTrack + 1; x < 255; x++)
    queue[x] = 0;
  // Queue mischen
  for (uint8_t i = 0; i < numTracksInFolder - firstTrack + 1; i++) {
    uint8_t j = random(0, numTracksInFolder - firstTrack + 1);
    uint8_t t = queue[i];
    queue[i] = queue[j];
    queue[j] = t;
  }
  /*  DEBUG_PRINTLN(F("Queue :"));
    for (uint8_t x = 0; x < numTracksInFolder - firstTrack + 1 ; x++)
      DEBUG_PRINTLN(queue[x]);
  */
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::writeSettingsToFlash() {
  DEBUG_PRINTLN(F("=== writeSettingsToFlash()"));
  int address = sizeof(myFolder->folder) * 100;
  EEPROM.put(address, mySettings);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::resetSettings() {
  DEBUG_PRINTLN(F("=== resetSettings()"));
  mySettings.cookie = NfcHandler::cardCookie;
  mySettings.version = 2;
  mySettings.maxVolume = 25;
  mySettings.minVolume = 5;
  mySettings.initVolume = 15;
  mySettings.eq = 1;
  mySettings.locked = false;
  mySettings.standbyTimer = 0;
  mySettings.invertVolumeButtons = false;
  mySettings.shortCuts[0].folder = 0;
  mySettings.shortCuts[1].folder = 0;
  mySettings.shortCuts[2].folder = 0;
  mySettings.shortCuts[3].folder = 0;
  mySettings.adminMenuLocked = 0;
  mySettings.adminMenuPin[0] = 1;
  mySettings.adminMenuPin[1] = 1;
  mySettings.adminMenuPin[2] = 1;
  mySettings.adminMenuPin[3] = 1;
  writeSettingsToFlash();
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::migrateSettings(int oldVersion) {
  if (oldVersion == 1) {
    DEBUG_PRINTLN(F("=== resetSettings()"));
    DEBUG_PRINTLN(F("1 -> 2"));
    mySettings.version = 2;
    mySettings.adminMenuLocked = 0;
    mySettings.adminMenuPin[0] = 1;
    mySettings.adminMenuPin[1] = 1;
    mySettings.adminMenuPin[2] = 1;
    mySettings.adminMenuPin[3] = 1;
    writeSettingsToFlash();
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::loadSettingsFromFlash() {
  DEBUG_PRINTLN(F("=== loadSettingsFromFlash()"));
  int address = sizeof(myFolder->folder) * 100;
  EEPROM.get(address, mySettings);
  if (mySettings.cookie != NfcHandler::cardCookie)
    resetSettings();
  migrateSettings(mySettings.version);

  DEBUG_PRINT(F("Version: "));
  DEBUG_PRINTLN(mySettings.version);

  DEBUG_PRINT(F("Maximal Volume: "));
  DEBUG_PRINTLN(mySettings.maxVolume);

  DEBUG_PRINT(F("Minimal Volume: "));
  DEBUG_PRINTLN(mySettings.minVolume);

  DEBUG_PRINT(F("Initial Volume: "));
  DEBUG_PRINTLN(mySettings.initVolume);

  DEBUG_PRINT(F("EQ: "));
  DEBUG_PRINTLN(mySettings.eq);

  DEBUG_PRINT(F("Locked: "));
  DEBUG_PRINTLN(mySettings.locked);

  DEBUG_PRINT(F("Sleep Timer: "));
  DEBUG_PRINTLN(mySettings.standbyTimer);

  DEBUG_PRINT(F("Inverted Volume Buttons: "));
  DEBUG_PRINTLN(mySettings.invertVolumeButtons);

  DEBUG_PRINT(F("Admin Menu locked: "));
  DEBUG_PRINTLN(mySettings.adminMenuLocked);

  DEBUG_PRINT(F("Admin Menu Pin: "));
  DEBUG_PRINT(mySettings.adminMenuPin[0]);
  DEBUG_PRINT(mySettings.adminMenuPin[1]);
  DEBUG_PRINT(mySettings.adminMenuPin[2]);
  DEBUG_PRINTLN(mySettings.adminMenuPin[3]);
}

/**************************************************************************************************************************************************************
 * Leider kann das Modul selbst keine Queue abspielen, daher müssen wir selbst die Queue verwalten
 */
void Tonuino::nextTrack(uint16_t track) {
  DEBUG_PRINTLN(track);
  if (activeModifier != NULL)
    if (activeModifier->handleNext() == true)
      return;

  if (track == lastTrackFinished) {
    return;
  }
  lastTrackFinished = track;

  if (cardKnown == false)
    // Wenn eine neue Karte angelernt wird soll das Ende eines Tracks nicht
    // verarbeitet werden
    return;

  DEBUG_PRINTLN(F("=== nextTrack()"));

  if (myFolder->mode == 1 || myFolder->mode == 7) {
    DEBUG_PRINTLN(F("Hörspielmodus ist aktiv -> keinen neuen Track spielen"));
    setStandbyTimer();
    //    mp3.sleep(); // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
  }
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      mp3.playFolderTrack(myFolder->folder, currentTrack);
      DEBUG_PRINT(F("Albummodus ist aktiv -> nächster Track: "));
      DEBUG_PRINT(currentTrack);
    } else
      //      mp3.sleep();   // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      setStandbyTimer();
    {}
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != numTracksInFolder - firstTrack + 1) {
      DEBUG_PRINT(F("Party -> weiter in der Queue "));
      currentTrack++;
    } else {
      DEBUG_PRINTLN(F("Ende der Queue -> beginne von vorne"));
      currentTrack = 1;
      //// Wenn am Ende der Queue neu gemischt werden soll bitte die Zeilen wieder aktivieren
      //     DEBUG_PRINTLN(F("Ende der Queue -> mische neu"));
      //     shuffleQueue();
    }
    DEBUG_PRINTLN(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }

  if (myFolder->mode == 4) {
    DEBUG_PRINTLN(F("Einzel Modus aktiv -> Strom sparen"));
    //    mp3.sleep();      // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
    setStandbyTimer();
  }
  if (myFolder->mode == 5) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      DEBUG_PRINT(F(
          "Hörbuch Modus ist aktiv -> nächster Track und "
          "Fortschritt speichern"));
      DEBUG_PRINTLN(currentTrack);
      mp3.playFolderTrack(myFolder->folder, currentTrack);
      // Fortschritt im EEPROM abspeichern
      EEPROM.update(myFolder->folder, currentTrack);
    } else {
      //      mp3.sleep();  // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      // Fortschritt zurück setzen
      EEPROM.update(myFolder->folder, 1);
      setStandbyTimer();
    }
  }
  delay(500);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::previousTrack() {
  DEBUG_PRINTLN(F("=== previousTrack()"));
  /*  if (myCard.mode == 1 || myCard.mode == 7) {
      DEBUG_PRINTLN(F("Hörspielmodus ist aktiv -> Track von vorne spielen"));
      mp3.playFolderTrack(myCard.folder, currentTrack);
    }*/
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    DEBUG_PRINTLN(F("Albummodus ist aktiv -> vorheriger Track"));
    if (currentTrack != firstTrack) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != 1) {
      DEBUG_PRINT(F("Party Modus ist aktiv -> zurück in der Qeueue "));
      currentTrack--;
    } else {
      DEBUG_PRINT(F("Anfang der Queue -> springe ans Ende "));
      currentTrack = numTracksInFolder;
    }
    DEBUG_PRINTLN(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  if (myFolder->mode == 4) {
    DEBUG_PRINTLN(F("Einzel Modus aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 5) {
    DEBUG_PRINTLN(F(
        "Hörbuch Modus ist aktiv -> vorheriger Track und "
        "Fortschritt speichern"));
    if (currentTrack != 1) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
    // Fortschritt im EEPROM abspeichern
    EEPROM.update(myFolder->folder, currentTrack);
  }
  delay(1000);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::setStandbyTimer() {
  DEBUG_PRINTLN(F("=== setStandbyTimer()"));
  if (mySettings.standbyTimer != 0)
    sleepAtMillis = millis() + (mySettings.standbyTimer * 60 * 1000);
  else
    sleepAtMillis = 0;
  DEBUG_PRINTLN(sleepAtMillis);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::disableStandbyTimer() {
  DEBUG_PRINTLN(F("=== disablestandby()"));
  sleepAtMillis = 0;
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::checkStandbyAtMillis() {
  if (sleepAtMillis != 0 && millis() > sleepAtMillis) {
    poweroff();
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::poweroff() {

  mp3.pause();

  /*
   * Hardware shutdown by MOSFET
   */
  DEBUG_PRINTLN(F("=== power off!"));
  ptrTonuino->ledHandler->setStatusLedColor(CRGB::Black);
  // enter sleep state
  disableDfplayerAmplifier();
  delay(500);
  digitalWrite(shutdownPin, LOW);
  delay(500);

  /*
   * Fallback: Software shutdown
   */

  // http://discourse.voss.earth/t/intenso-s10000-powerbank-automatische-abschaltung-software-only/805
  // powerdown to 27mA (powerbank switches off after 30-60s)
  nfcHandler.sleep();
  mp3.sleep();

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  cli();  // Disable interrupts
  sleep_mode();
}

/**************************************************************************************************************************************************************
 * 
 */
bool Tonuino::isPlaying() {
  return !digitalRead(busyPin);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::waitForTrackToFinish() {
  unsigned long currentTime = millis();
#define TIMEOUT 1000
  do {
    mp3.loop();
  } while (!isPlaying() && millis() < currentTime + TIMEOUT);
  delay(1000);
  do {
    mp3.loop();
  } while (isPlaying());
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::readButtons() {
  buttonPause.read();
  buttonUp.read();
  buttonDown.read();
#ifdef FIVEBUTTONS
  buttonFour.read();
  buttonFive.read();
#endif
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::volumeUpButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeUp() == true)
      return;

  DEBUG_PRINTLN(F("=== volumeUp()"));
  if (volume < mySettings.maxVolume) {
    mp3.increaseVolume();
    volume++;
    delay(LONG_PRESS_DELAY);
  }
  DEBUG_PRINTLN(volume);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::volumeDownButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeDown() == true)
      return;

  DEBUG_PRINTLN(F("=== volumeDown()"));
  if (volume > mySettings.minVolume) {
    mp3.decreaseVolume();
    volume--;
    delay(LONG_PRESS_DELAY);
  }
  DEBUG_PRINTLN(volume);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::nextButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleNextButton() == true)
      return;

  nextTrack(random(65536));
  delay(1000);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::previousButton() {
  if (activeModifier != NULL)
    if (activeModifier->handlePreviousButton() == true)
      return;

  previousTrack();
  delay(1000);
}

/**************************************************************************************************************************************************************
 * Disable DFPlayer amplifier by pulling ENABLE pin HIGH
 */
void Tonuino::disableDfplayerAmplifier() {
  digitalWrite(amplifierStandbyPin, HIGH);
}

/**************************************************************************************************************************************************************
 * Enable DFPlayer amplifier by pulling ENABLE pin LOW
 */
void Tonuino::enableDfplayerAmplifier() {
  digitalWrite(amplifierStandbyPin, LOW);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::playFolder() {
  DEBUG_PRINTLN(F("== playFolder()"));
  disableStandbyTimer();
  cardKnown = true;
  lastTrackFinished = 0;
  numTracksInFolder = mp3.getFolderTrackCount(myFolder->folder);
  firstTrack = 1;
  DEBUG_PRINT(numTracksInFolder);
  DEBUG_PRINT(F(" Dateien in Ordner "));
  DEBUG_PRINTLN(myFolder->folder);

  // Hörspielmodus: eine zufällige Datei aus dem Ordner
  if (myFolder->mode == 1) {
    DEBUG_PRINTLN(F("Hörspielmodus -> zufälligen Track wiedergeben"));
    currentTrack = random(1, numTracksInFolder + 1);
    DEBUG_PRINTLN(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Album Modus: kompletten Ordner spielen
  if (myFolder->mode == 2) {
    DEBUG_PRINTLN(F("Album Modus -> kompletten Ordner wiedergeben"));
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Party Modus: Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 3) {
    DEBUG_PRINTLN(
        F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
    shuffleQueue();
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  // Einzel Modus: eine Datei aus dem Ordner abspielen
  if (myFolder->mode == 4) {
    DEBUG_PRINTLN(
        F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
  if (myFolder->mode == 5) {
    DEBUG_PRINTLN(F(
        "Hörbuch Modus -> kompletten Ordner spielen und "
        "Fortschritt merken"));
    currentTrack = EEPROM.read(myFolder->folder);
    if (currentTrack == 0 || currentTrack > numTracksInFolder) {
      currentTrack = 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Spezialmodus Von-Bin: Hörspiel: eine zufällige Datei aus dem Ordner
  if (myFolder->mode == 7) {
    DEBUG_PRINTLN(F("Spezialmodus Von-Bin: Hörspiel -> zufälligen Track wiedergeben"));
    DEBUG_PRINT(myFolder->special);
    DEBUG_PRINT(F(" bis "));
    DEBUG_PRINTLN(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = random(myFolder->special, numTracksInFolder + 1);
    DEBUG_PRINTLN(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Album: alle Dateien zwischen Start und Ende spielen
  if (myFolder->mode == 8) {
    DEBUG_PRINTLN(F("Spezialmodus Von-Bis: Album: alle Dateien zwischen Start- und Enddatei spielen"));
    DEBUG_PRINT(myFolder->special);
    DEBUG_PRINT(F(" bis "));
    DEBUG_PRINTLN(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Party Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 9) {
    DEBUG_PRINTLN(
        F("Spezialmodus Von-Bis: Party -> Ordner in zufälliger Reihenfolge wiedergeben"));
    firstTrack = myFolder->special;
    numTracksInFolder = myFolder->special2;
    shuffleQueue();
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::playShortCut(uint8_t shortCut) {
  DEBUG_PRINTLN(F("=== playShortCut()"));
  DEBUG_PRINTLN(shortCut);
  if (mySettings.shortCuts[shortCut].folder != 0) {
    myFolder = &mySettings.shortCuts[shortCut];
    playFolder();
    disableStandbyTimer();
    delay(1000);
  } else
    DEBUG_PRINTLN(F("Shortcut not configured!"));
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::adminMenu(bool fromCard) {
  disableStandbyTimer();
  mp3.pause();
  DEBUG_PRINTLN(F("=== adminMenu()"));
  cardKnown = false;
  if (fromCard == false) {
    // Admin menu has been locked - it still can be trigged via admin card
    if (mySettings.adminMenuLocked == 1) {
      return;
    }
    // Pin check
    else if (mySettings.adminMenuLocked == 2) {
      uint8_t pin[4];
      mp3.playMp3FolderTrack(991);
      if (askCode(pin) == true) {
        if (checkTwo(pin, mySettings.adminMenuPin) == false) {
          return;
        }
      } else {
        return;
      }
    }
    // Match check
    else if (mySettings.adminMenuLocked == 3) {
      uint8_t a = random(10, 20);
      uint8_t b = random(1, 10);
      uint8_t c;
      mp3.playMp3FolderTrack(992);
      waitForTrackToFinish();
      mp3.playMp3FolderTrack(a);

      if (random(1, 3) == 2) {
        // a + b
        c = a + b;
        waitForTrackToFinish();
        mp3.playMp3FolderTrack(993);
      } else {
        // a - b
        b = random(1, a);
        c = a - b;
        waitForTrackToFinish();
        mp3.playMp3FolderTrack(994);
      }
      waitForTrackToFinish();
      mp3.playMp3FolderTrack(b);
      DEBUG_PRINTLN(c);
      uint8_t temp = voiceMenu(255, 0, 0, false);
      if (temp != c) {
        return;
      }
    }
  }
  int subMenu = voiceMenu(12, 900, 900, false, false, 0, true);
  if (subMenu == 0)
    return;
  if (subMenu == 1) {
    resetCard();
    nfcHandler.halt();
  } else if (subMenu == 2) {
    // Maximum Volume
    mySettings.maxVolume = voiceMenu(30 - mySettings.minVolume, 930, mySettings.minVolume, false, false, mySettings.maxVolume - mySettings.minVolume) + mySettings.minVolume;
  } else if (subMenu == 3) {
    // Minimum Volume
    mySettings.minVolume = voiceMenu(mySettings.maxVolume - 1, 931, 0, false, false, mySettings.minVolume);
  } else if (subMenu == 4) {
    // Initial Volume
    mySettings.initVolume = voiceMenu(mySettings.maxVolume - mySettings.minVolume + 1, 932, mySettings.minVolume - 1, false, false, mySettings.initVolume - mySettings.minVolume + 1) + mySettings.minVolume - 1;
  } else if (subMenu == 5) {
    // EQ
    mySettings.eq = voiceMenu(6, 920, 920, false, false, mySettings.eq);
    mp3.setEq(DfMp3_Eq(mySettings.eq - 1));
  } else if (subMenu == 6) {
    // create modifier card
    NfcTagObject tempCard;
    tempCard.cookie = NfcHandler::cardCookie;
    tempCard.version = 1;
    tempCard.nfcFolderSettings.folder = 0;
    tempCard.nfcFolderSettings.special = 0;
    tempCard.nfcFolderSettings.special2 = 0;
    tempCard.nfcFolderSettings.mode = voiceMenu(6, 970, 970, false, false, 0, true);

    if (tempCard.nfcFolderSettings.mode != 0) {
      if (tempCard.nfcFolderSettings.mode == 1) {
        switch (voiceMenu(4, 960, 960)) {
          case 1:
            tempCard.nfcFolderSettings.special = 5;
            break;
          case 2:
            tempCard.nfcFolderSettings.special = 15;
            break;
          case 3:
            tempCard.nfcFolderSettings.special = 30;
            break;
          case 4:
            tempCard.nfcFolderSettings.special = 60;
            break;
        }
      }
      mp3.playMp3FolderTrack(800);
      do {
        readButtons();
        if (buttonUp.wasReleased() || buttonDown.wasReleased()) {
          DEBUG_PRINTLN(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!nfcHandler.isNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (nfcHandler.readCardSerial()) {
        DEBUG_PRINTLN(F("schreibe Karte..."));
        nfcHandler.writeCard(tempCard);
        delay(100);
        nfcHandler.halt();
        waitForTrackToFinish();
      }
    }
  } else if (subMenu == 7) {
    uint8_t shortcut = voiceMenu(4, 940, 940);
    setupFolder(&mySettings.shortCuts[shortcut - 1]);
    mp3.playMp3FolderTrack(400);
  } else if (subMenu == 8) {
    switch (voiceMenu(5, 960, 960)) {
      case 1:
        mySettings.standbyTimer = 5;
        break;
      case 2:
        mySettings.standbyTimer = 15;
        break;
      case 3:
        mySettings.standbyTimer = 30;
        break;
      case 4:
        mySettings.standbyTimer = 60;
        break;
      case 5:
        mySettings.standbyTimer = 0;
        break;
    }
  } else if (subMenu == 9) {
    // Create Cards for Folder
    // Ordner abfragen
    NfcTagObject tempCard;
    tempCard.cookie = NfcHandler::cardCookie;
    tempCard.version = 1;
    tempCard.nfcFolderSettings.mode = 4;
    tempCard.nfcFolderSettings.folder = voiceMenu(99, 301, 0, true);
    uint8_t special = voiceMenu(mp3.getFolderTrackCount(tempCard.nfcFolderSettings.folder), 321, 0,
                                true, tempCard.nfcFolderSettings.folder);
    uint8_t special2 = voiceMenu(mp3.getFolderTrackCount(tempCard.nfcFolderSettings.folder), 322, 0,
                                 true, tempCard.nfcFolderSettings.folder, special);

    mp3.playMp3FolderTrack(936);
    waitForTrackToFinish();
    for (uint8_t x = special; x <= special2; x++) {
      mp3.playMp3FolderTrack(x);
      tempCard.nfcFolderSettings.special = x;
      DEBUG_PRINT(x);
      DEBUG_PRINTLN(F(" Karte auflegen"));
      do {
        readButtons();
        if (buttonUp.wasReleased() || buttonDown.wasReleased()) {
          DEBUG_PRINTLN(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!nfcHandler.isNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (nfcHandler.readCardSerial()) {
        DEBUG_PRINTLN(F("schreibe Karte..."));
        nfcHandler.writeCard(tempCard);
        delay(100);
        nfcHandler.halt();
        waitForTrackToFinish();
      }
    }
  } else if (subMenu == 10) {
    // Invert Functions for Up/Down Buttons
    int temp = voiceMenu(2, 933, 933, false);
    if (temp == 2) {
      mySettings.invertVolumeButtons = true;
    } else {
      mySettings.invertVolumeButtons = false;
    }
  } else if (subMenu == 11) {
    DEBUG_PRINTLN(F("Reset -> EEPROM wird gelöscht"));
    for (unsigned int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
    resetSettings();
    mp3.playMp3FolderTrack(999);
  }
  // lock admin menu
  else if (subMenu == 12) {
    int temp = voiceMenu(4, 980, 980, false);
    if (temp == 1) {
      mySettings.adminMenuLocked = 0;
    } else if (temp == 2) {
      mySettings.adminMenuLocked = 1;
    } else if (temp == 3) {
      uint8_t pin[4];
      mp3.playMp3FolderTrack(991);
      if (askCode(pin)) {
        memcpy(mySettings.adminMenuPin, pin, 4);
        mySettings.adminMenuLocked = 2;
      }
    } else if (temp == 4) {
      mySettings.adminMenuLocked = 3;
    }
  }
  writeSettingsToFlash();
  setStandbyTimer();
}

/**************************************************************************************************************************************************************
 * 
 */
bool Tonuino::askCode(uint8_t* code) {
  uint8_t x = 0;
  while (x < 4) {
    readButtons();
    if (buttonPause.pressedFor(LONG_PRESS))
      break;
    if (buttonPause.wasReleased())
      code[x++] = 1;
    if (buttonUp.wasReleased())
      code[x++] = 2;
    if (buttonDown.wasReleased())
      code[x++] = 3;
  }
  return true;
}

/**************************************************************************************************************************************************************
 * 
 */
uint8_t Tonuino::voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
                           bool preview, int previewFromFolder, int defaultValue, bool exitWithLongPress) {
  uint8_t returnValue = defaultValue;
  if (startMessage != 0)
    mp3.playMp3FolderTrack(startMessage);
  DEBUG_PRINT(F("=== voiceMenu() ("));
  DEBUG_PRINT(numberOfOptions);
  DEBUG_PRINTLN(F(" Options)"));
  do {
    if (Serial.available() > 0) {
      int optionSerial = Serial.parseInt();
      if (optionSerial != 0 && optionSerial <= numberOfOptions)
        return optionSerial;
    }
    readButtons();
    mp3.loop();
    if (buttonPause.pressedFor(LONG_PRESS)) {
      mp3.playMp3FolderTrack(802);
      ignorePauseButton = true;
      checkStandbyAtMillis();
      return defaultValue;
    }
    if (buttonPause.wasReleased()) {
      if (returnValue != 0) {
        DEBUG_PRINT(F("=== "));
        DEBUG_PRINT(returnValue);
        DEBUG_PRINTLN(F(" ==="));
        return returnValue;
      }
      delay(1000);
    }

    if (buttonUp.pressedFor(LONG_PRESS)) {
      returnValue = min(returnValue + 10, numberOfOptions);
      DEBUG_PRINTLN(returnValue);
      //mp3.pause();
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      waitForTrackToFinish();
      /*if (preview) {
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
        }*/
      ignoreUpButton = true;
    } else if (buttonUp.wasReleased()) {
      if (!ignoreUpButton) {
        returnValue = min(returnValue + 1, numberOfOptions);
        DEBUG_PRINTLN(returnValue);
        //mp3.pause();
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        if (preview) {
          waitForTrackToFinish();
          if (previewFromFolder == 0) {
            mp3.playFolderTrack(returnValue, 1);
          } else {
            mp3.playFolderTrack(previewFromFolder, returnValue);
          }
          delay(1000);
        }
      } else {
        ignoreUpButton = false;
      }
    }

    if (buttonDown.pressedFor(LONG_PRESS)) {
      returnValue = max(returnValue - 10, 1);
      DEBUG_PRINTLN(returnValue);
      //mp3.pause();
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      waitForTrackToFinish();
      /*if (preview) {
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
        }*/
      ignoreDownButton = true;
    } else if (buttonDown.wasReleased()) {
      if (!ignoreDownButton) {
        returnValue = max(returnValue - 1, 1);
        DEBUG_PRINTLN(returnValue);
        //mp3.pause();
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        if (preview) {
          waitForTrackToFinish();
          if (previewFromFolder == 0) {
            mp3.playFolderTrack(returnValue, 1);
          } else {
            mp3.playFolderTrack(previewFromFolder, returnValue);
          }
          delay(1000);
        }
      } else {
        ignoreDownButton = false;
      }
    }
  } while (true);
}

void Tonuino::resetCard() {
  mp3.playMp3FolderTrack(800);
  do {
    buttonPause.read();
    buttonUp.read();
    buttonDown.read();

    if (buttonUp.wasReleased() || buttonDown.wasReleased()) {
      DEBUG_PRINT(F("Abgebrochen!"));
      mp3.playMp3FolderTrack(802);
      return;
    }
  } while (!nfcHandler.isNewCardPresent());

  if (!nfcHandler.readCardSerial())
    return;

  DEBUG_PRINT(F("Karte wird neu konfiguriert!"));
  setupCard();
}

/**************************************************************************************************************************************************************
 * 
 */
bool Tonuino::setupFolder(FolderSettings* theFolder) {
  // Ordner abfragen
  theFolder->folder = voiceMenu(99, 301, 0, true, 0, 0, true);
  if (theFolder->folder == 0) return false;

  // Wiedergabemodus abfragen
  theFolder->mode = voiceMenu(9, 310, 310, false, 0, 0, true);
  if (theFolder->mode == 0) return false;

  //  // Hörbuchmodus -> Fortschritt im EEPROM auf 1 setzen
  //  EEPROM.update(theFolder->folder, 1);

  // Einzelmodus -> Datei abfragen
  if (theFolder->mode == 4)
    theFolder->special = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 320, 0,
                                   true, theFolder->folder);
  // Admin Funktionen
  if (theFolder->mode == 6) {
    //theFolder->special = voiceMenu(3, 320, 320);
    theFolder->folder = 0;
    theFolder->mode = 255;
  }
  // Spezialmodus Von-Bis
  if (theFolder->mode == 7 || theFolder->mode == 8 || theFolder->mode == 9) {
    theFolder->special = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 321, 0,
                                   true, theFolder->folder);
    theFolder->special2 = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 322, 0,
                                    true, theFolder->folder, theFolder->special);
  }
  return true;
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::setupCard() {
  mp3.pause();
  DEBUG_PRINTLN(F("=== setupCard()"));
  NfcTagObject newCard;
  if (setupFolder(&newCard.nfcFolderSettings) == true) {
    // Karte ist konfiguriert -> speichern
    mp3.pause();
    do {
    } while (isPlaying());
    nfcHandler.writeCard(newCard);
  }
  delay(1000);
}

/**************************************************************************************************************************************************************
 * 
 */
bool Tonuino::checkTwo(uint8_t a[], uint8_t b[]) {
  for (uint8_t k = 0; k < 4; k++) {  // Loop 4 times
    if (a[k] != b[k]) {              // IF a != b then false, because: one fails, all fail
      return false;
    }
  }
  return true;
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::setup() {

  // Power supply board ENABLE pin
  pinMode(shutdownPin, OUTPUT);
  digitalWrite(shutdownPin, HIGH);

  // Amplifier standby pin
  pinMode(amplifierStandbyPin, OUTPUT);
  enableDfplayerAmplifier();

  // Prepare additional feature modules
  ledHandler->setup();
  ledHandler->setStatusLedColor(CRGB::Blue);

#ifdef MY_DEBUG
  Serial.begin(115200);  // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
#endif

  // Wert für randomSeed() erzeugen durch das mehrfache Sammeln von rauschenden LSBs eines offenen Analogeingangs
  uint32_t ADC_LSB;
  uint32_t ADCSeed;
  for (uint8_t i = 0; i < 128; i++) {
    ADC_LSB = analogRead(openAnalogPin) & 0x1;
    ADCSeed ^= ADC_LSB << (i % 32);
  }
  randomSeed(ADCSeed);  // Zufallsgenerator initialisieren

  // Dieser Hinweis darf nicht entfernt werden
  DEBUG_PRINTLN(F("\n _____         _____ _____ _____ _____"));
  DEBUG_PRINTLN(F("|_   _|___ ___|  |  |     |   | |     |"));
  DEBUG_PRINTLN(F("  | | | . |   |  |  |-   -| | | |  |  |"));
  DEBUG_PRINTLN(F("  |_| |___|_|_|_____|_____|_|___|_____|\n"));
  DEBUG_PRINTLN(F("TonUINO Version 2.1"));
  DEBUG_PRINTLN(F("created by Thorsten Voß and licensed under GNU/GPL."));
  DEBUG_PRINTLN(F("Information and contribution at https://tonuino.de.\n"));

  // Busy Pin
  pinMode(busyPin, INPUT);

  // load Settings from EEPROM
  loadSettingsFromFlash();

  // activate standby timer
  setStandbyTimer();

  // DFPlayer Mini initialisieren
  mp3.begin();
  // Zwei Sekunden warten bis der DFPlayer Mini initialisiert ist
  delay(2000);
  volume = mySettings.initVolume;
  mp3.setVolume(volume);
  mp3.setEq(DfMp3_Eq(mySettings.eq - 1));
  // Fix für das Problem mit dem Timeout (ist jetzt in Upstream daher nicht mehr nötig!)
  //mySoftwareSerial.setTimeout(10000);

  // NFC Leser initialisieren
  nfcHandler.init();

  pinMode(BUTTON_PAUSE, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
#ifdef FIVEBUTTONS
  pinMode(buttonFourPin, INPUT_PULLUP);
  pinMode(buttonFivePin, INPUT_PULLUP);
#endif

  batteryHandler->setup();

  // Start Shortcut "at Startup" - e.g. Welcome Sound
  playShortCut(3);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::loop() {

  do {

#ifdef PAUSEONCARDREMOVAL
    int _rfid_error_counter = 0;
    // auf Lesefehler des RFID Moduls prüfen
    _rfid_error_counter += 1;
    if (_rfid_error_counter > 2) {
      card_present = false;
    }

    // Ist eine Karte aufgelegt?
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    MFRC522::StatusCode result = mfrc522.PICC_RequestA(bufferATQA, &bufferSize);

    if (result == mfrc522.STATUS_OK) {
      _rfid_error_counter = 0;
      card_present = true;
    }
#endif

    checkStandbyAtMillis();
    mp3.loop();

    // Modifier : WIP!
    if (activeModifier != NULL) {
      activeModifier->loop();
    }

    batteryHandler->loop();

    // Buttons werden nun über JS_Button gehandelt, dadurch kann jede Taste
    // doppelt belegt werden
    readButtons();

    // admin menu
    if ((buttonPause.pressedFor(LONG_PRESS) || buttonUp.pressedFor(LONG_PRESS) || buttonDown.pressedFor(LONG_PRESS)) && buttonPause.isPressed() && buttonUp.isPressed() && buttonDown.isPressed()) {
      mp3.pause();
      do {
        readButtons();
      } while (buttonPause.isPressed() || buttonUp.isPressed() || buttonDown.isPressed());
      readButtons();
      adminMenu();
      break;
    }

    if (buttonPause.wasReleased()) {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (ignorePauseButton == false) {
        if (isPlaying()) {
          mp3.pause();
          setStandbyTimer();
        } else if (cardKnown) {
          mp3.start();
          disableStandbyTimer();
        }
      }
      ignorePauseButton = false;
    } else if (buttonPause.pressedFor(LONG_PRESS) &&
               ignorePauseButton == false) {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (isPlaying()) {
        uint8_t advertTrack;
        if (myFolder->mode == 3 || myFolder->mode == 9) {
          advertTrack = (queue[currentTrack - 1]);
        } else {
          advertTrack = currentTrack;
        }
        // Spezialmodus Von-Bis für Album und Party gibt die Dateinummer relativ zur Startposition wieder
        if (myFolder->mode == 8 || myFolder->mode == 9) {
          advertTrack = advertTrack - myFolder->special + 1;
        }
        mp3.playAdvertisement(advertTrack);
      } else {
        //playShortCut(0);
        poweroff();
      }
      ignorePauseButton = true;
    }

    if (buttonUp.pressedFor(LONG_PRESS)) {
#ifndef FIVEBUTTONS
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeUpButton();
        } else {
          nextButton();
        }
      } else {
        playShortCut(1);
      }
      ignoreUpButton = true;
#endif
    } else if (buttonUp.wasReleased()) {
      if (!ignoreUpButton) {
        if (!mySettings.invertVolumeButtons) {
          nextButton();
        } else {
          volumeUpButton();
        }
      }
      ignoreUpButton = false;
    }

    if (buttonDown.pressedFor(LONG_PRESS)) {
#ifndef FIVEBUTTONS
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeDownButton();
        } else {
          previousButton();
        }
      } else {
        playShortCut(2);
      }
      ignoreDownButton = true;
#endif
    } else if (buttonDown.wasReleased()) {
      if (!ignoreDownButton) {
        if (!mySettings.invertVolumeButtons) {
          previousButton();
        } else {
          volumeDownButton();
        }
      }
      ignoreDownButton = false;
    }
#ifdef FIVEBUTTONS
    if (buttonFour.wasReleased()) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeUpButton();
        } else {
          nextButton();
        }
      } else {
        playShortCut(1);
      }
    }
    if (buttonFive.wasReleased()) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeDownButton();
        } else {
          previousButton();
        }
      } else {
        playShortCut(2);
      }
    }
#endif
    // Ende der Buttons

#ifndef PAUSEONCARDREMOVAL
  } while (!nfcHandler.isNewCardPresent());

  // RFID Karte wurde aufgelegt
  if (!nfcHandler.readCardSerial())
    return;

  if (nfcHandler.readCard(&myCard) == true) {
    if (myCard.cookie == NfcHandler::cardCookie && myCard.nfcFolderSettings.folder != 0 && myCard.nfcFolderSettings.mode != 0) {
      playFolder();
    }

    // Neue Karte konfigurieren
    else if (myCard.cookie != NfcHandler::cardCookie) {
      cardKnown = false;
      mp3.playMp3FolderTrack(300);
      waitForTrackToFinish();
      setupCard();
    }
  }
  nfcHandler.halt();

#endif
#ifdef PAUSEONCARDREMOVAL
  // solange keine Karte aufgelegt oder heruntergenommen wird
}
while (!(!card_present && card_present_prev) && !(card_present && !card_present_prev))
  ;

// RFID Karte wurde entfernt
if (!card_present && card_present_prev) {
  card_present_prev = card_present;
  // pausiere
  if (activeModifier != NULL)
    if (activeModifier->handlePause() == true)
      return;
  if (ignorePauseButton == false)
    if (isPlaying()) {
      mp3.pause();
      setStandbyTimer();
    } else if (cardKnown) {
      mp3.start();
      disableStandbyTimer();
    }
  ignorePauseButton = false;
  return;
}

// RFID Karte wurde aufgelegt
if (card_present && !card_present_prev) {
  card_present_prev = card_present;

  if (!mfrc522.PICC_ReadCardSerial())
    return;

  if (readCard(&myCard) == true) {
    if (myCard.cookie == cardCookie && myCard.nfcFolderSettings.folder != 0 && myCard.nfcFolderSettings.mode != 0) {
      playFolder();
    }

    // Neue Karte konfigurieren
    else if (myCard.cookie != cardCookie) {
      cardKnown = false;
      mp3.playMp3FolderTrack(300);
      waitForTrackToFinish();
      setupCard();
    }
  }
  mfrc522.PCD_StopCrypto1();
}
#endif
}
