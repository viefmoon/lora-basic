#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include "clsPCA9555.h"
#include "config.h"

class PowerManager {
private:
    PCA9555& ioExpander;

public:
    PowerManager(PCA9555& expander);
    bool begin();

    // Método común para todos los dispositivos
    void power3V3On();
    void power3V3Off();
    
    // Métodos disponibles solo para dispositivos ANALOGIC y MODBUS
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    void power12VOn();
    void power12VOff();
#endif

    // Método disponible solo para dispositivo ANALOGIC
#ifdef DEVICE_TYPE_ANALOGIC
    void power2V5On();
    void power2V5Off();
#endif

    void allPowerOff();
};

#endif 