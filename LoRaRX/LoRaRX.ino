#include <RadioLib.h>

// CONFIGURAÇÃO LoRa
#define RF_FREQUENCY           915.0
#define LORA_BANDWIDTH         125.0
#define LORA_SPREADING_FACTOR  7
#define LORA_CODINGRATE        5

#define START_BYTE   0x7E
#define TYPE_SENSOR  0x01
#define TYPE_IMAGE   0x02
#define TYPE_COMMAND 0X03

#define ADDR_GROUND  0x01 
#define ADDR_OBC     0x02 

// PINAGEM FIXA DA HELTEC V2 (SX1276)
// NSS: 18, DIO0: 26, NRST: 14, DIO1: 35
SX1276 radio = new Module(18, 26, 14, 35);

volatile bool operationDone = false; 
bool isTransmitting = false; 

struct Stats { 
    uint32_t totalRecebidos = 0; 
    uint32_t sensorCount = 0; 
    uint32_t imageCount = 0; 
    uint32_t commandCount = 0; 
    uint32_t desconhecidos = 0; 
    uint32_t startByteInvalido = 0; 
    uint32_t pacotesIncompletos = 0; 
    uint32_t comandosEnviados = 0; 
    float ultimoRSSI = 0; 
    float ultimoSNR = 0; 
}; 

Stats stats; 

// ESTRUTURA RECEBIDA
struct sensorsData {
    uint32_t seconds;
    int16_t temperatura;
    int16_t umidade;
    uint32_t pressao;
    int16_t altitude;
    int32_t latitude;
    int32_t longitude;
    uint8_t sats;
    int16_t accelX;
    int16_t accelY;
    int16_t accelZ;
};

// INTERRUPÇÃO
void setFlag() {
    operationDone = true;
}

void imprimirEstatisticas();

void enviarComando(const uint8_t* payload, uint8_t payloadLen)
{ 
    uint8_t buffer[65]; 
    uint8_t idx = 0; 

    buffer[idx++] = START_BYTE; 
    buffer[idx++] = TYPE_COMMAND; 
    buffer[idx++] = ADDR_GROUND;
    buffer[idx++] = ADDR_OBC; 

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
    if(size < 4) { 
        stats.pacotesIncompletos++; 
        return; 
    }

    uint8_t type = data[1];
 
    stats.ultimoRSSI = radio.getRSSI();
    stats.ultimoSNR = radio.getSNR(); 

    Serial.println();
    Serial.println("================================");
    Serial.println("       PACOTE RECEBIDO");
    Serial.println("================================");

    Serial.print("Origem      : ");
    Serial.println(data[2]);

    Serial.print("Destino     : ");
    Serial.println(data[3]);

    Serial.print("RSSI        : ");
    Serial.print(stats.ultimoRSSI); 
    Serial.println(" dBm");

    Serial.print("SNR         : ");
    Serial.print(stats.ultimoSNR); 
    Serial.println(" dB");

    Serial.print("Bytes       : ");
    Serial.println(size);

    if(type == TYPE_SENSOR)
    {
        if(size < (4 + sizeof(sensorsData)))
        {
            Serial.println("Pacote SENSOR incompleto!");
            stats.pacotesIncompletos++;
            return;
        }

        stats.sensorCount++; 

        sensorsData sensor;
        memcpy(&sensor, &data[4], sizeof(sensorsData));

        float temperatura = sensor.temperatura / 100.0f;
        float umidade     = sensor.umidade / 100.0f;
        float altitude    = sensor.altitude / 10.0f;

        float latitude    = sensor.latitude / 10000000.0f;
        float longitude   = sensor.longitude / 10000000.0f;

        Serial.println();
        Serial.println("===== SENSORES =====");

        Serial.print("Tempo ligado: ");
        Serial.print(sensor.seconds);
        Serial.println(" s");

        Serial.print("Temperatura : ");
        Serial.print(temperatura, 2);
        Serial.println(" C");

        Serial.print("Umidade     : ");
        Serial.print(umidade, 2);
        Serial.println(" %");

        Serial.print("Pressao     : ");
        Serial.print(sensor.pressao);
        Serial.println(" Pa");

        Serial.print("Altitude    : ");
        Serial.print(altitude, 1);
        Serial.println(" m");

        Serial.println();
        Serial.println("===== GPS =====");

        Serial.print("Latitude    : ");
        Serial.println(latitude, 7);

        Serial.print("Longitude   : ");
        Serial.println(longitude, 7);

        Serial.print("Satelites   : ");
        Serial.println(sensor.sats);

        Serial.println();
        Serial.println("===== MPU6050 =====");

        Serial.print("Accel X     : ");
        Serial.println(sensor.accelX);

        Serial.print("Accel Y     : ");
        Serial.println(sensor.accelY);

        Serial.print("Accel Z     : ");
        Serial.println(sensor.accelZ);
    }
    else if(type == TYPE_IMAGE)
    {
        if(size < 7)
        {
            Serial.println("Pacote IMAGEM inválido!");
            stats.pacotesIncompletos++;
            return;
        }

        stats.imageCount++; 

        uint8_t chunkIndex = data[4];
        uint8_t totalChunk = data[5];
        uint8_t dataLen    = data[6];

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
    else if(type == TYPE_COMMAND) 
    {
        stats.commandCount++;

        Serial.println();
        Serial.println("===== COMANDO (RX) =====");

        Serial.print("Payload     : ");
        for (uint16_t i = 4; i < size; i++)
        {
            Serial.print("0x");
            if (data[i] < 0x10) Serial.print("0");
            Serial.print(data[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    else
    {
        stats.desconhecidos++;
        Serial.print("Tipo desconhecido: ");
        Serial.println(type);
    }

    Serial.println("================================");
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
    Serial.println("Inicializando SX1276 (Heltec V2)...");

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