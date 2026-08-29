#include <Arduino.h>
#include <cstddef>   // offsetof
#include <cstring>   // memset / memcpy
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "taskTelemetria.h"
#include "telemetria.h"
#include "NetworkManager.h"
#include "slaves.h"
#include "slaveParse.h"
#include "DebugLog.h"

extern QueueHandle_t filaTelemetria;
extern QueueHandle_t filaCompleta;

// Definida em taskWifi.cpp
extern volatile bool imageReady;

// TODO: ajustar esses dois nomes para os que realmente existem em
// NetworkManager.h (ponteiro pro buffer da imagem recebida e seu tamanho).
// Exemplo do que provavelmente precisa existir lá:
//   uint8_t*  getImageBuffer();
//   uint32_t  getImageSize();
extern uint8_t* getImageBuffer();
extern uint32_t getImageSize();

LoraTelemetria lora(ADDR_OBC, ADDR_GROUND);

// Uplink: pacote vindo da estacao de solo
void onReceive(uint8_t* data, uint16_t size)
{
    if (!validateHeader(data, size)) {
        DBG_LORA(DBG_WARN, "pacote com cabecalho invalido (%u bytes)", size);
        return;
    }

    uint8_t tipo = packetType(data);
    uint8_t src  = packetSrc(data);
    uint8_t dst  = packetDst(data);

    if (dst != ADDR_OBC || src != ADDR_GROUND) {
        DBG_LORA(DBG_WARN, "pacote ignorado: src=0x%02X dst=0x%02X", src, dst);
        return;
    }

    const uint8_t* payload = packetPayload(data);
    uint16_t payloadLen = packetPayloadSize(size);

    switch (tipo)
    {
        case TYPE_COMMAND:
            if (payloadLen == 0) {
                DBG_LORA(DBG_WARN, "uplink TYPE_COMMAND sem payload");
                return;
            }
            DBG_LORA(DBG_INFO, "uplink: %u byte(s) de comando", payloadLen);
            // Cada byte do payload e um comando (1..5) repassado ao escravo correto
            for (uint16_t i = 0; i < payloadLen; i++) {
                enviarOrdem(payload[i]);
            }
            break;

        default:
            DBG_LORA(DBG_WARN, "uplink com tipo nao tratado: 0x%02X", tipo);
            break;
    }
}

void taskTelemetria(void *pvParameters)
{
    respost resp;
    memset(&resp, 0, sizeof(resp));

    //Cache do último dado de sensor recebido
    sensorsData ultimoSensor;
    bool temSensor = false;

    lora.init();
    serialInit();
    lora.onPacketReceived(onReceive);

    sensorsData dados;

    //Intervalo entre pacotes de telemetria
    unsigned long lastSensorSend = 0;
    const unsigned long SENSOR_INTERVAL = 1500;

    //Janela de proteção após recepção
    unsigned long lastRxTime = 0;
    const unsigned long RX_WINDOW = 500;

    while (true)
    {

        lora.process();

        unsigned long now = millis();
        preencherResposta(resp);
        if (!lora.isIdle())
        {
            lastRxTime = now;
        }

        if (now - lastRxTime < RX_WINDOW)
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        while (xQueueReceive(filaTelemetria, &dados, 0) == pdPASS)
        {
            ultimoSensor = dados;
            temSensor = true;
        }

        if (!lora.isIdle())
        {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }

        bool enviarTelemetria = false;

        if (temSensor &&
            (now - lastSensorSend >= SENSOR_INTERVAL))
        {
            enviarTelemetria = true;
        }

        if (enviarTelemetria)
        {
            memcpy(
                &resp.sensor,
                &ultimoSensor,
                offsetof(sensorsData, tempBat1)
            );

            // -----------------------------------------------------
            // Envia telemetria
            // -----------------------------------------------------

            lora.sendPacket(
                (uint8_t*)&resp,
                sizeof(respost),
                TYPE_RESPOST
            );

            lastSensorSend = now;
            limparRespostaEscravas(&resp);

            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // -----------------------------------------------------
        // Inicia envio de imagem (nova imagem chegou via WiFi
        // e nenhuma outra transmissão de imagem está ativa)
        // -----------------------------------------------------
        if (imageReady && !lora.isImageSending())
        {
            uint8_t* imgBuf  = getImageBuffer();
            uint32_t imgSize = getImageSize();

            if (imgBuf != nullptr && imgSize > 0)
            {
                lora.sendImageRaw(imgBuf, imgSize);
                DBG_LORA(DBG_INFO, "TX imagem disparada — %lu bytes", (unsigned long)imgSize);
            }
            else
            {
                DBG_LORA(DBG_WARN, "imageReady=true mas buffer/tamanho invalido");
            }

            imageReady = false;
        }

        if (lora.isImageSending())
        {
            lora.sendImageChunk();

            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        DebugMsg dbgMsg;

        if (xDebugQueue != nullptr)
        {
            if (xQueueReceive(xDebugQueue, &dbgMsg, 0) == pdPASS)
            {
                lora.sendPacket(
                    dbgMsg.text,
                    TYPE_DEBUG
                );
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}