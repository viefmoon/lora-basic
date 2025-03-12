#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

void parseKeyString(const String &keyStr, uint8_t *outArray, size_t expectedSize);
bool parseEUIString(const char* euiStr, uint64_t* eui);

/**
 * @brief Redondea un valor a un número específico de decimales, solo si el valor 
 *        realmente tiene más decimales que el límite indicado.
 * @param value Valor a redondear.
 * @param decimals Número máximo de decimales permitidos.
 * @return Valor redondeado o el valor original si ya tiene la precisión requerida.
 */

/**
 * @brief Redondea un valor flotante a un máximo de 3 decimales.
 * @param value Valor a redondear.
 * @return Valor redondeado a 3 decimales.
 */
float roundTo3Decimals(float value);

#endif // UTILITIES_H