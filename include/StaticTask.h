#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

template <size_t StackSize = configMINIMAL_STACK_SIZE * sizeof(StackType_t)> class StaticTask {

public:
  static_assert(StackSize >= sizeof(StackType_t) * configMINIMAL_STACK_SIZE);

  template <typename ArgType>
  TaskHandle_t create(TaskFunction_t taskFunction, ArgType arg, const char *name,
                      UBaseType_t priority = tskIDLE_PRIORITY + 1) {
    static_assert(sizeof(ArgType) == sizeof(void *));
    if (!handle)
      handle = xTaskCreateStatic(taskFunction, name, sizeof(stack) / sizeof(stack[0]), static_cast<void *>(arg),
                                 priority, stack, &taskBuffer);
    return handle;
  }

  TaskHandle_t create(voidFuncPtr taskFunction, const char *name, UBaseType_t priority = tskIDLE_PRIORITY + 1) {
    return create(static_cast<TaskFunction_t>(taskFunction), static_cast<void *>(nullptr), name, priority);
  }

  operator TaskHandle_t() const { return handle; }

private:
  TaskHandle_t handle = nullptr;
  StaticTask_t taskBuffer;
  StackType_t stack[StackSize / sizeof(StackType_t) + !!(StackSize % sizeof(StackType_t))];

#if configSUPPORT_STATIC_ALLOCATION == 1
  friend void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
                                             StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize);
  friend void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                                            uint32_t *pulIdleTaskStackSize);
#endif
};
