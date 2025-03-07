/*******************************************************************************************
 * Archivo: src/LoRaManager.cpp
 * Descripción: Implementación de la gestión de comunicaciones LoRa y LoRaWAN.
 *******************************************************************************************/

#include "LoRaManager.h"
#include <Preferences.h>
#include "debug.h"

// Inicialización de variables estáticas
LoRaWANNode* LoRaManager::node = nullptr;
SX1262* LoRaManager::radioModule = nullptr;

// Referencias a variables externas definidas en main.cpp
extern RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
extern RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin;
extern RTCManager rtcManager;

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
            delay(1000); // Pequeña pausa para estabilización
            node.setDatarate(3);
            
            // Intentar obtener DeviceTime
            DEBUG_PRINTLN("Solicitando DeviceTime...");
            bool macCommandSuccess = node.sendMacCommandReq(RADIOLIB_LORAWAN_MAC_DEVICE_TIME);
            if (macCommandSuccess) {
                // Enviar mensaje vacío para recibir el DeviceTime
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
                        // Continuar aunque falle el DeviceTime
                    }
                } else {
                    DEBUG_PRINTF("Error al recibir respuesta: %d\n", rxState);
                }
            } else {
                DEBUG_PRINTLN("Error al solicitar DeviceTime: comando no pudo ser encolado");
            }
            
            // Retornar éxito incluso si falla el DeviceTime
            bootCountSinceUnsuccessfulJoin = 0;
            store.end();
            return RADIOLIB_LORAWAN_NEW_SESSION;
        } else {
            DEBUG_PRINTF("Join falló: %d\n", state);
            bootCountSinceUnsuccessfulJoin++;
            store.end();
            // Aquí no llamamos a goToDeepSleep() directamente, sino que dejamos que el llamador maneje el fallo
            return state;
        }
    }

    store.end();
    return state;
}

void LoRaManager::sendFragmentedPayload(const std::vector<SensorReading>& readings, 
                                       LoRaWANNode& node,
                                       const String& deviceId, 
                                       const String& stationId, 
                                       RTCManager& rtcManager) {
    // Obtener el tamaño máximo del payload permitido por la configuración LoRaWAN.
    const int MAX_PAYLOAD = 200; // DR4 max payload is 250 bytes
    size_t sensorIndex = 0;
    int fragmentNumber = 0;
    
    while (sensorIndex < readings.size()) {
        // Crear un nuevo payload con cabecera
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> payload;
        payload["st"] = stationId;
        payload["d"] = deviceId;
        payload["vt"] = SensorManager::readBatteryVoltageADC();
        payload["ts"] = rtcManager.getEpochTime();
        JsonArray sensorArray = payload.createNestedArray("s");
        
        String fragmentStr;
        // Agregar lecturas de sensores mientras no se exceda el tamaño máximo del payload
        while (sensorIndex < readings.size()) {
            // Agregar la lectura al arreglo del payload
            JsonObject sensorObj = sensorArray.createNestedObject();
            sensorObj["id"] = readings[sensorIndex].sensorId;
            sensorObj["t"] = readings[sensorIndex].type;
            sensorObj["v"] = readings[sensorIndex].value;
            
            // Serializar para verificar el tamaño
            fragmentStr = "";
            serializeJson(payload, fragmentStr);
            
            // Si se excede el límite, eliminar la última lectura y salir del ciclo
            if (fragmentStr.length() > MAX_PAYLOAD) {
                sensorArray.remove(sensorArray.size() - 1);
                break;
            }
            
            sensorIndex++;
        }
        
        // Volver a serializar el payload final para este fragmento
        fragmentStr = "";
        serializeJson(payload, fragmentStr);
        DEBUG_PRINTF("Enviando fragmento %d con tamaño %d bytes\n", fragmentNumber, fragmentStr.length());
        DEBUG_PRINTLN(fragmentStr);
        
        int16_t state = node.sendReceive((uint8_t*)fragmentStr.c_str(), fragmentStr.length(), 1, true);
        if (state == RADIOLIB_ERR_NONE) {
            DEBUG_PRINTF("Fragmento %d enviado correctamente\n", fragmentNumber);
        } else {
            DEBUG_PRINTF("Error enviando fragmento %d: %d\n", fragmentNumber, state);
        }
        
        fragmentNumber++;
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
