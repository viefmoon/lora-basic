#ifndef SENSOR_TYPES_H
#define SENSOR_TYPES_H

#include <stdint.h>
#include <vector>
#include "config.h"
#include <map>

/************************************************************************
 * TIEMPOS DE ESTABILIZACIÓN PARA SENSORES MODBUS (en ms)
 ************************************************************************/
 
#define MODBUS_ENV4_STABILIZATION_TIME 5000   // Tiempo de estabilización para sensor ENV4 Modbus
// Añadir aquí otros tiempos de estabilización para sensores Modbus

/**
 * @brief Estructura para variables múltiples en un solo sensor.
 *        Por ejemplo, un sensor SHT30 que da Temperature y Humidity.
 */
struct SubValue {
    float value;     // El orden de los valores en el vector es importante
};

/************************************************************************
 * SECCIÓN PARA SENSORES ESTÁNDAR (NO MODBUS)
 ************************************************************************/

/**
 * @brief Enumeración de tipos de sensores disponibles.
 */
enum SensorType {
    // Sensores estándar (no-Modbus)
    N100K,    // NTC 100K
    N10K,     // NTC 10K
    HDS10,    // Condensation Humidity
    RTD,      // RTD
    DS18B20,  // DS18B20
    PH,       // pH
    COND,     // Conductividad
    CONDH,    // Humedad de Condensación
    SOILH,    // Soil Humidity
    TEMP_A,   // Temperatura ambiente
    HUM_A,    // Humedad ambiente
    PRESS_A,  // Presión atmosférica
    CO2,      // Dióxido de Carbono
    LIGHT,    // Luz Ambiental
    ROOTH,    // Humedad de Raíz
    LEAFH,    // Humedad de Hoja
    
    // Sensores múltiples (valor 100 en el mapa)
    SHT30 = 100,  // Sensor SHT30: [0]=Temperatura(°C), [1]=Humedad(%)
    
    // Sensores Modbus
    ENV4 = 101,   // Sensor ambiental 4 en 1: [0]=Humedad(%), [1]=Temperatura(°C), [2]=Presión(kPa), [3]=Iluminación(lux)
    // Aquí se pueden agregar más tipos de sensores Modbus
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

/**
 * @brief Estructura de configuración para sensores "normales" (no Modbus).
 */
struct SensorConfig {
    char configKey[20];
    char sensorId[20];
    SensorType type;
    bool enable;
};

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
/************************************************************************
 * SECCIÓN PARA SENSORES MODBUS
 ************************************************************************/

/**
 * @brief Estructura de configuración para sensores Modbus.
 */
struct ModbusSensorConfig {
    char sensorId[20];         // Identificador del sensor
    SensorType type;           // Tipo de sensor Modbus
    uint8_t address;           // Dirección Modbus del dispositivo
    bool enable;               // Si está habilitado o no
};

/**
 * @brief Estructura para almacenar la lectura completa de un sensor Modbus.
 */
struct ModbusSensorReading {
    char sensorId[20];         // Identificador del sensor
    SensorType type;           // Tipo de sensor Modbus
    std::vector<SubValue> subValues; // Subvalores reportados por el sensor
};
#endif // defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)

#endif // SENSOR_TYPES_H
