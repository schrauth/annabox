#include <Arduino.h>
#include "Config.h"
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


void setup() {
    Serial.begin(115200);
    Serial.println(F("AnnaBox Booting..."));
    
    // Initialize Power Control Pin
    pinMode(CONF_PIN_POWER_OFF, OUTPUT);
    digitalWrite(CONF_PIN_POWER_OFF, LOW); // Ensure we don't kill power immediately

    // Initialize Subsystems
    inputManager.begin();
    audioManager.begin(CONF_PIN_AUDIO_RX, CONF_PIN_AUDIO_TX);
    rfidManager.begin(CONF_PIN_RFID_SS, CONF_PIN_RFID_RST);
    ledManager.begin(CONF_PIN_LEDS, CONF_LED_COUNT);
    persistenceManager.begin();
    
    // Initialize Controller
    systemController.begin();
}

void loop() {
    // Delegate control to the System Controller
    systemController.update();
}
