#include "InputManager.h"

InputManager::InputManager() : _head(0), _tail(0) {
    // Setting up Button Configurations
    
    // Button 1 (A0): Short press -> PREV, Long press -> VOL_DOWN (Continuous)
    _configs[0] = {PIN_BTN_PREV, UserCommand::PREV, UserCommand::VOL_DOWN, true};

    // Button 2 (A1): Short press -> PLAY_PAUSE, Long press -> POWER_REQ (One-shot)
    _configs[1] = {PIN_BTN_PLAY, UserCommand::PLAY_PAUSE, UserCommand::POWER_REQ, false};

    // Button 3 (A2): Short press -> NEXT, Long press -> VOL_UP (Continuous)
    _configs[2] = {PIN_BTN_NEXT, UserCommand::NEXT, UserCommand::VOL_UP, true};

    // Clearing the command buffer
    for (uint8_t i = 0; i < CMD_BUFFER_SIZE; i++) {
        _cmdBuffer[i] = UserCommand::NONE;
    }
}

void InputManager::begin() {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        pinMode(_configs[i].pin, INPUT_PULLUP);
        
        // Reset states
        _states[i].stableState = false; // Assume released initially
        _states[i].lastReading = HIGH;  // Pullup implies HIGH is released
        _states[i].longPressActive = false;
        _states[i].lastDebounceTime = 0;
        _states[i].pressStartTime = 0;
        _states[i].lastRepeatTime = 0;
    }
}

void InputManager::update() {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        processButton(i);
    }
}

UserCommand InputManager::popCommand() {
    if (_head == _tail) {
        return UserCommand::NONE;
    }
    
    UserCommand cmd = _cmdBuffer[_tail];
    _tail = (_tail + 1) % CMD_BUFFER_SIZE;
    return cmd;
}

void InputManager::pushCommand(UserCommand cmd) {
    uint8_t nextHead = (_head + 1) % CMD_BUFFER_SIZE;
    
    // If the buffer is full, I'll drop the new command to preserve the oldest events.
    // In a responsive loop, this should rarely happen.
    if (nextHead != _tail) {
        _cmdBuffer[_head] = cmd;
        _head = nextHead;
    }
}

void InputManager::processButton(uint8_t index) {
    const ButtonConfig& config = _configs[index];
    ButtonState& state = _states[index];
    
    // 1. Reading Hardware (Active LOW)
    bool currentReading = digitalRead(config.pin); 
    uint32_t now = millis();

    // 2. Handling Software Debouncing
    if (currentReading != state.lastReading) {
        state.lastDebounceTime = now;
    }
    state.lastReading = currentReading;

    if ((now - state.lastDebounceTime) > DEBOUNCE_DELAY_MS) {
        // Determining physical state (Active LOW means Pressed)
        bool isPressed = (currentReading == LOW);

        // State Change Detection
        if (isPressed != state.stableState) {
            state.stableState = isPressed;

            if (state.stableState) {
                // Edge: Pressed
                state.pressStartTime = now;
                state.longPressActive = false;
            } else {
                // Edge: Released
                // If I haven't triggered a long press yet, it's a short press
                if (!state.longPressActive) {
                    Serial.print(F("BTN Short: ")); Serial.println(config.pin);
                    pushCommand(config.shortPressCmd);
                }
            }
        }
    }

    // 3. Long Press & Continuous Logic
    if (state.stableState) { // If button is held down
        uint32_t pressDuration = now - state.pressStartTime;

        if (pressDuration >= LONG_PRESS_DELAY_MS) {
            if (!state.longPressActive) {
                // First time crossing threshold
                state.longPressActive = true;
                Serial.print(F("BTN Long Start: ")); Serial.println(config.pin);
                pushCommand(config.longPressCmd);
                state.lastRepeatTime = now;
            } else if (config.continuousLongPress) {
                // Already active, checking for repeat interval
                if ((now - state.lastRepeatTime) >= REPEAT_DELAY_MS) {
                    // Serial.print(F("BTN Long Repeat: ")); Serial.println(config.pin);
                    pushCommand(config.longPressCmd);
                    // Use additive timing to maintain rhythm despite loop jitter
                    state.lastRepeatTime += REPEAT_DELAY_MS;
                }
            }
        }
    }
}
