#include <RadioLib.h>

// CONFIGURAÇÃO LoRa
#define RF_FREQUENCY           915.0
#define LORA_BANDWIDTH         125.0
#define LORA_SPREADING_FACTOR  7
#define LORA_CODINGRATE        5

#define START_BYTE   0x7E
#define TYPE_SENSOR  0x01
#define TYPE_IMAGE   0x02

// NSS, DIO1, NRST, BUSY
SX1262 radio = new Module(8, 14, 12, 13);

volatile bool receivedFlag = false;

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
    receivedFlag = true;
}

// CALLBACK RECEPÇÃO
void onReceive(uint8_t* data, uint16_t size)
{
    if(size < 4)
        return;

    uint8_t type = data[1];

    Serial.println();
    Serial.println("================================");
    Serial.println("       PACOTE RECEBIDO");
    Serial.println("================================");

    Serial.print("Origem      : ");
    Serial.println(data[2]);

    Serial.print("Destino     : ");
    Serial.println(data[3]);

    Serial.print("RSSI        : ");
    Serial.print(radio.getRSSI());
    Serial.println(" dBm");

    Serial.print("SNR         : ");
    Serial.print(radio.getSNR());
    Serial.println(" dB");

    Serial.print("Bytes       : ");
    Serial.println(size);

    if(type == TYPE_SENSOR)
    {
        if(size < (4 + sizeof(sensorsData)))
        {
            Serial.println("Pacote SENSOR incompleto!");
            return;
        }

        sensorsData sensor;

        memcpy(
            &sensor,
            &data[4],
            sizeof(sensorsData)
        );

        // Converte de volta para float
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
        Serial.println(" °C");

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
            return;
        }

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
    else
    {
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

    Serial.println();
    Serial.println("Inicializando SX1262...");

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

    Serial.println("LoRa pronto!");
}

// LOOP

void loop()
{
    if(!receivedFlag)
    {
        delay(1);
        return;
    }

    receivedFlag = false;

    uint8_t buffer[256];

    int len = radio.getPacketLength();

    if(len <= 0 || len > sizeof(buffer))
    {
        radio.startReceive();
        return;
    }

    int state = radio.readData(buffer, len);

    if(state == RADIOLIB_ERR_NONE)
    {
        if(buffer[0] == START_BYTE)
        {
            onReceive(buffer, len);
        }
        else
        {
            Serial.println("START_BYTE inválido");
        }
    }

    radio.startReceive();
}