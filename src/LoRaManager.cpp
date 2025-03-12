/*******************************************************************************************
 * Archivo: src/LoRaManager.cpp
 * Descripción: Implementación de la gestión de comunicaciones LoRa y LoRaWAN.
 *              Se ha modificado la forma de serializar los valores float a 3 decimales
 *              usando snprintf("%.3f", ...) antes de asignarlos al JSON.
 *******************************************************************************************/

#include "LoRaManager.h"
#include <Preferences.h>
#include "debug.h"

// Inicialización de variables estáticas
LoRaWANNode* LoRaManager::node = nullptr;
SX1262* LoRaManager::radioModule = nullptr;

// Referencias externas
extern RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
extern RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin;
extern RTCManager rtcManager;

/**
 * @brief Función auxiliar para formatear valores flotantes a cadenas con 3 decimales.
 * @param value Valor flotante a formatear.
 * @param buffer Buffer donde se almacenará la cadena formateada.
 * @param bufferSize Tamaño del buffer.
 */
void LoRaManager::formatFloatTo3Decimals(float value, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%.3f", value);
}

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

    DEBUG_PRINTLN("Recuperando nonces y sesión LoRaWAN");
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
                DEBUG_PRINTLN("Sesión restaurada exitosamente - activando");
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
        DEBUG_PRINTLN("Iniciando join a la red LoRaWAN");
        state = node.activateOTAA();

        // Guardar nonces en flash si el join fue exitoso
        if (state == RADIOLIB_LORAWAN_NEW_SESSION) {
            DEBUG_PRINTLN("Join exitoso - Guardando nonces en flash");
            uint8_t buffer[RADIOLIB_LORAWAN_NONCES_BUF_SIZE];
            uint8_t *persist = node.getBufferNonces();
            memcpy(buffer, persist, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);
            store.putBytes("nonces", buffer, RADIOLIB_LORAWAN_NONCES_BUF_SIZE);

            // Solicitar DeviceTime después de un join exitoso
            delay(1000); // Pausa para estabilización
            node.setDatarate(3);
            
            DEBUG_PRINTLN("Solicitando DeviceTime...");
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
                        rtcManager.setTimeFromServer(unixEpoch, fraction);
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
 * @brief Envía el payload de sensores estándar, fragmentando para no exceder el tamaño.  
 *        **Se añade formateo a 3 decimales con `snprintf("%.3f", ...)`.**
 */
void LoRaManager::sendFragmentedPayload(const std::vector<SensorReading>& readings, 
                                        LoRaWANNode& node,
                                        const String& deviceId, 
                                        const String& stationId, 
                                        RTCManager& rtcManager) 
{
    const int MAX_PAYLOAD = 200; // Límite para el tamaño del JSON (aprox)
    size_t sensorIndex = 0;
    int fragmentNumber = 0;
    
    while (sensorIndex < readings.size()) {
        // Crear JSON base
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> payload;
        payload.clear();
        
        // Volumen de batería con 3 decimales
        float battery = SensorManager::readBatteryVoltageADC();
        char batteryStr[16];
        LoRaManager::formatFloatTo3Decimals(battery, batteryStr, sizeof(batteryStr));

        payload["st"] = stationId;
        payload["d"]  = deviceId;
        payload["vt"] = batteryStr;  // Se almacena como string formateado
        payload["ts"] = rtcManager.getEpochTime();

        JsonArray sensorArray = payload.createNestedArray("s");
        String fragmentStr;

        // Agregar lecturas de sensores, controlando que no exceda MAX_PAYLOAD
        while (sensorIndex < readings.size()) {
            JsonObject sensorObj = sensorArray.createNestedObject();
            sensorObj["id"] = readings[sensorIndex].sensorId;
            sensorObj["t"]  = readings[sensorIndex].type;

            if (!readings[sensorIndex].subValues.empty()) {
                // Procesar subvalores (por ejemplo SHT30 -> T, H)
                JsonObject multiVals = sensorObj.createNestedObject("v");
                for (auto &sv : readings[sensorIndex].subValues) {
                    char valStr[16];
                    LoRaManager::formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
                    multiVals[sv.key] = valStr;
                }
            } else {
                // Sensor con un solo valor
                char valStr[16];
                LoRaManager::formatFloatTo3Decimals(readings[sensorIndex].value, valStr, sizeof(valStr));
                sensorObj["v"] = valStr;
            }

            // Revisar si el JSON excede
            fragmentStr.clear();
            serializeJson(payload, fragmentStr);
            if (fragmentStr.length() > MAX_PAYLOAD) {
                // Quitar el último sensor si excede
                sensorArray.remove(sensorArray.size() - 1);
                break;
            }
            sensorIndex++;
        }
        
        // Serializar el fragmento
        fragmentStr.clear();
        serializeJson(payload, fragmentStr);
        DEBUG_PRINTF("Enviando fragmento %d con tamaño %d bytes\n", fragmentNumber, fragmentStr.length());
        DEBUG_PRINTLN(fragmentStr);
        
        // Enviar
        uint8_t payloadBytes[MAX_PAYLOAD];
        size_t payloadLength = fragmentStr.length();
        memcpy(payloadBytes, fragmentStr.c_str(), payloadLength);

        uint8_t fPort = 1;
        int16_t state = node.sendReceive(payloadBytes, payloadLength, fPort);
        if (state == RADIOLIB_ERR_NONE) {
            DEBUG_PRINTLN("Fragmento enviado correctamente");
        } else {
            DEBUG_PRINTF("Error al enviar fragmento: %d\n", state);
        }
        
        fragmentNumber++;
        delay(500); // Pequeña pausa entre fragmentos
    }
}

/**
 * @brief Envía payload de sensores normales y Modbus, también con formateo a 3 decimales.
 */
void LoRaManager::sendFragmentedPayload(const std::vector<SensorReading>& normalReadings, 
                                        const std::vector<ModbusSensorReading>& modbusReadings,
                                        LoRaWANNode& node,
                                        const String& deviceId, 
                                        const String& stationId, 
                                        RTCManager& rtcManager) 
{
    const int MAX_PAYLOAD = 200;
    size_t normalIndex = 0;
    size_t modbusIndex = 0;
    int fragmentNumber = 0;
    bool processedAllNormal = false;
    bool processedAllModbus = false;
    
    while (!processedAllNormal || !processedAllModbus) {
        // Cabecera JSON
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> payload;
        payload.clear();

        float battery = SensorManager::readBatteryVoltageADC();
        char batteryStr[16];
        LoRaManager::formatFloatTo3Decimals(battery, batteryStr, sizeof(batteryStr));

        payload["st"] = stationId;
        payload["d"]  = deviceId;
        payload["vt"] = batteryStr;  
        payload["ts"] = rtcManager.getEpochTime();
        JsonArray sensorArray = payload.createNestedArray("s");

        String fragmentStr;

        // Agregar lecturas "normales"
        while (!processedAllNormal && normalIndex < normalReadings.size()) {
            JsonObject sensorObj = sensorArray.createNestedObject();
            sensorObj["id"] = normalReadings[normalIndex].sensorId;
            sensorObj["t"]  = normalReadings[normalIndex].type;

            if (!normalReadings[normalIndex].subValues.empty()) {
                JsonObject multiVals = sensorObj.createNestedObject("v");
                for (auto &sv : normalReadings[normalIndex].subValues) {
                    char valStr[16];
                    LoRaManager::formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
                    multiVals[sv.key] = valStr;
                }
            } else {
                char valStr[16];
                LoRaManager::formatFloatTo3Decimals(normalReadings[normalIndex].value, valStr, sizeof(valStr));
                sensorObj["v"] = valStr;
            }

            // Checar tamaño
            fragmentStr.clear();
            serializeJson(payload, fragmentStr);
            if (fragmentStr.length() > MAX_PAYLOAD) {
                sensorArray.remove(sensorArray.size() - 1);
                break;
            }
            normalIndex++;
            if (normalIndex >= normalReadings.size()) {
                processedAllNormal = true;
            }
        }
        
        // Agregar lecturas Modbus
        if (!processedAllModbus && modbusIndex < modbusReadings.size()) {
            while (modbusIndex < modbusReadings.size()) {
                JsonObject sensorObj = sensorArray.createNestedObject();
                sensorObj["id"] = modbusReadings[modbusIndex].sensorId;
                // offset 100 para distinguir tipos Modbus
                sensorObj["t"]  = modbusReadings[modbusIndex].type + 100;

                JsonObject multiVals = sensorObj.createNestedObject("v");
                for (auto &sv : modbusReadings[modbusIndex].subValues) {
                    char valStr[16];
                    LoRaManager::formatFloatTo3Decimals(sv.value, valStr, sizeof(valStr));
                    multiVals[sv.key] = valStr;
                }

                // Checar tamaño
                fragmentStr.clear();
                serializeJson(payload, fragmentStr);
                if (fragmentStr.length() > MAX_PAYLOAD) {
                    sensorArray.remove(sensorArray.size() - 1);
                    break;
                }
                modbusIndex++;
                if (modbusIndex >= modbusReadings.size()) {
                    processedAllModbus = true;
                    break;
                }
            }
        }

        // Serializar
        fragmentStr.clear();
        serializeJson(payload, fragmentStr);
        DEBUG_PRINTF("Enviando fragmento %d con tamaño %d bytes\n", fragmentNumber, fragmentStr.length());
        DEBUG_PRINTLN(fragmentStr);

        uint8_t payloadBytes[MAX_PAYLOAD];
        size_t payloadLength = fragmentStr.length();
        memcpy(payloadBytes, fragmentStr.c_str(), payloadLength);

        uint8_t fPort = 1;
        int16_t state = node.sendReceive(payloadBytes, payloadLength, fPort);
        if (state == RADIOLIB_ERR_NONE) {
            DEBUG_PRINTLN("Fragmento enviado correctamente");
        } else {
            DEBUG_PRINTF("Error al enviar fragmento: %d\n", state);
        }

        fragmentNumber++;
        delay(500);
    }
}

void LoRaManager::prepareForSleep(SX1262* radio) {
    if (radio) {
        radio->sleep(true);
    }
}

void LoRaManager::setDatarate(LoRaWANNode& node, uint8_t datarate) {
    node.setDatarate(datarate);
}
