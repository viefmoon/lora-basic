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
#include <RTClib.h>
#include "SensorManager.h"

// Código de error personalizado para fallo en sincronización RTC
#define RADIOLIB_ERR_RTC_SYNC_FAILED -5000

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
     * @brief Crea un payload optimizado con formato delimitado por | y , en lugar de JSON.
     * @param readings Vector con lecturas de sensores.
     * @param deviceId ID del dispositivo.
     * @param stationId ID de la estación.
     * @param battery Valor de la batería.
     * @param timestamp Timestamp del sistema.
     * @param buffer Buffer donde se almacenará el payload.
     * @param bufferSize Tamaño del buffer.
     * @return Tamaño del payload generado.
     */
    static size_t createDelimitedPayload(
        const std::vector<SensorReading>& readings,
        const String& deviceId,
        const String& stationId,
        float battery,
        uint32_t timestamp,
        char* buffer,
        size_t bufferSize
    );

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    /**
     * @brief Crea un payload optimizado con formato delimitado para sensores normales y Modbus.
     * @param normalReadings Vector con lecturas de sensores normales.
     * @param modbusReadings Vector con lecturas de sensores Modbus.
     * @param deviceId ID del dispositivo.
     * @param stationId ID de la estación.
     * @param battery Valor de la batería.
     * @param timestamp Timestamp del sistema.
     * @param buffer Buffer donde se almacenará el payload.
     * @param bufferSize Tamaño del buffer.
     * @return Tamaño del payload generado.
     */
    static size_t createDelimitedPayload(
        const std::vector<SensorReading>& normalReadings,
        const std::vector<ModbusSensorReading>& modbusReadings,
        const String& deviceId,
        const String& stationId,
        float battery,
        uint32_t timestamp,
        char* buffer,
        size_t bufferSize
    );
#endif

    /**
     * @brief Envía el payload de sensores estándar usando formato delimitado.
     * @param readings Vector con todas las lecturas de sensores.
     * @param node Referencia al nodo LoRaWAN
     * @param deviceId ID del dispositivo
     * @param stationId ID de la estación
     * @param rtc Referencia al RTC para obtener timestamp
     */
    static void sendDelimitedPayload(const std::vector<SensorReading>& readings, 
                                   LoRaWANNode& node,
                                   const String& deviceId, 
                                   const String& stationId, 
                                   RTC_DS3231& rtc);

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    /**
     * @brief Envía el payload de sensores estándar y Modbus usando formato delimitado.
     * @param normalReadings Vector con lecturas de sensores estándar
     * @param modbusReadings Vector con lecturas de sensores Modbus
     * @param node Referencia al nodo LoRaWAN
     * @param deviceId ID del dispositivo
     * @param stationId ID de la estación
     * @param rtc Referencia al RTC para obtener timestamp
     */
    static void sendDelimitedPayload(const std::vector<SensorReading>& normalReadings, 
                                   const std::vector<ModbusSensorReading>& modbusReadings,
                                   LoRaWANNode& node,
                                   const String& deviceId, 
                                   const String& stationId, 
                                   RTC_DS3231& rtc);
#endif

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
