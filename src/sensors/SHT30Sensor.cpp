#include "sensors/SHT30Sensor.h"

/**
 * @brief Lee temperatura y humedad del sensor SHT30
 * 
 * @param outTemp Variable donde se almacenará la temperatura en °C
 * @param outHum Variable donde se almacenará la humedad relativa en %
 */
void SHT30Sensor::read(float &outTemp, float &outHum) {
    float temperature = 0.0f;
    float humidity = 0.0f;

    int16_t error = sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, temperature, humidity);
    delay(20);
    if (error != NO_ERROR) {
        outTemp = NAN;
        outHum = NAN;
        return;
    }
    outTemp = temperature;
    outHum = humidity;
} 