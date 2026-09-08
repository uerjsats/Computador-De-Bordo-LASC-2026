#ifndef SLAVES_H
#define SLAVES_H

#include <stdint.h>
#include "protocol.h"

void serialInit();
void enviarOrdem(uint8_t comando);

// Drena as seriais das escravas e grava a resposta em `resp`:
//   controle de atitude (addr 3) -> resp.controle (cData)
//   suprimento de energia (addr 1) -> resp.sensor.tensao/corrente
// Retorna true se uma resposta valida foi preenchida nesta chamada.
bool preencherResposta(respost &resp);

#endif
