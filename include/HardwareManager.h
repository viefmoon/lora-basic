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
#include <SensirionI2cSht3x.h>

class HardwareManager {
public:
    /**
     * @brief Inicializa el bus I2C, la expansión de I/O y el PowerManager.
     * @param ioExpander Referencia al expansor de I/O
     * @param powerManager Referencia al gestor de energía
     * @param sht30Sensor Referencia al sensor SHT30
     * @return true si la inicialización fue exitosa, false en caso contrario
     */
    static bool initHardware(PCA9555& ioExpander, PowerManager& powerManager, SensirionI2cSht3x& sht30Sensor);

    /**
     * @brief Configura los pines SPI para los diferentes periféricos
     * @param spi Referencia al objeto SPI
     */
    static void initSPI(SPIClass& spi);

    /**
     * @brief Prepara el hardware para entrar en modo deep sleep
     * @param ioExpander Referencia al expansor de I/O
     */
    static void prepareHardwareForSleep(PCA9555& ioExpander);
};

#endif // HARDWARE_MANAGER_H
