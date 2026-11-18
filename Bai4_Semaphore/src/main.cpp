#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace
{

struct CounterTaskConfiguration
{
    const char *taskName;
    TickType_t delayBetweenIncrements;
};

SemaphoreHandle_t sharedCounterMutex = nullptr;
uint32_t sharedCounter = 0;

CounterTaskConfiguration firstTaskConfiguration =
{
    "Counter Task One",
    pdMS_TO_TICKS(1000)
};

CounterTaskConfiguration secondTaskConfiguration =
{
    "Counter Task Two",
    pdMS_TO_TICKS(1500)
};

void incrementSharedCounterTask(void *taskConfigurationParameter)
{
    const CounterTaskConfiguration *taskConfiguration =
        static_cast<CounterTaskConfiguration *>(taskConfigurationParameter);
    const char *taskName = taskConfiguration->taskName;

    for (;;)
    {
        Serial.printf("[%s] Waiting to acquire the mutex.\n", taskName);

        if (xSemaphoreTake(sharedCounterMutex, portMAX_DELAY) == pdTRUE)
        {
            Serial.printf("[%s] Acquired the mutex.\n", taskName);

            ++sharedCounter;
            Serial.printf("[%s] Shared counter value: %lu\n",
                          taskName,
                          static_cast<unsigned long>(sharedCounter));

            Serial.printf("[%s] Releasing the mutex.\n", taskName);

            if (xSemaphoreGive(sharedCounterMutex) != pdTRUE)
            {
                Serial.printf("[%s] Error: failed to release the mutex.\n",
                              taskName);
            }
        }
        else
        {
            Serial.printf("[%s] Error: failed to acquire the mutex.\n",
                          taskName);
        }

        vTaskDelay(taskConfiguration->delayBetweenIncrements);
    }
}

}

void setup()
{
    Serial.begin(115200);

    const unsigned long serialWaitStarted = millis();

    while (!Serial && millis() - serialWaitStarted < 5000)
    {
        delay(10);
    }

    sharedCounterMutex = xSemaphoreCreateMutex();

    if (sharedCounterMutex == nullptr)
    {
        Serial.println("Error: failed to create the shared counter mutex.");
        return;
    }

    Serial.println(
        "The mutex protects the shared counter and keeps each task's log sequence together.");

    if (xSemaphoreTake(sharedCounterMutex, portMAX_DELAY) != pdTRUE)
    {
        Serial.println("Error: failed to acquire the mutex during setup.");
        vSemaphoreDelete(sharedCounterMutex);
        sharedCounterMutex = nullptr;
        return;
    }

    TaskHandle_t firstTaskHandle = nullptr;
    TaskHandle_t secondTaskHandle = nullptr;

    const BaseType_t firstTaskCreationResult = xTaskCreate(
        incrementSharedCounterTask,
        "Counter Task One",
        4096,
        &firstTaskConfiguration,
        1,
        &firstTaskHandle);

    const BaseType_t secondTaskCreationResult = xTaskCreate(
        incrementSharedCounterTask,
        "Counter Task Two",
        4096,
        &secondTaskConfiguration,
        1,
        &secondTaskHandle);

    if (firstTaskCreationResult != pdPASS ||
        secondTaskCreationResult != pdPASS)
    {
        Serial.println("Error: failed to create both counter tasks.");

        if (firstTaskHandle != nullptr)
        {
            vTaskDelete(firstTaskHandle);
        }

        if (secondTaskHandle != nullptr)
        {
            vTaskDelete(secondTaskHandle);
        }

        xSemaphoreGive(sharedCounterMutex);
        vSemaphoreDelete(sharedCounterMutex);
        sharedCounterMutex = nullptr;
        return;
    }

    if (xSemaphoreGive(sharedCounterMutex) != pdTRUE)
    {
        Serial.println("Error: failed to release the mutex during setup.");
    }
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(1000));
}
