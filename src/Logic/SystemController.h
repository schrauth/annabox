#pragma once

#include "../HAL/InputManager.h"
#include "../HAL/AudioManager.h"
#include "../HAL/RfidManager.h"
#include "../HAL/LedManager.h"
#include "PersistenceManager.h"

enum class SystemState {
    IDLE,           // Low power, waiting for input
    PLAYING,        // Active playback
    PAUSED,         // Paused, waiting for resume or timeout
    SLEEP_TIMER,    // Active playback with countdown
    SHUTDOWN        // Prepare for power cut
};

class SystemController {
public:
    // Dependency Injection: Controller doesn't own hardware, it orchestrates it.
    SystemController(InputManager& input, AudioManager& audio, RfidManager& rfid, LedManager& leds, PersistenceManager& persist);
    
    void begin();
    
    // Main Logic Loop
    void update();

private:
    // References to subsystems
    InputManager& _input;
    AudioManager& _audio;
    RfidManager& _rfid;
    LedManager& _leds;
    PersistenceManager& _persist;

    SystemState _currentState;
    uint32_t _lastActivityTime;

    // FSM Handlers
    void handleStateIdle();
    void handleStatePlaying();
    void handleStatePaused();
    void handleStateShutdown();

    // Centralized State Transition Logic (Entry/Exit actions)
    void changeState(SystemState newState);

    // Event Processors
    void processInput(UserCommand cmd);
    void processRfidChange();
};