#pragma once
#include <Arduino.h>

// --- Hardware Pins ---
#define CONF_PIN_POWER_OFF   7
#define CONF_PIN_AUDIO_RX    2
#define CONF_PIN_AUDIO_TX    3
#define CONF_PIN_RFID_SS     10
#define CONF_PIN_RFID_RST    9
#define CONF_PIN_LEDS        4

#define CONF_PIN_BTN_PREV    A0
#define CONF_PIN_BTN_PLAY    A1
#define CONF_PIN_BTN_NEXT    A2

// --- Audio Settings ---
#define CONF_AUDIO_VOL_DEFAULT 7
#define CONF_AUDIO_VOL_MAX     30

// --- LED Settings ---
#define CONF_LED_COUNT       24
#define CONF_LED_BRIGHTNESS  25

// --- LED Animation Settings ---
#define CONF_LED_BREATHE_SPEED 5   // Higher = Slower
#define CONF_LED_SNAKE_SPEED   6  // Higher = Slower
#define CONF_LED_COLOR_SPEED   6  // 0 = Fixed, Higher = Slower
#define CONF_RFID_CHECK_INTERVAL_MS 100 // How often to poll the RFID hardware

// --- System Timeouts (ms) ---
#define CONF_TIMEOUT_IDLE          60000   // 1 minute
#define CONF_TIMEOUT_PAUSE         300000  // 5 minutes
#define CONF_TIMEOUT_RESUME_WINDOW 10000   // 10 seconds

// --- Input Settings ---
#define CONF_BTN_DEBOUNCE_MS    20
#define CONF_BTN_LONG_PRESS_MS  500
#define CONF_BTN_DOUBLE_CLICK_MS 400
#define CONF_BTN_REPEAT_MS      200

// --- Button Layouts ---
#define LAYOUT_STANDARD      0
#define LAYOUT_SIMPLE_VOLUME 1

#define CONF_BUTTON_LAYOUT   LAYOUT_SIMPLE_VOLUME