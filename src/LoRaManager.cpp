/*******************************************************************************************
 * Archivo: src/LoRaManager.cpp
 * Descripción: Implementación de la gestión de comunicaciones LoRa y LoRaWAN.
 *              Se ha modificado la forma de serializar los valores float a 3 decimales
 *              usando snprintf("%.3f", ...) antes de asignarlos al JSON.
 *******************************************************************************************/

#include "LoRaManager.h"
#include <Preferences.h>
#include "debug.h"
#include <RadioLib.h>
#include <RTClib.h>
#include "utilities.h"  // Incluido para acceder a formatFloatTo3Decimals
#include "config.h"     // Incluido para acceder a MAX_PAYLOAD
#include "sensor_types.h"  // Incluido para acceder a ModbusSensorReading
#include "config_manager.h"
#include "sensors/BatterySensor.h"

// Inicialización de variables estáticas
LoRaWANNode* LoRaManager::node = nullptr;
SX1262* LoRaManager::radioModule = nullptr;

// Referencias externas
extern RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
extern RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin;
extern RTC_DS3231 rtc;

int16_t LoRaManager::begin(SX1262* radio, const LoRaWANBand_t* region, uint8_t subBand) {
    radioModule = radio;
    int16_t state = radioModule->begin();
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("Error iniciando radio: %d\n", state);
        return state;
    }
    
    // Crear el nodo LoRaWAN
    node = new LoRaWANNode(radio, region, subBand);
    return RADIOLIB_ERR_NONE;
}

