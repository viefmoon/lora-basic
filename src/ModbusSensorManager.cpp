#include "ModbusSensorManager.h"
#include "config.h"    // Para SERIAL_BAUD

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)

#include "ModbusMaster.h"
#include "debug.h"     // Para DEBUG_END
#include "sensor_types.h" // Para todos los tipos y constantes de sensores
#include "utilities.h"
#include <string.h>

// Crear una instancia global de ModbusMaster
ModbusMaster modbus;

/**
 * @note 
 *  - Se usa la biblioteca ModbusMaster para la comunicación Modbus
 *  - Se asume que el pin DE/RE del transceiver RS485 está atado de forma que cuando
 *    se escribe en Serial, se habilita la transmisión, y al terminar, pasa a recepción.
 */

void ModbusSensorManager::beginModbus() {
    // Configurar Serial usando los parámetros definidos en config.h
    Serial.begin(MODBUS_BAUDRATE, MODBUS_SERIAL_CONFIG);
    
    // Inicializar ModbusMaster
    modbus.begin(0, Serial); // El slave ID se configurará en cada petición
}

void ModbusSensorManager::endModbus() {
    // Finalizar la comunicación Serial de Modbus
    Serial.end();
}

bool ModbusSensorManager::readHoldingRegisters(uint8_t address, uint16_t startReg, uint16_t numRegs, uint16_t* outData) {
    // Guardar los bytes recibidos para depuración posterior
    uint8_t result;
    
    // Establecer el slave ID
    modbus.begin(address, Serial);
    
    // Implementar reintentos de lectura
    for (uint8_t retry = 0; retry < MODBUS_MAX_RETRY; retry++) {
        // Registrar el tiempo de inicio para implementar timeout manual
        uint32_t startTime = millis();
        
        // Realizar la petición Modbus para leer registros holding
        result = modbus.readHoldingRegisters(startReg, numRegs);
        
        // Verificar si la lectura fue exitosa
        if (result == modbus.ku8MBSuccess) {
            // Extraer datos de los registros
            for (uint8_t i = 0; i < numRegs; i++) {
                outData[i] = modbus.getResponseBuffer(i);
            }
            
            return true;
        }
        
        // Verificar si se agotó el tiempo (timeout personalizado)
        if ((millis() - startTime) >= MODBUS_RESPONSE_TIMEOUT) {
            DEBUG_PRINTLN("Timeout en comunicación Modbus");
            break; // Salir del bucle de reintentos si se agota el tiempo
        }
        
        // Si no es el último intento, continuar con el siguiente intento
        DEBUG_PRINTF("Intento %d fallido, código: %d\n", retry + 1, result);
    }
    
    // Si llegamos aquí, todos los intentos fallaron
    DEBUG_PRINTF("Error Modbus después de %d intentos\n", MODBUS_MAX_RETRY);
    return false;
}

ModbusSensorReading ModbusSensorManager::readEnvSensor(const ModbusSensorConfig &cfg) {
    ModbusSensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId));
    reading.type = cfg.type;
    reading.subValues.clear();

    // Lectura de 8 registros (500..507)
    const uint16_t startReg = 500;
    const uint16_t numRegs = 8;
    uint16_t rawData[8];
    memset(rawData, 0, sizeof(rawData));
    
    bool ok = readHoldingRegisters(cfg.address, startReg, numRegs, rawData);
    if (!ok) {
        // Llenar con NAN si falló
        // Respetamos el orden: [0]=Humedad, [1]=Temperatura, [2]=Presión, [3]=Iluminación
        for (int i=0; i<4; i++){
            SubValue sv;
            sv.value = NAN;
            reading.subValues.push_back(sv);
        }
        return reading;
    }

    // Extraer según datasheet (versión 4 en 1):
    // rawData[0] = Humedad entera *10
    // rawData[1] = Temp entera *10 (16 bits con posible signo)
    // rawData[5] = Presión *10 (en kPa)
    // rawData[6] = LuxHigh (32 bits -> parte alta)
    // rawData[7] = LuxLow  (32 bits -> parte baja)
    // Los demás registros (ruido, PM2.5, PM10) vienen en 0 y se ignoran

    // Agregar Humedad como primer valor [0]
    {
        SubValue sH;
        sH.value = rawData[0] / 10.0f;
        reading.subValues.push_back(sH);
    }
    
    // Agregar Temperatura como segundo valor [1]
    {
        SubValue sT;
        // Ver si es negativo
        int16_t temp16 = (int16_t)rawData[1];
        sT.value = temp16 / 10.0f;
        reading.subValues.push_back(sT);
    }
    
    // Agregar Presión Atmosférica como tercer valor [2]
    {
        SubValue sP;
        sP.value = rawData[5] / 10.0f;
        reading.subValues.push_back(sP);
    }
    
    // Agregar Iluminación como cuarto valor [3]
    {
        SubValue sL;
        uint32_t lux = ((uint32_t)rawData[6] << 16) | rawData[7];
        sL.value = (float)lux;
        reading.subValues.push_back(sL);
    }
    
    return reading;
}

#endif // defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)