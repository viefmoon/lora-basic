#ifndef RTD_SENSOR_H
#define RTD_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "MAX31865.h"

// Variable externa
extern MAX31865_RTD rtd;

/**
 * @brief Clase para manejar el sensor de temperatura RTD (PT100)
 */
class RTDSensor {
public:
    /**
     * @brief Lee la temperatura del sensor RTD (PT100)
     * 
     * @return float Temperatura en °C, o NAN si hay error
     */
    static float read();
};

#endif // RTD_SENSOR_H 