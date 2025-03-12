/*******************************************************************************************
 * Archivo: src/BLE.cpp
 * Descripción: Implementación unificada de la gestión BLE para el ESP32.
 *******************************************************************************************/

#include "BLE.h"

// Implementación de los métodos de la clase ServerCallbacks
void BLEHandler::ServerCallbacks::onConnect(BLEServer* pServer) {
    DEBUG_PRINTLN(F("BLE Cliente conectado"));
}

void BLEHandler::ServerCallbacks::onDisconnect(BLEServer* pServer) {
    DEBUG_PRINTLN(F("BLE Cliente desconectado, reiniciando publicidad..."));
    pServer->getAdvertising()->start();
}

// Implementación de los métodos de BLEHandler
bool BLEHandler::checkConfigMode(PCA9555& ioExpander) {
    if (digitalRead(CONFIG_PIN) == LOW) {
        DEBUG_PRINTLN("Modo configuración activado");
        unsigned long startTime = millis();
        while (digitalRead(CONFIG_PIN) == LOW) {
            if (millis() - startTime >= CONFIG_TRIGGER_TIME) {
                // Obtener configuración LoRa para el nombre BLE
                LoRaConfig loraConfig = ConfigManager::getLoRaConfig();
                String bleName = BLE_DEVICE_PREFIX + String(loraConfig.devEUI);
                
                // Inicializar BLE
                BLEDevice::init(bleName.c_str());
                BLEServer* pServer = BLEDevice::createServer();
                pServer->setCallbacks(new ServerCallbacks());
                
                // Configurar servicio BLE
                BLEService* pService = setupService(pServer);
                
                // Configurar publicidad BLE
                BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
                pAdvertising->addServiceUUID(pService->getUUID());
                pAdvertising->setScanResponse(true);
                pAdvertising->setMinPreferred(0x06);
                pAdvertising->setMinPreferred(0x12);
                pAdvertising->start();
                
                // Entrar en bucle de configuración
                runConfigLoop(ioExpander);
                return true;
            }
        }
    }
    return false;
}

BLEServer* BLEHandler::initBLE(const String& devEUI) {
    String bleName = BLE_DEVICE_PREFIX + devEUI;
    BLEDevice::init(bleName.c_str());
    BLEServer* pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    return pServer;
}

void BLEHandler::runConfigLoop(PCA9555& ioExpander) {
    // Bucle de parpadeo del LED de configuración
    while (true) {
        ioExpander.digitalWrite(CONFIG_LED_PIN, HIGH);
        delay(500);
        ioExpander.digitalWrite(CONFIG_LED_PIN, LOW);
        delay(500);
    }
}

// Implementación de la configuración del servicio BLE
BLEService* BLEHandler::setupService(BLEServer* pServer) {
    // Crear el servicio de configuración utilizando el UUID definido
    BLEService* pService = pServer->createService(BLEUUID(BLE_SERVICE_UUID));

    // Característica del sistema - común para todos los tipos de dispositivo
    BLECharacteristic* pSystemChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_SYSTEM_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pSystemChar->setCallbacks(new SystemConfigCallback());

#ifdef DEVICE_TYPE_ANALOGIC
    // Para dispositivo analógico, se utilizan todas las callbacks
    
    // Característica para configuración NTC 100K
    BLECharacteristic* pNTC100KChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_NTC100K_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pNTC100KChar->setCallbacks(new NTC100KConfigCallback());
    
    // Característica para configuración NTC 10K
    BLECharacteristic* pNTC10KChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_NTC10K_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pNTC10KChar->setCallbacks(new NTC10KConfigCallback());
    
    // Característica para configuración de Conductividad
    BLECharacteristic* pCondChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_CONDUCTIVITY_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pCondChar->setCallbacks(new ConductivityConfigCallback());
    
    // Característica para configuración de pH
    BLECharacteristic* pPHChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_PH_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pPHChar->setCallbacks(new PHConfigCallback());
#endif

    // Característica para configuración de Sensores - común para BASIC y ANALOGIC
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    BLECharacteristic* pSensorsChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_SENSORS_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pSensorsChar->setCallbacks(new SensorsConfigCallback());
#endif
    
    // Característica para configuración de LoRa - común para todos los tipos
    BLECharacteristic* pLoRaConfigChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_LORA_CONFIG_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pLoRaConfigChar->setCallbacks(new LoRaConfigCallback());
    
    pService->start();
    return pService;
}

