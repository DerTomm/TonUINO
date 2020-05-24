#include <FastLED.h>

#define DATA_PIN 6
#define NUM_LEDS 1

class LedHandler {

 private:
  CRGB leds[NUM_LEDS];

 public:
  /************************************************************************************************************************************************************
   * 
   */
  void setup() {
    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(16);
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
};