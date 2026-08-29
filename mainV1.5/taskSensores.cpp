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


// Funções do seu módulo de sensores
void initSensores()
{
    gpsInit();
    mpuInit();

    if (!BMEinit()) {
        DBG_BME(DBG_ERROR, "Sensores ambientais degradados - BME280 Indisponivel");

    }


    dhtInit();
}

void lerSensores(float &latitude, float &longitude, int &sats,
                 float &temperatura, float &umidade,
                 float &pressao, float &altitude,
                 float &gyroX, float &gyroY, float &gyroZ,
                 float &accelX, float &accelY, float &accelZ)
{
    gpsUpdate(latitude, longitude, sats);
    mpuUpdate(gyroX, gyroY, gyroZ, accelX, accelY, accelZ);
    dhtUpdate(temperatura, umidade);
    bmeUpdate(pressao, altitude);
}

void taskSensores(void *pvParameters)
{
    // Inicializa debug
    debugInit();

    // Inicializa sensores
    initSensores();

    sensorsData dados;

    // GPS
    float latitude = 0;
    float longitude = 0;
    int sats = 0;

    // DHT
    float temperatura = 0;
    float umidade = 0;

    // BME
    float pressao = 0;
    float altitude = 0;

    // MPU
    float accelX = 0;
    float accelY = 0;
    float accelZ = 0;

    float gyroX = 0;
    float gyroY = 0;
    float gyroZ = 0;

    DBG_GPS(DBG_INFO, "task de sensores iniciada");

    while (true)
    {
        memset(&dados, 0, sizeof(sensorsData));

        // Lê sensores
        lerSensores(
            latitude,
            longitude,
            sats,
            temperatura,
            umidade,
            pressao,
            altitude,
            gyroX,
            gyroY,
            gyroZ,
            accelX,
            accelY,
            accelZ
        );

        // Timestamp
        dados.seconds = millis() / 1000;

        // Sensores ambientais
        dados.temperatura = temperatura * 100.0f;
        dados.umidade = umidade * 100.0f;
        dados.altitude = altitude * 10.0f;
        dados.pressao = pressao;

        // GPS
        dados.latitude = latitude * 10000000.0f;
        dados.longitude = longitude * 10000000.0f;
        dados.sats = sats;

        // Giroscópio
        dados.roll  = gyroX * 100.0f;
        dados.pitch = gyroY * 100.0f;
        dados.yaw   = gyroZ * 100.0f;

        // Envia para a fila
        xQueueSend(filaTelemetria, &dados, 0);

        // 200 ms = 5 Hz
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}