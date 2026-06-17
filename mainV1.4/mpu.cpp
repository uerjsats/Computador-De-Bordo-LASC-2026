#include "mpu.h"
#include "telemetria.h"
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include "DebugLog.h"

Adafruit_MPU6050 mpu;

//Flag de init — se falhou, mpuUpdate retorna zeros
static bool _mpuReady = false;

//Configurações da Média Móvel
const int numLeituras = 10; // Aumente para suavizar mais, diminua para ter menos atraso
float leiturasX[numLeituras], leiturasY[numLeituras], leiturasZ[numLeituras];
float leiturasGX[numLeituras], leiturasGY[numLeituras], leiturasGZ[numLeituras];
int indice = 0;
float somaX = 0, somaY = 0, somaZ = 0;
float somaGX = 0, somaGY = 0, somaGZ = 0;

void mpuInit() {
  if (!mpu.begin()) {
    DBG_MPU(DBG_ERROR, "falha no init — ignorando acelerometro");
    _mpuReady = false;
  } else {
    DBG_MPU(DBG_INFO, "init OK");
    _mpuReady = true;
  }
  
  //Inicializa os arrays com zero
  for (int i = 0; i < numLeituras; i++) {
    leiturasX[i] = 0;
    leiturasY[i] = 0;
    leiturasZ[i] = 0;
    leiturasGX[i] = 0;
    leiturasGY[i] = 0;
    leiturasGZ[i] = 0;
  }
}

void mpuUpdate(float &accelX, float &accelY, float &accelZ,
               float &gyroX,  float &gyroY,  float &gyroZ) {
  //Se o MPU não inicializou, retorna zeros silenciosamente
  if (!_mpuReady) {
    accelX = 0;
    accelY = 0;
    accelZ = 0;
    gyroX  = 0;
    gyroY  = 0;
    gyroZ  = 0;
    return;
  }

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  //Subtrai a leitura mais antiga da soma total
  somaX -= leiturasX[indice];
  somaY -= leiturasY[indice];
  somaZ -= leiturasZ[indice];
  somaGX -= leiturasGX[indice];
  somaGY -= leiturasGY[indice];
  somaGZ -= leiturasGZ[indice];

  //Armazena a nova leitura do sensor
  leiturasX[indice] = a.acceleration.x;
  leiturasY[indice] = a.acceleration.y;
  leiturasZ[indice] = a.acceleration.z;
  leiturasGX[indice] = g.gyro.x;
  leiturasGY[indice] = g.gyro.y;
  leiturasGZ[indice] = g.gyro.z;

  //Adiciona a nova leitura à soma
  somaX += leiturasX[indice];
  somaY += leiturasY[indice];
  somaZ += leiturasZ[indice];
  somaGX += leiturasGX[indice];
  somaGY += leiturasGY[indice];
  somaGZ += leiturasGZ[indice];

  //Avança para a próxima posição do array (buffer circular)
  indice++;
  if (indice >= numLeituras) {
    indice = 0;
  }

  //Calcula a média e salva na estrutura de telemetria
  accelX = somaX / numLeituras;
  accelY = somaY / numLeituras;
  accelZ = somaZ / numLeituras;

  // Giroscópio (rad/s → graus/s)
  gyroX = (somaGX / numLeituras) * 57.2958f;
  gyroY = (somaGY / numLeituras) * 57.2958f;
  gyroZ = (somaGZ / numLeituras) * 57.2958f;
}
