#ifndef TELEMETRIA_H
#define TELEMETRIA_H

#include <Arduino.h>
#include <RadioLib.h>

// =============================================
// Protocolo compartilhado com a estação de solo
// =============================================
#include "../LoRaRX/protocol.h"

#define BUFFER_SIZE 256
#define MAX_IMAGE_SIZE 80000

struct ImageTransaction {
    uint8_t* data;
    uint16_t size;
    uint16_t index;
    uint16_t total;
    bool ativo;
};

extern ImageTransaction imgTx;

class LoraTelemetria {
public:
    LoraTelemetria(uint8_t myAddress, uint8_t destAddress);

    void init();
    void process();

    void sendPacket(const char* payload, uint8_t type);
    void sendPacket(uint8_t* data, uint16_t size, uint8_t type);

    void sendImage(uint8_t* data, uint16_t size); 
    void sendImageRaw(uint8_t* data, uint32_t size);
    void sendImageChunk();

    bool isIdle();
    bool isImageSending(); 
    
    void setTxInterval(unsigned long interval);
    void onPacketReceived(void (*callback)(uint8_t*, uint16_t));

private:
    static void setFlag(void);
    void handleReceived(uint8_t *payload, uint16_t size);
    void binarize(uint8_t *input, uint8_t *output, int size, uint8_t threshold = 128);
    int encodeRLE(uint8_t *input, int size, uint8_t *output);

    uint8_t _myAddress;
    uint8_t _destAddress;
    bool _idle;
    unsigned long _txInterval;
    unsigned long _lastTxTime;
    uint8_t _txBuffer[BUFFER_SIZE];
    uint8_t _rxBuffer[BUFFER_SIZE];
    void (*_callback)(uint8_t*, uint16_t);
    static volatile bool _receivedFlag;
};

#endif