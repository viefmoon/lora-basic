/*******************************************************************************************
 * Archivo: include/LoRaManager.h
 * Descripción: Gestión de comunicaciones LoRa y LoRaWAN para el sistema de sensores.
 * Incluye funciones para inicialización, activación y envío de datos.
 *******************************************************************************************/

#ifndef LORA_MANAGER_H
#define LORA_MANAGER_H

#include <Arduino.h>
#include <RadioLib.h>
#include <vector>
#include <ArduinoJson.h>
#include "config_manager.h"
#include "utilities.h"
#include "sensor_types.h"
#include "RTCManager.h"
#include "SensorManager.h"

class LoRaManager {
public:
    /**
     * @brief Inicializa el módulo LoRa con la configuración especificada
     * @param radio Puntero al módulo de radio SX1262
     * @param region Región LoRaWAN a utilizar
     * @param subBand Sub-banda para la región (relevante para US915)
     * @return Estado de la inicialización
     */
    static int16_t begin(SX1262* radio, const LoRaWANBand_t* region, uint8_t subBand);

    /**
     * @brief Activa el nodo LoRaWAN restaurando la sesión o realizando un nuevo join
     * @param node Referencia al nodo LoRaWAN
     * @return Estado de la activación
     */
    static int16_t lwActivate(LoRaWANNode& node);

    /**
     * @brief Envía el payload de sensores fragmentado para no superar el tamaño máximo permitido.
     * @param readings Vector con todas las lecturas de sensores.
     * @param node Referencia al nodo LoRaWAN
     * @param deviceId ID del dispositivo
     * @param stationId ID de la estación
     * @param rtcManager Referencia al gestor RTC para obtener timestamp
     */
    static void sendFragmentedPayload(const std::vector<SensorReading>& readings, 
                                     LoRaWANNode& node,
                                     const String& deviceId, 
                                     const String& stationId, 
                                     RTCManager& rtcManager);

    /**
     * @brief Prepara el módulo LoRa para entrar en modo sleep
     * @param radio Puntero al módulo de radio SX1262
     */
    static void prepareForSleep(SX1262* radio);

    /**
     * @brief Configura el datarate para la transmisión
     * @param node Referencia al nodo LoRaWAN
     * @param datarate Valor del datarate a configurar
     */
    static void setDatarate(LoRaWANNode& node, uint8_t datarate);

private:
    static LoRaWANNode* node;
    static SX1262* radioModule;
};

#endif // LORA_MANAGER_H
