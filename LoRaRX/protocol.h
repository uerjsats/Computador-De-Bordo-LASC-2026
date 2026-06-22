#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <string.h>

// =============================================
// Protocolo LoRa compartilhado entre TX e RX
// =============================================

#define START_BYTE   0x7E

// Tipos de pacote — devem ser iguais no OBC (mainV1.4) e na estação de solo (LoRaRX)
#define TYPE_SENSOR  0x01
#define TYPE_GPS     0x02
#define TYPE_GYRO    0x03
#define TYPE_IMAGE   0x10
#define TYPE_DEBUG   0x20
#define TYPE_COMMAND 0x30

// Endereços
#define ADDR_GROUND  0x01
#define ADDR_OBC     0x02

// Tamanho mínimo de cabeçalho: START_BYTE + TYPE + SRC + DST
#define HEADER_SIZE  4

// =============================================
// Estrutura de dados de sensores
// Ordem e campos conforme Dados.md
// =============================================

#pragma pack(push, 1)
struct sensorsData {
    // 1. Tempo (s)
    uint32_t seconds;

    // 2. Temperatura (°C × 100)
    int16_t temperatura;

    // 3. Umidade (% × 100)
    int16_t umidade;

    // 4. Altitude (m × 10)
    int16_t altitude;

    // 5. Pressão (Pa)
    uint32_t pressao;

    // 6. Latitude (graus × 10^7)
    int32_t latitude;

    // 7. Longitude (graus × 10^7)
    int32_t longitude;

    // 8. Número de Satélites
    uint8_t sats;

    // 9.  Roll  / Giro X (graus × 100)
    int16_t roll;

    // 10. Pitch / Giro Y (graus × 100)
    int16_t pitch;

    // 11. Yaw   / Giro Z (graus × 100)
    int16_t yaw;

    // 12. Temperatura Bateria 1 (°C × 100)
    int16_t tempBat1;

    // 13. Temperatura Bateria 2 (°C × 100)
    int16_t tempBat2;

    // 14. Tensão (V × 100)
    int16_t tensao;

    // 15. Corrente (A × 100)
    int16_t corrente;
};
#pragma pack(pop)

// Struct para resposta completa (sensores + subsistemas)
struct respost {
    sensorsData sensor;       // Dados telemetria
    char controle[64];        // Resposta de controle de atitude
    char suprimento[64];      // Resposta de suprimento de energia
};

// =============================================
// Tamanho máximo de uma mensagem de debug
// =============================================

#define DBG_MSG_MAX_LEN 200

// =============================================
// Helpers de construção/parse de pacotes
// =============================================

// Constrói cabeçalho de 4 bytes no buffer. Retorna 4 (próximo offset).
inline uint8_t buildHeader(uint8_t* buf, uint8_t type, uint8_t src, uint8_t dst) {
    buf[0] = START_BYTE;
    buf[1] = type;
    buf[2] = src;
    buf[3] = dst;
    return HEADER_SIZE;
}

// Valida cabeçalho mínimo. Retorna true se válido.
inline bool validateHeader(const uint8_t* buf, uint16_t size) {
    if (size < HEADER_SIZE) return false;
    if (buf[0] != START_BYTE) return false;
    return true;
}

// Extrai tipo do pacote
inline uint8_t packetType(const uint8_t* buf) {
    return buf[1];
}

// Extrai endereço de origem
inline uint8_t packetSrc(const uint8_t* buf) {
    return buf[2];
}

// Extrai endereço de destino
inline uint8_t packetDst(const uint8_t* buf) {
    return buf[3];
}

// Payload começa no byte 4
inline const uint8_t* packetPayload(const uint8_t* buf) {
    return &buf[HEADER_SIZE];
}

inline uint16_t packetPayloadSize(uint16_t totalSize) {
    return (totalSize > HEADER_SIZE) ? (totalSize - HEADER_SIZE) : 0;
}

// Parse sensor data from payload (already stripped of header)
inline bool parseSensorData(const uint8_t* payload, uint16_t payloadSize, sensorsData* out) {
    if (payloadSize < sizeof(sensorsData)) return false;
    memcpy(out, payload, sizeof(sensorsData));
    return true;
}

// Parse debug message from payload (null-terminated string)
// Returns length of message copied (excluding null), or 0 on failure
inline uint16_t parseDebugMessage(const uint8_t* payload, uint16_t payloadSize, char* out, uint16_t outSize) {
    if (payloadSize == 0 || outSize == 0) return 0;
    uint16_t copyLen = (payloadSize < outSize - 1) ? payloadSize : (outSize - 1);
    memcpy(out, payload, copyLen);
    out[copyLen] = '\0';
    return copyLen;
}

#endif
