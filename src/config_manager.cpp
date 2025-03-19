#include "config_manager.h"
#include <ArduinoJson.h>
#include <vector>
#include "config.h"
#include "sensor_types.h"
#include <Preferences.h>
#include <Arduino.h> // Incluido para usar Serial

/* =========================================================================
   FUNCIONES AUXILIARES
   ========================================================================= */
// Funciones auxiliares para leer y escribir el JSON completo en cada namespace.
static void writeNamespace(const char* ns, const StaticJsonDocument<JSON_DOC_SIZE_MEDIUM>& doc) {
    Preferences prefs;
    prefs.begin(ns, false);
    String jsonString;
    serializeJson(doc, jsonString);
    // Se usa el mismo nombre del namespace como clave interna
    prefs.putString(ns, jsonString.c_str());
    prefs.end();
}

static void readNamespace(const char* ns, StaticJsonDocument<JSON_DOC_SIZE_MEDIUM>& doc) {
    Preferences prefs;
    prefs.begin(ns, true);
    String jsonString = prefs.getString(ns, "{}");
    prefs.end();
    deserializeJson(doc, jsonString);
}

// Configuración por defecto de sensores NO-modbus
const SensorConfig ConfigManager::defaultConfigs[] = DEFAULT_SENSOR_CONFIGS;

/* =========================================================================
   INICIALIZACIÓN Y CONFIGURACIÓN DEL SISTEMA
   ========================================================================= */
bool ConfigManager::checkInitialized() {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    return doc[KEY_INITIALIZED] | false;
}

void ConfigManager::initializeDefaultConfig() {
    /* -------------------------------------------------------------------------
       1. INICIALIZACIÓN DE CONFIGURACIÓN DEL SISTEMA
       ------------------------------------------------------------------------- */
    // Sistema unificado: NAMESPACE_SYSTEM (incluye system, sleep y device)
    // Común para todos los tipos de dispositivo
    {
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
        doc[KEY_STATION_ID] = DEFAULT_STATION_ID;
        doc[KEY_INITIALIZED] = true;
        doc[KEY_SLEEP_TIME] = DEFAULT_TIME_TO_SLEEP;
        doc[KEY_DEVICE_ID] = DEFAULT_DEVICE_ID;
        writeNamespace(NAMESPACE_SYSTEM, doc);
    }
    
#ifdef DEVICE_TYPE_ANALOGIC
    /* -------------------------------------------------------------------------
       2. INICIALIZACIÓN DE SENSORES ANALÓGICOS (Solo para dispositivo analógico)
       ------------------------------------------------------------------------- */
    // NTC 100K: NAMESPACE_NTC100K
    {
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
        doc[KEY_PH_V1] = PH_DEFAULT_V1;
        doc[KEY_PH_T1] = PH_DEFAULT_T1;
        doc[KEY_PH_V2] = PH_DEFAULT_V2;
        doc[KEY_PH_T2] = PH_DEFAULT_T2;
        doc[KEY_PH_V3] = PH_DEFAULT_V3;
        doc[KEY_PH_T3] = PH_DEFAULT_T3;
        doc[KEY_PH_CT] = PH_DEFAULT_TEMP;
        writeNamespace(NAMESPACE_PH, doc);
    }
#endif
    
    /* -------------------------------------------------------------------------
       3. INICIALIZACIÓN DE SENSORES NO-MODBUS
       ------------------------------------------------------------------------- */
    {
        Preferences prefs;
        prefs.begin(NAMESPACE_SENSORS, false);
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
        JsonArray sensorArray = doc.to<JsonArray>(); // Array raíz

        for (const auto& config : ConfigManager::defaultConfigs) {
            JsonObject sensorObj = sensorArray.createNestedObject();
            sensorObj[KEY_SENSOR] = config.configKey;
            sensorObj[KEY_SENSOR_ID] = config.sensorId;
            sensorObj[KEY_SENSOR_TYPE] = static_cast<int>(config.type);
            sensorObj[KEY_SENSOR_ENABLE] = config.enable;
        }
        
        String jsonString;
        serializeJson(doc, jsonString);
        prefs.putString(NAMESPACE_SENSORS, jsonString.c_str());
        prefs.end();
    }
    
    /* -------------------------------------------------------------------------
       4. INICIALIZACIÓN DE CONFIGURACIÓN DE LORA
       ------------------------------------------------------------------------- */
    {
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
        doc[KEY_LORA_JOIN_EUI]      = DEFAULT_JOIN_EUI;
        doc[KEY_LORA_DEV_EUI]       = DEFAULT_DEV_EUI;
        doc[KEY_LORA_NWK_KEY]       = DEFAULT_NWK_KEY;
        doc[KEY_LORA_APP_KEY]       = DEFAULT_APP_KEY;
        writeNamespace(NAMESPACE_LORAWAN, doc);
    }

#if defined(DEVICE_TYPE_MODBUS) || defined(DEVICE_TYPE_ANALOGIC)
    /* -------------------------------------------------------------------------
       5. INICIALIZACIÓN DE SENSORES MODBUS
       ------------------------------------------------------------------------- */
    {
        Preferences prefs;
        prefs.begin(NAMESPACE_SENSORS_MODBUS, false);
        StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
        JsonArray sensorArray = doc.to<JsonArray>(); 

        // Cargamos un default (definido en config.h)
        static const ModbusSensorConfig defaultModbusSensors[] = DEFAULT_MODBUS_SENSOR_CONFIGS;
        size_t count = sizeof(defaultModbusSensors)/sizeof(defaultModbusSensors[0]);

        for (size_t i = 0; i < count; i++) {
            JsonObject sensorObj = sensorArray.createNestedObject();
            sensorObj[KEY_MODBUS_SENSOR_ID]    = defaultModbusSensors[i].sensorId;
            sensorObj[KEY_MODBUS_SENSOR_TYPE]  = (int)defaultModbusSensors[i].type;
            sensorObj[KEY_MODBUS_SENSOR_ADDR] = defaultModbusSensors[i].address;
            sensorObj[KEY_MODBUS_SENSOR_ENABLE]   = defaultModbusSensors[i].enable;
        }
        
        String jsonString;
        serializeJson(doc, jsonString);
        prefs.putString(NAMESPACE_SENSORS_MODBUS, jsonString.c_str());
        prefs.end();
    }
#endif
}

