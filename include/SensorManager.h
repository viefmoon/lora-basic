#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "sensor_types.h"
#include "RTCManager.h"
#include "clsPCA9555.h"
#include "PowerManager.h"
#include "MAX31865.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// Necesitamos referenciar extern las variables y objetos globales
// que se declaran/definen en el main.ino.
// Esto permite que SensorManager.cpp pueda usarlas.

extern RTCManager rtcManager;
extern PCA9555 ioExpander;
extern PowerManager powerManager;
extern SPIClass spi;
extern SPISettings spiAdcSettings;
extern SPISettings spiRtdSettings;
extern MAX31865_RTD rtd;
extern OneWire oneWire;
extern DallasTemperature dallasTemp;
/**
 * @brief Clase estática que maneja la inicialización y lecturas de todos los sensores.
 */
class SensorManager {
  public:
    /**
     * @brief Inicializa los pines de SPI y los periféricos asociados a sensores (ADC, RTD, etc.).
     *        Debe llamarse durante o después de la inicialización del PCA9555.
     */
    static void beginSensors();

    /**
     * @brief Lee todos los sensores definidos en sensorConfigs y llena el array de lecturas.
     * @param readings Array de lectura donde se guardarán los datos.
     * @param numSensors Número de sensores a leer.
     */
    static void readAllSensors(SensorReading *readings, size_t numSensors);

    /**
     * @brief Construye un payload (en formato CSV, JSON, etc.) a partir de las lecturas de sensores.
     * @param readings Array de lecturas.
     * @param numSensors Número de sensores leídos.
     * @return String con el payload formateado.
     */
    static String buildPayload(SensorReading *readings, size_t numSensors);

    static SensorReading getSensorReading(const SensorConfig& cfg);

  private:
    static float readRtdSensor();
    static float readDallasSensor();
    static float readSensorValue(const SensorConfig &cfg);
    static void initializePreferences();
    static void initializeSPISSPins();
};

#endif // SENSOR_MANAGER_H
