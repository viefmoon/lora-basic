#include "config_manager.h"
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "sensor_types.h"
#include <Preferences.h>
#include <Arduino.h> // Incluido para usar Serial

// Funciones auxiliares para leer y escribir el JSON completo en cada namespace.
static const size_t JSON_DOC_SIZE = 1024;

static void writeNamespace(const char* ns, const StaticJsonDocument<JSON_DOC_SIZE>& doc) {
    Preferences prefs;
    prefs.begin(ns, false);
    String jsonString;
    serializeJson(doc, jsonString);
    // Se usa el mismo nombre del namespace como clave interna
    prefs.putString(ns, jsonString.c_str());
    prefs.end();
}

static void readNamespace(const char* ns, StaticJsonDocument<JSON_DOC_SIZE>& doc) {
    Preferences prefs;
    prefs.begin(ns, true);
    String jsonString = prefs.getString(ns, "{}");
    prefs.end();
    deserializeJson(doc, jsonString);
}

const SensorConfig ConfigManager::defaultConfigs[] = {
    {"0", "NTC1", N100K, 0, "", true},
    {"1", "NTC2", N100K, 1, "", true},
    {"2", "CH1", CONDH, 2, "", true},
    {"3", "SM1", SOILH, 3, "", true},
    {"4", "SM2", SOILH, 4, "", true},
    {"5", "CON1", COND, 5, "", true},
    {"7", "PH1", PH, 7, "", true},
    {"R", "RTD1", RTD, 0, "", true},
    {"D", "DS1", DS18B20, 0, "", true}
};

bool ConfigManager::checkInitialized() {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    return doc[KEY_INITIALIZED] | false;
}

void ConfigManager::initializeDefaultConfig() {
    // Sistema unificado: NAMESPACE_SYSTEM (incluye system, sleep y device)
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        doc[KEY_STATION_ID] = DEFAULT_STATION_ID;
        doc[KEY_INITIALIZED] = VALUE_INITIALIZED;
        doc[KEY_SLEEP_TIME] = DEFAULT_TIME_TO_SLEEP;
        doc[KEY_DEVICE_ID] = DEFAULT_DEVICE_ID;
        writeNamespace(NAMESPACE_SYSTEM, doc);
    }
    
    // NTC 100K: NAMESPACE_NTC100K
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        doc[KEY_NTC100K_T1] = DEFAULT_T1_100K;
        doc[KEY_NTC100K_R1] = DEFAULT_R1_100K;
        doc[KEY_NTC100K_T2] = DEFAULT_T2_100K;
        doc[KEY_NTC100K_R2] = DEFAULT_R2_100K;
        doc[KEY_NTC100K_T3] = DEFAULT_T3_100K;
        doc[KEY_NTC100K_R3] = DEFAULT_R3_100K;
        writeNamespace(NAMESPACE_NTC100K, doc);
    }
    
    // NTC 10K: NAMESPACE_NTC10K
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        doc[KEY_NTC10K_T1] = DEFAULT_T1_10K;
        doc[KEY_NTC10K_R1] = DEFAULT_R1_10K;
        doc[KEY_NTC10K_T2] = DEFAULT_T2_10K;
        doc[KEY_NTC10K_R2] = DEFAULT_R2_10K;
        doc[KEY_NTC10K_T3] = DEFAULT_T3_10K;
        doc[KEY_NTC10K_R3] = DEFAULT_R3_10K;
        writeNamespace(NAMESPACE_NTC10K, doc);
    }
    
    // Conductividad: NAMESPACE_COND
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        doc[KEY_CONDUCT_CT] = CONDUCTIVITY_DEFAULT_TEMP;
        doc[KEY_CONDUCT_CC] = TEMP_COEF_COMPENSATION;
        doc[KEY_CONDUCT_V1] = CONDUCTIVITY_DEFAULT_V1;
        doc[KEY_CONDUCT_T1] = CONDUCTIVITY_DEFAULT_T1;
        doc[KEY_CONDUCT_V2] = CONDUCTIVITY_DEFAULT_V2;
        doc[KEY_CONDUCT_T2] = CONDUCTIVITY_DEFAULT_T2;
        doc[KEY_CONDUCT_V3] = CONDUCTIVITY_DEFAULT_V3;
        doc[KEY_CONDUCT_T3] = CONDUCTIVITY_DEFAULT_T3;
        writeNamespace(NAMESPACE_COND, doc);
    }
    
    // pH: NAMESPACE_PH
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;
        doc[KEY_PH_V1] = PH_DEFAULT_V1;
        doc[KEY_PH_T1] = PH_DEFAULT_T1;
        doc[KEY_PH_V2] = PH_DEFAULT_V2;
        doc[KEY_PH_T2] = PH_DEFAULT_T2;
        doc[KEY_PH_V3] = PH_DEFAULT_V3;
        doc[KEY_PH_T3] = PH_DEFAULT_T3;
        doc[KEY_PH_CT] = PH_DEFAULT_TEMP;
        writeNamespace(NAMESPACE_PH, doc);
    }
    
    // Inicializar configuración de sensores
    initializeSensorConfigs();
    
    // LoRa: NAMESPACE_LORAWAN
    {
        StaticJsonDocument<JSON_DOC_SIZE> doc;

        //FOR OTAA
        doc[KEY_LORA_JOIN_EUI]      = DEFAULT_JOIN_EUI;
        doc[KEY_LORA_DEV_EUI]       = DEFAULT_DEV_EUI;
        doc[KEY_LORA_NWK_KEY]      = DEFAULT_NWK_KEY;
        doc[KEY_LORA_APP_KEY]      = DEFAULT_APP_KEY;
        writeNamespace(NAMESPACE_LORAWAN, doc);

    }
}