void ConfigManager::getSystemConfig(bool &initialized, uint32_t &sleepTime, String &deviceId, String &stationId) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    initialized = doc[KEY_INITIALIZED] | false;
    sleepTime = doc[KEY_SLEEP_TIME] | DEFAULT_TIME_TO_SLEEP;
    deviceId = String(doc[KEY_DEVICE_ID] | DEFAULT_DEVICE_ID);
    stationId = String(doc[KEY_STATION_ID] | DEFAULT_STATION_ID);
}

void ConfigManager::setSystemConfig(bool initialized, uint32_t sleepTime, const String &deviceId, const String &stationId) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_SYSTEM, doc);
    doc[KEY_INITIALIZED] = initialized;
    doc[KEY_SLEEP_TIME] = sleepTime;
    doc[KEY_DEVICE_ID] = deviceId;
    doc[KEY_STATION_ID] = stationId;
    writeNamespace(NAMESPACE_SYSTEM, doc);
}

/* =========================================================================
   CONFIGURACIÓN DE SENSORES NO-MODBUS
   ========================================================================= */
std::vector<SensorConfig> ConfigManager::getAllSensorConfigs() {
    std::vector<SensorConfig> configs;
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
        config.enable = sensorObj[KEY_SENSOR_ENABLE] | false;
        
        configs.push_back(config);
    }
    
    return configs;
}

std::vector<SensorConfig> ConfigManager::getEnabledSensorConfigs() {
    std::vector<SensorConfig> allSensors = getAllSensorConfigs();
    
    std::vector<SensorConfig> enabledSensors;
    for (const auto& sensor : allSensors) {
        if (sensor.enable && strlen(sensor.sensorId) > 0) {
            enabledSensors.push_back(sensor);
        }
    }
    
    return enabledSensors;
}

void ConfigManager::setSensorsConfigs(const std::vector<SensorConfig>& configs) {
    Preferences prefs;
    prefs.begin(NAMESPACE_SENSORS, false);
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    JsonArray sensorArray = doc.to<JsonArray>();
    
    for (const auto& sensor : configs) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj[KEY_SENSOR] = sensor.configKey;
        sensorObj[KEY_SENSOR_ID] = sensor.sensorId;
        sensorObj[KEY_SENSOR_TYPE] = static_cast<int>(sensor.type);
        sensorObj[KEY_SENSOR_ENABLE] = sensor.enable;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    prefs.putString(NAMESPACE_SENSORS, jsonString.c_str());
    prefs.end();
}

/* =========================================================================
   CONFIGURACIÓN DE LORA
   ========================================================================= */
LoRaConfig ConfigManager::getLoRaConfig() {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_LORAWAN, doc);
    
    LoRaConfig config;
    config.joinEUI  = doc[KEY_LORA_JOIN_EUI] | DEFAULT_JOIN_EUI;
    config.devEUI   = doc[KEY_LORA_DEV_EUI]  | DEFAULT_DEV_EUI;
    config.nwkKey   = doc[KEY_LORA_NWK_KEY]  | DEFAULT_NWK_KEY;
    config.appKey   = doc[KEY_LORA_APP_KEY]  | DEFAULT_APP_KEY;
    
    return config;
}

