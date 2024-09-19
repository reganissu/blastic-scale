#include "blastic.h"
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>

namespace blastic {

namespace ui {

void loop() {
  static ArduinoLEDMatrix leds;
  leds.begin();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(100));
    auto weight = blastic::scale::weight(config.scale);
    leds.beginDraw();
    leds.textFont(Font_4x6);
    leds.beginText(0, 1, 0xFFFFFF);
    if (isnan(weight)) {
      auto &calibration = config.scale.calibrations[uint8_t(config.scale.mode)];
      leds.print(calibration.weightRawRead - calibration.tareRawRead == 0 ? "cal" : "err");
    } else if (weight < 1) {
      leds.print('.');
      leds.print(int(round(weight * 100)));
    } else if (weight < 10) leds.print(weight, 3);
    else if (weight < 100) leds.print(int(round(weight)));
    else leds.print(":-O");
    leds.endText();
    leds.endDraw();
  }
}

} // namespace ui

} // namespace blastic
