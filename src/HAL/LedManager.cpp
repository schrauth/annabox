#include "LedManager.h"

LedManager::LedManager() 
    : _currentState(LedState::BOOT), 
      _strip(0, 0, NEO_GRB + NEO_KHZ800), // Initializing with dummy values; I'll update these in begin()
      _lastUpdate(0), _animStep(0),
      _isVolumeOverlay(false), _volumeStart(0) {}

void LedManager::begin(uint8_t pin, uint8_t numLeds) {
    _strip.setPin(pin);
    _strip.updateLength(numLeds);
    _strip.updateType(NEO_GRB + NEO_KHZ800);
    _strip.begin();
    _strip.setBrightness(30); // Using faint brightness for testing purposes
    _strip.show();
}

void LedManager::update() {
    uint32_t now = millis();

    // 1. Handling Volume Overlay (Temporary Override)
    if (_isVolumeOverlay) {
        if (now - _volumeStart > 1500) {
            _isVolumeOverlay = false; // Timeout expired, reverting to state animation
        } else {
            return; // I won't run background animations while showing volume
        }
    }

    // 2. Throttling Animation Framerate (~30 FPS)
    if (now - _lastUpdate < 33) return;
    _lastUpdate = now;

    // 3. Processing the LED State Machine
    switch (_currentState) {
        case LedState::IDLE_BREATHE: {
            // Simple Triangle Wave for Breathing (Blue)
            int val = (now / BREATHE_SPEED_FACTOR) % 512; 
            if (val > 255) val = 511 - val;
            
            // Mapping 0-255 to 30-255 to prevent fully off
            int brightness = map(val, 0, 255, 30, 255);

            _strip.fill(_strip.Color(0, 0, brightness / 2)); // Blue, max brightness ~128
            _strip.show();
            break;
        }
        case LedState::PLAYING: {
            // Rainbow Snake with sub-pixel rendering
            // Calculating head position as a float to allow smooth movement between LEDs
            uint16_t numLeds = _strip.numPixels();
            uint16_t cycle = numLeds * SNAKE_SPEED_FACTOR;
            float headPos = (float)(_animStep % cycle) / (float)SNAKE_SPEED_FACTOR;
            
            // Calculating global background color rotation
            uint16_t colorShift = 0;
            if (COLOR_SPEED_FACTOR > 0) {
                colorShift = _animStep / COLOR_SPEED_FACTOR;
            }

            for(uint16_t i=0; i< numLeds; i++) {
                // Calculating distance from head (wrapping)
                float dist = headPos - i;
                if (dist < 0) dist += numLeds;
                
                float fade = 0.0f;
                if (dist < 12.0f) { // Tail length (1/2 ring)
                    if (dist < 8.0f) {
                        fade = 1.0f; // Solid body
                    } else {
                        fade = 1.0f - ((dist - 8.0f) / 4.0f); // Quick fade out
                    }
                } else if (dist > numLeds - 1.0f) { // Fading in front (1 pixel ahead)
                    fade = 1.0f - (numLeds - dist);
                }

                if (fade > 0.0f) {
                    uint8_t brightness = (uint8_t)(fade * 255.0f);
                    
                    // Color: Pixel Position + Global Rotation (No propagation)
                    uint32_t color = Wheel(((i * 256 / numLeds) + colorShift) & 255);
                    
                    // Applying brightness to color with rounding to preserve low-light colors
                    uint8_t rIn = (uint8_t)(color >> 16);
                    uint8_t gIn = (uint8_t)(color >> 8);
                    uint8_t bIn = (uint8_t)color;

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
            int val = (now / BREATHE_SPEED_FACTOR) % 512; 
            if (val > 255) val = 511 - val;
            
            // Mapping 0-255 to 30-255 to prevent fully off
            int brightness = map(val, 0, 255, 30, 255);

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
}

void LedManager::setState(LedState state) {
    if (_currentState == state) return;
    _currentState = state;
    _animStep = 0;
    _isVolumeOverlay = false; // Canceling volume overlay on state change
}

void LedManager::showVolume(uint8_t currentVol, uint8_t maxVol) {
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