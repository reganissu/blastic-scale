#include <string>
#include <array>
#include "blastic.h"
#include <ArduinoGraphics.h>
#include <Arduino_LED_Matrix.h>

namespace blastic {

static ArduinoLEDMatrix matrix;
static const auto &font = Font_4x6;
static constexpr const int matrixWidth = 12, matrixHeight = 8, fullCharsOnMatrix = matrixWidth / 4;

static util::loopFunction clear() {
  return +[](uint32_t &) {
    matrix.clear();
    memset(framebuffer, 0, sizeof(framebuffer));
    return portMAX_DELAY;
  };
}

static util::loopFunction scroll(std::string &&str, unsigned int initialDelay = 1000, unsigned int scrollDelay = 100,
                                 unsigned int blinkPeriods = 0) {
  if (!str.size()) return clear();
  int textWidht = str.size() * font.width;
  scrollDelay = textWidht > matrixWidth ? scrollDelay : 0;
  return [=, str = std::move(str), blinkCounter = 0](uint32_t &counter) mutable {
    int shiftX = -int(counter), wrapShiftX = shiftX + textWidht + matrixWidth / 2;
    matrix.clear();
    if (blinkPeriods && (blinkCounter++ / blinkPeriods) & 1) {
      memset(framebuffer, 0, sizeof(framebuffer));
      goto end;
    }
    if (shiftX + textWidht > 0) {
      matrix.beginText(shiftX, 0);
      matrix.print(str.c_str());
      matrix.endText();
    }
    if (scrollDelay) {
      if (wrapShiftX < matrixWidth) {
        matrix.beginText(wrapShiftX, 0);
        matrix.print(str.c_str());
        matrix.endText();
      }
    }
  end:
    if (!scrollDelay) return portMAX_DELAY;
    if (!counter) return pdMS_TO_TICKS(initialDelay);
    // if we just drew the wrapShiftX string as if it was the shiftX string with shiftX = 0, wrap the counter
    if (!wrapShiftX) counter = 0;
    return pdMS_TO_TICKS(scrollDelay);
  };
}

static util::loopFunction show(float v) {
  if (isnan(v)) {
    std::string str("nan:   ");
    util::AnnotatedFloat(v).getAnnotation(str.data() + 4);
    return scroll(std::move(str));
  }
  if (isinf(v)) return scroll(v > 0 ? "+inf" : "-inf");
  float av = abs(v);
  constexpr const float flushThreshold = 0.000001;
  if (av < flushThreshold) return scroll("0");

  constexpr const auto digits = fullCharsOnMatrix < 7 ? fullCharsOnMatrix : 7;
  RingBufferN<digits> integerReverseStr;
  int order = -1;
  float iav, fav = modf(av, &iav);
  // get integer part digits in reverse order from units
  for (; iav >= 1; iav /= 10, order++) {
    if (integerReverseStr.isFull()) integerReverseStr.read_char();
    integerReverseStr.store_char('0' + remainder(iav, 10));
  }
  std::array<char, digits + 1> str;
  str[digits] = '\0';
  auto strStart = str.data(), fractionalPtr = strStart + integerReverseStr.available();
  if (integerReverseStr.available()) {
    // weight has an integer part, flush it to the string
    for (auto integerPtr = fractionalPtr - 1; integerPtr >= strStart; integerPtr--)
      *integerPtr = integerReverseStr.read_char();
    fav *= 10;
  } else
    while ((fav *= 10) < 1) order--; // there is no integer part, find the first non-zero digit
  // loop over the fractional part (fav) digits
  for (auto digitsEnd = strStart + digits; fractionalPtr < digitsEnd; fractionalPtr++, fav = modf(fav, &iav) * 10)
    *fractionalPtr = '0' + uint8_t(fav);
  // value positive: value aligned to top, dots below value
  // value negative: value aligned to bottom, dots above value
  // with the 4x6 font numbers are actually 3x5, so align dots at Y offset 6
  auto textYOffset = v >= 0 ? 0 : matrixHeight - font.height,
       dotsYOffset = v >= 0 ? textYOffset + font.height : textYOffset - 2;

  return [=](uint32_t &) {
    matrix.clear();
    matrix.beginText(0, textYOffset);
    matrix.print(str.data());
    matrix.endText();
    matrix.beginDraw();
    // floating decimal dot
    matrix.set((order + 1) * font.width, dotsYOffset, 1, 1, 1);
    // powers of 1/10, dots on the left
    for (int i = 0; i < max(-order, 0); i++) matrix.set(i, dotsYOffset, 1, 1, 1);
    // powers of 10, dots on the right
    for (int i = 0; i < max(order + 1 - fullCharsOnMatrix, 0); i++)
      matrix.set(matrixWidth - 1 - i, dotsYOffset, 1, 1, 1);
    matrix.endDraw();
    return portMAX_DELAY;
  };
}

void Submitter::gotInput() { lastInteractionMillis = millis(); }

Submitter::Action Submitter::idling() {
  painter = clear();
  constexpr const auto idleWeightInterval = 2000;
  while (true) {
    uint32_t cmd;
    float weight;
    if (xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(idleWeightInterval))) return toAction(cmd);
    weight = scale::weight(config.scale, 1, pdMS_TO_TICKS(1000));
    if (abs(weight) >= config.submit.threshold) {
      gotInput();
      return Action::NONE;
    }
  }
}

