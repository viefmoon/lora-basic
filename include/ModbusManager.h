#ifndef MODBUS_MANAGER_H
#define MODBUS_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "sensor_types.h"

// Definición de rangos de direcciones para diferentes tipos de sensores Modbus
#define MODBUS_SENSOR_4IN1_START_ADDR 1
#define MODBUS_SENSOR_4IN1_END_ADDR 10

// Estructura para almacenar los datos del sensor 4-en-1
struct Sensor4in1Data {
    float humidity;       // Humedad (%)
    float temperature;    // Temperatura (°C)
    float noise;          // Ruido (dB)
    float pm25;           // PM2.5
    float pm10;           // PM10
    float pressure;       // Presión barométrica (kPa)
    uint32_t illuminance; // Iluminación (lux)
};

class ModbusManager {
public:
    /**
     * @brief Inicializa el puerto serial para comunicación Modbus
     * @param baudRate Velocidad de comunicación (por defecto 9600)
     */
    static void begin(unsigned long baudRate = 9600);
    
    /**
     * @brief Lee un sensor Modbus 4-en-1 en la dirección especificada
     * @param deviceAddress Dirección del dispositivo Modbus (1-247)
     * @return Estructura con los datos leídos del sensor
     */
    static Sensor4in1Data readSensor4in1(uint8_t deviceAddress);
    
    /**
     * @brief Convierte los datos del sensor 4-en-1 a lecturas individuales
     * @param data Datos del sensor 4-en-1
     * @param deviceAddress Dirección del dispositivo para generar IDs únicos
     * @return Vector de lecturas de sensores
     */
    static std::vector<SensorReading> convertToSensorReadings(const Sensor4in1Data& data, uint8_t deviceAddress);

private:
    /**
     * @brief Calcula el CRC16 para Modbus
     * @param data Puntero a los datos
     * @param length Longitud de los datos
     * @return CRC16 calculado
     */
    static uint16_t calculateCRC16(uint8_t* data, uint8_t length);
    
    /**
     * @brief Envía un comando Modbus y recibe la respuesta
     * @param command Comando a enviar
     * @param commandLength Longitud del comando
     * @param response Buffer para almacenar la respuesta
     * @param maxResponseLength Longitud máxima de la respuesta
     * @return Longitud de la respuesta recibida o 0 si hay error
     */
    static uint8_t sendModbusCommand(uint8_t* command, uint8_t commandLength, 
                                    uint8_t* response, uint8_t maxResponseLength);
    
    /**
     * @brief Limpia el buffer del puerto serial
     */
    static void flushSerialBuffer();
};

#endif // MODBUS_MANAGER_H 