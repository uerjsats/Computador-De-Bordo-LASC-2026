#include <HardwareSerial.h>
#include <Arduino.h>
#include "slaves.h"
#include "slaveParse.h"
#include "DebugLog.h"

// Enderecos das placas escravas no protocolo "endereco:payload"
#define ENDERECO_SUPRIMENTO 1
#define ENDERECO_CONTROLE   3

HardwareSerial controleSerial(2);
HardwareSerial supSerial(1);

void serialInit()
{
    controleSerial.begin(115200, SERIAL_8N1, 33, 34);
    supSerial.begin(115200, SERIAL_8N1, 19, 20);
}

void enviarOrdem(uint8_t comando)
{
    switch (comando)
    {
        case 1: controleSerial.println("1"); break;
        case 2: controleSerial.println("2"); break;
        case 3: controleSerial.println("3"); break;
        case 4: supSerial.println("4"); break;
        case 5: supSerial.println("5"); break;
        default:
            DBG_SLAVES(DBG_WARN, "comando invalido: %u", comando);
    }
}

// Le uma linha "endereco:payload" da serial e roteia o payload para `resp`,
// conforme o endereco da escrava. Retorna true se gravou algo.
static bool lerEscrava(HardwareSerial &porta, respost &resp)
{
    char linha[64];
    size_t len = porta.readBytesUntil('\n', linha, sizeof(linha) - 1);
    linha[len] = '\0';

    int endereco;
    char payload[64];
    if (!parseSlaveLine(linha, &endereco, payload, sizeof(payload)))
        return false;

    if (endereco == ENDERECO_CONTROLE) {
        preencherControle(&resp, payload);
        return true;
    }
    if (endereco == ENDERECO_SUPRIMENTO) {
        preencherSuprimento(&resp.sensor, payload);
        return true;
    }

    DBG_SLAVES(DBG_WARN, "endereco de escrava desconhecido: %d", endereco);
    return false;
}

bool preencherResposta(respost &resp)
{
    if (controleSerial.available())
        return lerEscrava(controleSerial, resp);

    if (supSerial.available())
        return lerEscrava(supSerial, resp);

    return false;
}
