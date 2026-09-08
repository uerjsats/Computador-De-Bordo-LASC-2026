#include "DebugLog.h"
#include <stdarg.h>

//Instância da fila
QueueHandle_t xDebugQueue = nullptr;

// Nível → string de um caractere

static const char* levelChar(DebugLevel lvl) {
    switch (lvl) {
        case DBG_WARN:  return "W";
        case DBG_ERROR: return "E";
        default:        return "I";
    }
}

// debugInit — chamar uma vez na inicialização

void debugInit() {
    // Fila armazena até 16 mensagens.
    // Envios não bloqueantes serão descartados silenciosamente se cheia.
    xDebugQueue = xQueueCreate(16, sizeof(DebugMsg));
}

// debugLog — estilo printf, saída dupla

void debugLog(const char* module, DebugLevel level, const char* fmt, ...) {
    DebugMsg msg;

    // Constrói o corpo formatado do usuário primeiro
    char body[DBG_MSG_MAX_LEN - 20]; // reserva espaço para o prefixo [MODULE][L]
    va_list args;
    va_start(args, fmt);
    vsnprintf(body, sizeof(body), fmt, args);
    va_end(args);

    // Monta a string final: [MODULE][LEVEL] body
    snprintf(msg.text, DBG_MSG_MAX_LEN, "[%s][%s] %s",
             module, levelChar(level), body);

    // 1) Sempre imprime na Serial (imediato, não bloqueante)
    Serial.println(msg.text);

    // 2) Enfileira para transmissão LoRa (não bloqueante, descarta se cheia)
    if (xDebugQueue != nullptr) {
        xQueueSend(xDebugQueue, &msg, 0);  // 0 = não espera
    }
}