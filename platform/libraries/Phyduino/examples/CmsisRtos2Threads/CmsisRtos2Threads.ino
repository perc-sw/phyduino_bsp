// Runs threads with ARM's CMSIS-RTOS2 API, which works the same on any
// CMSIS-RTOS2 system (Keil RTX, Zephyr, other FreeRTOS boards).
// Needs Tools > Operating System > FreeRTOS (SMP) + CMSIS-RTOS2.
//
// The kernel is already running when setup() starts, so osKernelInitialize()
// and osKernelStart() are not needed. setup() starts two threads: one blinks
// the LED, the other reads A0 every 100 ms and puts each reading in a message
// queue. loop() is a thread too: it waits for readings and prints them.

#include <cmsis_os2.h>

struct Reading {
    uint32_t time_ms;
    uint32_t millivolts;
};

osMessageQueueId_t readings;

void blinkThread(void*) {
    for (;;) {
        digitalWrite(LED_BUILTIN, HIGH);
        osDelay(100);  // in ticks: 1 tick = 1 ms
        digitalWrite(LED_BUILTIN, LOW);
        osDelay(900);
    }
}

void sampleThread(void*) {
    uint32_t wake = osKernelGetTickCount();
    for (;;) {
        const Reading r = {osKernelGetTickCount(), analogReadMillivolts(A0)};
        if (osMessageQueuePut(readings, &r, 0, 0) != osOK) {
            Serial.println("Queue full: reading dropped");
        }
        wake += 100;
        osDelayUntil(wake);  // every 100 ms, however long the work took
    }
}

void setup() {
    Serial.begin(115200);
    while (!Serial) {}
    pinMode(LED_BUILTIN, OUTPUT);

    char kernel[32];
    osKernelGetInfo(nullptr, kernel, sizeof(kernel));
    Serial.print("Kernel: ");
    Serial.println(kernel);

    readings = osMessageQueueNew(20, sizeof(Reading), nullptr);

    osThreadAttr_t blink = {};  // unset fields: defaults (osPriorityNormal, like loop())
    blink.name = "blink";
    blink.stack_size = 1024;    // bytes
    osThreadNew(blinkThread, nullptr, &blink);

    osThreadAttr_t sample = {};
    sample.name = "sample";
    sample.stack_size = 2048;
    sample.priority = osPriorityAboveNormal;  // above loop(), so its timing holds
    osThreadNew(sampleThread, nullptr, &sample);
}

void loop() {
    Reading r;
    if (osMessageQueueGet(readings, &r, nullptr, osWaitForever) == osOK) {  // waits for a reading
        Serial.print(r.time_ms);
        Serial.print(" ms: A0 = ");
        Serial.print(r.millivolts);
        Serial.println(" mV");
    }
}
