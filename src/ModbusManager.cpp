#include "ModbusManager.h"
#include "PowerManager.h"

// Referencia al objeto PowerManager global
extern PowerManager powerManager;

// Puerto serial para Modbus
#define MODBUS_SERIAL Serial1

// Constantes para el protocolo Modbus
#define MODBUS_READ_HOLDING_REGISTERS 0x03
#define MODBUS_TIMEOUT 1000  // Timeout en ms

// Inicializa el puerto serial para comunicación Modbus
void ModbusManager::begin(unsigned long baudRate) {
    // Encender la alimentación de 12V para los sensores RS485
    powerManager.power12VOn();
    
    // Inicializar el puerto serial para Modbus
    MODBUS_SERIAL.begin(baudRate, SERIAL_8N1);
    
    // Pequeña pausa para estabilización
    delay(100);
    
    // Limpiar cualquier dato residual en el buffer
    flushSerialBuffer();
}

// Lee un sensor Modbus 4-en-1 en la dirección especificada
Sensor4in1Data ModbusManager::readSensor4in1(uint8_t deviceAddress) {
    Sensor4in1Data result = {0};
    
    // Comando para leer 8 registros desde la dirección 500 (0x01F4)
    // [Dirección][Función][Dir. Alta][Dir. Baja][Cant. Alta][Cant. Baja][CRC Bajo][CRC Alto]
    uint8_t command[8] = {
        deviceAddress,
        MODBUS_READ_HOLDING_REGISTERS,
        0x01, 0xF4,  // Dirección inicial 500 (0x01F4)
        0x00, 0x08,  // Cantidad de registros (8)
        0x00, 0x00   // Placeholder para CRC
    };
    
    // Calcular CRC y añadirlo al comando
    uint16_t crc = calculateCRC16(command, 6);
    command[6] = crc & 0xFF;         // Byte bajo del CRC
    command[7] = (crc >> 8) & 0xFF;  // Byte alto del CRC
    
    // Buffer para la respuesta
    // [Dirección][Función][Bytes][Datos...][CRC Bajo][CRC Alto]
    uint8_t response[30];
    
    // Enviar comando y recibir respuesta
    uint8_t responseLength = sendModbusCommand(command, 8, response, sizeof(response));
    
    // Verificar si la respuesta es válida
    if (responseLength >= 21) {  // Mínimo: 1(addr) + 1(func) + 1(bytes) + 16(data) + 2(crc)
        // Verificar dirección y función
        if (response[0] == deviceAddress && response[1] == MODBUS_READ_HOLDING_REGISTERS) {
            // Verificar CRC
            uint16_t receivedCrc = (response[responseLength-1] << 8) | response[responseLength-2];
            uint16_t calculatedCrc = calculateCRC16(response, responseLength-2);
            
            if (receivedCrc == calculatedCrc) {
                // Extraer datos
                // Registro 500: Humedad (multiplicado por 10)
                result.humidity = ((response[3] << 8) | response[4]) / 10.0f;
                
                // Registro 501: Temperatura (multiplicado por 10)
                result.temperature = ((response[5] << 8) | response[6]) / 10.0f;
                
                // Registro 502: Ruido (en caso de estar disponible)
                result.noise = ((response[7] << 8) | response[8]) / 10.0f;
                
                // Registro 503: PM2.5 (en caso de estar disponible)
                result.pm25 = ((response[9] << 8) | response[10]);
                
                // Registro 504: PM10 (en caso de estar disponible)
                result.pm10 = ((response[11] << 8) | response[12]);
                
                // Registro 505: Presión barométrica (kPa, multiplicado por 10)
                result.pressure = ((response[13] << 8) | response[14]) / 10.0f;
                
                // Registros 506-507: Iluminación (lux)
                uint32_t illumHigh = (response[15] << 8) | response[16];
                uint32_t illumLow = (response[17] << 8) | response[18];
                result.illuminance = (illumHigh << 16) | illumLow;
                
                // Imprimir los datos en el puerto serial para depuración
                Serial.println("Datos del sensor Modbus 4-en-1:");
                Serial.print("Humedad: "); Serial.print(result.humidity); Serial.println(" %");
                Serial.print("Temperatura: "); Serial.print(result.temperature); Serial.println(" °C");
                Serial.print("Ruido: "); Serial.print(result.noise); Serial.println(" dB");
                Serial.print("PM2.5: "); Serial.print(result.pm25); Serial.println(" µg/m³");
                Serial.print("PM10: "); Serial.print(result.pm10); Serial.println(" µg/m³");
                Serial.print("Presión: "); Serial.print(result.pressure); Serial.println(" kPa");
                Serial.print("Iluminación: "); Serial.print(result.illuminance); Serial.println(" lux");
            } else {
                Serial.println("Error: CRC inválido en la respuesta Modbus");
            }
        } else {
            Serial.println("Error: Dirección o función incorrecta en la respuesta Modbus");
        }
    } else {
        Serial.println("Error: Respuesta Modbus demasiado corta o no recibida");
    }
    
    return result;
}

