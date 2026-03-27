#include "SystemController.h"
#include "../Config.h"

// Timeout configuration
static constexpr uint32_t IDLE_TIMEOUT_MS = CONF_TIMEOUT_IDLE;
static constexpr uint32_t PAUSE_TIMEOUT_MS = CONF_TIMEOUT_PAUSE;
static constexpr uint32_t RESUME_WINDOW_MS = CONF_TIMEOUT_RESUME_WINDOW;

// State variables for RFID logic (Static to persist without modifying header)
static uint32_t s_lastUid = 0;
static uint32_t s_lastCardRemoveTime = 0;

// --- Card Mapping Configuration ---
struct CardMapping {
    uint32_t uid;
    uint8_t folder;
};

static const CardMapping s_knownCards[] = {
    {0x03F44306, 1}, // Test Card 1 -> Folder 01
    {0x4652F705, 2}, // Test Card 2 -> Folder 02 
    {0x71D18EF5, 3},
};
static const uint8_t s_numKnownCards = sizeof(s_knownCards) / sizeof(s_knownCards[0]);

SystemController::SystemController(InputManager& input, AudioManager& audio, RfidManager& rfid, LedManager& leds, PersistenceManager& persist)
    : _input(input), _audio(audio), _rfid(rfid), _leds(leds), _persist(persist), 
      _currentState(SystemState::IDLE), _lastActivityTime(0),
      _currentFolder(1), _currentTrack(1), _currentFolderTrackCount(0) {
}

void SystemController::begin() {
    // Load last state if needed, or just start fresh
    _lastActivityTime = millis();
    changeState(SystemState::IDLE);
}

void SystemController::update() {
    // 1. Update Input & Process Immediately (Low Latency)
    _input.update();
    
    UserCommand cmd = _input.popCommand();
    if (cmd != UserCommand::NONE) {
        processInput(cmd);
        _lastActivityTime = millis(); // Reset idle timer on interaction
    }

    // 2. Update RFID (Potentially Blocking)
    _rfid.update();
    if (_rfid.isTagChanged()) {
        processRfidChange();
    }

    // 3. Update Audio & LEDs
    _audio.update();
    _leds.update();

    // 4. Run State Logic
    switch (_currentState) {
        case SystemState::IDLE:        handleStateIdle(); break;
        case SystemState::PLAYING:     handleStatePlaying(); break;
        case SystemState::PAUSED:      handleStatePaused(); break;
        case SystemState::SLEEP_TIMER: handleStatePlaying(); break; // Re-use playing logic
        case SystemState::SHUTDOWN:    handleStateShutdown(); break;
    }
}

void SystemController::changeState(SystemState newState) {
    if (_currentState == newState) return;

    // --- EXIT LOGIC (Clean up previous state) ---
    switch (_currentState) {
        case SystemState::PLAYING:
        case SystemState::SLEEP_TIMER:
            // When leaving playing, we might want to save position
            // But we do that specifically in Pause/Shutdown logic usually
            break;
        default: break;
    }

    _currentState = newState;
    _lastActivityTime = millis(); // Reset timers on state change

    // --- ENTRY LOGIC (Setup new state) ---
    switch (_currentState) {
        case SystemState::IDLE:
            Serial.println(F("State: IDLE"));
            _leds.setState(LedState::IDLE_BREATHE);
            break;
        case SystemState::PLAYING:
            Serial.println(F("State: PLAYING"));
            _leds.setState(LedState::PLAYING);
            break;
        case SystemState::PAUSED:
            Serial.println(F("State: PAUSED"));
            _leds.setState(LedState::PAUSED);
            _audio.pause();
            // Save position immediately upon pausing
            // _persist.savePlaybackState(...); 
            break;
        case SystemState::SHUTDOWN:
            Serial.println(F("State: SHUTDOWN"));
            _leds.setState(LedState::SHUTDOWN);
            _audio.stop();
            // _persist.savePlaybackState(...);
            break;
        default: break;
    }
}

// --- FSM Handlers ---

void SystemController::handleStateIdle() {
    // If idle for too long, shutdown
    if (millis() - _lastActivityTime > IDLE_TIMEOUT_MS) {
        changeState(SystemState::SHUTDOWN);
    }
}

void SystemController::handleStatePlaying() {
    // Main playback logic is handled by Audio/RFID modules.
    // Here we check if audio finished naturally to advance to the next track.
    if (_audio.hasTrackFinished()) {
        Serial.println(F("Track finished, advancing..."));
        
        // Advance to the next track
        if (_currentFolderTrackCount > 0) {
            _currentTrack++;
            if (_currentTrack > _currentFolderTrackCount) {
                _currentTrack = 1; // Loop back to the start
            }
        } else {
            // If we don't know the track count, just increment.
            // The DFPlayer will fail to play if it's out of range,
            // which will be reported as an error in the AudioManager.
            _currentTrack++;
        }
        
        Serial.print(F("Playing next track: ")); Serial.println(_currentTrack);
        _audio.play(_currentFolder, _currentTrack);
    }
}

