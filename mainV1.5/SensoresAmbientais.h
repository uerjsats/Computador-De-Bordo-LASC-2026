#ifndef SENSORES_AMBIENTAIS_H
#define SENSORES_AMBIENTAIS_H

#include <Arduino.h>
#include <Wire.h>
#include <DHT.h>
#include <Wire.h>

bool BMEinit();

void bmeUpdate(float &pressao, float &altitude);

void dhtInit();

void dhtUpdate(float &temperatura, float &umidade);

#endif