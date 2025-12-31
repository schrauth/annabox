#pragma once
#include <Arduino.h>
#include <stdint.h>

class AudioManager {
public:
    AudioManager();

    // Initialize SoftwareSerial for DFPlayer
    void begin(uint8_t rxPin, uint8_t txPin);
    
    // Periodic updates if needed (e.g. querying status)
    void update();

    void play(uint8_t folder, uint8_t track);
    void pause();
    void resume();
    void stop();
    
    void setVolume(uint8_t volume); // Range 0-30
    uint8_t getVolume() const;
    
    bool isPlaying();

private:
    uint8_t _currentVolume;
    // TODO: Add DFPlayer driver instance (e.g. DFRobotDFPlayerMini)
};