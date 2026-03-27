#include "AudioManager.h"
#include "../Config.h"

AudioManager::AudioManager() : _currentVolume(CONF_AUDIO_VOL_DEFAULT), _trackFinished(false) {}

void AudioManager::begin(uint8_t rxPin, uint8_t txPin) {
    // Initialize SoftwareSerial dynamically to allow pin configuration in setup
    if (!_serial) {
        _serial = new SoftwareSerial(rxPin, txPin);
    }
    _serial->begin(9600);

    // Give the DFPlayer some time to boot up to prevent startup noise
    delay(1000);

    // Initialize DFPlayer
    // Disable ACK (second param = false) to prevent blocking on every command (like volume changes)
    if (!_player.begin(*_serial, false)) {
        Serial.println(F("DFPlayer Error: Check connections!"));
    } else {
        Serial.println(F("DFPlayer Online."));
        _player.setTimeOut(500); // Set serial communication timeout
        
        // Initialize with volume 0 to prevent popping, then configure
        _player.volume(0);
        delay(100); // Wait for command to process
        _player.EQ(DFPLAYER_EQ_NORMAL);
        delay(100);
        _player.outputDevice(DFPLAYER_DEVICE_SD);
        delay(100);
        _player.volume(_currentVolume);
        delay(100);
    }
}

void AudioManager::update() {
    // Drain the serial buffer to prevent overflow from status messages.
    // Keep this unthrottled because it's very lightweight.
    // Drain the SoftwareSerial buffer frequently to avoid overflows and missing errors.
    if (_player.available()) {
        uint8_t type = _player.readType();
        int value = _player.read();

        switch (type) {
            case DFPlayerPlayFinished:
                Serial.print(F("DFPlayer: Track finished: ")); Serial.println(value);
                _trackFinished = true;
                break;
            case DFPlayerError:
                Serial.print(F("DFPlayer Error: "));
                switch (value) {
                    case FileIndexOut: Serial.println(F("File Index Out")); break;
                    case FileMismatch: Serial.println(F("File Mismatch")); break;
                    default: Serial.print(F("Code ")); Serial.println(value); break;
                }
                break;
            default:
                // Other messages are ignored for now.
                break;
        }
    }
}

void AudioManager::play(uint8_t folder, uint8_t track, uint32_t startPositionMs) {
    _player.playFolder(folder, track);
    // Note: DFPlayer Mini generally plays from the start. 
    // Seeking immediately after play requires delay/feedback which is brittle.
    // This parameter is reserved for better hardware.
}

void AudioManager::seek(uint32_t positionMs) {
    // Stub for future hardware
}

void AudioManager::pause() {
    _player.pause();
}

void AudioManager::resume() {
    _player.start();
}

void AudioManager::stop() {
    _player.stop();
}

void AudioManager::setVolume(uint8_t volume) { 
    _currentVolume = volume; 
    _player.volume(volume);
}

uint8_t AudioManager::getVolume() const { return _currentVolume; }

bool AudioManager::isPlaying() { 
    // Queries the DFPlayer for its current status.
    // readState() returns:
    // - 0: Stopped, 1: Playing, 2: Paused, 3: Sleeping
    // - 513: Reading State (sometimes seen during transitions/noise)
    // Strictly check for 1 to confirm active playback.
    return _player.readState() == 1; 
}

uint32_t AudioManager::getPositionMs() {
    return 0; // Stub: DFPlayer readCurrentTime() is often slow/unreliable
}

uint32_t AudioManager::getDurationMs() {
    return 0; // Stub: DFPlayer readTotalTime() is often slow/unreliable
}

int AudioManager::getTrackCount(uint8_t folder) {
    return _player.readFileCountsInFolder(folder);
}

bool AudioManager::hasTrackFinished() {
    if (_trackFinished) {
        _trackFinished = false; // Reset flag after reading
        return true;
    }
    return false;
}