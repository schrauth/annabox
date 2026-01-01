#include "SystemController.h"

// Timeout configuration
static constexpr uint32_t IDLE_TIMEOUT_MS = 60000; // 1 minute to shutdown if idle
static constexpr uint32_t PAUSE_TIMEOUT_MS = 300000; // 5 minutes to shutdown if paused
static constexpr uint32_t RESUME_WINDOW_MS = 10000; // 10 seconds to resume after removal

// State variables for RFID logic (Static to persist without modifying header)
static uint32_t s_lastUid = 0;
static uint32_t s_lastCardRemoveTime = 0;

SystemController::SystemController(InputManager& input, AudioManager& audio, RfidManager& rfid, LedManager& leds, PersistenceManager& persist)
    : _input(input), _audio(audio), _rfid(rfid), _leds(leds), _persist(persist), 
      _currentState(SystemState::IDLE), _lastActivityTime(0) {
}

void SystemController::begin() {
    // Load last state if needed, or just start fresh
    _lastActivityTime = millis();
    changeState(SystemState::IDLE);
}

void SystemController::update() {
    // 1. Update Hardware Wrappers
    _input.update();
    _rfid.update();
    _audio.update();
    _leds.update();

    // 2. Process Global Inputs (RFID changes affect all states)
    if (_rfid.isTagChanged()) {
        processRfidChange();
    }

    // 3. Process User Input
    UserCommand cmd = _input.popCommand();
    if (cmd != UserCommand::NONE) {
        processInput(cmd);
        _lastActivityTime = millis(); // Reset idle timer on interaction
    }

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
    // Here we just check if audio finished naturally (if supported by HAL)
    // or handle specific playing-only logic.
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
}

// --- Event Processors ---

void SystemController::processRfidChange() {
    RfidTag tag = _rfid.getCurrentTag();

    if (tag.valid) {
        // New Tag Inserted
        Serial.print(F("Tag Found: ")); Serial.println(tag.uid);

        // Check for resume condition: Same card AND within time window
        if (tag.uid == s_lastUid && (millis() - s_lastCardRemoveTime < RESUME_WINDOW_MS)) {
             Serial.println(F("Resuming session..."));
             _audio.resume();
        } else {
             Serial.println(F("Starting new session..."));
             // TODO: Map UID to Folder/Track
             _audio.play(1, 1);
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
            _audio.setVolume(_audio.getVolume() + 1);
            _leds.showVolume(_audio.getVolume(), 30); // Assuming 30 is max for DFPlayer
            break;
            
        case UserCommand::VOL_DOWN:
            _audio.setVolume(_audio.getVolume() - 1);
            _leds.showVolume(_audio.getVolume(), 30);
            break;
            
        case UserCommand::POWER_REQ:
            changeState(SystemState::SHUTDOWN);
            break;
            
        // TODO: Handle NEXT/PREV
        default: break;
    }
}