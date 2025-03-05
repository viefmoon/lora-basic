#include "ble_service.h"
#include "config.h"
#include "ble_config_callbacks.h"

// Implementación de la función para configurar el servicio BLE y sus características
BLEService* setupBLEService(BLEServer* pServer) {
    // Crear el servicio de configuración utilizando el UUID definido
    BLEService* pService = pServer->createService(BLEUUID(BLE_SERVICE_UUID));

    // Característica del sistema
    BLECharacteristic* pSystemChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_SYSTEM_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pSystemChar->setCallbacks(new SystemConfigCallback());

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
    
    // Característica para configuración de Sensores
    BLECharacteristic* pSensorsChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_SENSORS_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pSensorsChar->setCallbacks(new SensorsConfigCallback());
    
    // Característica para configuración de LoRa
    BLECharacteristic* pLoRaConfigChar = pService->createCharacteristic(
        BLEUUID(BLE_CHAR_LORA_CONFIG_UUID),
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
    );
    pLoRaConfigChar->setCallbacks(new LoRaConfigCallback());
    
    pService->start();
    return pService;
} 