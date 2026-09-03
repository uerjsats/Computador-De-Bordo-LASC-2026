#include <RadioLib.h>
//Protocolo entre Computador de Bordo e Base
#include "protocol.h"

// CONFIGURAÇÃO LoRa
#define RF_FREQUENCY           920.5
#define LORA_BANDWIDTH         125.0
#define LORA_SPREADING_FACTOR  7
#define LORA_CODINGRATE        5

// PINAGEM FIXA DA HELTEC V3 (SX1262)
// NSS: 8, DIO1: 14, NRST: 12, BUSY: 13

//Pinagem Fixa Heltec V2(SX1276)

// NSS: 18, DIO1: 26, NRST: 14, BUSY: 35
SX1276 radio = new Module(18, 26, 14, 35);

volatile bool operationDone = false; 
bool isTransmitting = false; 

struct Stats { 
    uint32_t totalRecebidos = 0; 
    uint32_t sensorCount = 0; 
    uint32_t imageCount = 0; 
    uint32_t commandCount = 0; 
    uint32_t debugCount = 0;
    uint32_t desconhecidos = 0; 
    uint32_t startByteInvalido = 0; 
    uint32_t pacotesIncompletos = 0; 
    uint32_t comandosEnviados = 0; 
    uint32_t imagensCompletas = 0;
    uint32_t imagensDescartadas = 0;
    float ultimoRSSI = 0; 
    float ultimoSNR = 0; 
}; 

Stats stats; 

// ===================== RECONSTRUÇÃO DE IMAGEM =====================
// Limite atual do protocolo: chunkIndex/totalChunk são uint8_t (0-255),
// então o máximo teórico é 255 * PACKET_SIZE(180) = 45.900 bytes.
// Se precisar de imagens maiores, mude chunkIndex/totalChunk para
// uint16_t no protocol.h e aumente este valor de acordo.
#define MAX_IMAGE_SIZE       46000
#define IMAGE_CHUNK_TIMEOUT  5000   // ms sem novo chunk -> descarta imagem incompleta

uint8_t  imageBuffer[MAX_IMAGE_SIZE];
uint32_t imageWritePos        = 0;
uint8_t  imageExpectedChunks  = 0;
uint8_t  imageNextChunk       = 0;
bool     imageInProgress      = false;
uint32_t imageLastChunkMillis = 0;

// INTERRUPÇÃO
void setFlag() {
    operationDone = true;
}

void imprimirEstatisticas();

void enviarComando(uint8_t comando)
{
    uint8_t buffer[65];

    uint8_t idx = buildHeader(
        buffer,
        TYPE_COMMAND,
        ADDR_GROUND,
        ADDR_OBC
    );

    buffer[idx++] = comando;

    isTransmitting = true;
    operationDone = false;

    int state = radio.startTransmit(buffer, idx);

    if (state == RADIOLIB_ERR_NONE)
    {
        Serial.println("Comando enviado!");
    }
    else
    {
        Serial.print("Erro ao iniciar TX: ");
        Serial.println(state);

        isTransmitting = false;
        radio.startReceive();
    }
}

void imprimirEstatisticas() 
{ 
    Serial.println();
    Serial.println("========== DEBUG STATS ==========");

    Serial.print("Total recebidos     : ");
    Serial.println(stats.totalRecebidos);

    Serial.print("  - Sensores        : ");
    Serial.println(stats.sensorCount);

    Serial.print("  - Imagem (chunks) : ");
    Serial.println(stats.imageCount);

    Serial.print("  - Comandos RX     : ");
    Serial.println(stats.commandCount);

    Serial.print("  - Debug RX        : ");
    Serial.println(stats.debugCount);

    Serial.print("  - Desconhecidos   : ");
    Serial.println(stats.desconhecidos);

    Serial.print("Start byte invalido : ");
    Serial.println(stats.startByteInvalido);

    Serial.print("Pacotes incompletos : ");
    Serial.println(stats.pacotesIncompletos);

    Serial.print("Comandos enviados   : ");
    Serial.println(stats.comandosEnviados);

    Serial.print("Imagens completas   : ");
    Serial.println(stats.imagensCompletas);

    Serial.print("Imagens descartadas : ");
    Serial.println(stats.imagensDescartadas);

    Serial.print("Ultimo RSSI         : ");
    Serial.print(stats.ultimoRSSI);
    Serial.println(" dBm");
    
    Serial.print("Ultimo SNR          : ");
    Serial.print(stats.ultimoSNR);
    Serial.println(" dB");

    Serial.println("==================================");
}

// ---- Funções auxiliares de reconstrução de imagem ----

void resetImagem()
{
    imageInProgress     = false;
    imageWritePos        = 0;
    imageExpectedChunks  = 0;
    imageNextChunk       = 0;
}

