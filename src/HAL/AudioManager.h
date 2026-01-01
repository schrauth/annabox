#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

class AudioManager {
public:
    AudioManager();

    // Initialize SoftwareSerial for DFPlayer
    void begin(uint8_t rxPin, uint8_t txPin);
    
    // Periodic updates if needed (e.g. querying status)
    void update();

    // Playback Control
    // startPositionMs is added for future compatibility with advanced players
    void play(uint8_t folder, uint8_t track, uint32_t startPositionMs = 0);
    void seek(uint32_t positionMs);

    void pause();
    void resume();
    void stop();
    
    void setVolume(uint8_t volume); // Range 0-30
    uint8_t getVolume() const;
    
    bool isPlaying();
    
    // Status queries for future advanced players
    uint32_t getPositionMs();
    uint32_t getDurationMs();

private:
    uint8_t _currentVolume;
    SoftwareSerial* _serial = nullptr;
    DFRobotDFPlayerMini _player;
};