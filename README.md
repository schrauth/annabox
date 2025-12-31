# AnnaBox

**AnnaBox** is a embedded RFID-controlled audio player designed for children, built using PlatformIO and Arduino Nano microcontroller. It features a modular software architecture to manage audio playback, user input, and visual feedback via LEDs.

## Features

*   **RFID Control:** Playback is triggered by placing RFID tags (cards/fobs) on the device.
*   **Intuitive Controls:** Three physical buttons for navigation and volume:
    *   **Prev:** Short press for previous track, Long press for Volume Down.
    *   **Play/Pause:** Short press to toggle playback, Long press to request Power Off.
    *   **Next:** Short press for next track, Long press for Volume Up.
*   **Visual Feedback:** Neopixel (WS2812) LED ring provides status animations:
    *   **Idle:** Breathing Blue.
    *   **Playing:** Rainbow Snake animation.
    *   **Paused:** Breathing Amber.
    *   **Volume:** Visual gauge overlay when changing volume.
*   **Power Management:** Automatic shutdown after inactivity (Idle or Paused timeouts).

## Hardware

*   **Microcontroller:** Arduino Nano (ATmega328P)
*   **Audio:** DFPlayer Mini (UART)
*   **Input:** 3x Push Buttons
*   **RFID:** RC522 Module (SPI)
*   **LEDs:** Neopixel Ring/Strip
*   **Power:** Battery management circuit (Soft-latching power switch logic supported in software).

## Software Architecture

The project follows a layered architecture:

*   **HAL (Hardware Abstraction Layer):**
    *   `AudioManager`: Wraps DFPlayer Mini communication.
    *   `InputManager`: Handles button debouncing and short/long press detection.
    *   `LedManager`: Manages LED states and non-blocking animations.
    *   `RfidManager`: Handles tag detection (MFRC522).
*   **Logic:**
    *   `SystemController`: The central state machine coordinating inputs, audio, and LEDs.

## Building

1.  Install PlatformIO.
2.  Clone the repository.
3.  Build and upload to your Arduino Nano.