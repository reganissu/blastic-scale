#include <Arduino.h>
#include <Arduino_FreeRTOS.h>

/*
  FreeRTOS guide says "here is no real way to recover from a stack overflow
  when it occurs", so just spam the serial with the error in an endless loop.
*/
void vApplicationStackOverflowHook(TaskHandle_t, char *pcTaskName){
  for(;;){
    Serial.print(F("! stack overflow by task "));
    Serial.println(reinterpret_cast<const char *>(pcTaskName));
    for(auto start_pause = millis(); millis() - start_pause < 1000;);
  }
}

/*
  The Arduino Framework makes use (in some questionable places) of malloc/free,
  new/delete. Unfortunately, the C library (and libfsp, which provides heap space
  in a .heap section) are precompiled and cannot be modified. Use the -Wl--wrapper
  linker trick to make these calls safe under FreeRTOS.
  This is a copy of FreeRTOS heap management example Heap_3.c .
*/
extern "C" void* __real_malloc(size_t s);
extern "C" void* __wrap_malloc(size_t s){
  vTaskSuspendAll();
  auto ptr = __real_malloc(s);
  traceMALLOC(ptr, s);
  xTaskResumeAll();
#if(configUSE_MALLOC_FAILED_HOOK == 1)
  if(!ptr) vApplicationMallocFailedHook();
#endif
  return ptr;
}

extern "C" void __real_free(void *ptr);
extern "C" void __wrap_free(void *ptr){
  if(!ptr) return;
  vTaskSuspendAll();
  __real_free(ptr);
  traceFREE(ptr, 0);
  xTaskResumeAll();
}
