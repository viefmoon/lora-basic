#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <BLEDevice.h>
#include <BLEServer.h>

// Declaración de la función para configurar el servicio BLE
BLEService* setupBLEService(BLEServer* pServer);

#endif // BLE_SERVICE_H