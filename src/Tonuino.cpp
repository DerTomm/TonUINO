#include <EEPROM.h>
#include "Tonuino.h"

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
  /*  Serial.println(F("Queue :"));
    for (uint8_t x = 0; x < numTracksInFolder - firstTrack + 1 ; x++)
      Serial.println(queue[x]);
  */
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::writeSettingsToFlash() {
  Serial.println(F("=== writeSettingsToFlash()"));
  int address = sizeof(myFolder->folder) * 100;
  EEPROM.put(address, mySettings);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::resetSettings() {
  Serial.println(F("=== resetSettings()"));
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
    Serial.println(F("=== resetSettings()"));
    Serial.println(F("1 -> 2"));
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
  Serial.println(F("=== loadSettingsFromFlash()"));
  int address = sizeof(myFolder->folder) * 100;
  EEPROM.get(address, mySettings);
  if (mySettings.cookie != NfcHandler::cardCookie)
    resetSettings();
  migrateSettings(mySettings.version);

  Serial.print(F("Version: "));
  Serial.println(mySettings.version);

  Serial.print(F("Maximal Volume: "));
  Serial.println(mySettings.maxVolume);

  Serial.print(F("Minimal Volume: "));
  Serial.println(mySettings.minVolume);

  Serial.print(F("Initial Volume: "));
  Serial.println(mySettings.initVolume);

  Serial.print(F("EQ: "));
  Serial.println(mySettings.eq);

  Serial.print(F("Locked: "));
  Serial.println(mySettings.locked);

  Serial.print(F("Sleep Timer: "));
  Serial.println(mySettings.standbyTimer);

  Serial.print(F("Inverted Volume Buttons: "));
  Serial.println(mySettings.invertVolumeButtons);

  Serial.print(F("Admin Menu locked: "));
  Serial.println(mySettings.adminMenuLocked);

  Serial.print(F("Admin Menu Pin: "));
  Serial.print(mySettings.adminMenuPin[0]);
  Serial.print(mySettings.adminMenuPin[1]);
  Serial.print(mySettings.adminMenuPin[2]);
  Serial.println(mySettings.adminMenuPin[3]);
}

/**************************************************************************************************************************************************************
 * Leider kann das Modul selbst keine Queue abspielen, daher müssen wir selbst die Queue verwalten
 */
void Tonuino::nextTrack(uint16_t track) {
  Serial.println(track);
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

  Serial.println(F("=== nextTrack()"));

  if (myFolder->mode == 1 || myFolder->mode == 7) {
    Serial.println(F("Hörspielmodus ist aktiv -> keinen neuen Track spielen"));
    setStandbyTimer();
    //    mp3.sleep(); // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
  }
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      mp3.playFolderTrack(myFolder->folder, currentTrack);
      Serial.print(F("Albummodus ist aktiv -> nächster Track: "));
      Serial.print(currentTrack);
    } else
      //      mp3.sleep();   // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      setStandbyTimer();
    {}
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != numTracksInFolder - firstTrack + 1) {
      Serial.print(F("Party -> weiter in der Queue "));
      currentTrack++;
    } else {
      Serial.println(F("Ende der Queue -> beginne von vorne"));
      currentTrack = 1;
      //// Wenn am Ende der Queue neu gemischt werden soll bitte die Zeilen wieder aktivieren
      //     Serial.println(F("Ende der Queue -> mische neu"));
      //     shuffleQueue();
    }
    Serial.println(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }

  if (myFolder->mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Strom sparen"));
    //    mp3.sleep();      // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
    setStandbyTimer();
  }
  if (myFolder->mode == 5) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      Serial.print(F(
          "Hörbuch Modus ist aktiv -> nächster Track und "
          "Fortschritt speichern"));
      Serial.println(currentTrack);
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
  Serial.println(F("=== previousTrack()"));
  /*  if (myCard.mode == 1 || myCard.mode == 7) {
      Serial.println(F("Hörspielmodus ist aktiv -> Track von vorne spielen"));
      mp3.playFolderTrack(myCard.folder, currentTrack);
    }*/
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    Serial.println(F("Albummodus ist aktiv -> vorheriger Track"));
    if (currentTrack != firstTrack) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != 1) {
      Serial.print(F("Party Modus ist aktiv -> zurück in der Qeueue "));
      currentTrack--;
    } else {
      Serial.print(F("Anfang der Queue -> springe ans Ende "));
      currentTrack = numTracksInFolder;
    }
    Serial.println(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  if (myFolder->mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 5) {
    Serial.println(F(
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
  Serial.println(F("=== setStandbyTimer()"));
  if (mySettings.standbyTimer != 0)
    sleepAtMillis = millis() + (mySettings.standbyTimer * 60 * 1000);
  else
    sleepAtMillis = 0;
  Serial.println(sleepAtMillis);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::disableStandbyTimer() {
  Serial.println(F("=== disablestandby()"));
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
  Serial.println(F("=== power off!"));
  ledHandler.setStatusLedColor(CRGB::Black);
  // enter sleep state
  disableDfplayerAmplifier();
  delay(500);
  digitalWrite(shutdownPin, LOW);
  delay(500);

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

  Serial.println(F("=== volumeUp()"));
  if (volume < mySettings.maxVolume) {
    mp3.increaseVolume();
    volume++;
    delay(LONG_PRESS_DELAY);
  }
  Serial.println(volume);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::volumeDownButton() {
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeDown() == true)
      return;

  Serial.println(F("=== volumeDown()"));
  if (volume > mySettings.minVolume) {
    mp3.decreaseVolume();
    volume--;
    delay(LONG_PRESS_DELAY);
  }
  Serial.println(volume);
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
  Serial.println(F("== playFolder()"));
  disableStandbyTimer();
  cardKnown = true;
  lastTrackFinished = 0;
  numTracksInFolder = mp3.getFolderTrackCount(myFolder->folder);
  firstTrack = 1;
  Serial.print(numTracksInFolder);
  Serial.print(F(" Dateien in Ordner "));
  Serial.println(myFolder->folder);

  // Hörspielmodus: eine zufällige Datei aus dem Ordner
  if (myFolder->mode == 1) {
    Serial.println(F("Hörspielmodus -> zufälligen Track wiedergeben"));
    currentTrack = random(1, numTracksInFolder + 1);
    Serial.println(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Album Modus: kompletten Ordner spielen
  if (myFolder->mode == 2) {
    Serial.println(F("Album Modus -> kompletten Ordner wiedergeben"));
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Party Modus: Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 3) {
    Serial.println(
        F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
    shuffleQueue();
    currentTrack = 1;
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  // Einzel Modus: eine Datei aus dem Ordner abspielen
  if (myFolder->mode == 4) {
    Serial.println(
        F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  // Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
  if (myFolder->mode == 5) {
    Serial.println(F(
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
    Serial.println(F("Spezialmodus Von-Bin: Hörspiel -> zufälligen Track wiedergeben"));
    Serial.print(myFolder->special);
    Serial.print(F(" bis "));
    Serial.println(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = random(myFolder->special, numTracksInFolder + 1);
    Serial.println(currentTrack);
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Album: alle Dateien zwischen Start und Ende spielen
  if (myFolder->mode == 8) {
    Serial.println(F("Spezialmodus Von-Bis: Album: alle Dateien zwischen Start- und Enddatei spielen"));
    Serial.print(myFolder->special);
    Serial.print(F(" bis "));
    Serial.println(myFolder->special2);
    numTracksInFolder = myFolder->special2;
    currentTrack = myFolder->special;
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }

  // Spezialmodus Von-Bis: Party Ordner in zufälliger Reihenfolge
  if (myFolder->mode == 9) {
    Serial.println(
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
  Serial.println(F("=== playShortCut()"));
  Serial.println(shortCut);
  if (mySettings.shortCuts[shortCut].folder != 0) {
    myFolder = &mySettings.shortCuts[shortCut];
    playFolder();
    disableStandbyTimer();
    delay(1000);
  } else
    Serial.println(F("Shortcut not configured!"));
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::adminMenu(bool fromCard) {
  disableStandbyTimer();
  mp3.pause();
  Serial.println(F("=== adminMenu()"));
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
      Serial.println(c);
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
          Serial.println(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!nfcHandler.isNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (nfcHandler.readCardSerial()) {
        Serial.println(F("schreibe Karte..."));
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
      Serial.print(x);
      Serial.println(F(" Karte auflegen"));
      do {
        readButtons();
        if (buttonUp.wasReleased() || buttonDown.wasReleased()) {
          Serial.println(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!nfcHandler.isNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (nfcHandler.readCardSerial()) {
        Serial.println(F("schreibe Karte..."));
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
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
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
  Serial.print(F("=== voiceMenu() ("));
  Serial.print(numberOfOptions);
  Serial.println(F(" Options)"));
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
      return defaultValue;
    }
    if (buttonPause.wasReleased()) {
      if (returnValue != 0) {
        Serial.print(F("=== "));
        Serial.print(returnValue);
        Serial.println(F(" ==="));
        return returnValue;
      }
      delay(1000);
    }

    if (buttonUp.pressedFor(LONG_PRESS)) {
      returnValue = min(returnValue + 10, numberOfOptions);
      Serial.println(returnValue);
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
        Serial.println(returnValue);
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
      Serial.println(returnValue);
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
        Serial.println(returnValue);
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
      Serial.print(F("Abgebrochen!"));
      mp3.playMp3FolderTrack(802);
      return;
    }
  } while (!nfcHandler.isNewCardPresent());

  if (!nfcHandler.readCardSerial())
    return;

  Serial.print(F("Karte wird neu konfiguriert!"));
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
  Serial.println(F("=== setupCard()"));
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
float Tonuino::readBatteryVoltage() {
  int batteryVoltage = analogRead(A5);
  return batteryVoltage * (4.2 / 1023.0);
}

/**************************************************************************************************************************************************************
 * 
 */
void Tonuino::checkBatteryVoltage() {
  float batteryVoltage = readBatteryVoltage();
  Serial.print("Battery voltage: ");
  Serial.println(batteryVoltage);
  if (batteryVoltage <= 3.4) {
    ledHandler.setStatusLedColor(CRGB::Red);
  } else if (batteryVoltage <= 3.7) {
    ledHandler.setStatusLedColor(CRGB::Yellow);
  } else {
    ledHandler.setStatusLedColor(CRGB::Green);
  }
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

  // Battery voltage sense pin
  pinMode(A5, INPUT);

  ledHandler.setup();
  ledHandler.setStatusLedColor(CRGB::Blue);

  Serial.begin(115200);  // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle

  // Wert für randomSeed() erzeugen durch das mehrfache Sammeln von rauschenden LSBs eines offenen Analogeingangs
  uint32_t ADC_LSB;
  uint32_t ADCSeed;
  for (uint8_t i = 0; i < 128; i++) {
    ADC_LSB = analogRead(openAnalogPin) & 0x1;
    ADCSeed ^= ADC_LSB << (i % 32);
  }
  randomSeed(ADCSeed);  // Zufallsgenerator initialisieren

  // Dieser Hinweis darf nicht entfernt werden
  Serial.println(F("\n _____         _____ _____ _____ _____"));
  Serial.println(F("|_   _|___ ___|  |  |     |   | |     |"));
  Serial.println(F("  | | | . |   |  |  |-   -| | | |  |  |"));
  Serial.println(F("  |_| |___|_|_|_____|_____|_|___|_____|\n"));
  Serial.println(F("TonUINO Version 2.1"));
  Serial.println(F("created by Thorsten Voß and licensed under GNU/GPL."));
  Serial.println(F("Information and contribution at https://tonuino.de.\n"));

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

  // RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle EINSTELLUNGEN werden gelöscht
  if (digitalRead(BUTTON_PAUSE) == LOW &&
      digitalRead(BUTTON_UP) == LOW &&
      digitalRead(BUTTON_DOWN) == LOW) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (unsigned int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
    loadSettingsFromFlash();
  }

  // Start Shortcut "at Startup" - e.g. Welcome Sound
  playShortCut(3);

  // This initially sets status LED color
  checkBatteryVoltage();
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

    if (millis() - batteryMeasureTimestamp >= 30000) {
      checkBatteryVoltage();
      batteryMeasureTimestamp = millis();
    }

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