constexpr const auto idleTimeout = 60000;

HasTimedOut<Submitter::Action> Submitter::preview() {
  auto prevWeight = util::AnnotatedFloat("n/a");
  for (; millis() - lastInteractionMillis < idleTimeout;) {
    uint32_t cmd;
    if (xTaskNotifyWait(0, -1, &cmd, 0)) return toAction(cmd);
    auto weight = scale::weight(config.scale, 1, pdMS_TO_TICKS(1000));
    if (abs(weight) < config.submit.threshold) weight.f = 0;
    else gotInput();
    if (weight == prevWeight) continue;
    prevWeight = weight;
    if (weight == scale::weightCal) painter = scroll("uncalibrated");
    else if (weight == scale::weightErr) painter = scroll("sensor error");
    else if (weight == 0) painter = scroll("0");
    else painter = show(weight);
  }
  return {};
}

HasTimedOut<plastic> Submitter::plasticSelection() {
  painter = scroll("type");
  uint32_t cmd = 0;
  xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(2000));
  int i = 0;
  while (true) {
    painter = scroll(plasticName(plastics[i]));
    if (!xTaskNotifyWait(-1, -1, &cmd, pdMS_TO_TICKS(idleTimeout))) return {};
    gotInput();
    switch (toAction(cmd)) {
    case Action::PREVIOUS: i = (i + std::size(plastics) - 1) % std::size(plastics); continue;
    case Action::NEXT: i = (i + 1) % std::size(plastics); continue;
    case Action::OK: return plastics[i];
    case Action::BACK: return {};
    }
  }
}

void Submitter::loop() [[noreturn]] {
  matrix.begin();
  matrix.background(0);
  matrix.stroke(0xFFFFFF);
  matrix.textFont(font);
  matrix.beginText(0, 0, 0xFFFFFF);
  MSerial()->print("submitter: started lcd\n");
  gotInput();

  uint32_t cmd;
  while (true) {
    if (debug) MSerial()->print("submitter: preview\n");
    auto action = preview();
    if (action.timedOut) {
      if (debug) MSerial()->print("submitter: idling\n");
      action = idling();
    }
    gotInput();
    if (action != Action::OK) continue;
    if (debug) MSerial()->print("submitter: start submission\n");
    painter = scroll("...");
    auto weight = scale::weight(config.scale, 10);
    if (!(weight >= config.submit.threshold)) {
      if (weight < config.submit.threshold) painter = scroll("<= 0");
      else painter = scroll("bad value");
      xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(5000));
      continue;
    }
    for (int i = 0; i < 5; i++) {
      painter = show(weight);
      if (xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(200))) break;
      painter = clear();
      if (xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(200))) break;
    }
    auto plastic = plasticSelection();
    if (plastic.timedOut) continue;
    painter = scroll(plasticName(plastic), 200, 100, 2);
    xTaskNotifyWait(0, -1, &cmd, pdMS_TO_TICKS(10000));
    // TODO submit
  }
}

Submitter::Submitter(const char *name, UBaseType_t priority)
    : painter("Painter"), task(Submitter::loop, this, name, priority) {}

void Submitter::action(Action action) { xTaskNotify(task, uint8_t(action), eSetValueWithOverwrite); }

} // namespace blastic
