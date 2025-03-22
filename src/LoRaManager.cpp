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
            
            // Variable para controlar el número de intentos
            int rtcAttempts = 0;
            bool rtcUpdated = false;
            const int maxAttempts = 3; // Máximo número de intentos para actualizar RTC
            
            // Intentar solicitar y actualizar el tiempo del RTC hasta 3 veces
            while (!rtcUpdated && rtcAttempts < maxAttempts) {
                rtcAttempts++;
                
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
                                rtcUpdated = true; // Marca como actualizado para salir del bucle
                            } else {
                                DEBUG_PRINTLN("Error al actualizar RTC con tiempo del servidor");
                                // Si estamos en el último intento, continuamos de todas formas
                                if (rtcAttempts >= maxAttempts) {
                                    DEBUG_PRINTLN("Agotados los intentos de actualización de RTC");
                                } else {
                                    delay(1000); // Esperar un segundo antes del siguiente intento
                                }
                            }
                        } else {
                            DEBUG_PRINTF("Error al obtener DeviceTime: %d\n", dtState);
                            // Si estamos en el último intento, continuamos de todas formas
                            if (rtcAttempts >= maxAttempts) {
                                DEBUG_PRINTLN("Agotados los intentos de actualización de RTC");
                            } else {
                                delay(1000); // Esperar un segundo antes del siguiente intento
                            }
                        }
                    } else {
                        DEBUG_PRINTF("Error al recibir respuesta DeviceTime: %d\n", rxState);
                        // Si estamos en el último intento, continuamos de todas formas
                        if (rtcAttempts >= maxAttempts) {
                            DEBUG_PRINTLN("Agotados los intentos de actualización de RTC");
                        } else {
                            delay(1000); // Esperar un segundo antes del siguiente intento
                        }
                    }
                } else {
                    DEBUG_PRINTLN("Error al solicitar DeviceTime: comando no pudo ser encolado");
                    // Si estamos en el último intento, continuamos de todas formas
                    if (rtcAttempts >= maxAttempts) {
                        DEBUG_PRINTLN("Agotados los intentos de actualización de RTC");
                    } else {
                        delay(1000); // Esperar un segundo antes del siguiente intento
                    }
                }
            }
            
            // Si no se pudo actualizar el RTC después de los intentos máximos, retornar error
            if (!rtcUpdated) {
                DEBUG_PRINTLN("No se pudo actualizar el RTC después de los intentos máximos, entrando en deep sleep");
                bootCountSinceUnsuccessfulJoin = 0;
                store.end();
                return RADIOLIB_ERR_RTC_SYNC_FAILED; // Error personalizado para indicar fallo en sincronización RTC
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

    /*
    Lista de Data Rates (DR) para LoRaWAN US915

    - Cada DR define un conjunto específico de parámetros: 
        - SF (Spreading Factor): Cuanto mayor es, más alcance tiene pero menor velocidad de transmisión.
        - BW (Bandwidth): Mayor ancho de banda significa mayor velocidad pero menor alcance.
        - CR (Coding Rate): Relación de corrección de errores.
        - Payload Máximo: Tamaño máximo de datos en bytes que se pueden enviar en una sola transmisión.

    - NOTA: DR5, DR6 y DR7 no están definidos en US915.

    Índice | SF  | BW   | CR   | Payload Máximo | Comentario
    -------|-----|------|------|---------------|------------------------------------
    DR0    | SF10 | 125kHz | 4/5 | 19 bytes      | Mayor alcance, menor velocidad.
    DR1    | SF9  | 125kHz | 4/5 | 61 bytes      | Menor alcance, más velocidad que DR0.
    DR2    | SF8  | 125kHz | 4/5 | 133 bytes     | Velocidad media, menor alcance que DR1.
    DR3    | SF7  | 125kHz | 4/5 | 250 bytes     | Mayor velocidad en 125kHz, menor alcance.
    DR4    | SF8  | 500kHz | 4/5 | 250 bytes     | Mayor BW para mejorar la velocidad.
    DR5    | N/A  | N/A    | N/A | No definido   | No definido en US915.
    DR6    | N/A  | N/A    | N/A | No definido   | No definido en US915.
    DR7    | N/A  | N/A    | N/A | No definido   | No definido en US915.
    DR8    | SF12 | 500kHz | 4/5 | 41 bytes      | Mayor alcance en 500kHz, menor velocidad.
    DR9    | SF11 | 500kHz | 4/5 | 117 bytes     | Más velocidad que DR8, menos alcance.
    DR10   | SF10 | 500kHz | 4/5 | 230 bytes     | Más velocidad que DR9.
    DR11   | SF9  | 500kHz | 4/5 | 230 bytes     | Velocidad media en 500kHz.
    DR12   | SF8  | 500kHz | 4/5 | 230 bytes     | Más velocidad que DR11.
    DR13   | SF7  | 500kHz | 4/5 | 230 bytes     | Mayor velocidad en 500kHz, menor alcance.
    DR14   | N/A  | N/A    | N/A | No definido   | No definido en US915.

    - DR0 a DR3 se usan para **uplink** en los 64 canales de 125kHz.
    - DR4 se usa para **uplink** en los 8 canales de 500kHz.
    - DR8 a DR13 se usan para **downlink** en los 8 canales de 500kHz.
    - El payload máximo puede verse afectado por la opción **FOpt** en el MAC layer.
    */
    LoRaManager::setDatarate(node, 3);

    
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
