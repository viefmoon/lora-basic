#ifndef SHT30_SENSOR_H
#define SHT30_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "SHT31.h"

// Variable externa
extern SHT31 sht30Sensor;

/**
 * @brief Clase para manejar el sensor de temperatura y humedad SHT30
 */
class SHT30Sensor {
public:
    /**
     * @brief Lee temperatura y humedad del sensor SHT30
     * 
     * @param outTemp Variable donde se almacenará la temperatura en °C
     * @param outHum Variable donde se almacenará la humedad relativa en %
     */
    static void read(float &outTemp, float &outHum);
};

#endif // SHT30_SENSOR_H 