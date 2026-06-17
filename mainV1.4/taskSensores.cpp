//Bibliotecas do RTOS
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

//Bibliotecas Modularizadas
#include "telemetria.h"
#include "gps.h"
#include "mpu.h"
#include "SensoresAmbientais.h"
#include "DebugLog.h"

//Fila criada no main.cpp
extern QueueHandle_t filaTelemetria;

SensoresAmbientais sensores(48, DHT22, 0X76);

// Funções do seu módulo de sensores
void initSensores()
{
    gpsInit();

    if (!sensores.init(false)) {
        DBG_BME(DBG_ERROR, "sensores ambientais degradados — BME280 indisponivel");
        // continua — não interrompe os demais
    }

    mpuInit();
}

void lerSensores(float &latitude, float &longitude, int &sats,
                 float &temperatura, float &umidade,
                 float &pressao, float &altitude,
                 float &accelX, float &accelY, float &accelZ,
                 float &gyroX, float &gyroY, float &gyroZ)
{
    gpsUpdate(latitude, longitude, sats);
    sensores.lerDHT(temperatura, umidade);
    sensores.lerBME(pressao, altitude);
    mpuUpdate(accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
}

void taskSensores(void *pvParameters)
{
    //Inicializa o sistema de debug (deve ser chamado antes de qualquer debugLog)
    debugInit();

    //Inicializa sensores
    initSensores();
    sensorsData dados;

    //Variáveis locais
    //GPS
    float latitude = 0, longitude = 0;
    int sats = 0;

    //DHT
    float temperatura = 0, umidade = 0;

    //BME
    float pressao = 0, altitude = 0;

    //MPU — aceleração + giroscópio
    float accelX = 0, accelY = 0, accelZ = 0;
    float gyroX = 0, gyroY = 0, gyroZ = 0;

    DBG_GPS(DBG_INFO, "task de sensores iniciada");

    while (true)
    {
        //Limpa struct
        memset(&dados, 0, sizeof(sensorsData));

        //Lê todos os sensores
        lerSensores(
            latitude,
            longitude,
            sats,
            temperatura,
            umidade,
            pressao,
            altitude,
            accelX,
            accelY,
            accelZ,
            gyroX,
            gyroY,
            gyroZ
        );

        //Timestamp
        dados.seconds = millis() / 1000;

        //Preenche struct — ordem conforme Dados.md
        dados.temperatura = temperatura * 100.0f;
        dados.umidade = umidade * 100.0f;
        dados.altitude = altitude * 10.0f;
        dados.pressao = pressao;

        dados.latitude = latitude * 10000000.0f;
        dados.longitude = longitude * 10000000.0f;
        dados.sats = sats;

        // Giroscópio (graus × 100)
        dados.roll  = gyroX * 100.0f;
        dados.pitch = gyroY * 100.0f;
        dados.yaw   = gyroZ * 100.0f;

        // Dados de bateria/energia — preenchidos com 0 até que
        // o subsistema de suprimento esteja integrado
        dados.tempBat1  = 0;
        dados.tempBat2  = 0;
        dados.tensao    = 0;
        dados.corrente  = 0;

        //Envia para fila (não bloqueante)
        xQueueSend(filaTelemetria, &dados, 0);

        //Frequência da leitura
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}