void ConfigManager::getSystemConfig(bool &initialized, uint32_t &sleepTime, String &deviceId, String &stationId) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    initialized = doc[KEY_INITIALIZED] | false;
    sleepTime = doc[KEY_SLEEP_TIME] | DEFAULT_TIME_TO_SLEEP;
    deviceId = String(doc[KEY_DEVICE_ID] | DEFAULT_DEVICE_ID);
    stationId = String(doc[KEY_STATION_ID] | DEFAULT_STATION_ID);
}

void ConfigManager::setSystemConfig(bool initialized, uint32_t sleepTime, const String &deviceId, const String &stationId) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    doc[KEY_INITIALIZED] = initialized;
    doc[KEY_SLEEP_TIME] = sleepTime;
    doc[KEY_DEVICE_ID] = deviceId;
    doc[KEY_STATION_ID] = stationId;
    writeNamespace(NAMESPACE_SYSTEM, doc);
}

void ConfigManager::getNTC100KConfig(double& t1, double& r1, double& t2, double& r2, double& t3, double& r3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_NTC100K, doc);
    t1 = doc[KEY_NTC100K_T1] | DEFAULT_T1_100K;
    r1 = doc[KEY_NTC100K_R1] | DEFAULT_R1_100K;
    t2 = doc[KEY_NTC100K_T2] | DEFAULT_T2_100K;
    r2 = doc[KEY_NTC100K_R2] | DEFAULT_R2_100K;
    t3 = doc[KEY_NTC100K_T3] | DEFAULT_T3_100K;
    r3 = doc[KEY_NTC100K_R3] | DEFAULT_R3_100K;
}

void ConfigManager::setNTC100KConfig(double t1, double r1, double t2, double r2, double t3, double r3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_NTC100K, doc);
    doc[KEY_NTC100K_T1] = t1;
    doc[KEY_NTC100K_R1] = r1;
    doc[KEY_NTC100K_T2] = t2;
    doc[KEY_NTC100K_R2] = r2;
    doc[KEY_NTC100K_T3] = t3;
    doc[KEY_NTC100K_R3] = r3;
    writeNamespace(NAMESPACE_NTC100K, doc);
}

void ConfigManager::getNTC10KConfig(double& t1, double& r1, double& t2, double& r2, double& t3, double& r3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_NTC10K, doc);
    t1 = doc[KEY_NTC10K_T1] | DEFAULT_T1_10K;
    r1 = doc[KEY_NTC10K_R1] | DEFAULT_R1_10K;
    t2 = doc[KEY_NTC10K_T2] | DEFAULT_T2_10K;
    r2 = doc[KEY_NTC10K_R2] | DEFAULT_R2_10K;
    t3 = doc[KEY_NTC10K_T3] | DEFAULT_T3_10K;
    r3 = doc[KEY_NTC10K_R3] | DEFAULT_R3_10K;
}

