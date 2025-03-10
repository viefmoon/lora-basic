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
 * @brief Enumeración de tipos de sensores disponibles.
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
    SensorType type;           // Tipo de sensor
    float value;               // Valor único (si aplica)
    std::vector<SubValue> subValues; // Subvalores, si el sensor genera varias mediciones
};

struct SensorConfig {
    char configKey[20];
    char sensorId[20];
    SensorType type;
    uint8_t channel;
    char tempSensorId[20]; 
    bool enable;
};

#endif // SENSOR_TYPES_H
