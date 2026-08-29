#include "slaveParse.h"

#include <string.h>
#include <stdlib.h>

// Copia o token CSV de indice `idx` de `csv` em out (cap). out="" se ausente.
static void csvToken(const char* csv, int idx, char* out, size_t cap) {
    out[0] = '\0';
    const char* p = csv;
    for (int i = 0; i < idx && p != nullptr; i++) {
        p = strchr(p, ',');
        if (p != nullptr) p++;
    }
    if (p == nullptr) return;

    const char* fim = strchr(p, ',');
    size_t n = (fim != nullptr) ? (size_t)(fim - p) : strlen(p);
    if (n >= cap) n = cap - 1;
    memcpy(out, p, n);
    out[n] = '\0';
}

bool parseSlaveLine(const char* line, int* endereco, char* payloadOut, size_t cap) {
    if (line == nullptr || endereco == nullptr || payloadOut == nullptr || cap == 0)
        return false;

    const char* sep = strchr(line, ':');
    if (sep == nullptr || sep == line)   // sem ':' ou endereco vazio
        return false;

    *endereco = atoi(line);

    const char* payload = sep + 1;
    size_t n = strlen(payload);
    if (n >= cap) n = cap - 1;
    memcpy(payloadOut, payload, n);
    payloadOut[n] = '\0';
    return true;
}

void preencherSuprimento(sensorsData* sensor, const char* payload) {
    if (sensor == nullptr || payload == nullptr) return;
    csvToken(payload, 0, sensor->tensao,   SLAVE_STR_LEN);  // V
    csvToken(payload, 1, sensor->corrente, SLAVE_STR_LEN);  // A (P descartado)
}

void preencherControle(respost* resp, const char* payload) {
    if (resp == nullptr || payload == nullptr) return;
    strncpy(resp->controle, payload, CTRL_STR_LEN - 1);
    resp->controle[CTRL_STR_LEN - 1] = '\0';
}

void limparRespostaEscravas(respost* resp) {
    if (resp == nullptr) return;
    resp->controle[0]        = '\0';
    resp->sensor.tensao[0]   = '\0';
    resp->sensor.corrente[0] = '\0';
    resp->sensor.tempBat1[0] = '\0';
    resp->sensor.tempBat2[0] = '\0';
}
