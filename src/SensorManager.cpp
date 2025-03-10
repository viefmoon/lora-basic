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

// =============== Implementaciones de los métodos de SensorManager ===============

void SensorManager::beginSensors() {
    // Inicializar pines de SPI (SS) y luego SPI
    initializeSPISSPins();
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Encender alimentación 3.3V
    powerManager.power3V3On();

    // Inicializar RTD y configurarlo
    rtd.begin();
    {
        bool vBias = true;
        bool autoConvert = true;
        bool oneShot = false;
        bool threeWire = false;
        uint8_t faultCycle = MAX31865_FAULT_DETECTION_NONE;
        bool faultClear = true;
        bool filter50Hz = true;
        uint16_t lowTh = 0x0000;
        uint16_t highTh = 0x7fff;
        rtd.configure(vBias, autoConvert, oneShot, threeWire, faultCycle,
                      faultClear, filter50Hz, lowTh, highTh);
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

    // Inicializar SHT30 (si lo usas en todo el sistema, lo dejas siempre)
    sht30Sensor.begin(Wire, SHT30_I2C_ADDR_44);
    sht30Sensor.stopMeasurement();
    delay(1);
    sht30Sensor.softReset();
    delay(100);
    
    // Realizar una lectura inicial descartable para estabilizar el sensor
    float dummyTemp = 0.0f;
    float dummyHum = 0.0f;
    sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, dummyTemp, dummyHum);
    delay(20); // Dar tiempo suficiente para completar la medición
}

void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA que está conectado directamente al ESP32
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor I2C
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT);
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);

    // Si tuvieras más SS para ADC u otros, se configuran aquí.
}

// -----------------------------------------------------------------------------
// Lectura de batería (difiere según el tipo de dispositivo)

#if defined(DEVICE_TYPE_ANALOGIC)
float SensorManager::readBatteryVoltageADC() {
    // Configurar la resolución del ADC (0-4095 para 12 bits)
    analogReadResolution(12);
    
    // Realizar una única lectura
    int reading = analogRead(BATTERY_ADC_PIN);
    
    // Convertir la lectura a voltaje (ESP32 ADC 0-3.3V)
    float voltage = (reading / 4095.0f) * 3.3f;
    
    // Aplicar el factor de conversión del divisor de voltaje (R1 y R2)
    float batteryVoltage = voltage * conversionFactor;
    
    // Redondear a 3 decimales si es necesario
    return roundTo3Decimals(batteryVoltage);
}
#elif defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_MODBUS)
float SensorManager::readBatteryVoltageADC() {
    // Configurar la resolución del ADC (0-4095 para 12 bits)
    analogReadResolution(12);
    
    // Realizar una única lectura
    int reading = analogRead(BATTERY_PIN);
    
    // Convertir la lectura a voltaje (ESP32 ADC 0-3.3V)
    float voltage = (reading / 4095.0f) * 3.3f;
    
    // Aplicar el factor de conversión del divisor de voltaje (R1 y R2)
    float batteryVoltage = voltage * conversionFactor;
    
    // Redondear a 3 decimales si es necesario
    return roundTo3Decimals(batteryVoltage);
}
#endif

// -----------------------------------------------------------------------------
// Lecturas de sensores

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

float SensorManager::readSht30Temperature() {
    float temperature = 0.0f;
    float humidity = 0.0f;
    
    int16_t error = sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false,
                                               temperature, humidity);
    delay(20);
    if (error != NO_ERROR) {
        return NAN;
    }
    return temperature;
}

float SensorManager::readSht30Humidity() {
    float temperature = 0.0f;
    float humidity = 0.0f;

    int16_t error = sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false,
                                               temperature, humidity);
    delay(20);
    if (error != NO_ERROR) {
        return NAN;
    }
    return humidity;
}

float SensorManager::readSensorValue(const SensorConfig &cfg) {
    switch (cfg.type) {
        case N100K:
        case N10K:
        case WNTC10K:
        case PH:
        case COND:
        case SOILH:
        case CONDH:
            // Aquí implementa la lectura real de estos sensores si la tuvieras
            return 0.0f;

        case RTD:
            return readRtdSensor();

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
        case DS18B20:
            return readDallasSensor();
#endif
            
        case S30_T:
            return readSht30Temperature();
            
        case S30_H:
            return readSht30Humidity();

        default:
            return 0.0f;
    }
}

SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.sensorId[sizeof(reading.sensorId) - 1] = '\0';  // Asegurar terminación null
    reading.type = cfg.type;
    
    // Obtenemos la lectura cruda...
    float rawValue = readSensorValue(cfg);

    // ...y ahora la redondeamos a máximo 3 decimales.
    reading.value = roundTo3Decimals(rawValue);
    
    return reading;
}

// -----------------------------------------------------------------------------
// Implementación de la función de redondeo
float SensorManager::roundTo3Decimals(float value) {
    // Multiplicamos por 1000, redondeamos y volvemos a dividir por 1000
    // para obtener un float con máximo 3 decimales.
    return roundf(value * 1000.0f) / 1000.0f;
}