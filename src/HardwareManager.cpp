/*******************************************************************************************
 * Archivo: src/HardwareManager.cpp
 * Descripción: Implementación de la gestión de hardware del sistema.
 *******************************************************************************************/

#include "HardwareManager.h"
#include "debug.h"
// time execution < 10 ms
bool HardwareManager::initHardware(PCA9555& ioExpander, PowerManager& powerManager, SHT31& sht30Sensor, SPIClass& spi, const std::vector<SensorConfig>& enabledNormalSensors) {
    #ifdef DEVICE_TYPE_ANALOGIC || DEVICE_TYPE_BASIC
    // Configurar GPIO one wire con pull-up
    pinMode(ONE_WIRE_BUS, INPUT_PULLUP);
    #endif
    
    // Inicializar I2C con pines definidos
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Inicializar SPI con pines definidos
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Verificar si hay algún sensor SHT30
    bool sht30SensorEnabled = false;
    for (const auto& sensor : enabledNormalSensors) {
        if (sensor.type == SHT30 && sensor.enable) {
            sht30SensorEnabled = true;
            break;
        }
    }

    // Inicializar SHT30 solo si está habilitado en la configuración
    if (sht30SensorEnabled) {
        //Inicializar SHT30 para reset y dummy lectura
        sht30Sensor.begin();
        sht30Sensor.reset();
    }
    
    //Inicializar PCA9555 para expansión de I/O
    if (!ioExpander.begin()) {
        DEBUG_PRINTLN("Error al inicializar PCA9555");
        return false;
    }
    
    // Inicializar los pines de selección SPI (SS)
    initializeSPISSPins();
    
    // Inicializar PowerManager para control de energía
    powerManager.begin();
    
    return true;
}

void HardwareManager::initializeSPISSPins() {
    // Inicializar SS del LORA conectado directamente
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor
    extern PCA9555 ioExpander;
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT); // ss de p100
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);

#ifdef DEVICE_TYPE_ANALOGIC
    ioExpander.pinMode(ADS124S08_CS_PIN, OUTPUT); // ss del adc
    ioExpander.digitalWrite(ADS124S08_CS_PIN, HIGH);
#endif
}