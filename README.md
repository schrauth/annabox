# AnnaBox

**AnnaBox** is a embedded RFID-controlled audio player designed for children, built using PlatformIO and Arduino Nano microcontroller. It features a modular software architecture to manage audio playback, user input, and visual feedback via LEDs.

## Features

*   **RFID Control:** Playback is triggered by placing RFID tags (cards/fobs) on the device.
    *   Supports specific card-to-folder mapping.
    *   Fallback mode: Unknown cards map to folders 1-10 automatically.
*   **Intuitive Controls:** Three physical buttons for navigation and volume:
    *   **Prev:** Short press for previous track, Long press for Volume Down.
    *   **Play/Pause:** Short press to toggle playback, Long press to request Power Off.
    *   **Next:** Short press for next track, Long press for Volume Up.
*   **Visual Feedback:** Neopixel (WS2812) LED ring provides status animations:
    *   **Idle:** Breathing Blue.
    *   **Playing:** Rotating Rainbow Snake animation.
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

## SD Card Setup

The DFPlayer Mini requires a specific folder structure to work with the mapping logic:

*   **Root Directory**
    *   `01` (Folder) -> Contains `001.mp3`, `002.mp3`, etc.
    *   `02` (Folder) -> Contains `001.mp3`, ...
    *   ...
    *   `99` (Folder)

**Note:** Folder names must be 2 digits. Filenames must start with 3 digits.

## Configuration

The project uses a central configuration file located at `src/Config.h`. You can adjust:
*   **Pin Mappings:** Audio, RFID, LEDs, Buttons, Power.
*   **Audio Settings:** Default/Max volume.
*   **LED Settings:** Brightness, Animation speeds, Colors.
*   **Timeouts:** Idle, Pause, Resume Window.
*   **Input Timings:** Debounce, Long Press, Repeat delays.

## Adding New Cards

1.  Open the Serial Monitor (115200 baud).
2.  Place a new card on the reader.
3.  Note the UID printed (e.g., `Tag Found: 4652F705`).
4.  Add the UID to the `s_knownCards` array in `src/Logic/SystemController.cpp` to map it to a specific folder.

## Software Architecture

The project follows a layered architecture:

*   **HAL (Hardware Abstraction Layer):**
    *   `AudioManager`: Wraps DFPlayer Mini communication.
    *   `InputManager`: Handles button debouncing and short/long press detection.
    *   `LedManager`: Manages LED states and non-blocking animations.
    *   `RfidManager`: Handles tag detection (MFRC522).
*   **Logic:**
    *   `SystemController`: The central state machine coordinating inputs, audio, and LEDs.
    *   `PersistenceManager`: (Planned) Saves state to EEPROM.

## Building

1.  Install PlatformIO.
2.  Clone the repository.
3.  Build and upload to your Arduino Nano.