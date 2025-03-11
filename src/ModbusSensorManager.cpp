#include "ModbusSensorManager.h"
#include "ModbusMaster.h"
#include "config.h"    // Para SERIAL_BAUD
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
    
    // Asegurarse de dar un pequeño delay
    delay(MODBUS_INTER_FRAME_DELAY);
}

bool ModbusSensorManager::readHoldingRegisters(uint8_t address, uint16_t startReg, uint16_t numRegs, uint16_t* outData) {
    // Guardar los bytes recibidos para depuración posterior
    uint8_t result;
    
    // No hacer ningún Serial.print durante la comunicación Modbus
    Serial.begin(9600, SERIAL_8N1);

    
    // Establecer el slave ID
    modbus.begin(address, Serial);
    
    // Realizar la petición Modbus para leer registros holding
    result = modbus.readHoldingRegisters(startReg, numRegs);
    
    // Verificar si la lectura fue exitosa
    if (result == modbus.ku8MBSuccess) {
        // Extraer datos de los registros
        for (uint8_t i = 0; i < numRegs; i++) {
            outData[i] = modbus.getResponseBuffer(i);
        }
        
        return true;
    } else {
        // Esperar para que el sistema se estabilice antes de debug
        delay(5);
        
        // // Imprimir código de error
        // Serial.print("Error Modbus: ");
        // Serial.println(result);
        return false;
    }
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

    // Definir las claves para cada valor
    const char* keys[] = {"H", "T", "Noise", "PM2.5", "PM10", "P", "Lux"};
    
    bool ok = readHoldingRegisters(cfg.address, startReg, numRegs, rawData);
    if (!ok) {
        // Llenar con NAN si fallo, pero usando las claves correctas
        for (int i=0; i<7; i++){
            SubValue sv;
            strncpy(sv.key, keys[i], sizeof(sv.key));
            sv.value = NAN;
            reading.subValues.push_back(sv);
        }
        return reading;
    }

    // Extraer segun datasheet:
    // rawData[0] = Humedad entera *10
    // rawData[1] = Temp entera *10 (16 bits con posible signo)
    // rawData[2] = Ruido *10
    // rawData[3] = PM2.5
    // rawData[4] = PM10
    // rawData[5] = Presion *10 (en kPa)
    // rawData[6] = LuxHigh (32 bits -> parte alta)
    // rawData[7] = LuxLow  (32 bits -> parte baja)

    // Humedad
    {
        SubValue sH;
        strncpy(sH.key, keys[0], sizeof(sH.key)); 
        sH.value = rawData[0] / 10.0f;
        reading.subValues.push_back(sH);
    }
    // Temperatura
    {
        SubValue sT;
        strncpy(sT.key, keys[1], sizeof(sT.key));
        // Ver si es negativo
        int16_t temp16 = (int16_t)rawData[1];
        sT.value = temp16 / 10.0f;
        reading.subValues.push_back(sT);
    }
    // Ruido
    {
        SubValue sN;
        strncpy(sN.key, keys[2], sizeof(sN.key));
        sN.value = rawData[2] / 10.0f; 
        reading.subValues.push_back(sN);
    }
    // PM2.5
    {
        SubValue sPM25;
        strncpy(sPM25.key, keys[3], sizeof(sPM25.key));
        sPM25.value = (float)rawData[3]; 
        reading.subValues.push_back(sPM25);
    }
    // PM10
    {
        SubValue sPM10;
        strncpy(sPM10.key, keys[4], sizeof(sPM10.key));
        sPM10.value = (float)rawData[4]; 
        reading.subValues.push_back(sPM10);
    }
    // Presion
    {
        SubValue sP;
        strncpy(sP.key, keys[5], sizeof(sP.key));
        sP.value = rawData[5] / 10.0f;  // kPa
        reading.subValues.push_back(sP);
    }
    // Lux
    {
        uint32_t luxHigh = rawData[6];
        uint32_t luxLow  = rawData[7];
        uint32_t fullLux = (luxHigh << 16) | luxLow; 
        SubValue sL;
        strncpy(sL.key, keys[6], sizeof(sL.key));
        sL.value = (float)fullLux;
        reading.subValues.push_back(sL);
    }

    return reading;
}

// Ya no necesitamos esta función, usaremos el CRC de ModbusMaster
// uint16_t ModbusSensorManager::modbusCRC(const uint8_t* data, size_t length) {
//     // Función eliminada - Ahora usamos ModbusMaster
// }
