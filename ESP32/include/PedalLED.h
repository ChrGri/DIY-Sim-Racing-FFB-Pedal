#pragma once
#include <stdint.h>
#include "Main.h" 

#ifdef USING_LED
#include <Adafruit_NeoPixel.h>
#endif

typedef uint32_t crgb_t;

class PedalLED {
public:
    static const crgb_t L_RED = 0xff0000;
    static const crgb_t L_GREEN = 0x00ff00;
    static const crgb_t L_BLUE = 0x0000ff;
    static const crgb_t L_WHITE = 0xe0e0e0;
    static const crgb_t L_YELLOW = 0xffde21;
    static const crgb_t L_ORANGE = 0xffa500;
    static const crgb_t L_CYAN = 0x00ffff;
    static const crgb_t L_PURPLE = 0x800080;

private:
#ifdef USING_LED
    Adafruit_NeoPixel pixels;
#endif

public:
#ifdef USING_LED
    #ifndef LEDS_COUNT
    #define LEDS_COUNT 1
    #endif

    #ifdef LED_ENABLE_RGB
    PedalLED() : pixels(LEDS_COUNT, LED_GPIO_U8, NEO_RGB + NEO_KHZ800) {}
    #else
    PedalLED() : pixels(LEDS_COUNT, LED_GPIO_U8, NEO_GRB + NEO_KHZ800) {}
    #endif
#else
    PedalLED() {}
#endif

    void begin() {
#ifdef USING_LED
        pixels.begin();
#endif
    }

    void setBrightness(uint8_t b) {
#ifdef USING_LED
        pixels.setBrightness(b);
#endif
    }

    void setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
#ifdef USING_LED
        pixels.setPixelColor(n, r, g, b);
#endif
    }

    void setPixelColor(uint16_t n, uint32_t c) {
#ifdef USING_LED
        pixels.setPixelColor(n, c);
#endif
    }

    void show() {
#ifdef USING_LED
        pixels.show();
#endif
    }
};
