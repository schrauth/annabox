#include "LedManager.h"
#include "../Config.h"
#include <avr/pgmspace.h>

// Precomputed Gamma 2.2 table (0-255) for natural brightness perception.
// Stored in Flash (PROGMEM) to save RAM.
static const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

LedManager::LedManager() 
    : _currentState(LedState::BOOT), 
      _strip(0, 0, NEO_GRB + NEO_KHZ800), // Initialize with dummy values; update in begin()
      _lastUpdate(0), _animStep(0),
      _isVolumeOverlay(false), _volumeStart(0) {}

void LedManager::begin(uint8_t pin, uint8_t numLeds) {
#if CONF_ENABLE_LEDS
    _strip.setPin(pin);
    _strip.updateLength(numLeds);
    _strip.updateType(NEO_GRB + NEO_KHZ800);
    _strip.begin();
    _strip.setBrightness(CONF_LED_BRIGHTNESS);
    _strip.show();
#endif
}

void LedManager::update() {
#if CONF_ENABLE_LEDS
    uint32_t now = millis();

    // 1. Handling Volume Overlay (Temporary Override)
    if (_isVolumeOverlay) {
        if (now - _volumeStart > 1500) {
            _isVolumeOverlay = false; // Timeout expired, reverting to state animation
        } else {
            return; // Do not run background animations while showing volume
        }
    }

    // 2. Throttling Animation Framerate (~30 FPS)
    if (now - _lastUpdate < 33) return;
    _lastUpdate = now;

    // 3. Processing the LED State Machine
    switch (_currentState) {
        case LedState::IDLE_BREATHE: {
            // Simple Triangle Wave for Breathing (Blue)
            int val = (now / CONF_LED_BREATHE_SPEED) % 512; 
            if (val > 255) val = 511 - val;
            
            // Use Gamma table for natural breathing curve.
            // Map input to 50-255 to avoid the flat bottom of the gamma curve (indices 0-40 are very low)
            // which causes a visual "pause" at low brightness.
            uint8_t index = map(val, 0, 255, 50, 255);
            uint8_t gammaVal = pgm_read_byte(&gamma8[index]);
            int brightness = map(gammaVal, 0, 255, CONF_LED_BREATHE_MIN_BRIGHTNESS, 255);

            _strip.fill(_strip.Color(0, 0, brightness / 2)); // Blue, max brightness ~128
            _strip.show();
            break;
        }
        case LedState::PLAYING: {
            // Rainbow Snake with sub-pixel rendering (Optimized to use Integer Math)
            uint16_t numLeds = _strip.numPixels();
            uint16_t cycle = numLeds * CONF_LED_SNAKE_SPEED;
            
            // Fixed-point position: 8 bits for integer part, 8 bits for fraction (x256)
            uint32_t headPosFixed = ((_animStep % cycle) * 256) / CONF_LED_SNAKE_SPEED;
            
            // Calculating global background color rotation
            uint16_t colorShift = 0;
            if (CONF_LED_COLOR_SPEED > 0) {
                colorShift = _animStep / CONF_LED_COLOR_SPEED;
            }

            // Calculate the single global color for this frame (Solid color for all LEDs)
            uint32_t globalColor = Wheel(colorShift & 255);
            uint8_t rIn = (uint8_t)(globalColor >> 16);
            uint8_t gIn = (uint8_t)(globalColor >> 8);
            uint8_t bIn = (uint8_t)globalColor;

            for(uint16_t i=0; i< numLeds; i++) {
                // Calculating distance from head (wrapping) in fixed point
                uint32_t iFixed = i * 256;
                int32_t distFixed = headPosFixed - iFixed;
                if (distFixed < 0) distFixed += (numLeds * 256);
                
                uint8_t brightness = 0;
                
                // Tail logic: Length 12.0
                if (distFixed < (12 * 256)) { 
                    if (distFixed < (8 * 256)) {
                        brightness = 255; // Solid body
                    } else {
                        // Linear fade out over 4 pixels
                        // brightness = 255 - (dist - 8.0) * (255 / 4.0)
                        brightness = 255 - ((distFixed - (8 * 256)) / 4);
                    }
                } else if (distFixed > ((numLeds - 1) * 256)) { 
                    // Fading in front (1 pixel ahead)
                    brightness = 255 - ((numLeds * 256) - distFixed);
                }

                if (brightness > 0) {
                    // Applying brightness to the global color
                    uint8_t r = ((uint16_t)rIn * brightness + 127) / 255;
                    uint8_t g = ((uint16_t)gIn * brightness + 127) / 255;
                    uint8_t b = ((uint16_t)bIn * brightness + 127) / 255;
                    
                    _strip.setPixelColor(i, r, g, b);
                } else {
                    _strip.setPixelColor(i, 0);
                }
            }
            _strip.show();
            _animStep++;
            break;
        }
        case LedState::PAUSED: {
            // Breathing Amber
            int val = (now / CONF_LED_BREATHE_SPEED) % 512; 
            if (val > 255) val = 511 - val;
            
            // Use Gamma table, skipping flat bottom
            uint8_t index = map(val, 0, 255, 50, 255);
            uint8_t gammaVal = pgm_read_byte(&gamma8[index]);
            int brightness = map(gammaVal, 0, 255, CONF_LED_BREATHE_MIN_BRIGHTNESS, 255);

            // Amber is roughly 255, 100, 0. Scaling by brightness.
            _strip.fill(_strip.Color(brightness, (brightness * 100) / 255, 0));
            _strip.show();
            break;
        }
        case LedState::SHUTDOWN:
        case LedState::OFF:
            _strip.clear();
            _strip.show();
            break;
        default: break;
    }
#endif
}

void LedManager::setState(LedState state) {
    if (_currentState == state) return;
    _currentState = state;
    _animStep = 0;
    _isVolumeOverlay = false; // Canceling volume overlay on state change
}

void LedManager::showVolume(uint8_t currentVol, uint8_t maxVol) {
#if CONF_ENABLE_LEDS
    _isVolumeOverlay = true;
    _volumeStart = millis();

    uint8_t ledsLit = map(currentVol, 0, maxVol, 0, _strip.numPixels());
    
    _strip.clear();
    for (uint8_t i = 0; i < ledsLit; i++) {
        // Gradient from Green (low) to Red (high)
        int val = i * 18; 
        if (val > 255) val = 255;
        _strip.setPixelColor(i, _strip.Color(val, 255 - val, 0));
    }
    _strip.show();
#endif
}

/**
 * @brief Generates a color from a 0-255 position input (Rainbow Wheel).
 * The colours are a transition r - g - b - back to r.
 * @param WheelPos Position on the color wheel (0-255).
 */
uint32_t LedManager::Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        return _strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if(WheelPos < 170) {
        WheelPos -= 85;
        return _strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return _strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}