void SystemController::handleStatePaused() {
    // Check if card is missing
    if (!_rfid.getCurrentTag().valid) {
        // Card is gone. Check Resume Window.
        if (millis() - s_lastCardRemoveTime > RESUME_WINDOW_MS) {
             // Window expired. Reset session to IDLE.
             changeState(SystemState::IDLE);
        }
    } else {
        // Card is present (User paused). Long timeout.
        if (millis() - _lastActivityTime > PAUSE_TIMEOUT_MS) {
            changeState(SystemState::SHUTDOWN);
        }
    }
}

void SystemController::handleStateShutdown() {
    // Signal main loop or hardware to cut power
    // In this architecture, we might send the POWER_REQ command back to main
    // or handle the pin directly if we had access. 
    // Since InputManager handles the button, we rely on main.cpp to read the state 
    // or we trigger the shutdown pin here if we inject the pin number.
    
    // For now, we just stay here until power is cut.
    digitalWrite(CONF_PIN_POWER_OFF, HIGH);
}

// --- Event Processors ---

void SystemController::processRfidChange() {
    RfidTag tag = _rfid.getCurrentTag();

    if (tag.valid) {
        // New Tag Inserted
        Serial.print(F("Tag Found: ")); Serial.println(tag.uid, HEX);

        // Check for resume condition: Same card AND within time window
        if (tag.uid == s_lastUid && (millis() - s_lastCardRemoveTime < RESUME_WINDOW_MS)) {
             Serial.println(F("Resuming session..."));
             _audio.resume();
        } else {
             Serial.println(F("Starting new session..."));
             
             uint8_t folder = 1; // Default
             bool found = false;

             // Check explicit mappings
             for (uint8_t i = 0; i < s_numKnownCards; i++) {
                 if (s_knownCards[i].uid == tag.uid) {
                     folder = s_knownCards[i].folder;
                     found = true;
                     Serial.print(F("Mapping found -> Folder ")); Serial.println(folder);
                     break;
                 }
             }

             if (!found) {
                 // Fallback: Modulo 10 + 1 maps any UID to folders 01-10
                 folder = (tag.uid % 10) + 1;
                 Serial.println(F("Unknown Card - Using fallback mapping"));
             }
             
             Serial.print(F("Playing Folder: ")); Serial.println(folder);
             _currentFolder = folder;
             _currentTrack = 1;
             
             // Query track count for cyclic navigation
             _currentFolderTrackCount = _audio.getTrackCount(_currentFolder);
             
             _audio.play(_currentFolder, _currentTrack);
             s_lastUid = tag.uid;
        }
        changeState(SystemState::PLAYING);
    } else {
        // Tag Removed
        Serial.println(F("Tag Removed"));
        s_lastCardRemoveTime = millis();
        
        if (_currentState == SystemState::PLAYING || _currentState == SystemState::SLEEP_TIMER) {
            changeState(SystemState::PAUSED);
        } else if (_currentState == SystemState::PAUSED) {
            // Already paused, but now the card is physically gone.
            // The handleStatePaused() loop will now monitor the RESUME_WINDOW_MS.
            Serial.println(F("Card removed while PAUSED. Resume window active."));
        }
    }
}

void SystemController::processInput(UserCommand cmd) {
    Serial.print(F("CMD Received: ")); Serial.println((int)cmd);
    switch (cmd) {
        case UserCommand::PLAY_PAUSE:
            if (_currentState == SystemState::PLAYING) {
                changeState(SystemState::PAUSED);
            } else if (_currentState == SystemState::PAUSED || _currentState == SystemState::IDLE) {
                // Only resume if we have a valid tag context, otherwise ignore or play default
                if (_rfid.getCurrentTag().valid) {
                    _audio.resume();
                    changeState(SystemState::PLAYING);
                }
            }
            break;
            
        case UserCommand::VOL_UP:
            if (_audio.getVolume() < CONF_AUDIO_VOL_MAX) {
                _audio.setVolume(_audio.getVolume() + 1);
            }
            _leds.showVolume(_audio.getVolume(), CONF_AUDIO_VOL_MAX);
            break;
            
        case UserCommand::VOL_DOWN:
            if (_audio.getVolume() > 0) {
                _audio.setVolume(_audio.getVolume() - 1);
            }
            _leds.showVolume(_audio.getVolume(), CONF_AUDIO_VOL_MAX);
            break;
            
        case UserCommand::POWER_REQ:
            changeState(SystemState::SHUTDOWN);
            break;
            
        case UserCommand::NEXT:
            if (_currentFolderTrackCount > 0) {
                _currentTrack++;
                if (_currentTrack > _currentFolderTrackCount) _currentTrack = 1;
            } else {
                _currentTrack++;
            }
            _audio.play(_currentFolder, _currentTrack);
            break;

        case UserCommand::PREV:
            if (_currentFolderTrackCount > 0) {
                if (_currentTrack > 1) {
                    _currentTrack--;
                } else {
                    _currentTrack = _currentFolderTrackCount;
                }
            } else {
                if (_currentTrack > 1) _currentTrack--;
            }
            _audio.play(_currentFolder, _currentTrack);
            break;
            
        default: break; 
    }
}