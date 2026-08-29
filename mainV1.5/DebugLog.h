#ifndef DEBUG_LOG_H
#define DEBUG_LOG_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// Níveis de Severidade

enum DebugLevel : uint8_t {
    DBG_INFO  = 0,  // 'I' — informacional
    DBG_WARN  = 1,  // 'W' — aviso, degradado mas operacional
    DBG_ERROR = 2   // 'E' — erro, módulo falhou
};

// Tamanho máximo da mensagem (deve caber em um pacote LoRa)

#define DBG_MSG_MAX_LEN 200

// Struct interna enviada para a fila

struct DebugMsg {
    char text[DBG_MSG_MAX_LEN];
};

// Fila global — criada em DebugLog.cpp

extern QueueHandle_t xDebugQueue;

// API Principal

// Chame uma vez antes de qualquer chamada debugLog()
void debugInit();

// Log de debug estilo printf
// Imprime na Serial imediatamente, enfileira para LoRa
void debugLog(const char* module,
              DebugLevel level,
              const char* fmt, ...);

// Macros de conveniência — uma por módulo
// Uso: DBG_GPS(DBG_INFO, "sats=%d", n);

#define DBG_GPS(lvl, fmt, ...)     debugLog("GPS",    lvl, fmt, ##__VA_ARGS__)
#define DBG_MPU(lvl, fmt, ...)     debugLog("MPU",    lvl, fmt, ##__VA_ARGS__)
#define DBG_DHT(lvl, fmt, ...)     debugLog("DHT",    lvl, fmt, ##__VA_ARGS__)
#define DBG_BME(lvl, fmt, ...)     debugLog("BME",    lvl, fmt, ##__VA_ARGS__)
#define DBG_EEPROM(lvl, fmt, ...)  debugLog("EEPROM", lvl, fmt, ##__VA_ARGS__)
#define DBG_LORA(lvl, fmt, ...)    debugLog("LORA",   lvl, fmt, ##__VA_ARGS__)
#define DBG_WIFI(lvl, fmt, ...)    debugLog("WIFI",   lvl, fmt, ##__VA_ARGS__)
#define DBG_BRIDGE(lvl, fmt, ...)  debugLog("BRIDGE", lvl, fmt, ##__VA_ARGS__)
#define DBG_SLAVES(lvl, fmt, ...)  debugLog("SLAVES", lvl, fmt, ##__VA_ARGS__)

#endif
