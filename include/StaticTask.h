#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

namespace util {

/*
  Utility class to allocate static buffers for FreeRTOS tasks.
*/

template <size_t StackSize = configMINIMAL_STACK_SIZE * sizeof(StackType_t)> class StaticTask {

public:
  static_assert(StackSize >= sizeof(StackType_t) * configMINIMAL_STACK_SIZE);

  StaticTask_t taskBuffer;
  StackType_t stack[StackSize / sizeof(StackType_t) + !!(StackSize % sizeof(StackType_t))];

  StaticTask() : handle(nullptr) {}

  template <typename ArgType>
  StaticTask(TaskFunction_t taskFunction, ArgType arg, const char *name, UBaseType_t priority = tskIDLE_PRIORITY + 1)
      : handle(xTaskCreateStatic(taskFunction, name, sizeof(stack) / sizeof(stack[0]), static_cast<void *>(arg),
                                 priority, stack, &taskBuffer)) {
    static_assert(sizeof(ArgType) == sizeof(void *));
    configASSERT(handle);
  }

  StaticTask(voidFuncPtr taskFunction, const char *name, UBaseType_t priority = tskIDLE_PRIORITY + 1)
      : StaticTask(static_cast<TaskFunction_t>(taskFunction), nullptr, name, priority) {}

  StaticTask(const StaticTask &) = delete;
  StaticTask &operator=(const StaticTask &) = delete;

  operator TaskHandle_t() const { return handle; }

  ~StaticTask() {
    if (handle) vTaskDelete(handle);
  }

private:
  const TaskHandle_t handle;
};

} // namespace util