void ConfigManager::setNTC10KConfig(double t1, double r1, double t2, double r2, double t3, double r3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_NTC10K, doc);
    doc[KEY_NTC10K_T1] = t1;
    doc[KEY_NTC10K_R1] = r1;
    doc[KEY_NTC10K_T2] = t2;
    doc[KEY_NTC10K_R2] = r2;
    doc[KEY_NTC10K_T3] = t3;
    doc[KEY_NTC10K_R3] = r3;
    writeNamespace(NAMESPACE_NTC10K, doc);
}

void ConfigManager::getConductivityConfig(float& calTemp, float& coefComp, 
                                           float& v1, float& t1, float& v2, float& t2, float& v3, float& t3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_COND, doc);
    calTemp = doc[KEY_CONDUCT_CT] | CONDUCTIVITY_DEFAULT_TEMP;
    coefComp = doc[KEY_CONDUCT_CC] | TEMP_COEF_COMPENSATION;
    v1 = doc[KEY_CONDUCT_V1] | CONDUCTIVITY_DEFAULT_V1;
    t1 = doc[KEY_CONDUCT_T1] | CONDUCTIVITY_DEFAULT_T1;
    v2 = doc[KEY_CONDUCT_V2] | CONDUCTIVITY_DEFAULT_V2;
    t2 = doc[KEY_CONDUCT_T2] | CONDUCTIVITY_DEFAULT_T2;
    v3 = doc[KEY_CONDUCT_V3] | CONDUCTIVITY_DEFAULT_V3;
    t3 = doc[KEY_CONDUCT_T3] | CONDUCTIVITY_DEFAULT_T3;
}

void ConfigManager::setConductivityConfig(float calTemp, float coefComp,
                                           float v1, float t1, float v2, float t2, float v3, float t3) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_COND, doc);
    doc[KEY_CONDUCT_CT] = calTemp;
    doc[KEY_CONDUCT_CC] = coefComp;
    doc[KEY_CONDUCT_V1] = v1;
    doc[KEY_CONDUCT_T1] = t1;
    doc[KEY_CONDUCT_V2] = v2;
    doc[KEY_CONDUCT_T2] = t2;
    doc[KEY_CONDUCT_V3] = v3;
    doc[KEY_CONDUCT_T3] = t3;
    writeNamespace(NAMESPACE_COND, doc);
}

void ConfigManager::getPHConfig(float& v1, float& t1, float& v2, float& t2, float& v3, float& t3, float& defaultTemp) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_PH, doc);
    v1 = doc[KEY_PH_V1] | PH_DEFAULT_V1;
    t1 = doc[KEY_PH_T1] | PH_DEFAULT_T1;
    v2 = doc[KEY_PH_V2] | PH_DEFAULT_V2;
    t2 = doc[KEY_PH_T2] | PH_DEFAULT_T2;
    v3 = doc[KEY_PH_V3] | PH_DEFAULT_V3;
    t3 = doc[KEY_PH_T3] | PH_DEFAULT_T3;
    defaultTemp = doc[KEY_PH_CT] | PH_DEFAULT_TEMP;
}

void ConfigManager::setPHConfig(float v1, float t1, float v2, float t2, float v3, float t3, float defaultTemp) {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_PH, doc);
    doc[KEY_PH_V1] = v1;
    doc[KEY_PH_T1] = t1;
    doc[KEY_PH_V2] = v2;
    doc[KEY_PH_T2] = t2;
    doc[KEY_PH_V3] = v3;
    doc[KEY_PH_T3] = t3;
    doc[KEY_PH_CT] = defaultTemp;
    writeNamespace(NAMESPACE_PH, doc);
}

