#pragma once

#include <cstdint>
#include <cmath>

namespace util {

union AnnotatedFloat {
  float f;
  struct {
    uint32_t annotation : 22, isnan : 1, exponent : 8, sign : 1;
  };

  AnnotatedFloat() {}
  explicit constexpr AnnotatedFloat(float f) : f(f) {}

  explicit constexpr AnnotatedFloat(const char (&msg)[4]) : f(NAN) {
    annotation = 0;
    for (int i = 0; i < 3 && msg[i]; i++) annotation |= msg[i] << (i * 8);
  }

  void getAnnotation(char *msg) const {
    for (uint32_t annotation = std::isnan(f) ? this->annotation : 0, i = 0; i < 3; i++, annotation >>= 8)
      msg[i] = annotation;
    msg[3] = 0;
  }

  AnnotatedFloat &setAnnotation(const char (&msg)[4]) {
    if (!std::isnan(f)) return *this;
    annotation = 0;
    for (uint32_t i = 0, j = 0; i < 3; i++, msg[j] ? j++ : 0) annotation |= msg[j] << (i * 8);
    return *this;
  }

  bool operator==(const AnnotatedFloat &o) const {
    if (std::isnan(f) && std::isnan(o.f)) return annotation == o.annotation;
    return f == o.f;
  }
  bool operator!=(const AnnotatedFloat &o) const { return !(*this == o); }
  operator float() const { return f; }
  operator float &() { return f; }
};

static_assert(sizeof(AnnotatedFloat) == sizeof(float));

} // namespace util
