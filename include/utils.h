#pragma once

#include <cstring>

template <typename T, typename U, size_t alen> inline T *strcpy0(T (&dst)[alen], const U *src, size_t len = alen) {
  strncpy(dst, src, len);
  dst[len < alen ? len : alen - 1] = 0;
  return dst;
}
