#include "RfidManager.h"
#include "../Config.h"

RfidManager::RfidManager() 
    : _mfrc522(nullptr), _ssPin(0), _rstPin(0), 
      _lastStableTag{0, false}, _tagChanged(false),
      _lastCheckTime(0), _missingCount(0) {}

void RfidManager::begin(uint8_t ssPin, uint8_t rstPin) {
    _ssPin = ssPin;
    _rstPin = rstPin;

    SPI.begin();
    _mfrc522 = new MFRC522(_ssPin, _rstPin);
    delay(50); // Allow power to stabilize
    _mfrc522->PCD_Init();
    
    // Debug: Check if the reader is connected properly
    Serial.print(F("RFID FW Ver: "));
    _mfrc522->PCD_DumpVersionToSerial();

    // Set antenna gain to max to improve detection through casing
    _mfrc522->PCD_SetAntennaGain(_mfrc522->RxGain_max);
    
    Serial.println(F("RFID Manager Online."));
}

void RfidManager::update() {
    uint32_t now = millis();
    if (now - _lastCheckTime < CONF_RFID_CHECK_INTERVAL_MS) return;
    _lastCheckTime = now;

    _tagChanged = false; // Reset trigger for this frame
    bool hardwarePresent = checkHardware();

    if (hardwarePresent) {
        _missingCount = 0;

        // Read UID from the library instance
        uint32_t newUid = 0;
        for (byte i = 0; i < _mfrc522->uid.size; i++) {
            newUid = (newUid << 8) | _mfrc522->uid.uidByte[i];
        }

        // Check if it's a new tag or the same one
        if (!_lastStableTag.valid || _lastStableTag.uid != newUid) {
            Serial.print(F("RFID: New Tag Detected: "));
            Serial.println(newUid, HEX);
            _lastStableTag.uid = newUid;
            _lastStableTag.valid = true;
            _tagChanged = true;
        }

        // Halt the card so it stops talking, but we can wake it up next time
        _mfrc522->PICC_HaltA();
        _mfrc522->PCD_StopCrypto1();

    } else {
        // No card detected
        if (_lastStableTag.valid) {
            _missingCount++;
            if (_missingCount >= CONF_RFID_MISSING_THRESHOLD) {
                Serial.println(F("RFID: Tag Removed"));
                // Tag is definitely gone
                _lastStableTag.valid = false;
                _lastStableTag.uid = 0;
                _tagChanged = true;
                _missingCount = 0;
            }
        }
    }
}

bool RfidManager::checkHardware() {
    if (!_mfrc522) return false;

    // 1. Try standard detection (finds new or non-halted cards)
    if (_mfrc522->PICC_IsNewCardPresent() && _mfrc522->PICC_ReadCardSerial()) {
        return true;
    }

    // 2. Try to wake up a halted card (WUPA - 0x52)
    // This is necessary because we HaltA() the card in the previous loop.
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    if (_mfrc522->PICC_WakeupA(bufferATQA, &bufferSize) == MFRC522::STATUS_OK) {
        if (_mfrc522->PICC_ReadCardSerial()) return true;
    }

    return false;
}

RfidTag RfidManager::getCurrentTag() { return _lastStableTag; }
bool RfidManager::isTagChanged() { return _tagChanged; }