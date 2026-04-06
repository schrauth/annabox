#pragma once
#include <Arduino.h>
#include <stdint.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

/**
 * @brief Hardware Abstraction Layer for the Audio Player.
 * 
 * Currently wraps the DFRobotDFPlayerMini library.
 * Designed with a generalized API to support hardware upgrades 
 * (e.g., players that support seeking or duration queries).
 */
class AudioManager {
public:
    AudioManager();

    /**
     * @brief Initializes the audio hardware.
     * @param rxPin The RX pin on the Arduino (connected to TX on DFPlayer).
     * @param txPin The TX pin on the Arduino (connected to RX on DFPlayer).
     */
    void begin(uint8_t rxPin, uint8_t txPin);
    
    /**
     * @brief Periodic update function.
     * Should be called in the main loop to handle serial buffer maintenance.
     */
    void update();

    /**
     * @brief Starts playback of a specific track in a folder.
     * @param folder The folder number (1-99). Folders on SD must be named "01", "02", etc.
     * @param track The track number (1-255). Files must be named "001.mp3", etc.
     * @param startPositionMs Optional start position in milliseconds. 
     *                        (can be ignored if not available on very simple hardware).
     */
    void play(uint8_t folder, uint8_t track, uint32_t startPositionMs = 0);

    /**
     * @brief Seeks to a specific position in the current track.
     * @param positionMs Position in milliseconds. (Stub for future hardware).
     */
    void seek(uint32_t positionMs);

    void pause();
    void resume();
    void stop();
    
    /**
     * @brief Sets the volume.
     * @param volume Level from 0 to 30 (Hardware limit of DFPlayer - adjust for other hardware).
     */
    void setVolume(uint8_t volume);
    uint8_t getVolume() const;
    
    bool isPlaying();
    
    // Status queries (Stubs for future advanced players)
    uint32_t getPositionMs();
    uint32_t getDurationMs();
    
    /**
     * @brief Gets the number of tracks in a specific folder.
     * @return Number of tracks, or -1/0 on error.
     */
    int getTrackCount(uint8_t folder);

    /**
     * @brief Checks if a track has just finished playing.
     * @return True if a "track finished" event was received since the last check.
     *         This is a one-shot flag that resets after being read.
     */
    bool hasTrackFinished();

private:
    uint8_t _currentVolume;
    bool _trackFinished;
    SoftwareSerial* _serial = nullptr;
    DFRobotDFPlayerMini _player;
};