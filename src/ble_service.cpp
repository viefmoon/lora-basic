#include "ble_service.h"
#include "config.h"
#include "ble_config_callbacks.h"

// Implementación de la función para configurar el servicio BLE y sus características
BLEService* setupBLEService(BLEServer* pServer) {
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