std::vector<SensorConfig> ConfigManager::getAllSensorConfigs() {
    std::vector<SensorConfig> configs;
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_SENSORS, doc);
    
    if (!doc.is<JsonArray>()) {
        // Si no es un arreglo, no hay nada que leer
        return configs;
    }
    
    JsonArray sensorArray = doc.as<JsonArray>();
    for (JsonObject sensorObj : sensorArray) {
        SensorConfig config;
        const char* cKey = sensorObj[KEY_SENSOR] | "";
        strncpy(config.configKey, cKey, sizeof(config.configKey));
        const char* sensorId = sensorObj[KEY_SENSOR_ID] | "";
        strncpy(config.sensorId, sensorId, sizeof(config.sensorId));
        config.type = static_cast<SensorType>(sensorObj[KEY_SENSOR_TYPE] | 0);
        config.channel = sensorObj[KEY_SENSOR_CHANNEL] | 0;
        const char* tempSensorId = sensorObj[KEY_SENSOR_ID_TEMPERATURE_SENSOR] | "";
        strncpy(config.tempSensorId, tempSensorId, sizeof(config.tempSensorId));
        config.enable = sensorObj[KEY_SENSOR_ENABLE] | false;
        
        configs.push_back(config);
    }
    
    return configs;
}

void ConfigManager::initializeSensorConfigs() {
    Preferences prefs;
    prefs.begin(NAMESPACE_SENSORS, false);
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    JsonArray sensorArray = doc.to<JsonArray>(); // Array raíz

    for (const auto& config : ConfigManager::defaultConfigs) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj[KEY_SENSOR] = config.configKey;
        sensorObj[KEY_SENSOR_ID] = config.sensorId;
        sensorObj[KEY_SENSOR_TYPE] = static_cast<int>(config.type);
        sensorObj[KEY_SENSOR_CHANNEL] = config.channel;
        sensorObj[KEY_SENSOR_ID_TEMPERATURE_SENSOR] = config.tempSensorId;
        sensorObj[KEY_SENSOR_ENABLE] = config.enable;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    // Se guarda usando el mismo nombre del namespace
    prefs.putString(NAMESPACE_SENSORS, jsonString.c_str());
    prefs.end();
}

void ConfigManager::setSensorsConfigs(const std::vector<SensorConfig>& configs) {
    Preferences prefs;
    prefs.begin(NAMESPACE_SENSORS, false);
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    JsonArray sensorArray = doc.to<JsonArray>();
    
    for (const auto& sensor : configs) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj[KEY_SENSOR] = sensor.configKey;
        sensorObj[KEY_SENSOR_ID] = sensor.sensorId;
        sensorObj[KEY_SENSOR_TYPE] = static_cast<int>(sensor.type);
        sensorObj[KEY_SENSOR_ID_TEMPERATURE_SENSOR] = sensor.tempSensorId;
        sensorObj[KEY_SENSOR_ENABLE] = sensor.enable;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    prefs.putString(NAMESPACE_SENSORS, jsonString.c_str());
    prefs.end();
}

LoRaConfig ConfigManager::getLoRaConfig() {
    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_LORAWAN, doc);
    
    LoRaConfig config;
    //FOR OTAA
    config.joinEUI     = doc[KEY_LORA_JOIN_EUI]      | DEFAULT_JOIN_EUI;
    config.devEUI     = doc[KEY_LORA_DEV_EUI]      | DEFAULT_DEV_EUI;
    config.nwkKey     = doc[KEY_LORA_NWK_KEY]      | DEFAULT_NWK_KEY;
    config.appKey     = doc[KEY_LORA_APP_KEY]      | DEFAULT_APP_KEY;
    


    return config;
}

void ConfigManager::setLoRaConfig(
    const String &joinEUI, 
    const String &devEUI, 
    const String &nwkKey, 
    const String &appKey) {

    StaticJsonDocument<JSON_DOC_SIZE> doc;
    readNamespace(NAMESPACE_LORAWAN, doc);
    //FOR OTAA
    doc[KEY_LORA_JOIN_EUI]      = joinEUI;
    doc[KEY_LORA_DEV_EUI]       = devEUI;
    doc[KEY_LORA_NWK_KEY]      = nwkKey;
    doc[KEY_LORA_APP_KEY]      = appKey;
    writeNamespace(NAMESPACE_LORAWAN, doc);
}
