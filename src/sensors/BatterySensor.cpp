#include "sensors/BatterySensor.h"

#ifdef DEVICE_TYPE_ANALOGIC
#include "ADS124S08.h"
#include "AdcUtilities.h"
extern ADS124S08 ADC;
#endif

/**
 * @brief Lee el voltaje de la batería
 * 
 * @return float Voltaje de la batería en voltios, o NAN si hay error
 */
float BatterySensor::readVoltage() {
#ifdef DEVICE_TYPE_ANALOGIC
    // En DEVICE_TYPE_ANALOGIC, usamos el ADC externo para mayor precisión
    // Configurar el multiplexor para leer AIN9 con referencia a AINCOM (tierra)
    uint8_t muxConfig = ADS_P_AIN9 | ADS_N_AINCOM;
    
    // Asegurarse de que el ADC esté despierto
    ADC.sendCommand(WAKE_OPCODE_MASK);
    
    // Leer voltaje diferencial entre AIN9 y COMMON
    float voltage = AdcUtilities::measureAdcDifferential(muxConfig);
#else
    // Configurar la resolución del ADC a 12 bits
    analogReadResolution(12);
    
    // En otros tipos de dispositivo, usar BATTERY_PIN
    int reading = analogRead(BATTERY_PIN);
    
    // Verificar lectura válida
    if (reading < 0) {
        return NAN;
    }

    // Convertir la lectura del ADC a voltaje (para ADC de 12 bits, la máxima lectura es 4095)
    float voltage = (reading / 4095.0f) * 3.3f;
#endif

    // Calcular el voltaje real de la batería
    float batteryVoltage = calculateBatteryVoltage(voltage);
    return batteryVoltage;
}

/**
 * @brief Calcula el voltaje real de la batería a partir de la lectura del ADC
 * 
 * En config.h, las constantes están definidas como:
 * const double R1 = 470000.0;  // Resistencia conectada a GND
 * const double R2 = 1500000.0; // Resistencia conectada a la batería
 * 
 * @param adcVoltage Voltaje medido por el ADC
 * @return float Voltaje real de la batería
 */
float BatterySensor::calculateBatteryVoltage(float adcVoltage) {
    // Usando las constantes definidas en config.h
    // El voltaje de la batería se calcula como:
    // V_bat = V_adc * (R1 + R2) / R1
    return adcVoltage * ((R1 + R2) / R1);
} 