// Implementación de SystemConfigCallback
void BLEHandler::SystemConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: SystemConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera un JSON de la forma: { "system": { "initialized": <bool>, "sleep_time": <valor>, "device_id": "<valor>" } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> doc;
    DeserializationError error = deserializeJson(doc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando System config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject obj = doc[NAMESPACE_SYSTEM];
    bool initialized = obj[KEY_INITIALIZED] | false;
    uint32_t sleepTime = obj[KEY_SLEEP_TIME] | DEFAULT_TIME_TO_SLEEP;
    String deviceId = obj[KEY_DEVICE_ID] | "";
    String stationId = obj[KEY_STATION_ID] | "";
    
    DEBUG_PRINT(F("DEBUG: Configuración de sistema parseada: initialized="));
    DEBUG_PRINT(initialized);
    DEBUG_PRINT(F(", sleepTime="));
    DEBUG_PRINT(sleepTime);
    DEBUG_PRINT(F(", deviceId="));
    DEBUG_PRINT(deviceId);
    DEBUG_PRINT(F(", stationId="));
    DEBUG_PRINTLN(stationId);
    
    ConfigManager::setSystemConfig(initialized, sleepTime, deviceId, stationId);
}

void BLEHandler::SystemConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    bool initialized;
    uint32_t sleepTime;
    String deviceId;
    String stationId;
    ConfigManager::getSystemConfig(initialized, sleepTime, deviceId, stationId);
    
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> doc;
    JsonObject obj = doc.createNestedObject(NAMESPACE_SYSTEM);
    obj[KEY_INITIALIZED] = initialized;
    obj[KEY_SLEEP_TIME] = sleepTime;
    obj[KEY_DEVICE_ID] = deviceId;
    obj[KEY_STATION_ID] = stationId;

    String jsonString;
    serializeJson(doc, jsonString);
    DEBUG_PRINT(F("DEBUG: SystemConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}

#ifdef DEVICE_TYPE_ANALOGIC
// Implementación de NTC100KConfigCallback
void BLEHandler::NTC100KConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: NTC100KConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());

    // Se espera un JSON de la forma: { "ntc_100k": { <parámetros> } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando NTC100K config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject doc = fullDoc[NAMESPACE_NTC100K];
    DEBUG_PRINT(F("DEBUG: NTC100K valores parseados - T1: "));
    DEBUG_PRINT(doc[KEY_NTC100K_T1] | 0.0);
    DEBUG_PRINT(F(", R1: "));
    DEBUG_PRINT(doc[KEY_NTC100K_R1] | 0.0);
    DEBUG_PRINT(F(", T2: "));
    DEBUG_PRINT(doc[KEY_NTC100K_T2] | 0.0);
    DEBUG_PRINT(F(", R2: "));
    DEBUG_PRINT(doc[KEY_NTC100K_R2] | 0.0);
    DEBUG_PRINT(F(", T3: "));
    DEBUG_PRINT(doc[KEY_NTC100K_T3] | 0.0);
    DEBUG_PRINT(F(", R3: "));
    DEBUG_PRINTLN(doc[KEY_NTC100K_R3] | 0.0);
    
    ConfigManager::setNTC100KConfig(
        doc[KEY_NTC100K_T1] | 0.0,
        doc[KEY_NTC100K_R1] | 0.0,
        doc[KEY_NTC100K_T2] | 0.0,
        doc[KEY_NTC100K_R2] | 0.0,
        doc[KEY_NTC100K_T3] | 0.0,
        doc[KEY_NTC100K_R3] | 0.0
    );
}

void BLEHandler::NTC100KConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    double t1, r1, t2, r2, t3, r3;
    ConfigManager::getNTC100KConfig(t1, r1, t2, r2, t3, r3);
    
    DEBUG_PRINT(F("DEBUG: NTC100KConfigCallback onRead - Config: T1="));
    DEBUG_PRINT(t1);
    DEBUG_PRINT(F(", R1="));
    DEBUG_PRINT(r1);
    DEBUG_PRINT(F(", T2="));
    DEBUG_PRINT(t2);
    DEBUG_PRINT(F(", R2="));
    DEBUG_PRINT(r2);
    DEBUG_PRINT(F(", T3="));
    DEBUG_PRINT(t3);
    DEBUG_PRINT(F(", R3="));
    DEBUG_PRINTLN(r3);
    
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    // Crear objeto anidado con el namespace "ntc_100k"
    JsonObject doc = fullDoc.createNestedObject(NAMESPACE_NTC100K);
    doc[KEY_NTC100K_T1] = t1;
    doc[KEY_NTC100K_R1] = r1;
    doc[KEY_NTC100K_T2] = t2;
    doc[KEY_NTC100K_R2] = r2;
    doc[KEY_NTC100K_T3] = t3;
    doc[KEY_NTC100K_R3] = r3;
    
    String jsonString;
    serializeJson(fullDoc, jsonString);
    DEBUG_PRINT(F("DEBUG: NTC100KConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}

// Implementación de NTC10KConfigCallback
void BLEHandler::NTC10KConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: NTC10KConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera JSON: { "ntc_10k": { <parámetros> } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando NTC10K config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject doc = fullDoc[NAMESPACE_NTC10K];
    DEBUG_PRINT(F("DEBUG: NTC10K valores parseados - T1: "));
    DEBUG_PRINT(doc[KEY_NTC10K_T1] | 0.0);
    DEBUG_PRINT(F(", R1: "));
    DEBUG_PRINT(doc[KEY_NTC10K_R1] | 0.0);
    DEBUG_PRINT(F(", T2: "));
    DEBUG_PRINT(doc[KEY_NTC10K_T2] | 0.0);
    DEBUG_PRINT(F(", R2: "));
    DEBUG_PRINT(doc[KEY_NTC10K_R2] | 0.0);
    DEBUG_PRINT(F(", T3: "));
    DEBUG_PRINT(doc[KEY_NTC10K_T3] | 0.0);
    DEBUG_PRINT(F(", R3: "));
    DEBUG_PRINTLN(doc[KEY_NTC10K_R3] | 0.0);
    
    ConfigManager::setNTC10KConfig(
        doc[KEY_NTC10K_T1] | 0.0,
        doc[KEY_NTC10K_R1] | 0.0,
        doc[KEY_NTC10K_T2] | 0.0,
        doc[KEY_NTC10K_R2] | 0.0,
        doc[KEY_NTC10K_T3] | 0.0,
        doc[KEY_NTC10K_R3] | 0.0
    );
}

void BLEHandler::NTC10KConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    double t1, r1, t2, r2, t3, r3;
    ConfigManager::getNTC10KConfig(t1, r1, t2, r2, t3, r3);
    
    DEBUG_PRINT(F("DEBUG: NTC10KConfigCallback onRead - Config: T1="));
    DEBUG_PRINT(t1);
    DEBUG_PRINT(F(", R1="));
    DEBUG_PRINT(r1);
    DEBUG_PRINT(F(", T2="));
    DEBUG_PRINT(t2);
    DEBUG_PRINT(F(", R2="));
    DEBUG_PRINT(r2);
    DEBUG_PRINT(F(", T3="));
    DEBUG_PRINT(t3);
    DEBUG_PRINT(F(", R3="));
    DEBUG_PRINTLN(r3);
    
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    JsonObject doc = fullDoc.createNestedObject(NAMESPACE_NTC10K);
    doc[KEY_NTC10K_T1] = t1;
    doc[KEY_NTC10K_R1] = r1;
    doc[KEY_NTC10K_T2] = t2;
    doc[KEY_NTC10K_R2] = r2;
    doc[KEY_NTC10K_T3] = t3;
    doc[KEY_NTC10K_R3] = r3;
    
    String jsonString;
    serializeJson(fullDoc, jsonString);
    DEBUG_PRINT(F("DEBUG: NTC10KConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}

// Implementación de ConductivityConfigCallback
void BLEHandler::ConductivityConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: ConductivityConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera un JSON: { "cond": { <parámetros> } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando Conductivity config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject doc = fullDoc[NAMESPACE_COND];
    
    DEBUG_PRINT(F("DEBUG: Conductivity valores parseados - CT: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_CT] | 0.0f);
    DEBUG_PRINT(F(", CC: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_CC] | 0.0f);
    DEBUG_PRINT(F(", V1: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_V1] | 0.0f);
    DEBUG_PRINT(F(", T1: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_T1] | 0.0f);
    DEBUG_PRINT(F(", V2: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_V2] | 0.0f);
    DEBUG_PRINT(F(", T2: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_T2] | 0.0f);
    DEBUG_PRINT(F(", V3: "));
    DEBUG_PRINT(doc[KEY_CONDUCT_V3] | 0.0f);
    DEBUG_PRINT(F(", T3: "));
    DEBUG_PRINTLN(doc[KEY_CONDUCT_T3] | 0.0f);
    
    ConfigManager::setConductivityConfig(
        doc[KEY_CONDUCT_CT] | 0.0f,  // Temperatura de calibración
        doc[KEY_CONDUCT_CC] | 0.0f,  // Coeficiente de compensación
        doc[KEY_CONDUCT_V1] | 0.0f,
        doc[KEY_CONDUCT_T1] | 0.0f,
        doc[KEY_CONDUCT_V2] | 0.0f,
        doc[KEY_CONDUCT_T2] | 0.0f,
        doc[KEY_CONDUCT_V3] | 0.0f,
        doc[KEY_CONDUCT_T3] | 0.0f
    );
}

void BLEHandler::ConductivityConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    float calTemp, coefComp, v1, t1, v2, t2, v3, t3;
    ConfigManager::getConductivityConfig(calTemp, coefComp, v1, t1, v2, t2, v3, t3);
    
    DEBUG_PRINT(F("DEBUG: ConductivityConfigCallback onRead - Config: CT="));
    DEBUG_PRINT(calTemp);
    DEBUG_PRINT(F(", CC="));
    DEBUG_PRINT(coefComp);
    DEBUG_PRINT(F(", V1="));
    DEBUG_PRINT(v1);
    DEBUG_PRINT(F(", T1="));
    DEBUG_PRINT(t1);
    DEBUG_PRINT(F(", V2="));
    DEBUG_PRINT(v2);
    DEBUG_PRINT(F(", T2="));
    DEBUG_PRINT(t2);
    DEBUG_PRINT(F(", V3="));
    DEBUG_PRINT(v3);
    DEBUG_PRINT(F(", T3="));
    DEBUG_PRINTLN(t3);
    
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    JsonObject doc = fullDoc.createNestedObject(NAMESPACE_COND);
    doc[KEY_CONDUCT_CT] = calTemp;
    doc[KEY_CONDUCT_CC] = coefComp;
    doc[KEY_CONDUCT_V1] = v1;
    doc[KEY_CONDUCT_T1] = t1;
    doc[KEY_CONDUCT_V2] = v2;
    doc[KEY_CONDUCT_T2] = t2;
    doc[KEY_CONDUCT_V3] = v3;
    doc[KEY_CONDUCT_T3] = t3;
    
    String jsonString;
    serializeJson(fullDoc, jsonString);
    DEBUG_PRINT(F("DEBUG: ConductivityConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}

// Implementación de PHConfigCallback
void BLEHandler::PHConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: PHConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera JSON: { "ph": { <parámetros> } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando pH config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject doc = fullDoc[NAMESPACE_PH];
    DEBUG_PRINT(F("DEBUG: pH valores parseados - V1: "));
    DEBUG_PRINT(doc[KEY_PH_V1] | 0.0f);
    DEBUG_PRINT(F(", T1: "));
    DEBUG_PRINT(doc[KEY_PH_T1] | 0.0f);
    DEBUG_PRINT(F(", V2: "));
    DEBUG_PRINT(doc[KEY_PH_V2] | 0.0f);
    DEBUG_PRINT(F(", T2: "));
    DEBUG_PRINT(doc[KEY_PH_T2] | 0.0f);
    DEBUG_PRINT(F(", V3: "));
    DEBUG_PRINT(doc[KEY_PH_V3] | 0.0f);
    DEBUG_PRINT(F(", T3: "));
    DEBUG_PRINT(doc[KEY_PH_T3] | 0.0f);
    DEBUG_PRINT(F(", CT: "));
    DEBUG_PRINTLN(doc[KEY_PH_CT] | 25.0f);
    
    ConfigManager::setPHConfig(
        doc[KEY_PH_V1] | 0.0f,
        doc[KEY_PH_T1] | 0.0f,
        doc[KEY_PH_V2] | 0.0f,
        doc[KEY_PH_T2] | 0.0f,
        doc[KEY_PH_V3] | 0.0f,
        doc[KEY_PH_T3] | 0.0f,
        doc[KEY_PH_CT] | 25.0f
    );
}

void BLEHandler::PHConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    float v1, t1, v2, t2, v3, t3, calTemp;
    ConfigManager::getPHConfig(v1, t1, v2, t2, v3, t3, calTemp);
    
    DEBUG_PRINT(F("DEBUG: PHConfigCallback onRead - Config: V1="));
    DEBUG_PRINT(v1);
    DEBUG_PRINT(F(", T1="));
    DEBUG_PRINT(t1);
    DEBUG_PRINT(F(", V2="));
    DEBUG_PRINT(v2);
    DEBUG_PRINT(F(", T2="));
    DEBUG_PRINT(t2);
    DEBUG_PRINT(F(", V3="));
    DEBUG_PRINT(v3);
    DEBUG_PRINT(F(", T3="));
    DEBUG_PRINT(t3);
    DEBUG_PRINT(F(", CT="));
    DEBUG_PRINTLN(calTemp);
    
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    JsonObject doc = fullDoc.createNestedObject(NAMESPACE_PH);
    doc[KEY_PH_V1] = v1;
    doc[KEY_PH_T1] = t1;
    doc[KEY_PH_V2] = v2;
    doc[KEY_PH_T2] = t2;
    doc[KEY_PH_V3] = v3;
    doc[KEY_PH_T3] = t3;
    doc[KEY_PH_CT] = calTemp;
    
    String jsonString;
    serializeJson(fullDoc, jsonString);
    DEBUG_PRINT(F("DEBUG: PHConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}
#endif // DEVICE_TYPE_ANALOGIC

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
// Implementación de SensorsConfigCallback
void BLEHandler::SensorsConfigCallback::onWrite(BLECharacteristic *pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: SensorsConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera un JSON: { "sensors": [ {<sensor1>}, {<sensor2>}, ... ] }
    DynamicJsonDocument doc(JSON_DOC_SIZE_LARGE);
    DeserializationError error = deserializeJson(doc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando Sensors config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    
    std::vector<SensorConfig> configs;
    JsonArray sensorArray = doc[NAMESPACE_SENSORS];
    
    for (JsonVariant sensor : sensorArray) {
        SensorConfig config;
        strncpy(config.configKey, sensor[KEY_SENSOR] | "", sizeof(config.configKey));
        strncpy(config.sensorId, sensor[KEY_SENSOR_ID] | "", sizeof(config.sensorId));
        strncpy(config.tempSensorId, sensor[KEY_SENSOR_ID_TEMPERATURE_SENSOR] | "", sizeof(config.tempSensorId));
        config.type = static_cast<SensorType>(sensor[KEY_SENSOR_TYPE] | 0);
        config.enable = sensor[KEY_SENSOR_ENABLE] | false;
        
        DEBUG_PRINT(F("DEBUG: Sensor config parsed - key: "));
        DEBUG_PRINT(config.configKey);
        DEBUG_PRINT(F(", sensorId: "));
        DEBUG_PRINT(config.sensorId);
        DEBUG_PRINT(F(", tempSensorId: "));
        DEBUG_PRINT(config.tempSensorId);
        DEBUG_PRINT(F(", type: "));
        DEBUG_PRINT(static_cast<int>(config.type));
        DEBUG_PRINT(F(", enable: "));
        DEBUG_PRINTLN(config.enable ? "true" : "false");
        
        configs.push_back(config);
    }
    
    ConfigManager::setSensorsConfigs(configs);
}

void BLEHandler::SensorsConfigCallback::onRead(BLECharacteristic *pCharacteristic) {
    DynamicJsonDocument doc(JSON_DOC_SIZE_LARGE);
    JsonArray sensorArray = doc.createNestedArray(NAMESPACE_SENSORS);

    std::vector<SensorConfig> configs = ConfigManager::getAllSensorConfigs();
    
    DEBUG_PRINTLN(F("DEBUG: SensorsConfigCallback onRead - Configuraciones de sensores obtenidas:"));
    for (const auto& sensor : configs) {
        DEBUG_PRINT(F("DEBUG: Sensor config - key: "));
        DEBUG_PRINT(sensor.configKey);
        DEBUG_PRINT(F(", sensorId: "));
        DEBUG_PRINT(sensor.sensorId);
        DEBUG_PRINT(F(", type: "));
        DEBUG_PRINT(static_cast<int>(sensor.type));
        DEBUG_PRINT(F(", tempSensorId: "));
        DEBUG_PRINT(sensor.tempSensorId);
        DEBUG_PRINT(F(", enable: "));
        DEBUG_PRINTLN(sensor.enable ? "true" : "false");

        JsonObject obj = sensorArray.createNestedObject();
        obj[KEY_SENSOR]             = sensor.configKey;
        obj[KEY_SENSOR_ID]          = sensor.sensorId;
        obj[KEY_SENSOR_TYPE]        = static_cast<int>(sensor.type);
        obj[KEY_SENSOR_ID_TEMPERATURE_SENSOR]   = sensor.tempSensorId;
        obj[KEY_SENSOR_ENABLE]      = sensor.enable;
    }

    String jsonString;
    serializeJson(doc, jsonString);
    DEBUG_PRINT(F("DEBUG: SensorsConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
}
#endif

// Implementación de LoRaConfigCallback
void BLEHandler::LoRaConfigCallback::onWrite(BLECharacteristic* pCharacteristic) {
    DEBUG_PRINTLN(F("DEBUG: LoRaConfigCallback onWrite - JSON recibido:"));
    DEBUG_PRINTLN(pCharacteristic->getValue().c_str());
    
    // Se espera JSON: { "lorawan": { <parámetros> } }
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
    if (error) {
        DEBUG_PRINT(F("Error deserializando LoRa config: "));
        DEBUG_PRINTLN(error.c_str());
        return;
    }
    JsonObject doc = fullDoc[NAMESPACE_LORAWAN];
    String joinEUI     = doc[KEY_LORA_JOIN_EUI]      | "";
    String devEUI     = doc[KEY_LORA_DEV_EUI]      | "";
    String nwkKey     = doc[KEY_LORA_NWK_KEY]      | "";
    String appKey     = doc[KEY_LORA_APP_KEY]      | "";
    
    DEBUG_PRINT(F("DEBUG: LoRa valores parseados - joinEUI: "));
    DEBUG_PRINT(joinEUI);
    DEBUG_PRINT(F(", devEUI: "));
    DEBUG_PRINT(devEUI);
    DEBUG_PRINT(F(", nwkKey: "));
    DEBUG_PRINT(nwkKey);
    DEBUG_PRINT(F(", appKey: "));
    DEBUG_PRINTLN(appKey);
    
    ConfigManager::setLoRaConfig(joinEUI, devEUI, nwkKey, appKey);
}

void BLEHandler::LoRaConfigCallback::onRead(BLECharacteristic* pCharacteristic) {
    LoRaConfig config = ConfigManager::getLoRaConfig();
    
    DEBUG_PRINTLN(F("DEBUG: LoRaConfigCallback onRead - Config obtenido:"));
    DEBUG_PRINT(F("joinEUI: "));
    DEBUG_PRINTLN(config.joinEUI);
    DEBUG_PRINT(F("devEUI: "));
    DEBUG_PRINTLN(config.devEUI);
    DEBUG_PRINT(F("nwkKey: "));
    DEBUG_PRINTLN(config.nwkKey);
    
    // Aumentamos el tamaño del documento para asegurarnos de incluir todas las claves
    StaticJsonDocument<JSON_DOC_SIZE_SMALL> fullDoc;
    JsonObject doc = fullDoc.createNestedObject(NAMESPACE_LORAWAN);
    doc[KEY_LORA_JOIN_EUI]     = config.joinEUI;
    doc[KEY_LORA_DEV_EUI]     = config.devEUI;
    doc[KEY_LORA_NWK_KEY]     = config.nwkKey;
    doc[KEY_LORA_APP_KEY]     = config.appKey;
    String jsonString;
    serializeJson(fullDoc, jsonString);
    DEBUG_PRINT(F("DEBUG: LoRaConfigCallback onRead - JSON enviado: "));
    DEBUG_PRINTLN(jsonString);
    pCharacteristic->setValue(jsonString.c_str());
} 