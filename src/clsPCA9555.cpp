/**
 * @file    clsPCA9555.cpp
 * @author     Nico Verduin
 * @date      9-8-2015
 *
 * @mainpage  clsPCA9555
 * Class to enable pinMode(), digitalRead() and digitalWrite() functions on PCA9555 IO expanders
 *
 * Additional input received from Rob Tillaart (9-8-2015)
 *
 * @par License info
 *
 * Class to enable the use of single pins on PCA9555 IO Expander using
 * pinMode(), digitalRead() and digitalWrite().
 *
 * Copyright (C) 2015  Nico Verduin
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Program : clsPCA9555  Copyright (C) 2015  Nico Verduin
 * This is free software, and you are welcome to redistribute it.
 *
 */

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "clsPCA9555.h"
#include "Wire.h"
#include "debug.h"

PCA9555* PCA9555::instancePointer = 0;

/**
 * @name PCA9555 constructor
 * @param address I2C address of the IO Expander
 * Creates the class interface and sets the I2C Address of the port
 */
PCA9555::PCA9555(uint8_t address, int interruptPin) {
    _address         = address;        // save the address id
    _valueRegister   = 0;
    Wire.begin();                      // start I2C communication

    if(interruptPin >= 0)
    {
    instancePointer = this;
    attachInterrupt(digitalPinToInterrupt(interruptPin), PCA9555::alertISR, LOW); // Set to low for button presses
    }
}

PCA9555::PCA9555(uint8_t address, int sda, int scl, int interruptPin) {
    _address = address;
    _sda = sda;
    _scl = scl;
    _valueRegister = 0;
    Wire.begin(sda, scl); // Inicia I2C con los pines especificados

    if (interruptPin >= 0) {
        instancePointer = this;
        attachInterrupt(digitalPinToInterrupt(interruptPin), PCA9555::alertISR, LOW);
    }
}

// Checks if PCA9555 is responsive. Refer to Wire.endTransmission() from Arduino for details.
bool PCA9555::begin() {

    // Intentar varias veces la inicialización
    for (int intento = 0; intento < 3; intento++) {
        Wire.beginTransmission(_address);
        Wire.write(0x02);
        _error = Wire.endTransmission();

        if (_error == 0) {
            // Configuración inicial
            _valueRegister = 0x0000;          
            _configurationRegister = 0x0000;
            
            // Añadir pequeños delays entre escrituras I2C
            I2CSetValue(_address, NXP_OUTPUT, _valueRegister_low);
            delayMicroseconds(100);
            I2CSetValue(_address, NXP_OUTPUT + 1, _valueRegister_high);
            delayMicroseconds(100);
            I2CSetValue(_address, NXP_CONFIG, _configurationRegister_low);
            delayMicroseconds(100);
            I2CSetValue(_address, NXP_CONFIG + 1, _configurationRegister_high);

            return true;
        }
        
        delayMicroseconds(100);  // Esperar antes del siguiente intento
    }
    
    return false;
}

/**
 * @name pinMode
 * @param pin       pin number
 * @param IOMode    mode of pin INPUT or OUTPUT
 * sets the mode of this IO pin
 */
void PCA9555::pinMode(uint8_t pin, uint8_t IOMode) {

    //
    // check if valid pin first
    //
    if (pin <= 15) {
        //
        // now set the correct bit in the configuration register
        //
        if (IOMode == OUTPUT) {
            //
            // mask correct bit to 0 by inverting x so that only
            // the correct bit is LOW. The rest stays HIGH
            //
            _configurationRegister = _configurationRegister & ~(1 << pin);
        } else {
            //
            // or just the required bit to 1
            //
            _configurationRegister = _configurationRegister | (1 << pin);
        }
        //
        // write configuration register to chip
        //
        I2CSetValue(_address, NXP_CONFIG    , _configurationRegister_low);
        I2CSetValue(_address, NXP_CONFIG + 1, _configurationRegister_high);
    }
}
/**
 * @name digitalRead Reads the high/low value of specified pin
 * @param pin
 * @return value of pin
 * Reads the selected pin.
 */
uint8_t PCA9555::digitalRead(uint8_t pin) {
    uint16_t _inputData = 0;
    //
    // we wil only process pins <= 15
    //
    if (pin > 15 ) return 255;
    _inputData  = I2CGetValue(_address, NXP_INPUT);
    _inputData |= I2CGetValue(_address, NXP_INPUT + 1) << 8;
    //
    // now mask the bit required and see if it is a HIGH
    //
    if ((_inputData & (1 << pin)) > 0){
        //
        // the bit is HIGH otherwise we would return a LOW value
        //
        return HIGH;
    } else {
        return LOW;
    }
}

