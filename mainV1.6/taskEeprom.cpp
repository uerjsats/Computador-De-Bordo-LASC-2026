#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "telemetria.h"
#include "eepromI2C.h"
#include "taskTelemetria.h"
#include "DebugLog.h"


static unsigned long lastSave = 0;

extern QueueHandle_t filaTelemetria;

static const unsigned long saveInterval = 2000;

void taskEEPROM(void *pvParameters)
{
    sensorsData dados;

    while (true)
    {
        if (xQueueReceive(
                filaTelemetria,
                &dados,
                portMAX_DELAY) == pdPASS)
        {
            bool tempoOK =
                millis() - lastSave >= saveInterval;

            bool altitudeOK =
                dados.altitude >= 5000;

            bool memoriaOK =
                currentAddress + sizeof(sensorsData) < EEPROM_MAX_SIZE;

            bool radioOK =
                lora.isIdle();

            if (
                tempoOK &&
                altitudeOK &&
                memoriaOK &&
                radioOK
            )
            {
                eepromWriteBytes(
                    currentAddress,
                    (uint8_t*)&dados,
                    sizeof(sensorsData)
                );

                currentAddress += sizeof(sensorsData);

                lastSave = millis();

                DBG_EEPROM(DBG_INFO, "salvo no ender 0x%04X (%d bytes em uso)", currentAddress, currentAddress);
            }
            else if (!memoriaOK)
            {
                DBG_EEPROM(DBG_WARN, "memoria cheia — ender 0x%04X, max 0x%04X", currentAddress, EEPROM_MAX_SIZE);
            }
        }
    }
}