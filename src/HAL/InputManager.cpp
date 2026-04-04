#include "InputManager.h"
#include "../Config.h"

InputManager::InputManager() : _head(0), _tail(0) {
    // Setting up Button Configurations
    
#if CONF_BUTTON_LAYOUT == LAYOUT_SIMPLE_VOLUME
    // Layout: Simple Volume
    // Btn 1: Short/Long -> Vol Down
    _configs[0] = {PIN_BTN_PREV, UserCommand::VOL_DOWN, UserCommand::NONE, UserCommand::VOL_DOWN, true};
    // Btn 2: Short -> Play/Pause, Double -> Next, Long -> Power
    _configs[1] = {PIN_BTN_PLAY, UserCommand::PLAY_PAUSE, UserCommand::NEXT, UserCommand::POWER_REQ, false};
    // Btn 3: Short/Long -> Vol Up
    _configs[2] = {PIN_BTN_NEXT, UserCommand::VOL_UP, UserCommand::NONE, UserCommand::VOL_UP, true};
#else
    // Layout: Standard
    // Btn 1: Short -> Prev, Long -> Vol Down
    _configs[0] = {PIN_BTN_PREV, UserCommand::PREV, UserCommand::NONE, UserCommand::VOL_DOWN, true};
    // Btn 2: Short -> Play/Pause, Long -> Power
    _configs[1] = {PIN_BTN_PLAY, UserCommand::PLAY_PAUSE, UserCommand::NONE, UserCommand::POWER_REQ, false};
    // Btn 3: Short -> Next, Long -> Vol Up
    _configs[2] = {PIN_BTN_NEXT, UserCommand::NEXT, UserCommand::NONE, UserCommand::VOL_UP, true};
#endif

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
        _states[i].waitingForDoubleClick = false;
        _states[i].lastReleaseTime = 0;
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

bool InputManager::isButtonPressed(uint8_t pin) const {
    for (uint8_t i = 0; i < NUM_BUTTONS; i++) {
        if (_configs[i].pin == pin) {
            return _states[i].stableState;
        }
    }
    return false;
}

void InputManager::pushCommand(UserCommand cmd) {
    uint8_t nextHead = (_head + 1) % CMD_BUFFER_SIZE;
    
    // If the buffer is full, drop the new command to preserve the oldest events.
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
                // If a long press hasn't been triggered yet, check for short or double press
                if (!state.longPressActive) {
                    if (config.doublePressCmd != UserCommand::NONE) {
                        if (state.waitingForDoubleClick) {
                            // Second click detected!
                            Serial.print(F("BTN Double: ")); Serial.println(config.pin);
                            pushCommand(config.doublePressCmd);
                            state.waitingForDoubleClick = false;
                        } else {
                            // First click, start waiting
                            state.waitingForDoubleClick = true;
                            state.lastReleaseTime = now;
                        }
                    } else {
                        // No double click configured, trigger short press immediately
                        Serial.print(F("BTN Short: ")); Serial.println(config.pin);
                        pushCommand(config.shortPressCmd);
                    }
                }
            }
        }
    }

    // 4. Double Click Timeout
    // If we are waiting for a second click, the button is released, and time has passed -> Trigger Short Press
    if (state.waitingForDoubleClick && !state.stableState && (now - state.lastReleaseTime > DOUBLE_CLICK_MS)) {
        state.waitingForDoubleClick = false;
        Serial.print(F("BTN Short (Delayed): ")); Serial.println(config.pin);
        pushCommand(config.shortPressCmd);
    }

    // 3. Long Press & Continuous Logic
    if (state.stableState) { // If button is held down
        uint32_t pressDuration = now - state.pressStartTime;

        if (pressDuration >= LONG_PRESS_DELAY_MS) {
            if (!state.longPressActive) {
                // First time crossing threshold
                state.longPressActive = true;
                state.waitingForDoubleClick = false; // Cancel any pending double click
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

                    // Safety: If we drifted too far behind (e.g. > 2 steps), reset to now.
                    // This prevents a burst of commands if the system was blocked for a long time.
                    if (now - state.lastRepeatTime > (REPEAT_DELAY_MS * 2)) {
                        state.lastRepeatTime = now;
                    }
                }
            }
        }
    }
}