void PCA9555::digitalWrite(uint8_t pin, uint8_t value) {
    //
    // check valid pin first
    //
    if (pin > 15 ){
        _error = 255;            // invalid pin
        return;                  // exit
    }
    //
    // if the value is LOW we will and the register value with correct bit set to zero
    // if the value is HIGH we will or the register value with correct bit set to HIGH
    //
    if (value > 0) {
        //
        // this is a High value so we will or it with the value register
        //
        _valueRegister = _valueRegister | (1 << pin);    // and OR bit in register
    } else {
        //
        // this is a LOW value so we have to AND it with 0 into the _valueRegister
        //
        _valueRegister = _valueRegister & ~(1 << pin);    // AND all bits
    }
    I2CSetValue(_address, NXP_OUTPUT    , _valueRegister_low);
    I2CSetValue(_address, NXP_OUTPUT + 1, _valueRegister_high);
}

// This is the actual ISR
// Stores states of all pins in _stateOfPins
void PCA9555::pinStates(){
  _stateOfPins = I2CGetValue(_address, NXP_INPUT);
  _stateOfPins|= I2CGetValue(_address, NXP_INPUT + 1) << 8;
}

// Returns to user the state of desired pin
uint8_t PCA9555::stateOfPin(uint8_t pin){
  if ((_stateOfPins & (1 << pin)) > 0){
    //
    // the bit is HIGH otherwise we would return a LOW value
    //
    return HIGH;
  } else {
    return LOW;
  }
}

/**
 * @name setClock modifies the clock frequency for I2C communication
 * @param clockFrequency
 * clockFrequency: the value (in Hertz) of desired communication clock.
 * The PCA9555 supports a 400kHz clock.
 * Accepted values are:
 *    10000 low speed mode, supported on some processors
 *    100000, standard mode
 *    400000, fast mode
 */
void PCA9555::setClock(uint32_t clockFrequency){
  Wire.setClock(clockFrequency);
}

void PCA9555::alertISR()
{
  if (instancePointer != 0)
  {
    instancePointer->pinStates(); // Points to the actual ISR
  }
}

//
// low level hardware methods
//

/**
 * @name I2CGetValue
 * @param address Address of I2C chip
 * @param reg    Register to read from
 * @return data in register
 * Reads the data from addressed chip at selected register. \n
 * If the value is above 255, an error is set. \n
 * error codes : \n
 * 256 = either 0 or more than one byte is received from the chip
 */
uint16_t PCA9555::I2CGetValue(uint8_t address, uint8_t reg) {
    uint16_t _inputData;
    //
    // read the address input register
    //
    Wire.beginTransmission(address);          // setup read registers
    Wire.write(reg);
    _error = Wire.endTransmission();
    //
    // ask for 2 bytes to be returned
    //
    if (Wire.requestFrom((int)address, 1) != 1)
    {
        //
        // we are not receing the bytes we need
        //
        return 256;                            // error code is above normal data range
    };
    //
    // read both bytes
    //
    _inputData = Wire.read();
    return _inputData;
}

/**
 * @name I2CSetValue(uint8_t address, uint8_t reg, uint8_t value)
 * @param address Address of I2C chip
 * @param reg    register to write to
 * @param value    value to write to register
 * Write the value given to the register set to selected chip.
 */
void PCA9555::I2CSetValue(uint8_t address, uint8_t reg, uint8_t value){
    //
    // write output register to chip
    //
    Wire.beginTransmission(address);              // setup direction registers
    Wire.write(reg);                              // pointer to configuration register address 0
    Wire.write(value);                            // write config register low byte
    _error = Wire.endTransmission();
}

