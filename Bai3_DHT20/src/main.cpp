#include <Arduino.h>
#include "temp_humi_monitor.h"

void setup() 
{   // Initialize serial communication for debugging
    Serial.begin(115200);
    const unsigned long serialWaitStarted = millis();
    while (!Serial && millis() - serialWaitStarted < 5000) 
    {
        delay(10);
    }

    // Create a task for monitoring temperature and humidity
    const BaseType_t taskCreated = xTaskCreate(
        temp_humi_monitor,
        "DHT20 Monitor",
        6144,
        nullptr,
        2,
        nullptr
    );
    // Check if the task was created successfully
    if (taskCreated != pdPASS) 
    {
        Serial.println("Failed to create DHT20 monitoring task.");
    }
}

void loop() 
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
