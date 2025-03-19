#include "sensors/DS18B20Sensor.h"

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)

/**
 * @brief Lee la temperatura del sensor DS18B20
 * 
 * @return float Temperatura en °C, o NAN si hay error
 */
float DS18B20Sensor::read() {
    dallasTemp.requestTemperatures();
    float temp = dallasTemp.getTempCByIndex(0);
    if (temp == DEVICE_DISCONNECTED_C) {
        return NAN;
    }
    return temp;
}

#endif // defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC) 