/*******************************************************************************************
 * Archivo: include/modbus_sensor_constants.h
 * Descripción: Constantes y definiciones para sensores Modbus
 *******************************************************************************************/

#ifndef MODBUS_SENSOR_CONSTANTS_H
#define MODBUS_SENSOR_CONSTANTS_H

/**
 * @brief Claves para sensores Modbus específicos
 */
// Claves para el sensor ambiental Modbus 4 en 1 (ENV4)
#define MODBUS_ENV4_KEY_HUMIDITY "H"       // Clave para Humedad en sensor ENV4
#define MODBUS_ENV4_KEY_TEMPERATURE "T"    // Clave para Temperatura en sensor ENV4
#define MODBUS_ENV4_KEY_PRESSURE "P"       // Clave para Presión Atmosférica en sensor ENV4
#define MODBUS_ENV4_KEY_ILLUMINATION "Lux" // Clave para Iluminación en sensor ENV4

/**
 * @brief Tiempos de estabilización para sensores Modbus (en ms)
 */
#define MODBUS_ENV4_STABILIZATION_TIME 5000   // Tiempo de estabilización para sensor ENV4 Modbus
// Añadir aquí otros tiempos de estabilización para sensores Modbus

#endif // MODBUS_SENSOR_CONSTANTS_H