// Convierte los datos del sensor 4-en-1 a lecturas individuales
std::vector<SensorReading> ModbusManager::convertToSensorReadings(const Sensor4in1Data& data, uint8_t deviceAddress) {
    std::vector<SensorReading> readings;
    SensorReading reading;
    
    // Temperatura
    sprintf(reading.sensorId, "MB_T_%d", deviceAddress);
    reading.type = MB_TEMP;
    reading.value = data.temperature;
    readings.push_back(reading);
    
    // Humedad
    sprintf(reading.sensorId, "MB_H_%d", deviceAddress);
    reading.type = MB_HUM;
    reading.value = data.humidity;
    readings.push_back(reading);
    
    // Solo agregar lecturas con valores válidos (distintos de cero)
    if (data.noise > 0) {
        sprintf(reading.sensorId, "MB_N_%d", deviceAddress);
        reading.type = MB_NOISE;
        reading.value = data.noise;
        readings.push_back(reading);
    }
    
    if (data.pm25 > 0) {
        sprintf(reading.sensorId, "MB_P25_%d", deviceAddress);
        reading.type = MB_PM25;
        reading.value = data.pm25;
        readings.push_back(reading);
    }
    
    if (data.pm10 > 0) {
        sprintf(reading.sensorId, "MB_P10_%d", deviceAddress);
        reading.type = MB_PM10;
        reading.value = data.pm10;
        readings.push_back(reading);
    }
    
    if (data.pressure > 0) {
        sprintf(reading.sensorId, "MB_PR_%d", deviceAddress);
        reading.type = MB_PRES;
        reading.value = data.pressure;
        readings.push_back(reading);
    }
    
    if (data.illuminance > 0) {
        sprintf(reading.sensorId, "MB_IL_%d", deviceAddress);
        reading.type = MB_ILLUM;
        reading.value = data.illuminance;
        readings.push_back(reading);
    }
    
    return readings;
}

// Calcula el CRC16 para Modbus
uint16_t ModbusManager::calculateCRC16(uint8_t* data, uint8_t length) {
    uint16_t crc = 0xFFFF;
    
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

// Envía un comando Modbus y recibe la respuesta
uint8_t ModbusManager::sendModbusCommand(uint8_t* command, uint8_t commandLength, 
                                        uint8_t* response, uint8_t maxResponseLength) {
    // Limpiar buffer antes de enviar
    flushSerialBuffer();
    
    // Enviar comando
    MODBUS_SERIAL.write(command, commandLength);
    MODBUS_SERIAL.flush();
    
    // Esperar respuesta con timeout
    unsigned long startTime = millis();
    uint8_t bytesReceived = 0;
    
    while ((millis() - startTime) < MODBUS_TIMEOUT && bytesReceived < maxResponseLength) {
        if (MODBUS_SERIAL.available()) {
            response[bytesReceived++] = MODBUS_SERIAL.read();
            startTime = millis();  // Reiniciar timeout después de recibir un byte
        }
        yield();  // Permitir que el ESP32 maneje otras tareas
    }
    
    // Imprimir bytes recibidos para depuración
    if (bytesReceived > 0) {
        Serial.print("Respuesta Modbus recibida (");
        Serial.print(bytesReceived);
        Serial.print(" bytes): ");
        
        for (uint8_t i = 0; i < bytesReceived; i++) {
            Serial.print(response[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
    }
    
    return bytesReceived;
}

// Limpia el buffer del puerto serial
void ModbusManager::flushSerialBuffer() {
    while (MODBUS_SERIAL.available()) {
        MODBUS_SERIAL.read();
    }
} 