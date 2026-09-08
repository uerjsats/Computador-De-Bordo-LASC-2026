#include <Arduino.h>
#include "NetworkManager.h"
#include "SerialBridge.h"
#include "DebugLog.h"

volatile bool imageReady = false;

void taskWiFi(void *pvParameters)
{
    setupNetwork();

    bool firstCycle = true;

    while (true)
    {
        if (firstCycle)
        {
            firstCycle = false;
            vTaskDelay(pdMS_TO_TICKS(7000));
        }

        //DBG_WIFI(DBG_INFO, "verificando recebimento de imagem...");

        bool received = false;

        if (WiFi.status() == WL_CONNECTED)
        {
            received = receiveImage();
        }

        if (received)
        {
            // DBG_WIFI(DBG_INFO, "imagem recebida");
            imageReady = true;
        }
        else
        {
            // DBG_WIFI(DBG_INFO, "nenhuma imagem recebida");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}