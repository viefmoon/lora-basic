#include "sensors/SHT30Sensor.h"

/**
 * @brief Lee temperatura y humedad del sensor SHT30
 * 
 * @param outTemp Variable donde se almacenará la temperatura en °C
 * @param outHum Variable donde se almacenará la humedad relativa en %
 */
void SHT30Sensor::read(float &outTemp, float &outHum) {
    // Intentar hasta 10 veces obtener una lectura válida
    for (int i = 0; i < 15; i++) {
        if (sht30Sensor.read()) {
            float temp = sht30Sensor.getTemperature();
            float hum = sht30Sensor.getHumidity();
            
            // Verificar que los valores sean válidos (no cero y dentro de rangos razonables)
            if (temp != 0.0f && hum != 0.0f && temp > -40.0f && temp < 125.0f && hum > 0.0f && hum <= 100.0f) {
                outTemp = temp;
                outHum = hum;
                return; // Retornar inmediatamente con la primera lectura válida
            }
        }
        delay(1); // Pausa entre mediciones
    }

    // Si no se encontró ninguna lectura válida
    outTemp = NAN;
    outHum = NAN;
} 