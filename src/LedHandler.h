#include <FastLED.h>

#define DATA_PIN 6
#define NUM_LEDS 1

#define MAX_BRIGHTNESS 16

class LedHandler {

 private:
  CRGB leds[NUM_LEDS];

 public:
  /************************************************************************************************************************************************************
   * 
   */
  void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(MAX_BRIGHTNESS);
  }

  /************************************************************************************************************************************************************
   * 
   */
  void setStatusLedColor(CRGB color) {
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = color;
    }
    FastLED.show();
  }

  /************************************************************************************************************************************************************
   * 
   */
  void indicateErrorState() {
    for (int i = 0; i < 3; i++) {
      setStatusLedColor(CRGB::Red);
      delay(500);
      setStatusLedColor(CRGB::Black);
      delay(500);
    }
  }

  /************************************************************************************************************************************************************
   * 
   */
  static void setBrightness(uint8_t brightness) {
    FastLED.setBrightness(brightness);
    FastLED.show();
  }

  /************************************************************************************************************************************************************
   * 
   */
  static void resetBrightness() {
    FastLED.setBrightness(MAX_BRIGHTNESS);
    FastLED.show();
  }
};