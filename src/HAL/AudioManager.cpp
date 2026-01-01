#include "AudioManager.h"

AudioManager::AudioManager() : _currentVolume(15) {}

void AudioManager::begin(uint8_t rxPin, uint8_t txPin) {
    // Initialize SoftwareSerial dynamically to allow pin configuration in setup
    if (!_serial) {
        _serial = new SoftwareSerial(rxPin, txPin);
    }
    _serial->begin(9600);

    // Initialize DFPlayer
    if (!_player.begin(*_serial)) {
        Serial.println(F("DFPlayer Error: Check connections!"));
    } else {
        Serial.println(F("DFPlayer Online."));
        _player.volume(_currentVolume);
    }
}

void AudioManager::update() {
    // Drain the serial buffer to prevent overflow from status messages
    if (_player.available()) {
        // We could handle events here (like track finished), 
        // but for now we just keep the buffer clean.
        _player.readType(); 
    }
}

void AudioManager::play(uint8_t folder, uint8_t track) {
    _player.playFolder(folder, track);
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
    // 1 = DFPlayerPlay, 513 = Reading State? 
    // readState() returns the status constant.
    return _player.readState() == 1; 
}