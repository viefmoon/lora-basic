#ifndef ADC_UTILITIES_H
#define ADC_UTILITIES_H

#include <Arduino.h>

/**
 * @brief Clase para gestionar funciones de utilidad relacionadas con el ADC
 */
class AdcUtilities {
public:
    /**
     * @brief Mide el voltaje diferencial configurando el MUX del ADS124S08.
     * @param muxConfig Valor del registro INPMUX (p.ej. ADS_P_AIN1 | ADS_N_AIN0)
     * @return Voltaje diferencial medido (en V), o NAN en caso de error.
     */
    static float measureAdcDifferential(uint8_t muxConfig);
};

#endif // ADC_UTILITIES_H 