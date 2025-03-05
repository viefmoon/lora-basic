#include "PowerManager.h"

PowerManager::PowerManager(PCA9555& expander) : ioExpander(expander) {
}

bool PowerManager::begin() {
    // Configurar pines como salidas
    ioExpander.pinMode(POWER_3V3_PIN, OUTPUT);
    ioExpander.pinMode(POWER_12V_PIN, OUTPUT);
    ioExpander.pinMode(POWER_2V5_PIN, OUTPUT);
    
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

void PowerManager::power12VOn() {
    ioExpander.digitalWrite(POWER_12V_PIN, HIGH);
    delay(POWER_STABILIZE_DELAY);
}

void PowerManager::power12VOff() {
    ioExpander.digitalWrite(POWER_12V_PIN, LOW);
}

void PowerManager::power2V5On() {
    ioExpander.digitalWrite(POWER_2V5_PIN, HIGH);
    delay(POWER_STABILIZE_DELAY);
}

void PowerManager::power2V5Off() {
    ioExpander.digitalWrite(POWER_2V5_PIN, LOW);
}

void PowerManager::allPowerOff() {
    power2V5Off();
    power3V3Off();
    power12VOff();
} 