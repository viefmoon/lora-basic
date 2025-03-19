#ifndef BATTERY_SENSOR_H
#define BATTERY_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include "debug.h"

/**
 * @brief Clase para manejar la lectura del voltaje de la batería
 */
class BatterySensor {
public:
    /**
     * @brief Lee el voltaje de la batería
     * 
     * @return float Voltaje de la batería en voltios, o NAN si hay error
     */
    static float readVoltage();

private:
    /**
     * @brief Calcula el voltaje real de la batería a partir de la lectura del ADC
     * 
     * El circuito es un divisor de voltaje:
     * 
     * Batería (+) ---- R2 ---- | ---- R1 ---- GND
     *                          |
     *                          +--- ADC Pin
     * 
     * Donde:
     * - R2 es la resistencia de arriba (conectada a la batería)
     * - R1 es la resistencia de abajo (conectada a GND)
     * - La tensión medida por el ADC es V_adc = V_bat * R1 / (R1 + R2)
     * - Por lo tanto, V_bat = V_adc * (R1 + R2) / R1
     * 
     * @param adcVoltage Voltaje medido por el ADC
     * @return float Voltaje real de la batería
     */
    static float calculateBatteryVoltage(float adcVoltage);
};

#endif // BATTERY_SENSOR_H 