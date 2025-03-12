#ifndef MODBUS_SENSOR_MANAGER_H
#define MODBUS_SENSOR_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "sensor_types.h"
#include "config.h"  // Para las definiciones de tipo de dispositivo

#if defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)

/**
 * @brief Clase para manejar la lectura de sensores Modbus.
 *        Usa Serial por defecto. Asegurarse de no usar debug durante la medición.
 *        Utiliza la biblioteca ModbusMaster para comunicación.
 */
class ModbusSensorManager {
public:
    /**
     * @brief Inicializa el bus RS485/Modbus (configura Serial a 9600, 8N1, etc.)
     *        Debe llamarse una sola vez al principio.
     */
    static void beginModbus();

    /**
     * @brief Finaliza la comunicación Modbus (cierra Serial)
     *        Debe llamarse después de completar todas las lecturas Modbus
     */
    static void endModbus();

    /**
     * @brief Lee un sensor Modbus de tipo ENV_SENSOR (ejemplo del datasheet).
     *        Regresa la lectura con subvalores (T, H, Ruido, PM2.5, PM10, Presión, Iluminación).
     * @param cfg Configuración del sensor (dirección, etc.)
     * @return Estructura ModbusSensorReading con los subvalores.
     */
    static ModbusSensorReading readEnvSensor(const ModbusSensorConfig &cfg);

private:
    /**
     * @brief Envía un frame Modbus (Función 0x03) y recibe la respuesta utilizando ModbusMaster.
     *        Para simplificar el ejemplo, se hace una lectura consecutiva de registros.
     * @param address Dirección Modbus del dispositivo
     * @param startReg Registro inicial
     * @param numRegs  Cantidad de registros
     * @param outData  Buffer de salida donde se almacenan los valores de cada registro
     * @return true si la lectura fue exitosa, false en caso de error
     */
    static bool readHoldingRegisters(uint8_t address, uint16_t startReg, uint16_t numRegs, uint16_t* outData);
};

#endif // defined(DEVICE_TYPE_ANALOGIC) || defined(DEVICE_TYPE_MODBUS)

#endif // MODBUS_SENSOR_MANAGER_H