void ConfigManager::setLoRaConfig(
    const String &joinEUI, 
    const String &devEUI, 
    const String &nwkKey, 
    const String &appKey) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_LORAWAN, doc);
    
    doc[KEY_LORA_JOIN_EUI] = joinEUI;
    doc[KEY_LORA_DEV_EUI]  = devEUI;
    doc[KEY_LORA_NWK_KEY]  = nwkKey;
    doc[KEY_LORA_APP_KEY]  = appKey;
    
    writeNamespace(NAMESPACE_LORAWAN, doc);
}

/* =========================================================================
   CONFIGURACIÓN DE SENSORES MODBUS
   ========================================================================= */
#if defined(DEVICE_TYPE_MODBUS) || defined(DEVICE_TYPE_ANALOGIC)
// Definición de la variable estática
const ModbusSensorConfig ConfigManager::defaultModbusSensors[] = DEFAULT_MODBUS_SENSOR_CONFIGS;

void ConfigManager::setModbusSensorsConfigs(const std::vector<ModbusSensorConfig>& configs) {
    Preferences prefs;
    prefs.begin(NAMESPACE_SENSORS_MODBUS, false);
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    JsonArray sensorArray = doc.to<JsonArray>();
    
    for (const auto& sensor : configs) {
        JsonObject sensorObj = sensorArray.createNestedObject();
        sensorObj[KEY_MODBUS_SENSOR_ID] = sensor.sensorId;
        sensorObj[KEY_MODBUS_SENSOR_TYPE] = static_cast<int>(sensor.type);
        sensorObj[KEY_MODBUS_SENSOR_ADDR] = sensor.address;
        sensorObj[KEY_MODBUS_SENSOR_ENABLE] = sensor.enable;
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    prefs.putString(NAMESPACE_SENSORS_MODBUS, jsonString.c_str());
    prefs.end();
}

std::vector<ModbusSensorConfig> ConfigManager::getAllModbusSensorConfigs() {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_SENSORS_MODBUS, doc);
    
    std::vector<ModbusSensorConfig> configs;
    
    if (doc.is<JsonArray>()) {
        JsonArray array = doc.as<JsonArray>();
        
        for (JsonObject sensorObj : array) {
            ModbusSensorConfig config;
            strlcpy(config.sensorId, sensorObj[KEY_MODBUS_SENSOR_ID] | "", sizeof(config.sensorId));
            config.type = static_cast<SensorType>(sensorObj[KEY_MODBUS_SENSOR_TYPE] | 0);
            config.address = sensorObj[KEY_MODBUS_SENSOR_ADDR] | 1;
            config.enable = sensorObj[KEY_MODBUS_SENSOR_ENABLE] | false;
            
            configs.push_back(config);
        }
    }
    
    return configs;
}

std::vector<ModbusSensorConfig> ConfigManager::getEnabledModbusSensorConfigs() {
    std::vector<ModbusSensorConfig> all = getAllModbusSensorConfigs();
    std::vector<ModbusSensorConfig> enabled;
    for (auto &m : all) {
        if (m.enable) {
            enabled.push_back(m);
        }
    }
    return enabled;
}
#endif

/* =========================================================================
   CONFIGURACIÓN DE SENSORES ANALÓGICOS (Solo para dispositivo analógico)
   ========================================================================= */
#ifdef DEVICE_TYPE_ANALOGIC

void ConfigManager::getNTC100KConfig(double& t1, double& r1, double& t2, double& r2, double& t3, double& r3) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_NTC100K, doc);
    t1 = doc[KEY_NTC100K_T1] | DEFAULT_T1_100K;
    r1 = doc[KEY_NTC100K_R1] | DEFAULT_R1_100K;
    t2 = doc[KEY_NTC100K_T2] | DEFAULT_T2_100K;
    r2 = doc[KEY_NTC100K_R2] | DEFAULT_R2_100K;
    t3 = doc[KEY_NTC100K_T3] | DEFAULT_T3_100K;
    r3 = doc[KEY_NTC100K_R3] | DEFAULT_R3_100K;
}

void ConfigManager::setNTC100KConfig(double t1, double r1, double t2, double r2, double t3, double r3) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
    readNamespace(NAMESPACE_NTC10K, doc);
    t1 = doc[KEY_NTC10K_T1] | DEFAULT_T1_10K;
    r1 = doc[KEY_NTC10K_R1] | DEFAULT_R1_10K;
    t2 = doc[KEY_NTC10K_T2] | DEFAULT_T2_10K;
    r2 = doc[KEY_NTC10K_R2] | DEFAULT_R2_10K;
    t3 = doc[KEY_NTC10K_T3] | DEFAULT_T3_10K;
    r3 = doc[KEY_NTC10K_R3] | DEFAULT_R3_10K;
}

void ConfigManager::setNTC10KConfig(double t1, double r1, double t2, double r2, double t3, double r3) {
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
    StaticJsonDocument<JSON_DOC_SIZE_MEDIUM> doc;
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
#endif
