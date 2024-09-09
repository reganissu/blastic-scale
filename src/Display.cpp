#include <cstdio>
#include "Display.h"
#include "blastic.h"

namespace ui {
void loop() {
  static ArduinoLEDMatrix leds;
  leds.begin();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
    auto weight = blastic::scale.read();
    leds.beginDraw();
    leds.textFont(Font_4x6);
    leds.beginText(0, 0, 0xFFFFFF);
    leds.println(weight / 1000, 4);
    leds.endText();
    leds.endDraw();
  }
}
} // namespace ui
