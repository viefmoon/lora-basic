#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <vector>

/**
 * @brief Estructura para variables múltiples en un solo sensor.
 *        Por ejemplo, un sensor SHT30 que da Temperature (T) y Humidity (H).
 */
struct SubValue {
    char key[10];    // Nombre corto de la variable, por ej: "T", "H", etc.
    float value;
};

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

/* =========================================================================
   NUEVA SECCIÓN PARA SENSORES MODBUS
   ========================================================================= */

/**
 * @brief Enumeración de tipos de sensores Modbus (si deseas varios tipos).
 *        Aquí solo se define un caso de ejemplo "ENV_SENSOR".
 */
enum ModbusSensorType {
    ENV_SENSOR = 0,   // Sensor ambiental del datasheet (T, H, ruido, PM2.5, etc.)
    // Agrega más tipos Modbus si lo requieres...
};

/**
 * @brief Estructura de configuración para sensores Modbus.
 *        - id: nombre identificador
 *        - type: tipo de sensor (ENV_SENSOR, etc.)
 *        - address: dirección Modbus (por defecto 1)
 *        - enable: si está activo o no
 */
struct ModbusSensorConfig {
    char sensorId[20];
    ModbusSensorType type;
    uint8_t address;
    bool enable;
};

/**
 * @brief Estructura para almacenar la lectura completa de un sensor Modbus.
 *        Al ser un sensor que reporta varias variables (T, H, ruido, PM, presión, etc.),
 *        se maneja un vector de SubValue similar a la estructura general.
 */
struct ModbusSensorReading {
    char sensorId[20];         
    ModbusSensorType type;     
    std::vector<SubValue> subValues; 
};

#endif // SENSOR_TYPES_H
