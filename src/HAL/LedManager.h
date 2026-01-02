#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <Adafruit_NeoPixel.h>

enum class LedState {
    OFF,
    BOOT,
    IDLE_BREATHE,
    PLAYING,
    PAUSED,
    SHUTDOWN
};

class LedManager {
public:
    LedManager();

    // Initialize the LED strip
    void begin(uint8_t pin, uint8_t numLeds);

    // Update animations (call in loop)
    void update();

    // Set the visual state
    void setState(LedState state);

    // Temporary overlay for volume feedback
    void showVolume(uint8_t currentVol, uint8_t maxVol);

private:
    LedState _currentState;
    Adafruit_NeoPixel _strip;
    
    uint32_t _lastUpdate;
    uint16_t _animStep;
    
    bool _isVolumeOverlay;
    uint32_t _volumeStart;
    
    uint32_t Wheel(byte WheelPos);
};