#ifndef CONDUCTIVITY_SENSOR_H
#define CONDUCTIVITY_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"
#include "config_manager.h"

#ifdef DEVICE_TYPE_ANALOGIC

/**
 * @brief Clase para manejar el sensor de conductividad
 */
class ConductivitySensor {
public:
    /**
     * @brief Lee el sensor de conductividad conectado al canal AIN6 del ADC
     * 
     * @return float Valor de conductividad/TDS en ppm, o NAN si hay error
     */
    static float read();

    /**
     * @brief Convierte el voltaje medido a conductividad/TDS en ppm
     * 
     * @param voltage Voltaje medido del sensor
     * @param tempC Temperatura actual del agua para compensación
     * @return float Valor de TDS en ppm (partes por millón)
     */
    static float convertVoltageToConductivity(float voltage, float tempC);
};

#endif // DEVICE_TYPE_ANALOGIC

#endif // CONDUCTIVITY_SENSOR_H 