void PCA9555::sleep() {
    // 1) Prepara registros locales (16 bits) para la configuración y el valor de salida
    //    Empezamos con todo en 0 (por defecto, consideraremos OUTPUT=0 y LOW=0).
    uint16_t tempConfig = 0x0000;  // 0 => OUTPUT, 1 => INPUT
    uint16_t tempOutput = 0x0000;  // 0 => LOW,    1 => HIGH

#ifdef DEVICE_TYPE_BASIC
    // Configuración para dispositivo BASIC
    // Para los pines que sean INPUT (por ejemplo pin 2, 5, 6, 7, 10...15),
    // ponemos su bit correspondiente en 1 dentro de tempConfig.
    // Ejemplo: pin 2 como INPUT => set bit 2 de tempConfig a 1:
    tempConfig |= (1 << 1);   // pin 1 => INPUT
    tempConfig |= (1 << 2);   // pin 2 => INPUT
    tempConfig |= (1 << 4);   // pin 4 => INPUT
    tempConfig |= (1 << 5);   // pin 5 => INPUT
    tempConfig |= (1 << 6);   // pin 6 => INPUT
    tempConfig |= (1 << 11);  // pin 11 => INPUT
    tempConfig |= (1 << 12);  // pin 12 => INPUT
    tempConfig |= (1 << 13);  // pin 13 => INPUT
    tempConfig |= (1 << 14);  // pin 14 => INPUT
    tempConfig |= (1 << 15);  // pin 15 => INPUT
    // Si no se pone un bit en 1 explícitamente, queda en 0 => OUTPUT.

    // Para los pines que son OUTPUT y queramos poner en HIGH (p. ej. pin 1, 4, 9),
    // establecemos el bit correspondiente en tempOutput:
    tempOutput |= (1 << 3);  // pin 3 => HIGH //ss de pt100
    // Los pines OUTPUT sin setear aquí quedan en LOW (por defecto tempOutput=0).
#endif

#ifdef DEVICE_TYPE_MODBUS
    // Configuración para dispositivo MODBUS
    //Power manager commentados para que esten como output bajo
    //tempConfig |= (1 << 0);  // pin 1 => INPUT
    //tempOutput |= (1 << 1);  // pin 1 => INPUT

    //unused pins
    tempConfig |= (1 << 2);  // pin 11 => INPUT
    tempConfig |= (1 << 3);  // pin 12 => INPUT //cs de pt100, gasta menos con estado input flotante
    tempConfig |= (1 << 4);  // pin 12 => INPUT
    tempConfig |= (1 << 5);  // pin 13 => INPUT
    tempConfig |= (1 << 6);  // pin 14 => INPUT
    tempConfig |= (1 << 11);  // pin 11 => INPUT
    tempConfig |= (1 << 12);  // pin 12 => INPUT
    tempConfig |= (1 << 13);  // pin 13 => INPUT
    tempConfig |= (1 << 14);  // pin 14 => INPUT
    tempConfig |= (1 << 15);  // pin 15 => INPUT

    //Actuadores se quedan en output bajo
    tempConfig |= (1 << 7);  // pin 15 => INPUT
    tempConfig |= (1 << 8);  // pin 15 => INPUT

    // //Leds comentados para que sean output bajo
    // tempConfig |= (1 << 9);  // pin 15 => INPUT
    // tempConfig |= (1 << 10);  // pin 15 => INPUT
    tempOutput |= (0 << 9);  // pin 9 => LOW //ss de pt100
    tempOutput |= (0 << 10);  // pin 10 => LOW //ss de pt100
#endif

#ifdef DEVICE_TYPE_ANALOGIC
    // Configuración para dispositivo ANALOGIC
    // Para los pines que sean INPUT, ponemos su bit correspondiente en 1 dentro de tempConfig
    tempConfig |= (1 << 1);   // pin 1 => INPUT
    tempConfig |= (1 << 2);   // pin 2 => INPUT
    tempConfig |= (1 << 4);   // pin 4 => INPUT
    tempConfig |= (1 << 5);   // pin 5 => INPUT
    tempConfig |= (1 << 6);   // pin 6 => INPUT
    tempConfig |= (1 << 10);  // pin 10 => INPUT //Entrada analógica
    tempConfig |= (1 << 11);  // pin 11 => INPUT
    tempConfig |= (1 << 12);  // pin 12 => INPUT
    tempConfig |= (1 << 13);  // pin 13 => INPUT
    tempConfig |= (1 << 14);  // pin 14 => INPUT
    tempConfig |= (1 << 15);  // pin 15 => INPUT
    
    // Para los pines que son OUTPUT y queramos poner en HIGH
    tempOutput |= (1 << 3);   // pin 3 => HIGH //ss de pt100
    tempOutput |= (1 << 8);   // pin 8 => HIGH //Activación de circuito analógico
#endif

    // -------------------------------------------------------------
    // 2) Volcamos estos valores a las variables miembros y escribimos en el PCA9555
    //    Primero el registro de OUTPUT, luego el de CONFIG.
    //    (OJO: cuando un pin es INPUT, da igual el bit de salida que pongas).
    _valueRegister         = tempOutput;
    _configurationRegister = tempConfig;

    I2CSetValue(_address, NXP_OUTPUT, _valueRegister_low);
    I2CSetValue(_address, NXP_OUTPUT + 1, _valueRegister_high);

    I2CSetValue(_address, NXP_CONFIG, _configurationRegister_low);
    I2CSetValue(_address, NXP_CONFIG + 1, _configurationRegister_high);
}
