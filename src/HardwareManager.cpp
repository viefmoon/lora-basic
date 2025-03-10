/*******************************************************************************************
 * Archivo: src/HardwareManager.cpp
 * Descripción: Implementación de la gestión de hardware del sistema.
 *******************************************************************************************/

#include "HardwareManager.h"
#include "debug.h"

bool HardwareManager::initHardware(PCA9555& ioExpander, PowerManager& powerManager, SensirionI2cSht3x& sht30Sensor) {
    // Inicializar I2C con pines definidos
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Inicializar SHT30 para reset
    sht30Sensor.begin(Wire, SHT30_I2C_ADDR_44);
    sht30Sensor.stopMeasurement();
    delay(1);
    sht30Sensor.softReset();
    delay(100);
    
    // Inicializar PCA9555 para expansión de I/O
    if (!ioExpander.begin()) {
        DEBUG_PRINTLN("Error al inicializar PCA9555");
        return false;
    }
    
    // Inicializar PowerManager para control de energía
    if (!powerManager.begin()) {
        DEBUG_PRINTLN("Error al inicializar PowerManager");
        return false;
    }
    
    return true;
}

void HardwareManager::initSPI(SPIClass& spi) {
    // Inicializar SPI para comunicación con periféricos
    spi.begin();
}

void HardwareManager::prepareHardwareForSleep(PCA9555& ioExpander) {
    // Apagar buses y periféricos
    Wire.end();
    
    // Poner el PCA9555 en modo sleep
    ioExpander.sleep();
}
