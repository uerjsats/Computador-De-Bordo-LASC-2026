#include <Wire.h>
#include <Adafruit_INA219.h>

Adafruit_INA219 ina219;

// Endereco deste escravo no protocolo "endereco:dados"
#define ENDERECO_SUPRIMENTO 1

// --- CORREÇÃO: Adicionado as variáveis do buffer que faltavam ---
const size_t TAMANHO_MAX_BUFFER = 8; // Limite de 64 bits (8 bytes)
char bufferSerial[TAMANHO_MAX_BUFFER];
size_t indiceBuffer = 0;

void setup()
{
    Serial.begin(115200); 

    Wire.begin();
    if (!ina219.begin())
    {
        Serial.println("ERRO: INA219 nao encontrado");
        while (1) { delay(10); }
    }
}

void enviarDadosBateria()
{
    float busVoltage_V    = ina219.getBusVoltage_V();
    float shuntVoltage_mV = ina219.getShuntVoltage_mV();
    float current_mA      = ina219.getCurrent_mA();
    float power_mW        = ina219.getPower_mW();

    float loadVoltage_V = busVoltage_V + (shuntVoltage_mV / 1000.0);

    char dados[64];

    snprintf(
        dados,
        sizeof(dados),
        "%d:%.2f:%.2f:%.2f",
        ENDERECO_SUPRIMENTO,
        loadVoltage_V,
        current_mA,
        power_mW
    );

    Serial.println(dados);
}

void loop()
{
    while (Serial.available() > 0)
    {
        char c = Serial.read(); 

        // Se chegou ao fim da linha, processa o comando completo
        if (c == '\n')
        { 
            bufferSerial[indiceBuffer] = '\0'; // Finaliza a string
            
            int comando = atoi(bufferSerial); // Converte para número

            // --- CORREÇÃO: O switch deve ficar DENTRO da verificação do '\n' ---
            switch (comando)
            {
                case 4: 
                    enviarDadosBateria();
                    break;

                case 5: 
                    // TODO: implementar acao do comando 5
                    break;

                default:
                    break;
            }

            indiceBuffer = 0; // CORREÇÃO: Nome corrigido e zerado após processar o comando
        }
        // --- CORREÇÃO: O else if agora faz parte da leitura de caracteres corretamente ---
        else if (c != '\r' && indiceBuffer < (TAMANHO_MAX_BUFFER - 1))
        { 
            bufferSerial[indiceBuffer] = c; // Armazena o caractere
            indiceBuffer++; 
        }
        // Proteção contra estouro de buffer (limite de 64 bits excedido)
        else if (indiceBuffer >= (TAMANHO_MAX_BUFFER - 1))
        {
            indiceBuffer = 0; // Descarta pacote inválido
        }
    }
}