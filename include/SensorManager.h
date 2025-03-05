#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "sensor_types.h"
#include "RTCManager.h"
#include "clsPCA9555.h"
#include "PowerManager.h"
#include "ADS131M08.h"
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
// extern ADS131M04 adc1;
// extern ADS131M04 adc2;
extern ADS131M08 adc;
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

    /**
     * @brief Actualiza las lecturas del ADC con timeout
     * @param timeout_ms Tiempo máximo de espera en milisegundos
     * @return true si la lectura fue exitosa, false si hubo timeout
     */
    static bool updateADCReadings(uint32_t timeout_ms = 1000);

    static float readBatteryVoltage();

  private:
    // Funciones internas para leer cada tipo de sensor
    static float readAnalogSensor(const SensorConfig &cfg);
    static float readRtdSensor();
    static float readDallasSensor();
    static float readSensorValue(const SensorConfig &cfg);
    static void saveNTCCalibration(bool is100K, double T1, double R1, double T2, double R2, double T3, double R3);
    static void saveConductivityCalibration(float calTemp, float coefComp, float V1, float T1, float V2, float T2, float V3, float T3);
    static void initializePreferences();
    static void initializeSPISSPins();
};

#endif // SENSOR_MANAGER_H
