#include "SensorManager.h"
#include <Wire.h>
#include <SPI.h>
#include <DallasTemperature.h>
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

    // Inicializar RTD Y configurarlo
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
        rtd.configure(vBias, autoConvert, oneShot, threeWire, faultCycle, faultClear, filter50Hz, lowTh, highTh);
    }

    // Inicializar DS18B20
    dallasTemp.begin();
    dallasTemp.setResolution(12);
    // Lectura inicial (descartable)
    dallasTemp.requestTemperatures();
    delay(750);
    dallasTemp.getTempCByIndex(0);
}

void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA que está conectado directamente al ESP32
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor I2C
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT);  
    ioExpander.pinMode(ADC_CS_PIN, OUTPUT);  

    // Establecer todos los SS del expansor en HIGH
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);
    ioExpander.digitalWrite(ADC_CS_PIN, HIGH);
}

// ========== Funciones de conversión ==========


float convertBattery(float voltage) {
    //divisor de voltaje
    const double R1 = 470000.0; //resistencias fija
    const double R2 = 1500000.0;
    const double conversionFactor = (R1 + R2) / R1;
    float batteryVoltage = (float)(voltage * conversionFactor);
    return batteryVoltage;
}

float SensorManager::readRtdSensor() {
    uint8_t status = rtd.read_all();
    if (status == 0) {
        return rtd.temperature(); // °C
    } else {
        return NAN; 
    }
}

float SensorManager::readDallasSensor() {
    dallasTemp.requestTemperatures();
    float temp = dallasTemp.getTempCByIndex(0);
    if (temp == DEVICE_DISCONNECTED_C) {
        return NAN;
    }
    return temp;
}

float SensorManager::readSensorValue(const SensorConfig &cfg) {
    switch(cfg.type) {
        case N100K:
        case N10K:
        case WNTC10K:
        case PH:
        case COND: 
        case SOILH:
        case CONDH:
            return readAnalogSensor(cfg);
        case RTD:
            return readRtdSensor();
        case DS18B20:
            return readDallasSensor();
        default:
            return 0.0;
    }
}

SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.type = cfg.type;

    // Realizar la lectura del sensor
    reading.value = readSensorValue(cfg);

    return reading;
}