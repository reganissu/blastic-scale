#ifndef _HEADER_MutexedPrint
#define _HEADER_MutexedPrint

#include <type_traits>
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace cli {

template <auto &obj> class MutexedPrint {
  static_assert(std::is_base_of_v<arduino::Print, std::remove_reference_t<decltype(obj)>>);
  inline static StaticSemaphore_t buffer;
  inline static SemaphoreHandle_t mutex = xSemaphoreCreateRecursiveMutexStatic(&buffer);

public:
  MutexedPrint() { configASSERT(xSemaphoreTakeRecursive(MutexedPrint::mutex, portMAX_DELAY)); }
  MutexedPrint(const MutexedPrint &) = delete;
  MutexedPrint &operator=(const MutexedPrint &) = delete;
  ~MutexedPrint() { configASSERT(xSemaphoreGiveRecursive(MutexedPrint::mutex)); }

  template <typename... Args> size_t write(Args &&...args) { return obj.write(std::forward<Args>(args)...); }
  template <typename... Args> size_t print(Args &&...args) { return obj.print(std::forward<Args>(args)...); }
  template <typename... Args> size_t println(Args &&...args) { return obj.println(std::forward<Args>(args)...); }
};

} // namespace cli

#endif
