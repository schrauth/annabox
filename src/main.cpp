#include <Arduino.h>
#include "HAL/InputManager.h"
#include "HAL/AudioManager.h"
#include "HAL/RfidManager.h"
#include "HAL/LedManager.h"
#include "Logic/PersistenceManager.h"
#include "Logic/SystemController.h"

// 1. Instantiate Hardware Modules
InputManager inputManager;
AudioManager audioManager;
RfidManager rfidManager;
LedManager ledManager;

// 2. Instantiate Logic Modules
PersistenceManager persistenceManager;

// 3. Instantiate System Controller (Dependency Injection)
SystemController systemController(inputManager, audioManager, rfidManager, ledManager, persistenceManager);

// Hardware Configuration
constexpr uint8_t PIN_POWER_OFF = 7; // D7 signals Pololu to cut power
// Audio Pins (SoftwareSerial)
constexpr uint8_t PIN_AUDIO_RX = 2;
constexpr uint8_t PIN_AUDIO_TX = 3;
// RFID Pins (SPI) - Standard Nano SPI pins (11, 12, 13) + Configurable SS/RST
constexpr uint8_t PIN_RFID_SS = 10;
constexpr uint8_t PIN_RFID_RST = 9;
// LED Pin
constexpr uint8_t PIN_LEDS = 4;
constexpr uint8_t NUM_LEDS = 24; // Standard ring size

void setup() {
    Serial.begin(115200);
    Serial.println(F("AnnaBox Booting..."));
    
    // Initialize Power Control Pin
    pinMode(PIN_POWER_OFF, OUTPUT);
    digitalWrite(PIN_POWER_OFF, LOW); // Ensure we don't kill power immediately

    // Initialize Subsystems
    inputManager.begin();
    audioManager.begin(PIN_AUDIO_RX, PIN_AUDIO_TX);
    rfidManager.begin(PIN_RFID_SS, PIN_RFID_RST);
    ledManager.begin(PIN_LEDS, NUM_LEDS);
    persistenceManager.begin();
    
    // Initialize Controller
    systemController.begin();
}

void loop() {
    // Delegate control to the System Controller
    systemController.update();
}
