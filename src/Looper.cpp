#include "Looper.h"

namespace util {

namespace details {

void looperLoop(QueueHandle_t queue) [[noreturn]] {
  loopFunction *current = nullptr;
  while (true) {
    while (!current) configASSERT(xQueueReceive(queue, &current, portMAX_DELAY));
    loopFunction *next = nullptr;
    for (uint32_t counter = 0;; counter++) {
      auto requestedWait = (*current)(counter);
      if (requestedWait == portMAX_DELAY || xQueueReceive(queue, &next, requestedWait)) break;
    }
    delete current;
    current = next;
  }
}

void looperTerminate(QueueHandle_t queue, TaskHandle_t looperTask) {
  /*
  Send a function that sends back a null in the queue and blocks indefinitely, so that
  the task that calls the destructor know when it is safe to progress in the destructor.
  */
  configASSERT(xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
  configASSERT(xTaskGetCurrentTaskHandle() != looperTask);
  loopFunction terminator = [queue](uint32_t &) [[noreturn]] {
    void *eof = nullptr;
    configASSERT(xQueueSend(queue, &eof, portMAX_DELAY));
    vTaskDelay(portMAX_DELAY);
    return 0;
  };
  auto terminatorPtr = &terminator;
  configASSERT(xQueueSend(queue, &terminatorPtr, portMAX_DELAY));
  void *eof;
  configASSERT(xQueueReceive(queue, &eof, portMAX_DELAY));
  configASSERT(eof == nullptr);
  vQueueDelete(queue);
}

} // namespace details

} // namespace util
