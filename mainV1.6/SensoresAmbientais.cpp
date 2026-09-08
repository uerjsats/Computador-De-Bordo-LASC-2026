#include "SensoresAmbientais.h"
#include <math.h>
#include <Wire.h>
#include <DHT.h>

#define DHTPIN 48

#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

byte BME_ADDR = 0x00;

int i;

// Coeficientes de calibração para Temperatura e Pressão
uint16_t dig_T1; int16_t dig_T2; int16_t dig_T3;
uint16_t dig_P1; int16_t dig_P2; int16_t dig_P3; int16_t dig_P4; 
int16_t dig_P5; int16_t dig_P6; int16_t dig_P7; int16_t dig_P8; int16_t dig_P9;

int32_t t_fine; // Variável de controle compartilhada entre temperatura e pressão

bool BMEinit() {
  byte enderecosPossiveis[] = {0x76, 0x77};

  for (byte idx = 0; idx < sizeof(enderecosPossiveis); idx++) {
    byte addr = enderecosPossiveis[idx];
    
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Wire.beginTransmission(addr);
      Wire.write(0xD0);
      if (Wire.endTransmission(false) == 0) {
        Wire.requestFrom((int)addr, 1);
        if (Wire.available()) {
          byte id = Wire.read();
          
          if (id == 0x60 || id == 0x58) {
            BME_ADDR = addr;
            
            Wire.beginTransmission(BME_ADDR);
            Wire.write(0x88);
            Wire.endTransmission(false);
            Wire.requestFrom((int)BME_ADDR, 24); 

            if (Wire.available() >= 24) {
                auto read16_LE = []() -> int16_t {
                    uint8_t lsb = Wire.read();
                    uint8_t msb = Wire.read();
                    return (int16_t)((msb << 8) | lsb);
                };

                dig_T1 = (uint16_t)read16_LE();
                dig_T2 = read16_LE();
                dig_T3 = read16_LE();

                dig_P1 = (uint16_t)read16_LE();
                dig_P2 = read16_LE();
                dig_P3 = read16_LE();
                dig_P4 = read16_LE();
                dig_P5 = read16_LE();
                dig_P6 = read16_LE();
                dig_P7 = read16_LE();
                dig_P8 = read16_LE();
                dig_P9 = read16_LE();
            }
            
            Wire.beginTransmission(BME_ADDR);
            Wire.write(0xF4);
            Wire.write(0x27);
            Wire.endTransmission();
            
            return true;
          }
        }
      }
    }
  }
  return false;
}


void bmeUpdate(float &pressao, float &altitude) {
  // Constante da sua altitude real conhecida em metros
  const float ALTITUDE_REAL_LOCAL = 422.0; 
  
  // Guarda a pressão ajustada para o nível do mar da sua região hoje
  static float pressaoNivelDoMarHoje = 101325.0f;
  static bool primeiraLeitura = true;

  // Solicita os dados brutos sequenciais (0xF7 a 0xFC: 3 bytes de pressão + 3 bytes de temperatura)
  Wire.beginTransmission(BME_ADDR);
  Wire.write(0xF7); 
  Wire.endTransmission(false);
  Wire.requestFrom((int)BME_ADDR, 6, (int)true);
  
  if (Wire.available() >= 6) {
    // 1. Lê a Pressão Bruta (20 bits)
    uint32_t p_msb  = Wire.read();
    uint32_t p_lsb  = Wire.read();
    uint32_t p_xlsb = Wire.read();
    int32_t adc_P = (p_msb << 12) | (p_lsb << 4) | (p_xlsb >> 4);
    
    // 2. Lê a Temperatura Bruta (20 bits)
    uint32_t t_msb  = Wire.read();
    uint32_t t_lsb  = Wire.read();
    uint32_t t_xlsb = Wire.read();
    int32_t adc_T = (t_msb << 12) | (t_lsb << 4) | (t_xlsb >> 4);

    // 3. Compensação de Temperatura (Gera o t_fine necessário para a pressão)
    int32_t var1_T = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2_T = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1_T + var2_T;

    // 4. Compensação de Pressão (Fórmula Oficial Bosch)
    int64_t var1_P = ((int64_t)t_fine) - 128000;
    int64_t var2_P = var1_P * var1_P * (int64_t)dig_P6;
    var2_P = var2_P + ((var1_P * (int64_t)dig_P5) << 17);
    var2_P = var2_P + (((int64_t)dig_P4) << 35);
    var1_P = ((var1_P * var1_P * (int64_t)dig_P3) >> 8) + ((var1_P * (int64_t)dig_P2) << 12);
    var1_P = (((((int64_t)1) << 47) + var1_P)) * ((int64_t)dig_P1) >> 33;
    
    if (var1_P != 0) { // Evita divisão por zero se o sensor falhar
      int64_t p_calc = 1048576 - adc_P;
      p_calc = (((p_calc << 31) - var2_P) * 3125) / var1_P;
      var1_P = (((int64_t)dig_P9) * (p_calc >> 13) * (p_calc >> 13)) >> 25;
      var2_P = (((int64_t)dig_P8) * p_calc) >> 19;
      p_calc = ((p_calc + var1_P + var2_P) >> 8) + (((int64_t)dig_P7) << 4);
      
      // Converte a pressão atual medida de Pascal para Hectopascal (hPa)
      pressao = (float)p_calc;
      
      // Na primeira leitura, calcula qual deveria ser a pressão ao nível do mar HOJE
      if (primeiraLeitura && pressao > 0.0) {
        // Inversão da fórmula barométrica para achar a referência corrigida pelo clima atual
        pressaoNivelDoMarHoje = pressao / pow(1.0 - (ALTITUDE_REAL_LOCAL / 44330.0), 5.2558);
        primeiraLeitura = false;
      }
      
      // 5. Cálculo de Altitude Real corrigida
      altitude = 44330.0 * (1.0 - pow(pressao / pressaoNivelDoMarHoje, 0.1903));
    }
  }
}


void dhtInit() {
  dht.begin(); // Inicializa a biblioteca do DHT
}

void dhtUpdate(float &temperatura, float &umidade) {
  // Lê a umidade e a temperatura em Celsius
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // Verifica se a leitura falhou antes de atualizar as variáveis
  // Se falhar (retornar NaN), mantém o último valor conhecido ou não mexe nelas
  if (!isnan(h) && !isnan(t)) {
    umidade = h;
    temperatura = t;
  }
}