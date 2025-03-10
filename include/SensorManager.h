#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "sensor_types.h"
#include "RTCManager.h"
#include "clsPCA9555.h"
#include "PowerManager.h"
#include "MAX31865.h"
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#include <SensirionI2cSht3x.h>

// Variables y objetos globales declarados en main.cpp
extern RTCManager rtcManager;
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
 * @brief Clase que maneja la inicialización y lecturas de todos los sensores.
 */
class SensorManager {
  public:
    /**
     * @brief Inicializa pines de SPI, periféricos (ADC, RTD, etc.), OneWire, etc.
     */
    static void beginSensors();

    /**
     * @brief Devuelve la lectura (o lecturas) de un sensor según su configuración.
     *        - Si el sensor es de tipo "normal" (ej. RTD, DS18B20, etc.), retornará un valor simple en `value`.
     *        - Si el sensor maneja subvalores (ej. SHT30 con T y H), estos se almacenarán en `reading.subValues`.
     */
    static SensorReading getSensorReading(const SensorConfig& cfg);

    /**
     * @brief Lee el voltaje de la batería a través del pin analógico del ESP32.
     * @return Voltaje de la batería en voltios, considerando el divisor de voltaje.
     */
    static float readBatteryVoltageADC();

  private:
    // Métodos de lectura internos
    static float readRtdSensor();
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
    static float readDallasSensor();
#endif
    // Lectura unificada de SHT30
    static void readSht30(float& outTemp, float& outHum);

    /**
     * @brief Redondea un valor flotante a un máximo de 3 decimales.
     */
    static float roundTo3Decimals(float value);

    static void initializeSPISSPins();

    /**
     * @brief Determina el valor (o valores) del sensor según su tipo. 
     *        Para la mayoría, se retorna un solo valor float; 
     *        para SHT30, se llenan `subValues` de la estructura.
     */
    static float readSensorValue(const SensorConfig &cfg, SensorReading &reading);
};

#endif // SENSOR_MANAGER_H
