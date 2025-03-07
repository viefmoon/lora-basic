#include "PowerManager.h"

PowerManager::PowerManager(PCA9555& expander) : ioExpander(expander) {
}

bool PowerManager::begin() {
    // Configurar pines como salidas
    ioExpander.pinMode(POWER_3V3_PIN, OUTPUT);
    
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    ioExpander.pinMode(POWER_12V_PIN, OUTPUT);
#endif

#ifdef DEVICE_TYPE_ANALOGIC
    ioExpander.pinMode(POWER_2V5_PIN, OUTPUT);
#endif
    
    // Asegurar que todas las fuentes están apagadas al inicio
    allPowerOff();
    return true;
}

void PowerManager::power3V3On() {
    ioExpander.digitalWrite(POWER_3V3_PIN, HIGH);
    delay(POWER_STABILIZE_DELAY);
}

void PowerManager::power3V3Off() {
    ioExpander.digitalWrite(POWER_3V3_PIN, LOW);
}

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
void PowerManager::power12VOn() {
    ioExpander.digitalWrite(POWER_12V_PIN, HIGH);
    delay(POWER_STABILIZE_DELAY);
}

void PowerManager::power12VOff() {
    ioExpander.digitalWrite(POWER_12V_PIN, LOW);
}
#endif

#ifdef DEVICE_TYPE_ANALOGIC
void PowerManager::power2V5On() {
    ioExpander.digitalWrite(POWER_2V5_PIN, HIGH);
    delay(POWER_STABILIZE_DELAY);
}

void PowerManager::power2V5Off() {
    ioExpander.digitalWrite(POWER_2V5_PIN, LOW);
}
#endif

void PowerManager::allPowerOff() {
#ifdef DEVICE_TYPE_ANALOGIC
    power2V5Off();
#endif
    power3V3Off();
#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    power12VOff();
#endif
}