#ifndef HDS10_SENSOR_H
#define HDS10_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"

#ifdef DEVICE_TYPE_ANALOGIC

/**
 * @brief Clase para manejar el sensor de humedad HDS10
 */
class HDS10Sensor {
public:
    /**
     * @brief Lee el sensor HDS10 conectado al canal AIN5/AIN8 del ADC
     * 
     * @return float Porcentaje de humedad (0-100%) según calibración definida
     *               o NAN si ocurre un error o no es posible leer
     */
    static float read();

    /**
     * @brief Convierte la resistencia del sensor HDS10 a porcentaje de humedad
     * 
     * @param resistance Resistencia del sensor en ohms
     * @return float Porcentaje de humedad relativa (50-100%)
     */
    static float convertResistanceToHumidity(float resistance);
};

#endif // DEVICE_TYPE_ANALOGIC

#endif // HDS10_SENSOR_H 