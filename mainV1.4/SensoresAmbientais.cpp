#include "SensoresAmbientais.h"
#include <math.h>
#include "DebugLog.h"

// Construtor
SensoresAmbientais::SensoresAmbientais(int dhtPin, int dhtType, byte bmeAddress)
    : _dht(dhtPin, dhtType),
      _bmeAddress(bmeAddress),
      _refPressure(1013.25F) // pressão média ao nível do mar (hPa)
{
}

// Inicialização
bool SensoresAmbientais::init(bool initWire)
{
    if (initWire)
    {
        Wire.begin();
    }

    _dht.begin();
    DBG_DHT(DBG_INFO, "init OK");

    bool bmeOk = _bme.begin(_bmeAddress);

    if (!bmeOk)
    {
        DBG_BME(DBG_ERROR, "falha no init no ender 0x%02X", _bmeAddress);
        _refPressure = NAN;
        return false;
    }

    DBG_BME(DBG_INFO, "init OK no ender 0x%02X", _bmeAddress);
    return true;
}

// DHT
bool SensoresAmbientais::lerDHT(float &temperatura, float &umidade)
{
    temperatura = _dht.readTemperature();
    umidade = _dht.readHumidity();

    if (isnan(temperatura) || isnan(umidade))
    {
        DBG_DHT(DBG_WARN, "falha na leitura — NaN");
        temperatura = NAN;
        umidade = NAN;
        return false;
    }

    return true;
}

// BME280
bool SensoresAmbientais::lerBME(float &pressao, float &altitude)
{
    float pressaoPa = _bme.readPressure();

    if (isnan(pressaoPa) || pressaoPa <= 0)
    {
        DBG_BME(DBG_WARN, "falha na leitura — pressao invalida");
        pressao = NAN;
        altitude = NAN;
        return false;
    }

    // Converte de Pascal para hPa
    pressao = pressaoPa / 100.0F;

    // Altitude em metros
    altitude = _bme.readAltitude(_refPressure);

    return true;
}