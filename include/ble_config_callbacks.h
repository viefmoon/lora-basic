#pragma once

// Definición de tamaño general para StaticJsonDocument
#define JSON_DOC_SIZE 300

#include <BLECharacteristic.h>
#include <BLEServer.h>  // Necesario para BLEServerCallbacks
#include "config_manager.h"
#include <ArduinoJson.h>
#include "config.h"

/**
 * Callback para el manejo de eventos del servidor BLE.
 * Al desconectar, se reinicia la publicidad para que el dispositivo
 * siga siendo detectable y se pueda reconectar.
 */
class MyBLEServerCallbacks: public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override {
        Serial.println(F("BLE Cliente conectado"));
    }

    void onDisconnect(BLEServer* pServer) override {
        Serial.println(F("BLE Cliente desconectado, reiniciando publicidad..."));
        pServer->getAdvertising()->start();
    }
};

// Callback unificada para la configuración del sistema (system, sleep y device)
class SystemConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: SystemConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera un JSON de la forma: { "system": { "initialized": <bool>, "sleep_time": <valor>, "device_id": "<valor>" } }
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        DeserializationError error = deserializeJson(doc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando System config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject obj = doc[NAMESPACE_SYSTEM];
        bool initialized = obj[KEY_INITIALIZED] | false;
        uint32_t sleepTime = obj[KEY_SLEEP_TIME] | DEFAULT_TIME_TO_SLEEP;
        String deviceId = obj[KEY_DEVICE_ID] | "";
        String stationId = obj[KEY_STATION_ID] | "";
        Serial.print(F("DEBUG: Configuración de sistema parseada: initialized="));
        Serial.print(initialized);
        Serial.print(F(", sleepTime="));
        Serial.print(sleepTime);
        Serial.print(F(", deviceId="));
        Serial.print(deviceId);
        Serial.print(F(", stationId="));
        Serial.println(stationId);

        ConfigManager::setSystemConfig(initialized, sleepTime, deviceId, stationId);
    }
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        bool initialized;
        uint32_t sleepTime;
        String deviceId;
        String stationId;
        ConfigManager::getSystemConfig(initialized, sleepTime, deviceId, stationId);
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        JsonObject obj = doc.createNestedObject(NAMESPACE_SYSTEM);
        obj[KEY_INITIALIZED] = initialized;
        obj[KEY_SLEEP_TIME] = sleepTime;
        obj[KEY_DEVICE_ID] = deviceId;
        obj[KEY_STATION_ID] = stationId;

        String jsonString;
        serializeJson(doc, jsonString);
        Serial.print(F("DEBUG: SystemConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para NTC 100K usando JSON anidado en el namespace "ntc_100k"
class NTC100KConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: NTC100KConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());

        // Se espera un JSON de la forma: { "ntc_100k": { <parámetros> } }
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando NTC100K config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject doc = fullDoc[NAMESPACE_NTC100K];
        Serial.print(F("DEBUG: NTC100K valores parseados - T1: "));
        Serial.print(doc[KEY_NTC100K_T1] | 0.0);
        Serial.print(F(", R1: "));
        Serial.print(doc[KEY_NTC100K_R1] | 0.0);
        Serial.print(F(", T2: "));
        Serial.print(doc[KEY_NTC100K_T2] | 0.0);
        Serial.print(F(", R2: "));
        Serial.print(doc[KEY_NTC100K_R2] | 0.0);
        Serial.print(F(", T3: "));
        Serial.print(doc[KEY_NTC100K_T3] | 0.0);
        Serial.print(F(", R3: "));
        Serial.println(doc[KEY_NTC100K_R3] | 0.0);
        
        ConfigManager::setNTC100KConfig(
            doc[KEY_NTC100K_T1] | 0.0,
            doc[KEY_NTC100K_R1] | 0.0,
            doc[KEY_NTC100K_T2] | 0.0,
            doc[KEY_NTC100K_R2] | 0.0,
            doc[KEY_NTC100K_T3] | 0.0,
            doc[KEY_NTC100K_R3] | 0.0
        );
    }
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        double t1, r1, t2, r2, t3, r3;
        ConfigManager::getNTC100KConfig(t1, r1, t2, r2, t3, r3);
        
        Serial.print(F("DEBUG: NTC100KConfigCallback onRead - Config: T1="));
        Serial.print(t1);
        Serial.print(F(", R1="));
        Serial.print(r1);
        Serial.print(F(", T2="));
        Serial.print(t2);
        Serial.print(F(", R2="));
        Serial.print(r2);
        Serial.print(F(", T3="));
        Serial.print(t3);
        Serial.print(F(", R3="));
        Serial.println(r3);
        
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
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
        Serial.print(F("DEBUG: NTC100KConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para Conductividad usando JSON anidado en el namespace "cond"
class ConductivityConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: ConductivityConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera un JSON: { "cond": { <parámetros> } }
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando Conductivity config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject doc = fullDoc[NAMESPACE_COND];
        
        Serial.print(F("DEBUG: Conductivity valores parseados - CT: "));
        Serial.print(doc[KEY_CONDUCT_CT] | 0.0f);
        Serial.print(F(", CC: "));
        Serial.print(doc[KEY_CONDUCT_CC] | 0.0f);
        Serial.print(F(", V1: "));
        Serial.print(doc[KEY_CONDUCT_V1] | 0.0f);
        Serial.print(F(", T1: "));
        Serial.print(doc[KEY_CONDUCT_T1] | 0.0f);
        Serial.print(F(", V2: "));
        Serial.print(doc[KEY_CONDUCT_V2] | 0.0f);
        Serial.print(F(", T2: "));
        Serial.print(doc[KEY_CONDUCT_T2] | 0.0f);
        Serial.print(F(", V3: "));
        Serial.print(doc[KEY_CONDUCT_V3] | 0.0f);
        Serial.print(F(", T3: "));
        Serial.println(doc[KEY_CONDUCT_T3] | 0.0f);
        
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
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        float calTemp, coefComp, v1, t1, v2, t2, v3, t3;
        ConfigManager::getConductivityConfig(calTemp, coefComp, v1, t1, v2, t2, v3, t3);
        
        Serial.print(F("DEBUG: ConductivityConfigCallback onRead - Config: CT: "));
        Serial.print(calTemp);
        Serial.print(F(", CC: "));
        Serial.print(coefComp);
        Serial.print(F(", V1: "));
        Serial.print(v1);
        Serial.print(F(", T1: "));
        Serial.print(t1);
        Serial.print(F(", V2: "));
        Serial.print(v2);
        Serial.print(F(", T2: "));
        Serial.print(t2);
        Serial.print(F(", V3: "));
        Serial.print(v3);
        Serial.print(F(", T3: "));
        Serial.println(t3);
        
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
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
        Serial.print(F("DEBUG: ConductivityConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para NTC 10K usando JSON anidado en el namespace "ntc_10k"
class NTC10KConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: NTC10KConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera JSON: { "ntc_10k": { <parámetros> } }
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando NTC10K config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject doc = fullDoc[NAMESPACE_NTC10K];
        Serial.print(F("DEBUG: NTC10K valores parseados - T1: "));
        Serial.print(doc[KEY_NTC10K_T1] | 0.0);
        Serial.print(F(", R1: "));
        Serial.print(doc[KEY_NTC10K_R1] | 0.0);
        Serial.print(F(", T2: "));
        Serial.print(doc[KEY_NTC10K_T2] | 0.0);
        Serial.print(F(", R2: "));
        Serial.print(doc[KEY_NTC10K_R2] | 0.0);
        Serial.print(F(", T3: "));
        Serial.print(doc[KEY_NTC10K_T3] | 0.0);
        Serial.print(F(", R3: "));
        Serial.println(doc[KEY_NTC10K_R3] | 0.0);
        
        ConfigManager::setNTC10KConfig(
            doc[KEY_NTC10K_T1] | 0.0,
            doc[KEY_NTC10K_R1] | 0.0,
            doc[KEY_NTC10K_T2] | 0.0,
            doc[KEY_NTC10K_R2] | 0.0,
            doc[KEY_NTC10K_T3] | 0.0,
            doc[KEY_NTC10K_R3] | 0.0
        );
    }
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        double t1, r1, t2, r2, t3, r3;
        ConfigManager::getNTC10KConfig(t1, r1, t2, r2, t3, r3);
        
        Serial.print(F("DEBUG: NTC10KConfigCallback onRead - Config: T1="));
        Serial.print(t1);
        Serial.print(F(", R1="));
        Serial.print(r1);
        Serial.print(F(", T2="));
        Serial.print(t2);
        Serial.print(F(", R2="));
        Serial.print(r2);
        Serial.print(F(", T3="));
        Serial.print(t3);
        Serial.print(F(", R3="));
        Serial.println(r3);
        
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        JsonObject doc = fullDoc.createNestedObject(NAMESPACE_NTC10K);
        doc[KEY_NTC10K_T1] = t1;
        doc[KEY_NTC10K_R1] = r1;
        doc[KEY_NTC10K_T2] = t2;
        doc[KEY_NTC10K_R2] = r2;
        doc[KEY_NTC10K_T3] = t3;
        doc[KEY_NTC10K_R3] = r3;
        
        String jsonString;
        serializeJson(fullDoc, jsonString);
        Serial.print(F("DEBUG: NTC10KConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para pH usando JSON anidado en el namespace "ph"
class PHConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: PHConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera JSON: { "ph": { <parámetros> } }
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando pH config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject doc = fullDoc[NAMESPACE_PH];
        Serial.print(F("DEBUG: pH valores parseados - V1: "));
        Serial.print(doc[KEY_PH_V1] | 0.0f);
        Serial.print(F(", T1: "));
        Serial.print(doc[KEY_PH_T1] | 0.0f);
        Serial.print(F(", V2: "));
        Serial.print(doc[KEY_PH_V2] | 0.0f);
        Serial.print(F(", T2: "));
        Serial.print(doc[KEY_PH_T2] | 0.0f);
        Serial.print(F(", V3: "));
        Serial.print(doc[KEY_PH_V3] | 0.0f);
        Serial.print(F(", T3: "));
        Serial.print(doc[KEY_PH_T3] | 0.0f);
        Serial.print(F(", CT: "));
        Serial.println(doc[KEY_PH_CT] | 25.0f);
        
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
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        float v1, t1, v2, t2, v3, t3, calTemp;
        ConfigManager::getPHConfig(v1, t1, v2, t2, v3, t3, calTemp);
        
        Serial.print(F("DEBUG: PHConfigCallback onRead - Config: V1="));
        Serial.print(v1);
        Serial.print(F(", T1="));
        Serial.print(t1);
        Serial.print(F(", V2="));
        Serial.print(v2);
        Serial.print(F(", T2="));
        Serial.print(t2);
        Serial.print(F(", V3="));
        Serial.print(v3);
        Serial.print(F(", T3="));
        Serial.print(t3);
        Serial.print(F(", CT="));
        Serial.println(calTemp);
        
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
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
        Serial.print(F("DEBUG: PHConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para Sensors (manteniendo su estructura original)
class SensorsConfigCallback: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.println(F("DEBUG: SensorsConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera un JSON: { "sensors": [ {<sensor1>}, {<sensor2>}, ... ] }
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando Sensors config: "));
            Serial.println(error.c_str());
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
            
            Serial.print(F("DEBUG: Sensor config parsed - key: "));
            Serial.print(config.configKey);
            Serial.print(F(", sensorId: "));
            Serial.print(config.sensorId);
            Serial.print(F(", tempSensorId: "));
            Serial.print(config.tempSensorId);
            Serial.print(F(", type: "));
            Serial.print(static_cast<int>(config.type));
            Serial.print(F(", enable: "));
            Serial.println(config.enable ? "true" : "false");
            
            configs.push_back(config);
        }
        
        ConfigManager::setSensorsConfigs(configs);
    }
    
    void onRead(BLECharacteristic *pCharacteristic) override {
        DynamicJsonDocument doc(2048);
        JsonArray sensorArray = doc.createNestedArray(NAMESPACE_SENSORS);

        std::vector<SensorConfig> configs = ConfigManager::getAllSensorConfigs();
        
        Serial.println(F("DEBUG: SensorsConfigCallback onRead - Configuraciones de sensores obtenidas:"));
        for (const auto& sensor : configs) {
            Serial.print(F("DEBUG: Sensor config - key: "));
            Serial.print(sensor.configKey);
            Serial.print(F(", sensorId: "));
            Serial.print(sensor.sensorId);
            Serial.print(F(", type: "));
            Serial.print(static_cast<int>(sensor.type));
            Serial.print(F(", tempSensorId: "));
            Serial.print(sensor.tempSensorId);
            Serial.print(F(", enable: "));
            Serial.println(sensor.enable ? "true" : "false");

            JsonObject obj = sensorArray.createNestedObject();
            obj[KEY_SENSOR]             = sensor.configKey;
            obj[KEY_SENSOR_ID]          = sensor.sensorId;
            obj[KEY_SENSOR_TYPE]        = static_cast<int>(sensor.type);
            obj[KEY_SENSOR_ID_TEMPERATURE_SENSOR]   = sensor.tempSensorId;
            obj[KEY_SENSOR_ENABLE]      = sensor.enable;
        }

        String jsonString;
        serializeJson(doc, jsonString);
        Serial.print(F("DEBUG: SensorsConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};

// Callback para LoRa usando JSON anidado en el namespace "lorawan"
class LoRaConfigCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* pCharacteristic) override {
        Serial.println(F("DEBUG: LoRaConfigCallback onWrite - JSON recibido:"));
        Serial.println(pCharacteristic->getValue().c_str());
        
        // Se espera JSON: { "lorawan": { <parámetros> } }
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        DeserializationError error = deserializeJson(fullDoc, pCharacteristic->getValue());
        if (error) {
            Serial.print(F("Error deserializando LoRa config: "));
            Serial.println(error.c_str());
            return;
        }
        JsonObject doc = fullDoc[NAMESPACE_LORAWAN];
        String joinEUI     = doc[KEY_LORA_JOIN_EUI]      | "";
        String devEUI     = doc[KEY_LORA_DEV_EUI]      | "";
        String nwkKey     = doc[KEY_LORA_NWK_KEY]      | "";
        String appKey     = doc[KEY_LORA_APP_KEY]      | "";
        
        Serial.print(F("DEBUG: LoRa valores parseados - joinEUI: "));
        Serial.print(joinEUI);
        Serial.print(F(", devEUI: "));
        Serial.print(devEUI);
        Serial.print(F(", nwkKey: "));
        Serial.print(nwkKey);
        Serial.print(F(", appKey: "));
        Serial.println(appKey);
        
        ConfigManager::setLoRaConfig(joinEUI, devEUI, nwkKey, appKey);
    }
    
    void onRead(BLECharacteristic* pCharacteristic) override {
        LoRaConfig config = ConfigManager::getLoRaConfig();
        
        Serial.println(F("DEBUG: LoRaConfigCallback onRead - Config obtenido:"));
        Serial.print(F(", joinEUI: "));
        Serial.println(config.joinEUI);
        Serial.print(F(", devEUI: "));
        Serial.println(config.devEUI);
        Serial.print(F(", nwkKey: "));
        Serial.println(config.nwkKey);
        
        // Aumentamos el tamaño del documento para asegurarnos de incluir todas las claves
        StaticJsonDocument<JSON_DOC_SIZE> fullDoc;
        JsonObject doc = fullDoc.createNestedObject(NAMESPACE_LORAWAN);
        doc[KEY_LORA_JOIN_EUI]     = config.joinEUI;
        doc[KEY_LORA_DEV_EUI]     = config.devEUI;
        doc[KEY_LORA_NWK_KEY]     = config.nwkKey;
        doc[KEY_LORA_APP_KEY]     = config.appKey;
        String jsonString;
        serializeJson(fullDoc, jsonString);
        Serial.print(F("DEBUG: LoRaConfigCallback onRead - JSON enviado: "));
        Serial.println(jsonString);
        pCharacteristic->setValue(jsonString.c_str());
    }
};