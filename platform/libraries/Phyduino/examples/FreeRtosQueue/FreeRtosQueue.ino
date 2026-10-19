// Passes measurements from one FreeRTOS task to another through a queue.
// Needs Tools > Operating System > FreeRTOS (SMP).
//
// A sampling task reads A0 exactly every 100 ms and puts each reading on a
// queue; loop() waits for readings and prints them. The sampling task has the
// higher priority, so its timing holds even while loop() is busy printing.

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

struct Reading {
    unsigned long time_ms;
    uint32_t millivolts;
};

QueueHandle_t readings;

void sampleTask(void*) {
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        const Reading r = {millis(), analogReadMillivolts(A0)};
        if (xQueueSend(readings, &r, 0) != pdPASS) {
            Serial.println("Queue full: reading dropped");
        }
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(100));  // every 100 ms, however long the work took
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    readings = xQueueCreate(20, sizeof(Reading));
    xTaskCreate(sampleTask, "sample", 1024, nullptr, 2, nullptr);
}

void loop() {
    Reading r;
    if (xQueueReceive(readings, &r, portMAX_DELAY) == pdPASS) {  // waits until a reading arrives
        Serial.print(r.time_ms);
        Serial.print(" ms: A0 = ");
        Serial.print(r.millivolts);
        Serial.println(" mV");
    }
}
