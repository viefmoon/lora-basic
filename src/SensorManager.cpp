#include "SensorManager.h"
#include <Wire.h>
#include <SPI.h>
#include <cmath>  // Para fabs() y otras funciones matemáticas
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include <DallasTemperature.h>
#endif
#include "MAX31865.h"
#include "sensor_types.h"
#include "config.h"
#include <Preferences.h>
#include "config_manager.h"
#include "debug.h"
#include "sensor_constants.h"  // Para tiempos de estabilización
#include "utilities.h"

#ifdef DEVICE_TYPE_ANALOGIC
#include "ADS124S08.h"
#include "NtcManager.h"
#include "AdcUtilities.h"
#include "sensors/PHSensor.h"
#include "sensors/ConductivitySensor.h"
#include "sensors/HDS10Sensor.h"
extern ADS124S08 ADC;
#endif

// Inclusión de nuevos sensores
#include "sensors/RTDSensor.h"
#include "sensors/SHT30Sensor.h"
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include "sensors/DS18B20Sensor.h"
#endif

// -------------------------------------------------------------------------------------
// Métodos de la clase SensorManager
// -------------------------------------------------------------------------------------

void SensorManager::beginSensors() {
    // Inicializar pines de SPI (SS) y luego SPI
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Encender alimentación 3.3V
    powerManager.power3V3On();
    
#ifdef DEVICE_TYPE_ANALOGIC
    // Encender alimentación 2.5V solo para dispositivos ANALOGIC
    powerManager.power2V5On();
#endif

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

#ifdef DEVICE_TYPE_ANALOGIC
    ADC.begin();
    // Reset del ADC
    ADC.sendCommand(RESET_OPCODE_MASK);
    delay(10);

    // Asegurarse de que el ADC esté despierto
    ADC.sendCommand(WAKE_OPCODE_MASK);
    
    // Configurar ADC con referencia interna
    ADC.regWrite(REF_ADDR_MASK, ADS_REFINT_ON_ALWAYS | ADS_REFSEL_INT);
    
    // Deshabilitar PGA (bypass)
    ADC.regWrite(PGA_ADDR_MASK, ADS_PGA_BYPASS); // PGA_EN = 0, ganancia ignorada
    
    // Ajustar velocidad de muestreo
    ADC.regWrite(DATARATE_ADDR_MASK, ADS_DR_4000);
    
    // Iniciar conversión continua
    ADC.reStart();
#endif
}

void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA conectado directamente
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT); // ss de p100
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);

#ifdef DEVICE_TYPE_ANALOGIC
    ioExpander.pinMode(ADS124S08_CS_PIN, OUTPUT); // ss del adc
    ioExpander.digitalWrite(ADS124S08_CS_PIN, HIGH);
#endif
}

SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.sensorId[sizeof(reading.sensorId) - 1] = '\0';
    reading.type = cfg.type;
    reading.value = NAN;

    readSensorValue(cfg, reading);

    return reading;
}

/**
 * @brief Lógica principal para leer el valor de cada sensor normal (no Modbus) según su tipo.
 */
float SensorManager::readSensorValue(const SensorConfig &cfg, SensorReading &reading) {
    switch (cfg.type) {
        case N100K:
#ifdef DEVICE_TYPE_ANALOGIC
            // Usar NtcManager para obtener la temperatura
            reading.value = NtcManager::readNtc100kTemperature(cfg.configKey);
#else
            reading.value = NAN;
#endif
            break;

        case N10K:
#ifdef DEVICE_TYPE_ANALOGIC
            // Usar NtcManager para obtener la temperatura del NTC de 10k
            reading.value = NtcManager::readNtc10kTemperature();
#else
            reading.value = NAN;
#endif
            break;
            
        case HDS10:
#ifdef DEVICE_TYPE_ANALOGIC
            // Leer sensor HDS10 y obtener el porcentaje de humedad
            reading.value = HDS10Sensor::read();
#else
            reading.value = NAN;
#endif
            break;
            
        case PH:
#ifdef DEVICE_TYPE_ANALOGIC
            // Leer sensor de pH y obtener valor
            reading.value = PHSensor::read();
#else
            reading.value = NAN;
#endif
            break;

        case COND:
#ifdef DEVICE_TYPE_ANALOGIC
            // Leer sensor de conductividad y obtener valor en ppm
            reading.value = ConductivitySensor::read();
#else
            reading.value = NAN;
#endif
            break;
        
        case SOILH:
            reading.value = NAN; 
            break;

        case RTD:
            reading.value = RTDSensor::read();
            break;

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
        case DS18B20:
            reading.value = DS18B20Sensor::read();
            break;
#endif

        case SHT30:
        {
            float tmp = 0.0f, hum = 0.0f;
            SHT30Sensor::read(tmp, hum);
            reading.subValues.clear();
            {
                SubValue sT; strncpy(sT.key, "T", sizeof(sT.key)); sT.value = tmp;
                reading.subValues.push_back(sT);
            }
            {
                SubValue sH; strncpy(sH.key, "H", sizeof(sH.key)); sH.value = hum;
                reading.subValues.push_back(sH);
            }
            // Asignar el valor principal como NAN si alguno de los valores falló
            reading.value = (isnan(tmp) || isnan(hum)) ? NAN : tmp;
        }
        break;

        default:
            reading.value = NAN;
            break;
    }
    return reading.value;
}

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
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
#endif

void SensorManager::getAllSensorReadings(std::vector<SensorReading>& normalReadings
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
                                        , std::vector<ModbusSensorReading>& modbusReadings
#endif
                                        ) {
    // Obtener configuraciones de sensores habilitados
    auto enabledNormalSensors = ConfigManager::getEnabledSensorConfigs();
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    auto enabledModbusSensors = ConfigManager::getEnabledModbusSensorConfigs();
#endif
    
    // Reservar espacio para los vectores
    normalReadings.reserve(enabledNormalSensors.size());
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    modbusReadings.reserve(enabledModbusSensors.size());
#endif
    
    // Leer sensores normales
    for (const auto &sensor : enabledNormalSensors) {
        normalReadings.push_back(getSensorReading(sensor));
    }
    
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
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
                    sensorStabilizationTime = 500; // Tiempo predeterminado si no se especifica
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
#endif
}

#ifdef DEVICE_TYPE_ANALOGIC
// Las funciones de sensores se han movido a sus propias clases
#endif