int16_t LoRaManager::lwActivate(LoRaWANNode& node) {
    int16_t state = RADIOLIB_ERR_UNKNOWN;
    Preferences store;
    
    // Obtener configuración LoRa
    LoRaConfig loraConfig = ConfigManager::getLoRaConfig();
    
    // Convertir strings de EUIs a uint64_t
    uint64_t joinEUI = 0, devEUI = 0;
    if (!parseEUIString(loraConfig.joinEUI.c_str(), &joinEUI) ||
        !parseEUIString(loraConfig.devEUI.c_str(), &devEUI)) {
        DEBUG_PRINTLN("Error al parsear EUIs");
        return state;
    }
    
    // Parsear las claves
    uint8_t nwkKey[16], appKey[16];
    parseKeyString(loraConfig.nwkKey, nwkKey, 16);
    parseKeyString(loraConfig.appKey, appKey, 16);

    // Configurar la sesión OTAA
    node.beginOTAA(joinEUI, devEUI, nwkKey, appKey);

    store.begin("radiolib");

    // Intentar restaurar nonces si existen
    if (store.isKey("nonces")) {
        uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
        store.getBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
        state = node.setBufferNonces(buffer);
        
        if (state == RADIOLIB_ERR_NONE) {
            // Intentar restaurar sesión desde RTC
            state = node.setBufferSession(LWsession);
            
            if (state == RADIOLIB_ERR_NONE) {
                state = node.activateOTAA();
                
                if (state == RADIOLIB_LORAWAN_SESSION_RESTORED) {
                    store.end();
                    return state;
                }
            }
        }
    } else {
        DEBUG_PRINTLN("No hay nonces guardados - iniciando nuevo join");
    }

    // Si llegamos aquí, necesitamos hacer un nuevo join
    state = RADIOLIB_ERR_NETWORK_NOT_JOINED;
    while (state != RADIOLIB_LORAWAN_NEW_SESSION) {
        state = node.activateOTAA();

        // Guardar nonces en flash si el join fue exitoso
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
            uint8_t *persist = node.getBufferNonces();
            memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
            store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

            // Solicitar DeviceTime después de un join exitoso
            delay(1000); // Pausa para estabilización
            node.setDatarate(3);
            
            bool macCommandSuccess = node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
            if (macCommandSuccess) {
                // Enviar mensaje vacío
                uint8_t fPort = 1;
                uint8_t downlinkPayload[255];
                size_t downlinkSize = 0;
                
                int16_t rxState = node.sendReceive(nullptr, 0, fPort, downlinkPayload, &downlinkSize, true);
                if (rxState == RADIOLIB_ERR_NONE) {
                    // Obtener y procesar DeviceTime
                    uint32_t unixEpoch;
                    uint8_t fraction;
                    int16_t dtState = node.getMacDeviceTimeAns(&unixEpoch, &fraction, true);
                    if (dtState == RADIOLIB_ERR_NONE) {
                        DEBUG_PRINTF("DeviceTime recibido: epoch = %lu s, fraction = %u\n", unixEpoch, fraction);
                        // Convertir el tiempo unix a DateTime
                        DateTime serverTime(unixEpoch);
                        
                        // Ajustar el RTC con el tiempo del servidor
                        rtc.adjust(serverTime);
                        
                        // Verificar si se ajustó correctamente
                        if (abs((int32_t)rtc.now().unixtime() - (int32_t)unixEpoch) < 10) {
                            DEBUG_PRINTLN("RTC actualizado exitosamente con tiempo del servidor");
                        } else {
                            DEBUG_PRINTLN("Error al actualizar RTC con tiempo del servidor");
                        }
                    } else {
                        DEBUG_PRINTF("Error al obtener DeviceTime: %d\n", dtState);
                    }
                } else {
                    DEBUG_PRINTF("Error al recibir respuesta DeviceTime: %d\n", rxState);
                }
            } else {
                DEBUG_PRINTLN("Error al solicitar DeviceTime: comando no pudo ser encolado");
            }
            
            bootCountSinceUnsuccessfulJoin = 0;
            store.end();
            return RADIOLIB_LORAWAN_NEW_SESSION;
        } else {
            DEBUG_PRINTF("Join falló: %d\n", state);
            bootCountSinceUnsuccessfulJoin++;
            store.end();
            return state;
        }
    }

    store.end();
    return state;
}

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
size_t LoRaManager::createDelimitedPayload(
    const std::vector<SensorReading>& readings,
    const String& deviceId,
    const String& stationId,
    float battery,
    uint32_t timestamp,
    char* buffer,
    size_t bufferSize
) {
    // Inicializar buffer
    buffer[0] = '\0';
    size_t offset = 0;
    
    // Formatear batería con hasta 3 decimales
    char batteryStr[16];
    formatFloatTo3Decimals(battery, batteryStr, sizeof(batteryStr));
    
    // Formato: st|d|vt|ts|sensor1_id,sensor1_type,val1,val2,...|sensor2_id,...
    
    // Añadir encabezado: st|d|vt|ts
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "%s|%s|%s|%lu", 
                      stationId.c_str(), 
                      deviceId.c_str(), 
                      batteryStr, 
                      timestamp);
    
    // Añadir cada sensor
    for (const auto& reading : readings) {
        if (offset >= bufferSize - 1) break; // Evitar desbordamiento
        
        // Añadir separador de sensor
        buffer[offset++] = '|';
        buffer[offset] = '\0';
        
        // Añadir ID y tipo del sensor
        offset += snprintf(buffer + offset, bufferSize - offset, 
                          "%s,%d", 
                          reading.sensorId, 
                          reading.type);
        
        // Añadir valores
        if (reading.subValues.empty()) {
            // Un solo valor
            char valStr[16];
            formatFloatTo3Decimals(reading.value, valStr, sizeof(valStr));
            offset += snprintf(buffer + offset, bufferSize - offset, ",%s", valStr);
        } else {
            // Múltiples valores (subValues)
            for (const auto& sv : reading.subValues) {
                char valStr[16];
                formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
                offset += snprintf(buffer + offset, bufferSize - offset, ",%s", valStr);
            }
        }
    }
    
    return offset;
}

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
size_t LoRaManager::createDelimitedPayload(
    const std::vector<SensorReading>& normalReadings,
    const std::vector<ModbusSensorReading>& modbusReadings,
    const String& deviceId,
    const String& stationId,
    float battery,
    uint32_t timestamp,
    char* buffer,
    size_t bufferSize
) {
    // Inicializar buffer
    buffer[0] = '\0';
    size_t offset = 0;
    
    // Formatear batería con hasta 3 decimales
    char batteryStr[16];
    formatFloatTo3Decimals(battery, batteryStr, sizeof(batteryStr));
    
    // Formato: st|d|vt|ts|sensor1_id,sensor1_type,val1,val2,...|sensor2_id,...
    
    // Añadir encabezado: st|d|vt|ts
    offset += snprintf(buffer + offset, bufferSize - offset, 
                      "%s|%s|%s|%lu", 
                      stationId.c_str(), 
                      deviceId.c_str(), 
                      batteryStr, 
                      timestamp);
    
    // Añadir sensores normales
    for (const auto& reading : normalReadings) {
        if (offset >= bufferSize - 1) break; // Evitar desbordamiento
        
        // Añadir separador de sensor
        buffer[offset++] = '|';
        buffer[offset] = '\0';
        
        // Añadir ID y tipo del sensor
        offset += snprintf(buffer + offset, bufferSize - offset, 
                          "%s,%d", 
                          reading.sensorId, 
                          reading.type);
        
        // Añadir valores
        if (reading.subValues.empty()) {
            // Un solo valor
            char valStr[16];
            formatFloatTo3Decimals(reading.value, valStr, sizeof(valStr));
            offset += snprintf(buffer + offset, bufferSize - offset, ",%s", valStr);
        } else {
            // Múltiples valores (subValues)
            for (const auto& sv : reading.subValues) {
                char valStr[16];
                formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
                offset += snprintf(buffer + offset, bufferSize - offset, ",%s", valStr);
            }
        }
    }
    
    // Añadir sensores Modbus
    for (const auto& reading : modbusReadings) {
        if (offset >= bufferSize - 1) break; // Evitar desbordamiento
        
        // Añadir separador de sensor
        buffer[offset++] = '|';
        buffer[offset] = '\0';
        
        // Añadir ID y tipo del sensor
        offset += snprintf(buffer + offset, bufferSize - offset, 
                          "%s,%d", 
                          reading.sensorId, 
                          reading.type);
        
        // Añadir valores
        for (const auto& sv : reading.subValues) {
            char valStr[16];
            formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
            offset += snprintf(buffer + offset, bufferSize - offset, ",%s", valStr);
        }
    }
    
    return offset;
}
#endif

