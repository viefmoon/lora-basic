/*******************************************************************************************
 * Archivo: src/HardwareManager.cpp
 * Descripción: Implementación de la gestión de hardware del sistema.
 *******************************************************************************************/

#include "HardwareManager.h"
#include "debug.h"

// time execution < 10 ms
bool HardwareManager::initHardware(PCA9555& ioExpander, PowerManager& powerManager, SensirionI2cSht3x& sht30Sensor, SPIClass& spi) {
    #ifdef DEVICE_TYPE_ANALOGIC || DEVICE_TYPE_BASIC
    // Configurar GPIO one wire con pull-up
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    #endif
    
    // Inicializar I2C con pines definidos
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Inicializar SPI con pines definidos
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    //Inicializar SHT30 para reset y dummy lectura
    sht30Sensor.begin(Wire, SHT30_I2C_ADDR_44);
    sht30Sensor.stopMeasurement();
    sht30Sensor.softReset();

    // // time execution ≈ 13 ms
    // float dummyTemp = 0.0f, dummyHum = 0.0f;
    // sht30Sensor.measureSingleShot(REPEATABILITY_HIGH, false, dummyTemp, dummyHum);
    // ////////////////////////////////////////////////////////////////
    
    //Inicializar PCA9555 para expansión de I/O
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

void HardwareManager::prepareHardwareForSleep(PCA9555& ioExpander) {
    // Apagar buses y periféricos
    Wire.end();
    
    // Poner el PCA9555 en modo sleep
    ioExpander.sleep();
}
