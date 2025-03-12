#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <vector>
#include "sensor_constants.h"

/**
 * @brief Estructura para variables múltiples en un solo sensor.
 *        Por ejemplo, un sensor SHT30 que da Temperature (T) y Humidity (H).
 */
struct SubValue {
    char key[10];    // Nombre corto de la variable, por ej: "T", "H", etc.
    float value;
};

/************************************************************************
 * SECCIÓN PARA SENSORES ESTÁNDAR (NO MODBUS)
 ************************************************************************/

/**
 * @brief Enumeración de tipos de sensores disponibles (no-Modbus).
 */
enum SensorType {
    N100K,    // NTC 100K
    N10K,     // NTC 10K
    WNTC10K,  // Water NTC 10K
    RTD,      // RTD
    DS18B20,  // DS18B20
    PH,       // pH
    COND,     // Conductividad
    CONDH,    // Condensation Humidity
    SOILH,    // Soil Humidity
    SHT30     // Nuevo tipo unificado para SHT30
};

/**
 * @brief Estructura básica para almacenar una lectura de sensor.
 *        Para sensores con múltiples variables, se utilizará 'subValues'.
 */
struct SensorReading {
    char sensorId[20];         // Identificador del sensor (ej. "SHT30_1")
    SensorType type;           // Tipo de sensor (no-Modbus)
    float value;               // Valor único (si aplica)
    std::vector<SubValue> subValues; // Subvalores, si el sensor genera varias mediciones
};

/**
 * @brief Estructura de configuración para sensores "normales" (no Modbus).
 */
struct SensorConfig {
    char configKey[20];
    char sensorId[20];
    SensorType type;
    uint8_t channel;
    char tempSensorId[20]; 
    bool enable;
};

/************************************************************************
 * SECCIÓN PARA SENSORES MODBUS
 ************************************************************************/

/**
 * @brief Enumeración de tipos de sensores Modbus.
 */
enum ModbusSensorType {
    ENV4 = 0,   // Sensor ambiental 4 en 1
    // Aquí se pueden agregar más tipos de sensores Modbus
};

/**
 * @brief Estructura de configuración para sensores Modbus.
 */
struct ModbusSensorConfig {
    char sensorId[20];         // Identificador del sensor
    ModbusSensorType type;     // Tipo de sensor Modbus
    uint8_t address;           // Dirección Modbus del dispositivo
    bool enable;               // Si está habilitado o no
};

/**
 * @brief Estructura para almacenar la lectura completa de un sensor Modbus.
 */
struct ModbusSensorReading {
    char sensorId[20];         // Identificador del sensor
    ModbusSensorType type;     // Tipo de sensor Modbus
    std::vector<SubValue> subValues; // Subvalores reportados por el sensor
};

#endif // SENSOR_TYPES_H
