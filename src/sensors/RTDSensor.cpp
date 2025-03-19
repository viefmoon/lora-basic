#include "sensors/RTDSensor.h"

/**
 * @brief Lee la temperatura del sensor RTD (PT100)
 * 
 * @return float Temperatura en °C, o NAN si hay error
 */
float RTDSensor::read() {
    uint8_t status = rtd.read_all();
    if (status == 0) {
        return rtd.temperature();
    } else {
        return NAN;
    }
} 