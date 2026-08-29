#ifndef SLAVE_PARSE_H
#define SLAVE_PARSE_H

#include <stddef.h>
#include "protocol.h"

// =============================================
// Parse puro (sem Arduino) das respostas das placas escravas.
// Testavel nativamente com g++ (ver tests/test_slave_parse.cpp).
// Protocolo das escravas: "endereco:payload\n"
//   - suprimento (addr 1): payload = "V,A,P" (tensao, corrente, potencia)
//   - controle  (addr 3): payload = status ("0" aberto / "1" fechado)
// =============================================

// Divide "endereco:payload" (ex.: "1:12.34,56.78,90.12").
// Retorna true e preenche *endereco/payloadOut; false se malformado
// (sem ':', endereco vazio, ou argumentos nulos).
bool parseSlaveLine(const char* line, int* endereco, char* payloadOut, size_t cap);

// Preenche sensor->tensao/corrente a partir de "V,A[,P]" (placa addr 1).
// Campos ausentes viram "" (nunca lixo). tempBat1/tempBat2 nao sao tocados
// (sem sensor de temperatura de bateria ainda).
void preencherSuprimento(sensorsData* sensor, const char* payload);

// Copia a resposta de controle de atitude (placa addr 3) em resp->controle (cData).
void preencherControle(respost* resp, const char* payload);

// Zera as strings preenchidas pelas escravas, para o proximo envio sair "vazio".
void limparRespostaEscravas(respost* resp);

#endif
