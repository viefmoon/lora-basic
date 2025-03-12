#ifndef UTILITIES_H
#define UTILITIES_H

#include <Arduino.h>

void parseKeyString(const String &keyStr, uint8_t *outArray, size_t expectedSize);
bool parseEUIString(const char* euiStr, uint64_t* eui);

/**
 * @brief Formatea un valor flotante con hasta 3 decimales, eliminando ceros finales.
 * @param value Valor flotante a formatear.
 * @param buffer Buffer donde se almacenará la cadena formateada.
 * @param bufferSize Tamaño del buffer.
 */
void formatFloatTo3Decimals(float value, char* buffer, size_t bufferSize);

#endif // UTILITIES_H