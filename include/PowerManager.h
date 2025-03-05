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

    void power2V5On();
    void power2V5Off();
    void power3V3On();
    void power3V3Off();
    void power12VOn();
    void power12VOff();
    void allPowerOff();
};

#endif 