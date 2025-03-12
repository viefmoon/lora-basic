/*******************************************************************************************
 * Archivo: include/sensor_constants.h
 * Descripción: Constantes y definiciones para sensores
 *******************************************************************************************/

#ifndef SENSOR_CONSTANTS_H
#define SENSOR_CONSTANTS_H

/**
 * @brief Claves para sensores específicos
 */
// Claves para el sensor ambiental 4 en 1 (ENV4)
#define ENV4_KEY_HUMIDITY "H"       // Clave para Humedad en sensor ENV4
#define ENV4_KEY_TEMPERATURE "T"    // Clave para Temperatura en sensor ENV4
#define ENV4_KEY_PRESSURE "P"       // Clave para Presión Atmosférica en sensor ENV4
#define ENV4_KEY_ILLUMINATION "Lux" // Clave para Iluminación en sensor ENV4

/**
 * @brief Tiempos de estabilización para sensores (en ms)
 */
// Para sensores Modbus
#define SENSOR_MODBUS_ENV4_STABILIZATION_TIME 5000   // Tiempo de estabilización para sensor ENV4
// Añadir aquí otros tiempos de estabilización

#endif // SENSOR_CONSTANTS_H
