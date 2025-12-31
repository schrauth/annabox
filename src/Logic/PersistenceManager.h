#pragma once
#include <stdint.h>

struct PlaybackState {
    uint8_t folder;
    uint8_t track;
    uint8_t volume;
    bool isValid;
};

class PersistenceManager {
public:
    void begin();
    
    // Save current position (call when pausing or periodically)
    void savePlaybackState(const PlaybackState& state);
    
    // Retrieve last known position on boot
    PlaybackState loadPlaybackState();
};