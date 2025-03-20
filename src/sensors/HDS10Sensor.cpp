#include "sensors/HDS10Sensor.h"

#ifdef DEVICE_TYPE_ANALOGIC

#include <cmath>
#include "ADS124S08.h"
#include "AdcUtilities.h"

// Variables globales declaradas en main.cpp
extern ADS124S08 ADC;

/**
 * @brief Convierte la resistencia del sensor HDS10 a porcentaje de humedad usando interpolación logarítmica
 * @param sensorR Resistencia del sensor en ohms
 * @return Porcentaje de humedad relativa (50-100%)
 */
float HDS10Sensor::convertResistanceToHumidity(float sensorR) {
    // Tabla de valores aproximados de la curva "Average" del sensor
    // Valores en kΩ vs. %HR
    static const float Rvals[] = { 1.0f,   2.0f,   5.0f,   10.0f,  50.0f,  100.0f, 200.0f };
    static const float Hvals[] = { 50.0f, 60.0f, 70.0f,  80.0f, 90.0f, 95.0f, 100.0f };
    static const int   NPOINTS = sizeof(Rvals)/sizeof(Rvals[0]);
    
    // Pasar ohms a kΩ
    float Rk = sensorR * 1e-3f;
    
    // Si está por debajo de la primera resistencia, limitamos a Hvals[0]
    if (Rk <= Rvals[0]) {
        return Hvals[0];
    }
    
    // Si está por encima de la última resistencia, limitamos a Hvals[NPOINTS-1]
    if (Rk >= Rvals[NPOINTS-1]) {
        return Hvals[NPOINTS-1];
    }

    // Buscar en qué tramo cae Rk
    for (int i=0; i < NPOINTS-1; i++) {
        float R1 = Rvals[i];
        float R2 = Rvals[i+1];
        if (Rk >= R1 && Rk <= R2) {
            // Interpolación logarítmica entre R1 y R2
            float logR   = log10(Rk);
            float logR1  = log10(R1);
            float logR2  = log10(R2);

            float HR1 = Hvals[i];
            float HR2 = Hvals[i+1];

            float humidity = HR1 + (HR2 - HR1) * ((logR - logR1) / (logR2 - logR1));
            return humidity;
        }
    }

    // Deberíamos haber retornado dentro del for, pero por seguridad:
    return Hvals[NPOINTS-1];
}

/**
 * @brief Lee el sensor HDS10 conectado al canal AIN5/AIN8 del ADC
 * 
 * @return float Porcentaje de humedad (0-100%) según calibración definida
 *               o NAN si ocurre un error o no es posible leer
 */
float HDS10Sensor::read() {
    // Asegurarse de que el ADC esté despierto
    ADC.sendCommand(WAKE_OPCODE_MASK);
    
    // Configurar el multiplexor para leer AIN5 con referencia a AIN8
    uint8_t muxConfig = ADS_P_AIN5 | ADS_N_AIN8;
    
    // Usar AdcUtilities para leer el voltaje diferencial
    float voltage = AdcUtilities::measureAdcDifferential(muxConfig);
    
    // Verificar si el voltaje está en rango válido
    if (voltage <= 0.0f || voltage >= 2.5f) {
        return NAN; // Valor fuera de rango
    }
    
    // Calcular la resistencia del sensor basado en el divisor de voltaje correcto
    // El circuito es: 2.5V --- R1(220K) --- [Punto de medición] --- R2(220K) --- HDS10 --- GND
    // Y estamos midiendo la caída de voltaje en R2+HDS10
    
    float r1 = 220000.0f; // Primera resistencia de 220K
    float r2 = 220000.0f; // Segunda resistencia de 220K
    
    // Calculamos la corriente en el circuito (que es la misma en toda la serie)
    // I = (2.5V - voltage) / R1
    float current = (2.5f - voltage) / r1;
    
    // El voltaje medido es la caída en R2+HDS10
    // voltage = I * (R2 + HDS10)
    // Por lo tanto, HDS10 = voltage/I - R2
    float sensorR = (voltage / current) - r2;
    
    if (sensorR < 0.0f) {
        return NAN; // Valor no válido
    }
    
    // Usar la función de conversión basada en interpolación logarítmica
    float percentage = convertResistanceToHumidity(sensorR);
    
    return percentage;
}

#endif // DEVICE_TYPE_ANALOGIC 