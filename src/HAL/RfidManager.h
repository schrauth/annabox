#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>

struct RfidTag {
    uint32_t uid;
    bool valid;
};

class RfidManager {
public:
    RfidManager();

    void begin(uint8_t ssPin, uint8_t rstPin);
    void update();

    RfidTag getCurrentTag();
    bool isTagChanged();

private:
    MFRC522* _mfrc522;
    uint8_t _ssPin;
    uint8_t _rstPin;

    RfidTag _lastStableTag;
    bool _tagChanged;

    // State tracking for debouncing removal
    uint32_t _lastCheckTime;
    uint8_t _missingCount;

    bool checkHardware();
};