#include "telemetria.h"
#include "DebugLog.h"

#define RF_FREQUENCY 920.5
#define TX_OUTPUT_POWER 20
#define LORA_BANDWIDTH 125.0
#define LORA_SPREADING_FACTOR 7
#define LORA_CODINGRATE 5

#define PACKET_SIZE 180
#define MAX_IMAGE_SIZE 50000

SX1262 radio = new Module(8, 14, 12, 13);

volatile bool LoraTelemetria::_receivedFlag = false;

static uint8_t binaryBuffer[MAX_IMAGE_SIZE];
static uint8_t rleBuffer[MAX_IMAGE_SIZE];

ImageTransaction imgTx;

LoraTelemetria::LoraTelemetria(uint8_t myAddress, uint8_t destAddress)
: _myAddress(myAddress), _destAddress(destAddress)
{
    _idle = true;
    _txInterval = 300;
    _lastTxTime = 0;
    _callback = nullptr;
}

void LoraTelemetria::init()
{
    int state = radio.begin(RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, LORA_CODINGRATE);
    if(state != RADIOLIB_ERR_NONE) {
        DBG_LORA(DBG_ERROR, "radio.begin falhou, codigo=%d", state);
        return;
    }

    radio.setOutputPower(TX_OUTPUT_POWER);
    radio.setPacketReceivedAction(setFlag);
    radio.startReceive();
    DBG_LORA(DBG_INFO, "init OK — freq=%.1f BW=%.1f SF=%d addr=0x%02X dest=0x%02X",
             RF_FREQUENCY, LORA_BANDWIDTH, LORA_SPREADING_FACTOR, _myAddress, _destAddress);
}

void LoraTelemetria::setFlag(void) {
    _receivedFlag = true;
}

void LoraTelemetria::process()
{
    if(_receivedFlag) {
        _receivedFlag = false;

        if (!_idle) {
            int txState = radio.finishTransmit();
            if (txState != RADIOLIB_ERR_NONE) {
                DBG_LORA(DBG_WARN, "finishTransmit retornou erro=%d", txState);
            }
            _idle = true;
            radio.startReceive();
            return;
        }

        int len = radio.getPacketLength();

        if(len > 0 && len <= BUFFER_SIZE) {
            int state = radio.readData(_rxBuffer, len);
            if(state == RADIOLIB_ERR_NONE) {
                DBG_LORA(DBG_INFO, "RX %d bytes", len);
                handleReceived(_rxBuffer, len);
            } else {
                DBG_LORA(DBG_WARN, "erro readData RX, codigo=%d", state);
            }
        }

        radio.startReceive();
    }
}

void LoraTelemetria::sendPacket(uint8_t* data, uint16_t size, uint8_t type)
{
    if (!_idle) return;
    if (millis() - _lastTxTime < _txInterval) return;
    if ((uint32_t)size + HEADER_SIZE > BUFFER_SIZE) return;

    uint8_t idx = buildHeader(_txBuffer, type, _myAddress, _destAddress);
    memcpy(&_txBuffer[idx], data, size);

    _idle = false;
    radio.startTransmit(_txBuffer, size + idx);
    _lastTxTime = millis();
}

void LoraTelemetria::sendPacket(const char* payload, uint8_t type)
{
    sendPacket((uint8_t*)payload, strlen(payload), type);
}

void LoraTelemetria::handleReceived(uint8_t *payload, uint16_t size)
{
    if (!validateHeader(payload, size)) {
        DBG_LORA(DBG_WARN, "cabecalho invalido no pacote recebido");
        return;
    }

    uint8_t src = packetSrc(payload);
    uint8_t dst = packetDst(payload);

    // So aceita pacotes enderecados a este dispositivo e vindos do par esperado
    if (dst != _myAddress || src != _destAddress) {
        DBG_LORA(DBG_WARN, "pacote descartado: src=0x%02X dst=0x%02X (esperado src=0x%02X dst=0x%02X)",
                 src, dst, _destAddress, _myAddress);
        return;
    }

    // Repassa o pacote INTEIRO (com cabecalho) para o callback.
    // O callback usa packetType()/packetPayload()/packetPayloadSize() de protocol.h
    // para interpretar, exatamente como o LoRaRX.ino ja faz.
    if (_callback)
        _callback(payload, size);
}

void LoraTelemetria::onPacketReceived(void (*callback)(uint8_t*, uint16_t))
{
    _callback = callback;
}

void LoraTelemetria::binarize(uint8_t *input, uint8_t *output, int size, uint8_t threshold)
{
    for (int i = 0; i < size; i++) {
        output[i] = (input[i] > threshold) ? 1 : 0;
    }
}

int LoraTelemetria::encodeRLE(uint8_t *input, int size, uint8_t *output)
{
    int outIndex = 0;
    uint8_t current = input[0];
    uint8_t count = 1;

    for (int i = 1; i < size; i++) {
        if (input[i] == current && count < 255) {
            count++;
        } else {
            output[outIndex++] = count;
            output[outIndex++] = current;
            current = input[i];
            count = 1;
        }
    }

    output[outIndex++] = count;
    output[outIndex++] = current;

    return outIndex;
}

void LoraTelemetria::sendImageRaw(uint8_t* data, uint32_t size)
{
    if (data == nullptr || size == 0) return;

    if (size > MAX_IMAGE_SIZE) {
        DBG_LORA(DBG_ERROR, "imagem muito grande (%lu bytes, max %d)", (unsigned long)size, MAX_IMAGE_SIZE);
        return;
    }

    DBG_LORA(DBG_INFO, "sendImageRaw %lu bytes", (unsigned long)size);
    sendImage(data, size);
}

void LoraTelemetria::sendImage(uint8_t* data, uint16_t size)
{
    if (imgTx.ativo) return;

    imgTx.data = data;
    imgTx.size = size;
    imgTx.index = 0;
    imgTx.total = (size + PACKET_SIZE - 1) / PACKET_SIZE;
    imgTx.ativo = true;

    DBG_LORA(DBG_INFO, "TX imagem iniciada — %u bytes, %u chunks", size, imgTx.total);
}

void LoraTelemetria::sendImageChunk()
{
    if (!imgTx.ativo) return;
    if (!_idle) return;
    if (millis() - _lastTxTime < _txInterval) return;

    if (imgTx.index >= imgTx.total) {
        imgTx.ativo = false;
        DBG_LORA(DBG_INFO, "TX imagem concluida");
        return;
    }

    int start = imgTx.index * PACKET_SIZE;
    int remaining = imgTx.size - start;
    int len = (remaining > PACKET_SIZE) ? PACKET_SIZE : remaining;

    uint8_t packet[PACKET_SIZE + 7];

    uint8_t idx = buildHeader(packet, TYPE_IMAGE, _myAddress, _destAddress);
    packet[idx++] = (uint8_t)imgTx.index;
    packet[idx++] = (uint8_t)imgTx.total;
    packet[idx++] = (uint8_t)len;

    memcpy(packet + idx, imgTx.data + start, len);

    _idle = false;

    int state = radio.startTransmit(packet, len + idx);

    if (state != RADIOLIB_ERR_NONE) {
        DBG_LORA(DBG_ERROR, "falha no TX chunk imagem, codigo=%d", state);
        imgTx.ativo = false;
        _idle = true;
        return;
    }

    _lastTxTime = millis();
    imgTx.index++;
}

bool LoraTelemetria::isIdle()
{
    return _idle;
}

void LoraTelemetria::setTxInterval(unsigned long interval)
{
    _txInterval = interval;
}

bool LoraTelemetria::isImageSending() {
    return imgTx.ativo;
}