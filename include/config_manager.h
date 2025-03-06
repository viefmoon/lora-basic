#pragma once
#include <Preferences.h>
#include <vector>
#include <Arduino.h>  // Se incluye para utilizar el tipo String
#include "sensor_types.h"
#include <RadioLib.h> // Añadido para RADIOLIB_LORAWAN_SESSION_BUF_SIZE
#include "config.h"

// Definición de la estructura para la configuración de LoRa
struct LoRaConfig {
    //FOR OTAA
    String joinEUI;
    String devEUI;
    String nwkKey;
    String appKey;
};

class ConfigManager {
public:
    // Banderas de inicialización
    static bool checkInitialized();
    static void initializeDefaultConfig();
    
    // Getters y setters para calibración NTC - Solo para dispositivo analógico
    static void getNTC100KConfig(double& t1, double& r1, double& t2, double& r2, double& t3, double& r3);
    static void setNTC100KConfig(double t1, double r1, double t2, double r2, double t3, double r3);
    static void getNTC10KConfig(double& t1, double& r1, double& t2, double& r2, double& t3, double& r3);
    static void setNTC10KConfig(double t1, double r1, double t2, double r2, double t3, double r3);
    
    // Getters y setters para calibración de conductividad - Solo para dispositivo analógico
    static void getConductivityConfig(float& calTemp, float& coefComp, 
                                    float& v1, float& t1, float& v2, float& t2, float& v3, float& t3);
    static void setConductivityConfig(float calTemp, float coefComp,
                                    float v1, float t1, float v2, float t2, float v3, float t3);
    
    // Getters y setters para calibración de pH - Solo para dispositivo analógico
    static void getPHConfig(float& v1, float& t1, float& v2, float& t2, float& v3, float& t3, float& defaultTemp);
    static void setPHConfig(float v1, float t1, float v2, float t2, float v3, float t3, float defaultTemp);

    // Métodos para configuración de sensores - Común para todos los tipos
    static void setSensorsConfigs(const std::vector<SensorConfig>& configs);
    static std::vector<SensorConfig> getAllSensorConfigs();
    static std::vector<SensorConfig> getEnabledSensorConfigs();
    static void initializeSensorConfigs();

    // Métodos para la configuración de LoRa - Común para todos los tipos
    static LoRaConfig getLoRaConfig();
    static void setLoRaConfig(
        const String &joinEUI, 
        const String &devEUI, 
        const String &nwkKey, 
        const String &appKey);
    
    // Métodos unificados para la configuración de sistema - Común para todos los tipos
    static void getSystemConfig(bool &initialized, uint32_t &sleepTime, String &deviceId, String &stationId);
    static void setSystemConfig(bool initialized, uint32_t sleepTime, const String &deviceId, const String &stationId);

private:
    static const SensorConfig defaultConfigs[];
};