/**
 * @brief Envía el payload de sensores estándar usando formato delimitado.
 */
void LoRaManager::sendDelimitedPayload(const std::vector<SensorReading>& readings, 
                                     LoRaWANNode& node,
                                     const String& deviceId, 
                                     const String& stationId, 
                                     RTC_DS3231& rtc) 
{
    char payloadBuffer[MAX_LORA_PAYLOAD + 1];
    
    // Crear payload delimitado
    float battery = BatterySensor::readVoltage();
    uint32_t timestamp = rtc.now().unixtime();
    
    size_t payloadLength = createDelimitedPayload(
        readings, deviceId, stationId, battery, timestamp, 
        payloadBuffer, sizeof(payloadBuffer)
    );
    
    DEBUG_PRINTF("Enviando payload delimitado con tamaño %d bytes\n", payloadLength);
    DEBUG_PRINTLN(payloadBuffer);
    
    // Enviar
    uint8_t fPort = 1;
    uint8_t downlinkPayload[255];
    size_t downlinkSize = 0;
    
    int16_t state = node.sendReceive(
        (uint8_t*)payloadBuffer, 
        payloadLength, 
        fPort, 
        downlinkPayload, 
        &downlinkSize
    );
    
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("Transmisión exitosa!");
        if (downlinkSize > 0) {
            DEBUG_PRINTF("Recibidos %d bytes de downlink\n", downlinkSize);
        }
    } else {
        DEBUG_PRINTF("Error en transmisión: %d\n", state);
    }
}

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
/**
 * @brief Envía el payload de sensores estándar y Modbus usando formato delimitado.
 */
void LoRaManager::sendDelimitedPayload(const std::vector<SensorReading>& normalReadings, 
                                     const std::vector<ModbusSensorReading>& modbusReadings,
                                     LoRaWANNode& node,
                                     const String& deviceId, 
                                     const String& stationId, 
                                     RTC_DS3231& rtc)
{
    char payloadBuffer[MAX_LORA_PAYLOAD + 1];
    
    // Crear payload delimitado
    float battery = BatterySensor::readVoltage();
    uint32_t timestamp = rtc.now().unixtime();
    
    size_t payloadLength = createDelimitedPayload(
        normalReadings, modbusReadings, deviceId, stationId, battery, timestamp, 
        payloadBuffer, sizeof(payloadBuffer)
    );
    
    DEBUG_PRINTF("Enviando payload delimitado con tamaño %d bytes\n", payloadLength);
    DEBUG_PRINTLN(payloadBuffer);
    
    // Enviar
    uint8_t fPort = 1;
    uint8_t downlinkPayload[255];
    size_t downlinkSize = 0;
    
    // int16_t state = node.sendReceive(
    //     (uint8_t*)payloadBuffer, 
    //     payloadLength, 
    //     fPort, 
    //     downlinkPayload, 
    //     &downlinkSize
    // );

    int16_t state = node.uplink(
        (uint8_t*)payloadBuffer, 
        payloadLength, 
        fPort
    );
    
    if (state == RADIOLIB_ERR_NONE) {
        DEBUG_PRINTLN("Transmisión exitosa!");
        if (downlinkSize > 0) {
            DEBUG_PRINTF("Recibidos %d bytes de downlink\n", downlinkSize);
        }
    } else {
        DEBUG_PRINTF("Error en transmisión: %d\n", state);
    }
}
#endif

void LoRaManager::prepareForSleep(SX1262* radio) {
    if (radio) {
        radio->sleep(true);
    }
}

void LoRaManager::setDatarate(LoRaWANNode& node, uint8_t datarate) {
    node.setDatarate(datarate);
}
