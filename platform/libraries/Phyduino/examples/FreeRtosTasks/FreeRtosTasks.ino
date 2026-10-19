// Runs FreeRTOS tasks side by side. Needs Tools > Operating System > FreeRTOS (SMP).
//
// setup() starts two tasks: one blinks the LED, the other prints a count.
// loop() is a task too; every five seconds it prints the list of tasks, with
// the cores each one is allowed on (0x1 core 0, 0x2 core 1, 0xffffffff either). Each task waits with vTaskDelay() (delay()
// does the same with FreeRTOS), which leaves the cores free for the others.

#include <FreeRTOS.h>
#include <task.h>

void blinkTask(void*) {
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void countTask(void*) {
    unsigned long count = 0;
    for (;;) {
        Serial.print("countTask on core ");
        Serial.print(rp2040.cpuid());
        Serial.print(": ");
        Serial.println(count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    // Function, name, stack (in 4-byte words), parameter, priority, handle.
    xTaskCreate(blinkTask, "blink", 512, nullptr, 1, nullptr);
    xTaskCreate(countTask, "count", 1024, nullptr, 1, nullptr);
}

void loop() {
    static char list[640];
    vTaskListTasks(list, sizeof(list));
    Serial.println("Name           \tState\tPrio\tStack\tNum\tCores");
    Serial.print(list);  // State: X running, R ready, B blocked, S suspended; Stack: least free, in words
    Serial.println();
    delay(5000);
}
