#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "sensor_types.h"
#include <RTClib.h>
#include "clsPCA9555.h"
#include "PowerManager.h"
#include "MAX31865.h"
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#include <SensirionI2cSht3x.h>
#include "ModbusSensorManager.h"

// Variables y objetos globales declarados en main.cpp
extern RTC_DS3231 rtc;
extern PCA9555 ioExpander;
extern PowerManager powerManager;
extern SPIClass spi;
#ifdef DEVICE_TYPE_ANALOGIC
extern SPISettings spiAdcSettings;
#endif
extern SPISettings spiRtdSettings;
extern MAX31865_RTD rtd;
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
extern OneWire oneWire;
extern DallasTemperature dallasTemp;
#endif
extern SensirionI2cSht3x sht30Sensor;

/**
 * @brief Clase que maneja la inicialización y lecturas de todos los sensores
 *        incluyendo sensores normales y Modbus.
 */
class SensorManager {
  public:
    // Inicializa pines, periféricos (ADC, RTD, etc.), OneWire, etc.
    static void beginSensors();

    // Devuelve la lectura (o lecturas) de un sensor NO-Modbus según su configuración.
    static SensorReading getSensorReading(const SensorConfig& cfg);
    
    // Devuelve la lectura de un sensor Modbus según su configuración
    static ModbusSensorReading getModbusSensorReading(const ModbusSensorConfig& cfg);
    
    // Obtiene todas las lecturas de sensores (normales y Modbus) habilitados
    static void getAllSensorReadings(std::vector<SensorReading>& normalReadings, 
                                     std::vector<ModbusSensorReading>& modbusReadings);

    // Lee el voltaje de la batería a través del pin analógico del ESP32.
    static float readBatteryVoltageADC();

  private:
    // Métodos de lectura internos
    static float readRtdSensor();
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
    static float readDallasSensor();
#endif
    // Lectura unificada de SHT30
    static void readSht30(float& outTemp, float& outHum);

    static void initializeSPISSPins();
    static float readSensorValue(const SensorConfig &cfg, SensorReading &reading);
};

#endif // SENSOR_MANAGER_H
