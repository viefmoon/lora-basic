/*******************************************************************************************
 * Archivo: include/HardwareManager.h
 * Descripción: Gestión de inicialización y configuración del hardware del sistema.
 * Incluye funciones para inicialización de periféricos y control de energía.
 *******************************************************************************************/

#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"
#include "PowerManager.h"
#include "clsPCA9555.h"
#include "SHT31.h"
#include "sensor_types.h"
#include <vector>

class HardwareManager {
public:
    /**
     * @brief Inicializa el bus I2C, la expansión de I/O y el PowerManager.
     * @param ioExpander Referencia al expansor de I/O
     * @param powerManager Referencia al gestor de energía
     * @param sht30Sensor Referencia al sensor SHT30
     * @param spi Referencia a la interfaz SPI
     * @param enabledNormalSensors Vector con las configuraciones de sensores habilitados
     * @return true si la inicialización fue exitosa, false en caso contrario
     */
    static bool initHardware(PCA9555& ioExpander, PowerManager& powerManager, 
                           SHT31& sht30Sensor, SPIClass& spi,
                           const std::vector<SensorConfig>& enabledNormalSensors);

    /**
     * @brief Inicializa los pines de selección SPI (SS)
     */
    static void initializeSPISSPins();

};

#endif // HARDWARE_MANAGER_H
