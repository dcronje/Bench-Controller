#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "FreeRTOS.h"
#include "task.h"

#include "wifi.h"
#include "settings.h"
#include "control.h"
#include "sensors.h"
#include "display.h"
#include "interaction.h"
#include "isr-handlers.h"
#include "extractor.h"
#include "lights.h"

#define WATCHDOG_TIMEOUT_MS 5000 // Watchdog timeout in milliseconds

extern "C"
{
    void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
    {
        printf("Stack overflow in task: %s\n", pcTaskName);
        while (1)
            ; // Hang here for debugging.
    }

    void vApplicationMallocFailedHook(void)
    {
        printf("Malloc failed!\n");
        while (1)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    void Default_Handler(void)
    {
        uint32_t irq_num;
        asm volatile("mrs %0, ipsr" : "=r"(irq_num));
        printf("Unhandled IRQ: %ld\n", irq_num & 0x1FF); // Extract IRQ number
        while (1)
            ;
    }
}

void watchdogKickTask(void *params)
{
    while (1)
    {
        // Feed the watchdog to reset its timer
        watchdog_update();

        // Delay for less than the watchdog timeout to ensure it's regularly reset
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_TIMEOUT_MS / 2));
    }
}

int main()
{
    stdio_init_all();
    gpio_set_irq_callback(&sharedISR);
    printf("Starting FreeRTOS with Watchdog...\n");

    // Initialize the watchdog with a timeout, this will reset the system if not regularly kicked

    initDisplay();
    initSettings();
    // initControl();
    initWifi();
    initSensors();
    initInteraction();
    initExtractor();
    initLights();

    // requestSettingsReset();
    watchdog_enable(WATCHDOG_TIMEOUT_MS, 1);

    xTaskCreate(displayTask, "DisplayTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(wifiTask, "WiFiTask", 4096, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(settingsTask, "SettingsTask", 256, NULL, tskIDLE_PRIORITY + 1, NULL);
    // xTaskCreate(controlTask, "ControlTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(lightSensorTask, "LightSensorsTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(tempSensorTask, "TempSensorsTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(extractorTask, "ExtractorTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(lightsTask, "LightsTask", 256, NULL, tskIDLE_PRIORITY + 2, NULL);

    xTaskCreate(interactionTask, "InteractionTask", 256, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(watchdogKickTask, "WatchdogTask", 256, NULL, tskIDLE_PRIORITY + 3, NULL);

    vTaskStartScheduler();

    // If the scheduler returns, this should never happen.
    printf("Ending main, which should not happen.\n");
    return 0;
}
