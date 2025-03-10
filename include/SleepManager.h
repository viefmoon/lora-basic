/*******************************************************************************************
 * Archivo: include/SleepManager.h
 * Descripción: Gestión del modo deep sleep para el ESP32.
 * Incluye funciones para configurar y entrar en modo de bajo consumo.
 *******************************************************************************************/

#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "PowerManager.h"
#include "clsPCA9555.h"
#include "config.h"
#include <RadioLib.h>
#include <SPI.h>
#include <Wire.h>

class SleepManager {
public:
    /**
     * @brief Configura y entra en modo deep sleep.
     * @param timeToSleep Tiempo en segundos para permanecer en deep sleep
     * @param powerManager Referencia al gestor de energía
     * @param ioExpander Referencia al expansor de I/O
     * @param radio Puntero al módulo de radio LoRa
     * @param node Referencia al nodo LoRaWAN para guardar sesión
     * @param LWsession Buffer para almacenar la sesión LoRaWAN
     * @param spi Referencia al objeto SPI
     */
    static void goToDeepSleep(uint32_t timeToSleep, 
                             PowerManager& powerManager,
                             PCA9555& ioExpander,
                             SX1262* radio,
                             LoRaWANNode& node,
                             uint8_t* LWsession,
                             SPIClass& spi);
    
    /**
     * @brief Configura los pines no utilizados en alta impedancia para reducir el consumo durante deep sleep.
     */
    static void configurePinsForDeepSleep();

    /**
     * @brief Libera el estado de retención (hold) de los pines que fueron configurados para deep sleep.
     * Esto permite que los pines puedan ser reconfigurados adecuadamente tras salir del deep sleep.
     */
    static void releaseHeldPins();
};

#endif // SLEEP_MANAGER_H
