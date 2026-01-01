#pragma once

#include <Arduino.h>
#include <stdint.h>
#include "../Config.h"

// Logical commands decoupled from hardware implementation
enum class UserCommand : uint8_t {
    NONE = 0,
    PLAY_PAUSE,
    PREV,
    NEXT,
    VOL_UP,
    VOL_DOWN,
    POWER_REQ // Power logic to be handled by the main state machine
};

class InputManager {
public:
    InputManager();

    // Configures pin modes and resets internal states
    void begin();

    // Reads inputs, handles debouncing, and detects press types.
    // Call this method in the main loop().
    void update();

    // Retrieves the next command from the event queue.
    // Returns UserCommand::NONE if the queue is empty.
    UserCommand popCommand();

private:
    // Configuration for a specific button hardware mapping
    struct ButtonConfig {
        uint8_t pin;
        UserCommand shortPressCmd;
        UserCommand longPressCmd;
        bool continuousLongPress; // If true, repeats command while held
    };

    // Internal state tracking for a button
    struct ButtonState {
        bool stableState;           // Debounced logical state (true = PRESSED)
        bool lastReading;           // Immediate hardware reading
        bool longPressActive;       // Flag if long press threshold was crossed
        uint32_t lastDebounceTime;  // Timestamp of last signal toggle
        uint32_t pressStartTime;    // Timestamp when button became stable PRESSED
        uint32_t lastRepeatTime;    // Timestamp for continuous trigger timing
    };

    // Timing Constants (constexpr for compile-time optimization)
    static constexpr uint8_t NUM_BUTTONS = 3;
    static constexpr uint32_t DEBOUNCE_DELAY_MS = CONF_BTN_DEBOUNCE_MS;
    static constexpr uint32_t LONG_PRESS_DELAY_MS = CONF_BTN_LONG_PRESS_MS;
    static constexpr uint32_t REPEAT_DELAY_MS = CONF_BTN_REPEAT_MS; // Speed of volume change
    static constexpr uint8_t CMD_BUFFER_SIZE = 4;    // Small buffer for events

    // Hardware Pin Configuration
    static constexpr uint8_t PIN_BTN_PREV = CONF_PIN_BTN_PREV;
    static constexpr uint8_t PIN_BTN_PLAY = CONF_PIN_BTN_PLAY;
    static constexpr uint8_t PIN_BTN_NEXT = CONF_PIN_BTN_NEXT;

    // Member Data
    ButtonConfig _configs[NUM_BUTTONS];
    ButtonState _states[NUM_BUTTONS];

    // Circular Buffer for Commands
    UserCommand _cmdBuffer[CMD_BUFFER_SIZE];
    uint8_t _head;
    uint8_t _tail;

    // Internal Helpers
    void processButton(uint8_t index);
    void pushCommand(UserCommand cmd);
};
