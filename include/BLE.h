#ifndef BLE_H
#define BLE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include <BLECharacteristic.h>
#include <ArduinoJson.h>
#include "clsPCA9555.h"
#include "config.h"
#include "config_manager.h"
#include "debug.h"

// Clase para gestionar toda la funcionalidad BLE
class BLEHandler {
public:
    // Variables para control de estado BLE
    static bool isConnected;
    static unsigned long connectionStartTime;
    static const unsigned long connectionTimeout = CONFIG_BLE_MAX_CONN_TIME; // Usar constante de config.h
    static BLEServer* pBLEServer; // Referencia global al servidor BLE
    static bool shouldExitOnDisconnect; // Indica si debemos salir del modo configuración al desconectar
    
    /**
     * @brief Verifica si se mantuvo presionado el botón de configuración y activa el modo BLE.
     * @param ioExpander Referencia al expansor de I/O para controlar el LED
     * @return true si se activó el modo BLE, false en caso contrario
     */
    static bool checkConfigMode(PCA9555& ioExpander);

    /**
     * @brief Inicializa el BLE con el nombre del dispositivo basado en el devEUI
     * @param devEUI Identificador único del dispositivo
     * @return Puntero al servidor BLE creado
     */
    static BLEServer* initBLE(const String& devEUI);

    /**
     * @brief Configura los servicios y características BLE
     * @param pServer Servidor BLE donde se configurarán los servicios
     * @return Puntero al servicio BLE creado
     */
    static BLEService* setupService(BLEServer* pServer);

    /**
     * @brief Ejecuta el bucle de parpadeo del LED en modo configuración
     * @param ioExpander Referencia al expansor de I/O para controlar el LED
     */
    static void runConfigLoop(PCA9555& ioExpander);

private:
    // Callback para eventos del servidor BLE
    class ServerCallbacks: public BLEServerCallbacks {
    public:
        void onConnect(BLEServer* pServer) override;
        void onDisconnect(BLEServer* pServer) override;
    };

    // Callback para configuración del sistema
    class SystemConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };

#ifdef DEVICE_TYPE_ANALOGIC
    // Callback para NTC 100K
    class NTC100KConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };

    // Callback para NTC 10K
    class NTC10KConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };

    // Callback para Conductividad
    class ConductivityConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };

    // Callback para pH
    class PHConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };
#endif

    // Callback para configuración de sensores
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)
    class SensorsConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };
#endif

    // Callback para configuración de LoRa
    class LoRaConfigCallback: public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic *pCharacteristic) override;
        void onRead(BLECharacteristic *pCharacteristic) override;
    };
};

#endif // BLE_H 