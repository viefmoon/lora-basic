#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "sensor_types.h"
#include "RTCManager.h"
#include "clsPCA9555.h"
#include "PowerManager.h"
#include "MAX31865.h"
#include <OneWire.h>
#include <DallasTemperature.h>
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
extern OneWire oneWire;
extern DallasTemperature dallasTemp;
extern SensirionI2cSht3x sht30Sensor;

/**
 * @brief Clase que maneja la inicialización y lecturas de todos los sensores.
 */
class SensorManager {
  public:
    /**
     * @brief Inicializa los pines de SPI y los periféricos asociados a sensores (ADC, RTD, etc.).
     *        Debe llamarse durante o después de la inicialización del PCA9555.
     */
    static void beginSensors();

    /**
     * @brief Devuelve la lectura de un sensor según su configuración.
     */
    static SensorReading getSensorReading(const SensorConfig& cfg);

    /**
     * @brief Lee el voltaje de la batería a través del pin analógico del ESP32.
     * @return Voltaje de la batería en voltios, considerando el divisor de voltaje.
     */
    static float readBatteryVoltageADC();

  private:
    // Métodos de lectura para cada sensor
    static float readRtdSensor();
    static float readDallasSensor();
    static float readSht30Temperature();
    static float readSht30Humidity();

    // Método interno para determinar el valor de un sensor
    static float readSensorValue(const SensorConfig &cfg);

    static void initializeSPISSPins();

    /**
     * @brief Redondea un valor flotante a un máximo de 3 decimales.
     *        Si el valor tiene menos de 3 decimales, se conserva.
     *        Si tiene más, se redondea a 3.
     */
    static float roundTo3Decimals(float value);
};

#endif // SENSOR_MANAGER_H
