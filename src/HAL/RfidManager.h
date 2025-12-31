#pragma once
#include <Arduino.h>
#include <stdint.h>

struct RfidTag {
    uint32_t uid;
    bool valid; // True if a tag is actually present
};

class RfidManager {
public:
    RfidManager();

    // Initialize SPI and RC522
    void begin(uint8_t ssPin, uint8_t rstPin);

    // Checks hardware and manages grace period timers. Call in loop.
    void update();

    // Returns the current logical tag (stable state after anti-jitter)
    RfidTag getCurrentTag(); 
    
    // Returns true if the logical tag has changed in this frame
    bool isTagChanged();

private:
    static constexpr uint32_t GRACE_PERIOD_MS = 2000;
    
    // TODO: Add MFRC522 driver instance
    // TODO: Add state variables for grace period timer
    RfidTag _lastStableTag;
};