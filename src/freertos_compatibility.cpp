#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "StaticTask.h"

#if configSUPPORT_STATIC_ALLOCATION == 1

/*
  Static allocation functions, as suggested in the FreeRTOS guide.
*/

void vApplicationGetTimerTaskMemory(StaticTask_t **taskBuffer, StackType_t **stackBuffer, uint32_t *stackSize) {
  static util::StaticTask timerTaskBuffers;
  *taskBuffer = &timerTaskBuffers.taskBuffer;
  *stackBuffer = timerTaskBuffers.stack;
  *stackSize = sizeof(timerTaskBuffers.stack) / sizeof(timerTaskBuffers.stack[0]);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **taskBuffer, StackType_t **stackBuffer, uint32_t *stackSize) {
  static util::StaticTask idleTaskBuffers;
  *taskBuffer = &idleTaskBuffers.taskBuffer;
  *stackBuffer = idleTaskBuffers.stack;
  *stackSize = sizeof(idleTaskBuffers.stack) / sizeof(idleTaskBuffers.stack[0]);
}

#endif

/*
  FreeRTOS guide says "here is no real way to recover from a stack overflow
  when it occurs". Here we trigger an hardware fault (call to invalid address)
  in order to trigger a crash dump on Serial. The fault log is provided by
  https://github.com/armink/CmBacktrace , which eventually prints an addr2line
  command to show the stack trace.

  The function is naked to avoid messing with the stack in an already broken
  situation.
*/
[[gnu::naked]] void vApplicationStackOverflowHook(TaskHandle_t, char *) { reinterpret_cast<voidFuncPtr>(0)(); }

/*
  The Arduino Framework makes use (in some questionable places) of malloc/free,
  new/delete. Unfortunately, the C library (and libfsp, which provides heap
  space in a .heap section) are precompiled and cannot be modified. Use the
  -Wl--wrapper linker trick to make these calls safe under FreeRTOS. This is a
  copy of FreeRTOS heap management example Heap_3.c, implementing serialized access
  to malloc() and free().
*/
extern "C" void *__real_malloc(size_t s);
extern "C" void *__wrap_malloc(size_t s) {
  vTaskSuspendAll();
  auto ptr = __real_malloc(s);
  traceMALLOC(ptr, s);
  xTaskResumeAll();
#if (configUSE_MALLOC_FAILED_HOOK == 1)
  if (!ptr) vApplicationMallocFailedHook();
#endif
  return ptr;
}

extern "C" void __real_free(void *ptr);
extern "C" void __wrap_free(void *ptr) {
  if (!ptr) return;
  vTaskSuspendAll();
  __real_free(ptr);
  traceFREE(ptr, 0);
  xTaskResumeAll();
}

// empty loop, necessary for linking purposes but never run
void loop() {}
