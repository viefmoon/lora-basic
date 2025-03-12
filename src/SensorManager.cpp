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
#include "Debug.h"
#include "sensor_constants.h"  // Para tiempos de estabilización
#include "utilities.h"  // Para roundTo3Decimals

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
    // Lectura inicial
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
    
    float dummyTemp = 0.0f, dummyHum = 0.0f;
    sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, dummyTemp, dummyHum);
    delay(20);
}

void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA conectado directamente
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT);
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);
}

SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.sensorId[sizeof(reading.sensorId) - 1] = '\0';
    reading.type = cfg.type;
    reading.value = NAN;

    readSensorValue(cfg, reading);

    if (!isnan(reading.value)) {
        reading.value = ::roundTo3Decimals(reading.value);
    }
    for (auto &sv : reading.subValues) {
        sv.value = ::roundTo3Decimals(sv.value);
    }

    return reading;
}

float SensorManager::readBatteryVoltageADC() {
#if defined(DEVICE_TYPE_ANALOGIC)
    analogReadResolution(12);
    int reading = analogRead(BATTERY_ADC_PIN);
    if (reading < 0) {
        return NAN;
    }
    float voltage = (reading / 4095.0f) * 3.3f;
    float batteryVoltage = voltage * conversionFactor;
    return ::roundTo3Decimals(batteryVoltage);
#elif defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_MODBUS)
    analogReadResolution(12);
    int reading = analogRead(BATTERY_PIN);
    if (reading < 0) {
        return NAN;
    }
    float voltage = (reading / 4095.0f) * 3.3f;
    float batteryVoltage = voltage * conversionFactor;
    return ::roundTo3Decimals(batteryVoltage);
#endif
}

float SensorManager::readRtdSensor() {
    uint8_t status = rtd.read_all();
    if (status == 0) {
        return rtd.temperature();
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

void SensorManager::readSht30(float &outTemp, float &outHum) {
    float temperature = 0.0f;
    float humidity = 0.0f;

    int16_t error = sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, temperature, humidity);
    delay(20);
    if (error != NO_ERROR) {
        outTemp = NAN;
        outHum = NAN;
        return;
    }
    outTemp = temperature;
    outHum = humidity;
}

float SensorManager::roundTo3Decimals(float value) {
    return ::roundTo3Decimals(value);  // Usa la función global
}

float SensorManager::readSensorValue(const SensorConfig &cfg, SensorReading &reading) {
    switch (cfg.type) {
        case N100K:
        case N10K:
        case WNTC10K:
        case PH:
        case COND:
        case SOILH:
        case CONDH:
            // No implementado pero debe devolver NAN, no cero
            reading.value = NAN; 
            break;

        case RTD:
            reading.value = readRtdSensor();
            break;

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
        case DS18B20:
            reading.value = readDallasSensor();
            break;
#endif

        case SHT30: {
            float tmp = 0.0f, hum = 0.0f;
            readSht30(tmp, hum);
            reading.subValues.clear();
            {
                SubValue sT; strncpy(sT.key, "T", sizeof(sT.key)); sT.value = tmp;
                reading.subValues.push_back(sT);
            }
            {
                SubValue sH; strncpy(sH.key, "H", sizeof(sH.key)); sH.value = hum;
                reading.subValues.push_back(sH);
            }
            // Asignar el valor principal como NAN si alguno de los valores del sensor falló
            reading.value = (isnan(tmp) || isnan(hum)) ? NAN : tmp;
            break;
        }

        default:
            reading.value = NAN;
            break;
    }
    return reading.value;
}

ModbusSensorReading SensorManager::getModbusSensorReading(const ModbusSensorConfig& cfg) {
    ModbusSensorReading reading;
    
    // Copiar el ID del sensor
    strlcpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId));
    reading.type = cfg.type;
    
    // Leer sensor según su tipo
    switch (cfg.type) {
        case ENV4:
            reading = ModbusSensorManager::readEnvSensor(cfg);
            break;
        // Añadir casos para otros tipos de sensores Modbus
        default:
            DEBUG_PRINTLN("Tipo de sensor Modbus no soportado");
            break;
    }
    
    return reading;
}

void SensorManager::getAllSensorReadings(std::vector<SensorReading>& normalReadings, 
                                        std::vector<ModbusSensorReading>& modbusReadings) {
    // Obtener configuraciones de sensores habilitados
    auto enabledNormalSensors = ConfigManager::getEnabledSensorConfigs();
    auto enabledModbusSensors = ConfigManager::getEnabledModbusSensorConfigs();
    
    // Reservar espacio para los vectores
    normalReadings.reserve(enabledNormalSensors.size());
    modbusReadings.reserve(enabledModbusSensors.size());
    
    // Leer sensores normales
    for (const auto &sensor : enabledNormalSensors) {
        normalReadings.push_back(getSensorReading(sensor));
    }
    
    // Si hay sensores Modbus, inicializar comunicación, leerlos y finalizar
    if (!enabledModbusSensors.empty()) {
        // Determinar el tiempo máximo de estabilización necesario
        uint32_t maxStabilizationTime = 0;
        
        // Revisar cada sensor habilitado para encontrar el tiempo máximo
        for (const auto &sensor : enabledModbusSensors) {
            uint32_t sensorStabilizationTime = 0;
            
            // Obtener el tiempo de estabilización según el tipo de sensor
            switch (sensor.type) {
                case ENV4:
                    sensorStabilizationTime = SENSOR_MODBUS_ENV4_STABILIZATION_TIME;
                    break;
                // Añadir casos para otros tipos de sensores Modbus con sus respectivos tiempos
                default:
                    sensorStabilizationTime = 1000; // Tiempo predeterminado si no se especifica
                    break;
            }
            
            // Actualizar el tiempo máximo si este sensor necesita más tiempo
            if (sensorStabilizationTime > maxStabilizationTime) {
                maxStabilizationTime = sensorStabilizationTime;
            }
        }
        
        // Encender alimentación de 12V para sensores Modbus
        #if defined(DEVICE_TYPE_MODBUS) || defined(DEVICE_TYPE_ANALOGIC)
            powerManager.power12VOn();
            DEBUG_PRINTF("Esperando %u ms para estabilización de sensores Modbus\n", maxStabilizationTime);
            delay(maxStabilizationTime);
        #endif
        
        // Inicializar comunicación Modbus antes de comenzar las mediciones
        ModbusSensorManager::beginModbus();
        
        // Leer todos los sensores Modbus
        for (const auto &sensor : enabledModbusSensors) {
            modbusReadings.push_back(getModbusSensorReading(sensor));
        }
        
        // Finalizar comunicación Modbus después de completar todas las lecturas
        ModbusSensorManager::endModbus();
        
        // Apagar alimentación de 12V después de completar las lecturas
        #if defined(DEVICE_TYPE_MODBUS) || defined(DEVICE_TYPE_ANALOGIC)
            powerManager.power12VOff();
        #endif
    }
}
