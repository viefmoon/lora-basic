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
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
#include "ModbusSensorManager.h"
#ifdef DEVICE_TYPE_ANALOGIC
#include "sensors/NtcManager.h"
#endif
#endif

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

    // Inicializa los pines de selección SPI (SS)
    static void initializeSPISSPins();

    // Devuelve la lectura (o lecturas) de un sensor NO-Modbus según su configuración.
    static SensorReading getSensorReading(const SensorConfig& cfg);
    
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    // Devuelve la lectura de un sensor Modbus según su configuración
    static ModbusSensorReading getModbusSensorReading(const ModbusSensorConfig& cfg);
#endif
    
    // Obtiene todas las lecturas de sensores (normales y Modbus) habilitados
    static void getAllSensorReadings(std::vector<SensorReading>& normalReadings
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
                                    , std::vector<ModbusSensorReading>& modbusReadings
#endif
                                    );

#ifdef DEVICE_TYPE_ANALOGIC
    
    // Ya no son necesarios, se movieron a sus propias clases
    // static float readHDS10Sensor();
    // static float convertResistanceToHumidity(float resistance);
    // static float readConductivitySensor();
    // static float convertVoltageToConductivity(float voltage, float tempC);
    // static float readPHSensor();
    // static float convertVoltageToPH(float voltage, float tempC);
#endif

  private:
    // Métodos de lectura internos
    // Ya no son necesarios, se han movido a sus propias clases
    // static float readRtdSensor();
    // static float readDallasSensor();
    // static void readSht30(float& outTemp, float& outHum);

    static float readSensorValue(const SensorConfig &cfg, SensorReading &reading);
};

#endif // SENSOR_MANAGER_H
