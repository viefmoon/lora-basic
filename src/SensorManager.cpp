#include "SensorManager.h"
#include <Wire.h>
#include <SPI.h>
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include <DallasTemperature.h>
#endif
#include "MAX31865.h"
#include "RTCManager.h"
#include "sensor_types.h"
#include "config.h"
#include <Preferences.h>
#include "config_manager.h"

/*-------------------------------------------------------------------------------------------
   Inicialización de los sensores
-------------------------------------------------------------------------------------------*/
void SensorManager::beginSensors() {
    // Inicializar pines de SPI (SS) y luego SPI
    initializeSPISSPins();
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Encender alimentación 3.3V (depende del dispositivo)
    powerManager.power3V3On();

    // Inicializar RTD y configurarlo
    rtd.begin();
    {
        bool vBias = true;
        bool autoConvert = true;
        bool oneShot = false;
        bool threeWire = false;
        uint8_t faultCycle = 0; // MAX31865_FAULT_DETECTION_NONE
        bool faultClear = true;
        bool filter50Hz = true;
        uint16_t lowTh = 0x0000;
        uint16_t highTh = 0x7fff;
        rtd.configure(vBias, autoConvert, oneShot, threeWire, faultCycle, faultClear, filter50Hz, lowTh, highTh);
    }

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
    // Inicializar DS18B20
    dallasTemp.begin();
    dallasTemp.setResolution(12);
    // Lectura inicial (descartable)
    dallasTemp.requestTemperatures();
    delay(750);
    dallasTemp.getTempCByIndex(0);
#endif

    // Inicializar SHT30
    sht30Sensor.begin(Wire, SHT30_I2C_ADDR_44);
    sht30Sensor.stopMeasurement();
    delay(1);
    sht30Sensor.softReset();
    delay(100);

    // Lectura inicial descartable para estabilizar
    float dummyTemp = 0.0f, dummyHum = 0.0f;
    sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, dummyTemp, dummyHum);
    delay(20);
}

/*-------------------------------------------------------------------------------------------
   Helpers
-------------------------------------------------------------------------------------------*/
void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA que está conectado directamente al ESP32
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor I2C
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT);
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);

    // Otros SS se configuran si fueran necesarios (ADC, etc.)
}

float SensorManager::roundTo3Decimals(float value) {
    return roundf(value * 1000.0f) / 1000.0f;
}

/*-------------------------------------------------------------------------------------------
   Lectura de Batería
-------------------------------------------------------------------------------------------*/
#if defined(DEVICE_TYPE_ANALOGIC)
float SensorManager::readBatteryVoltageADC() {
    // Configurar la resolución del ADC (0-4095 para 12 bits)
    analogReadResolution(12);
    
    int reading = analogRead(BATTERY_ADC_PIN);
    float voltage = (reading / 4095.0f) * 3.3f;
    float batteryVoltage = voltage * conversionFactor;
    return roundTo3Decimals(batteryVoltage);
}
#elif defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_MODBUS)
float SensorManager::readBatteryVoltageADC() {
    analogReadResolution(12);
    
    int reading = analogRead(BATTERY_PIN);
    float voltage = (reading / 4095.0f) * 3.3f;
    float batteryVoltage = voltage * conversionFactor;
    return roundTo3Decimals(batteryVoltage);
}
#endif

/*-------------------------------------------------------------------------------------------
   Lecturas de sensores internos
-------------------------------------------------------------------------------------------*/
float SensorManager::readRtdSensor() {
    uint8_t status = rtd.read_all();
    if (status == 0) {
        return rtd.temperature(); // °C
    } else {
        return NAN;
    }
}

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
float SensorManager::readDallasSensor() {
    dallasTemp.requestTemperatures();
    float temp = dallasTemp.getTempCByIndex(0);
    if (temp == DEVICE_DISCONNECTED_C) {
        return NAN;
    }
    return temp;
}
#endif

/**
 * @brief Lectura interna unificada de SHT30.
 * @param outTemp temperatura medida.
 * @param outHum  humedad medida.
 */
void SensorManager::readSht30(float &outTemp, float &outHum) {
    float temperature = 0.0f;
    float humidity = 0.0f;

    int16_t error = sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, temperature, humidity);
    delay(20);
    if (error != NO_ERROR) {
        // Si falla la lectura, devolvemos NAN
        outTemp = NAN;
        outHum = NAN;
        return;
    }
    outTemp = temperature;
    outHum = humidity;
}

/*-------------------------------------------------------------------------------------------
   Método principal para obtener la lectura de un sensor, con subvalores si aplica.
-------------------------------------------------------------------------------------------*/
SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.sensorId[sizeof(reading.sensorId) - 1] = '\0';
    reading.type = cfg.type;
    reading.value = NAN;  // Por defecto NAN, a veces se usará subValues en su lugar

    // Llamamos a la lectura real. Este método se encargará de asignar reading.value o reading.subValues
    readSensorValue(cfg, reading);

    // Redondear si es un valor único
    if (!isnan(reading.value)) {
        reading.value = roundTo3Decimals(reading.value);
    }

    // Si hay subvalores, también se redondean:
    for (auto &sv : reading.subValues) {
        sv.value = roundTo3Decimals(sv.value);
    }

    return reading;
}

/*-------------------------------------------------------------------------------------------
   Determina el valor o valores de un sensor según su tipo
-------------------------------------------------------------------------------------------*/
float SensorManager::readSensorValue(const SensorConfig &cfg, SensorReading &reading) {
    switch (cfg.type) {
        case N100K:
        case N10K:
        case WNTC10K:
        case PH:
        case COND:
        case SOILH:
        case CONDH:
            // Aquí se podría implementar la lectura real de estos sensores
            reading.value = 0.0f; 
            break;

        case RTD:
            reading.value = readRtdSensor();
            break;

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
        case DS18B20:
            reading.value = readDallasSensor();
            break;
#endif

        // Se unifican T y H en un solo tipo SHT30
        case SHT30: {
            float tmp = 0.0f, hum = 0.0f;
            readSht30(tmp, hum);

            // En vez de usar reading.value, guardamos 2 subvalores
            reading.subValues.clear();
            {
                SubValue sT;
                strncpy(sT.key, "T", sizeof(sT.key));
                sT.value = tmp;
                reading.subValues.push_back(sT);
            }
            {
                SubValue sH;
                strncpy(sH.key, "H", sizeof(sH.key));
                sH.value = hum;
                reading.subValues.push_back(sH);
            }

            break;
        }

        default:
            reading.value = 0.0f;
            break;
    }
    return reading.value;
}
