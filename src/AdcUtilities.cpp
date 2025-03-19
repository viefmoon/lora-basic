#include "AdcUtilities.h"
#include "config_manager.h"
#include "debug.h"

#ifdef DEVICE_TYPE_ANALOGIC
#include "ADS124S08.h"
extern ADS124S08 ADC;

float AdcUtilities::measureAdcDifferential(uint8_t muxConfig)
{
    // Configurar MUX
    ADC.regWrite(INPMUX_ADDR_MASK, muxConfig);
    
    // Iniciar lectura
    uint8_t dummy1 = 0, dummy2 = 0;
    int32_t rawData = ADC.dataRead(&dummy1, &dummy2, &dummy2);
    if (rawData & 0x00800000) {
        // Extender signo si el bit 23 está en 1
        rawData |= 0xFF000000;
    }

    // Convertir a voltaje asumiendo referencia interna de 2.5V
    // ADC de 24 bits => rango ±2^23
    float voltage = (float)rawData / 8388608.0f * 2.5f;
    return voltage;
}

#endif // DEVICE_TYPE_ANALOGIC 