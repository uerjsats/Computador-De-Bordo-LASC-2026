#include <RadioLib.h>

// =============================================
// Protocolo compartilhado com o OBC (mainV1.4)
// =============================================
#include "protocol.h"

// CONFIGURAÇÃO LoRa
#define RF_FREQUENCY           915.0
#define LORA_BANDWIDTH         125.0
#define LORA_SPREADING_FACTOR  7
#define LORA_CODINGRATE        5

// PINAGEM FIXA DA HELTEC V3 (SX1262)
// NSS: 8, DIO1: 14, NRST: 12, BUSY: 13
SX1262 radio = new Module(8, 14, 12, 13);

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
    float ultimoRSSI = 0; 
    float ultimoSNR = 0; 
}; 

Stats stats; 

// INTERRUPÇÃO
void setFlag() {
    operationDone = true;
}

void imprimirEstatisticas();

void enviarComando(const uint8_t* payload, uint8_t payloadLen)
{ 
    uint8_t buffer[65]; 
    uint8_t idx = buildHeader(buffer, TYPE_COMMAND, ADDR_GROUND, ADDR_OBC);

    for (uint8_t i = 0; i < payloadLen; i++) { 
        buffer[idx++] = payload[i]; 
    }

    Serial.println(); 
    Serial.println("---- ENVIANDO COMANDO ----");
    Serial.print("Tamanho: "); 
    Serial.println(idx); 

    isTransmitting = true; 
    operationDone = false; 

    int state = radio.startTransmit(buffer, idx); 

    if(state == RADIOLIB_ERR_NONE) { 
        Serial.println("Transmissao iniciada com sucesso."); 
    }
    else { 
        Serial.print("Erro ao iniciar TX: "); 
        Serial.println(state); 
        isTransmitting = false; 
        radio.startReceive(); 
    }
}

void processarSerial() 
{ 
    if(!Serial.available())
        return; 
    
    String linha = Serial.readStringUntil('\n');
    linha.trim(); 

    if(linha.length() == 0)
        return; 

    if (linha.equalsIgnoreCase("STATS"))
    { 
        imprimirEstatisticas(); 
        return; 
    }

    if (linha.startsWith("CMD"))
    { 
        String hexPart = linha.substring(3); 
        hexPart.trim(); 

        uint8_t payload[32]; 
        uint8_t payloadLen = 0; 

        int start = 0; 
        while (start < hexPart.length() && payloadLen < sizeof(payload))
        { 
            int space = hexPart.indexOf(' ', start);
            String token; 

            if(space == -1)
            { 
                token = hexPart.substring(start); 
                start = hexPart.length();
            }
            else 
            { 
                token = hexPart.substring(start, space); 
                start = space + 1; 
            }
            token.trim(); 
            
            if(token.length() > 0)
            { 
                payload[payloadLen++] = (uint8_t) strtol(token.c_str(), nullptr, 16);
            }
        }
        if (payloadLen == 0)
        { 
            Serial.println("Comando vazio. Use: CMD <byte1> <byte2> ...");
            Serial.println("Exemplo: CMD 01 02"); 
            return; 
        }

        enviarComando(payload, payloadLen);
        stats.comandosEnviados++; 
        return; 
    }
    Serial.println("Comando nao reconhecido."); 
    Serial.println("Use: CMD <hex bytes> ou STATS"); 
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

    Serial.print("Ultimo RSSI         : ");
    Serial.print(stats.ultimoRSSI);
    Serial.println(" dBm");
    
    Serial.print("Ultimo SNR          : ");
    Serial.print(stats.ultimoSNR);
    Serial.println(" dB");

    Serial.println("==================================");
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

    if(type == TYPE_SENSOR)
    {
        sensorsData sensor;
        if(!parseSensorData(payload, payloadLen, &sensor))
        {
            Serial.println("Pacote SENSOR incompleto!");
            stats.pacotesIncompletos++;
            return;
        }

        stats.sensorCount++; 

        // Conversões de escala (inverso do que o OBC aplica)
        float temperatura = sensor.temperatura / 100.0f;
        float umidade     = sensor.umidade / 100.0f;
        float altitude    = sensor.altitude / 10.0f;

        float latitude    = sensor.latitude / 10000000.0f;
        float longitude   = sensor.longitude / 10000000.0f;

        float roll_deg    = sensor.roll  / 100.0f;
        float pitch_deg   = sensor.pitch / 100.0f;
        float yaw_deg     = sensor.yaw   / 100.0f;

        float tempBat1    = sensor.tempBat1 / 100.0f;
        float tempBat2    = sensor.tempBat2 / 100.0f;
        float tensao      = sensor.tensao   / 100.0f;
        float corrente    = sensor.corrente / 100.0f;

        // Formato para plotagem no AbaTrack
        // Tempo:Temperatura:Umidade:Altitude:Pressao:Latitude:Longitude:Sats:Roll:Pitch:Yaw:TempBat1:TempBat2:Tensao:Corrente:NumPacotes:RSSI:TamPacote
        Serial.print(sensor.seconds); Serial.print(":");
        Serial.print(temperatura, 2); Serial.print(":");
        Serial.print(umidade, 2); Serial.print(":");
        Serial.print(altitude, 1); Serial.print(":");
        Serial.print(sensor.pressao); Serial.print(":");
        Serial.print(latitude, 7); Serial.print(":");
        Serial.print(longitude, 7); Serial.print(":");
        Serial.print(sensor.sats); Serial.print(":");
        Serial.print(roll_deg, 2); Serial.print(":");
        Serial.print(pitch_deg, 2); Serial.print(":");
        Serial.print(yaw_deg, 2); Serial.print(":");
        Serial.print(tempBat1, 2); Serial.print(":");
        Serial.print(tempBat2, 2); Serial.print(":");
        Serial.print(tensao, 2); Serial.print(":");
        Serial.print(corrente, 2); Serial.print(":");
        Serial.print(stats.totalRecebidos); Serial.print(":");
        Serial.print(stats.ultimoRSSI); Serial.print(":");
        Serial.println(size);
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

            Serial.println();
            Serial.println("===== IMAGEM =====");

            Serial.print("Chunk       : ");
            Serial.print(chunkIndex + 1);
            Serial.print("/");
            Serial.println(totalChunk);

            Serial.print("Dados       : ");
            Serial.print(dataLen);
            Serial.println(" bytes");
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
    Serial.println("Inicializando SX1262 (Heltec V2)...");

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

    Serial.println("LoRa pronto! (Estação de Solo Híbrida V2)"); 
    Serial.println("Digite 'STATS' para ver estatisticas");
    Serial.println("Digite 'CMD <hex bytes>' para enviar comando ao OBC");
    Serial.println("Exemplo: CMD 01 05");
}

// LOOP
void loop()
{
    processarSerial(); 

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