#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include "StaticTask.h"

#if configSUPPORT_STATIC_ALLOCATION == 1

/*
  Static allocation functions, as suggested in the FreeRTOS guide.
*/

#if configUSE_TIMERS == 1

void vApplicationGetTimerTaskMemory(StaticTask_t **taskBuffer, StackType_t **stackBuffer, uint32_t *stackSize) {
  static util::StaticTask timerTaskBuffers;
  *taskBuffer = &timerTaskBuffers.taskBuffer;
  *stackBuffer = timerTaskBuffers.stack;
  *stackSize = sizeof(timerTaskBuffers.stack) / sizeof(timerTaskBuffers.stack[0]);
}

#endif

void vApplicationGetIdleTaskMemory(StaticTask_t **taskBuffer, StackType_t **stackBuffer, uint32_t *stackSize) {
  static util::StaticTask idleTaskBuffers;
  *taskBuffer = &idleTaskBuffers.taskBuffer;
  *stackBuffer = idleTaskBuffers.stack;
  *stackSize = sizeof(idleTaskBuffers.stack) / sizeof(idleTaskBuffers.stack[0]);
}

#endif

#if configCHECK_FOR_STACK_OVERFLOW > 0

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

#endif

/*
  The Arduino Framework makes use (in some questionable places) of malloc/free,
  new/delete. Unfortunately, the C library (and libfsp, which provides heap
  space in a .heap section) are precompiled and cannot be modified. Use the
  -Wl--wrapper linker trick to make these calls safe under FreeRTOS.

  Arduino Framework uses picolib, which makes malloc and friends safe by using
  a lock (compiled to a no-op, since Arduino assumes it is single-thread). Here
  hook _malloc_lock/__malloc_unlock to vTaskSuspendAll/xTaskResumeAll. This
  ensures that all access to the heap (malloc/realloc/calloc...) is safe.
*/
extern "C" void __real___malloc_lock(_reent *);
extern "C" void __wrap___malloc_lock(_reent *) { vTaskSuspendAll(); }

extern "C" void __real___malloc_unlock(_reent *);
extern "C" void __wrap___malloc_unlock(_reent *) { xTaskResumeAll(); }

#if configUSE_MALLOC_FAILED_HOOK

/*
  Trace all failed allocations.
*/

void vApplicationMallocFailedHook() { configASSERT(false && "pvPortMalloc() failed"); }

extern "C" void *__real__malloc_r(struct _reent *, size_t);
extern "C" void *__wrap__malloc_r(struct _reent *r, size_t s) {
  auto ptr = __real__malloc_r(r, s);
  configASSERT(ptr && "_malloc_r() failed");
  return ptr;
}

#endif

void loop() [[noreturn]] {
  Serial.print("setup: starting FreeRTOS scheduler\n");
  vTaskStartScheduler();
  configASSERT(false && "vTaskStartScheduler() should never return");
}