void finalizarImagem()
{
    Serial.print("IMAGE_BEGIN\n");

    Serial.printf("SIZE:%lu\n", imageWritePos);

    Serial.write(imageBuffer, imageWritePos);

    Serial.print("IMAGE_END\n");

    stats.imagensCompletas++;
    resetImagem();
}

void verificarTimeoutImagem()
{
    if(imageInProgress && (millis() - imageLastChunkMillis > IMAGE_CHUNK_TIMEOUT))
    {
        Serial.println("Imagem incompleta descartada (timeout)");
        stats.imagensDescartadas++;
        resetImagem();
    }
}

// CALLBACK RECEPÇÃO
void onReceive(uint8_t* data, uint16_t size)
{
    if(!validateHeader(data, size)) { 
        stats.pacotesIncompletos++; 
        return; 
    }

    uint8_t type = packetType(data);
    uint8_t src  = packetSrc(data);
    uint8_t dst  = packetDst(data);

    const uint8_t* payload    = packetPayload(data);
    uint16_t       payloadLen = packetPayloadSize(size);
 
    stats.ultimoRSSI = radio.getRSSI();
    stats.ultimoSNR = radio.getSNR(); 

    if(type == TYPE_RESPOST)
    {
        respost resp;
        if(!parseRespost(payload, payloadLen, &resp))
        {
            Serial.println("Pacote RESPOST incompleto!");
            stats.pacotesIncompletos++;
            return;
        }

        stats.sensorCount++;

        const sensorsData &sensor = resp.sensor;

        // Conversões de escala (inverso do que o OBC aplica)
        float temperatura = sensor.temperatura / 100.0f;
        float umidade     = sensor.umidade / 100.0f;
        float altitude    = sensor.altitude / 10.0f;
        float pressao     = sensor.pressao;
        float latitude    = sensor.latitude / 10000000.0f;
        float longitude   = sensor.longitude / 10000000.0f;

        float roll_deg    = sensor.roll  / 100.0f;
        float pitch_deg   = sensor.pitch / 100.0f;
        float yaw_deg     = sensor.yaw   / 100.0f;

        // Formato para plotagem no AbaTrack (18 campos). tempBat1/tempBat2/
        // tensao/corrente agora sao strings cruas da placa de suprimento —
        // campo em branco quando a escrava ainda nao respondeu.
        // Tempo:Temperatura:Umidade:Altitude:Pressao:Latitude:Longitude:Sats:Roll:Pitch:Yaw:TempBat1:TempBat2:Tensao:Corrente:NumPacotes:RSSI:TamPacote
        Serial.print(sensor.seconds); Serial.print(":");
        Serial.print(temperatura, 2); Serial.print(":");
        Serial.print(umidade, 2); Serial.print(":");
        Serial.print(altitude, 1); Serial.print(":");
        Serial.print(pressao); Serial.print(":");
        Serial.print(latitude, 7); Serial.print(":");
        Serial.print(longitude, 7); Serial.print(":");
        Serial.print(sensor.sats); Serial.print(":");
        Serial.print(roll_deg, 2); Serial.print(":");
        Serial.print(pitch_deg, 2); Serial.print(":");
        Serial.print(yaw_deg, 2); Serial.print(":");
        Serial.print(sensor.tempBat1); Serial.print(":");
        Serial.print(sensor.tempBat2); Serial.print(":");
        Serial.print(sensor.tensao); Serial.print(":");
        Serial.print(sensor.corrente); Serial.print(":");
        Serial.print(stats.totalRecebidos); Serial.print(":");
        Serial.print(stats.ultimoRSSI); Serial.print(":");
        Serial.println(size);

        // Resposta de controle de atitude (cData) em linha propria, para
        // nao alterar a contagem de campos que o AbaTrack espera na linha acima.
        if (strlen(resp.controle) > 0) {
            Serial.print("CTRL:");
            Serial.println(resp.controle);
        }
    }
    else
    {
        Serial.println();
        Serial.println("================================");
        Serial.println("       PACOTE RECEBIDO");
        Serial.println("================================");

        Serial.print("Origem      : ");
        Serial.println(src);

        Serial.print("Destino     : ");
        Serial.println(dst);

        Serial.print("RSSI        : ");
        Serial.print(stats.ultimoRSSI); 
        Serial.println(" dBm");

        Serial.print("SNR         : ");
        Serial.print(stats.ultimoSNR); 
        Serial.println(" dB");

        Serial.print("Bytes       : ");
        Serial.println(size);

        if(type == TYPE_IMAGE)
        {
            if(payloadLen < 3)
            {
                Serial.println("Pacote IMAGEM inválido!");
                stats.pacotesIncompletos++;
                return;
            }

            stats.imageCount++; 

            uint8_t chunkIndex = payload[0];
            uint8_t totalChunk = payload[1];
            uint8_t dataLen    = payload[2];
            const uint8_t* chunkData = payload + 3;

            if(payloadLen < (uint16_t)(3 + dataLen))
            {
                Serial.println("Pacote IMAGEM com dados incompletos!");
                stats.pacotesIncompletos++;
                return;
            }

            Serial.println();
            Serial.println("===== IMAGEM =====");

            Serial.print("Chunk       : ");
            Serial.print(chunkIndex + 1);
            Serial.print("/");
            Serial.println(totalChunk);

            Serial.print("Dados       : ");
            Serial.print(dataLen);
            Serial.println(" bytes");

            // Chunk 0 -> inicia (ou reinicia) a reconstrução da imagem
            if(chunkIndex == 0)
            {
                if(imageInProgress)
                {
                    Serial.println("Nova imagem chegou antes da anterior terminar - descartando anterior");
                    stats.imagensDescartadas++;
                }
                resetImagem();
                imageInProgress     = true;
                imageExpectedChunks = totalChunk;
                imageNextChunk      = 0;
            }

            if(!imageInProgress)
            {
                Serial.println("Chunk recebido sem chunk 0 anterior - ignorado");
            }
            else if(chunkIndex != imageNextChunk || totalChunk != imageExpectedChunks)
            {
                Serial.println("Chunk fora de ordem/inconsistente - imagem descartada");
                stats.imagensDescartadas++;
                resetImagem();
            }
            else if(imageWritePos + dataLen > MAX_IMAGE_SIZE)
            {
                Serial.println("Imagem excede MAX_IMAGE_SIZE - descartada");
                stats.imagensDescartadas++;
                resetImagem();
            }
            else
            {
                memcpy(imageBuffer + imageWritePos, chunkData, dataLen);
                imageWritePos += dataLen;
                imageNextChunk++;
                imageLastChunkMillis = millis();

                bool ultimoChunk = (imageNextChunk == totalChunk);

                if(ultimoChunk)
                {
                    finalizarImagem();
                }
            }
        }
        else if(type == TYPE_DEBUG)
        {
            stats.debugCount++;

            char debugMsg[DBG_MSG_MAX_LEN];
            parseDebugMessage(payload, payloadLen, debugMsg, sizeof(debugMsg));

            Serial.println();
            Serial.println("===== DEBUG (OBC) =====");
            Serial.println(debugMsg);
        }
        else if(type == TYPE_COMMAND) 
        {
            stats.commandCount++;

            Serial.println();
            Serial.println("===== COMANDO (RX) =====");

            Serial.print("Payload     : ");
            for (uint16_t i = 0; i < payloadLen; i++)
            {
                Serial.print("0x");
                if (payload[i] < 0x10) Serial.print("0");
                Serial.print(payload[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
        else
        {
            stats.desconhecidos++;
            Serial.print("Tipo desconhecido: 0x");
            Serial.println(type, HEX);
        }

        Serial.println("================================");
    }
}

void processarSerial()
{
    if (Serial.available())
    {
        char entrada = Serial.read();

        if (entrada >= '1' && entrada <= '5')
        {
            uint8_t comando = entrada - '0';

            enviarComando(comando);
        }
    }
}

// SETUP
void setup()
{
    Serial.begin(115200);
    delay(2000);

    // RESET MANUAL DO CHIP LORA (Obrigatório na Heltec V2)
    pinMode(14, OUTPUT);
    digitalWrite(14, LOW);
    delay(50);
    digitalWrite(14, HIGH);
    delay(50);

    Serial.println();

    int state = radio.begin(
        RF_FREQUENCY,
        LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR,
        LORA_CODINGRATE
    );

    if(state != RADIOLIB_ERR_NONE)
    {
        Serial.print("Erro RadioLib: ");
        Serial.println(state);

        while(true)
        {
            delay(1000);
        }
    }

    radio.setPacketReceivedAction(setFlag);
    radio.startReceive();
}

// LOOP
void loop()
{
    processarSerial(); 
    verificarTimeoutImagem();

    if(!operationDone)
    {
        delay(1);
        return;
    }

    operationDone = false;

    if(isTransmitting)
    { 
        int state = radio.finishTransmit(); 

        if(state == RADIOLIB_ERR_NONE)
        { 
            Serial.println("Comando transmitido com sucesso!"); 
        }
        else 
        { 
            Serial.print("Erro ao finalizar TX: "); 
            Serial.println(state); 
        }

        isTransmitting = false; 
        radio.startReceive(); 
        return; 
    }

    uint8_t buffer[256];
    int len = radio.getPacketLength();

    if(len <= 0 || len > (int)sizeof(buffer))
    {
        radio.startReceive();
        return;
    }

    int state = radio.readData(buffer, len);

    if(state == RADIOLIB_ERR_NONE)
    {
        if(buffer[0] == START_BYTE)
        {
            stats.totalRecebidos++; 
            onReceive(buffer, len);
        }
        else
        {
            stats.startByteInvalido++; 
            Serial.println("START_BYTE inválido");
        }
    }

    radio.startReceive();
}
