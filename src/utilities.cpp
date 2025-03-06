#include <Arduino.h>  // Agregar esta línea para definir la clase String
#include <cmath>     // Necesario para pow, round y fabs
#include "utilities.h"

// Función auxiliar para convertir un string tipo 
// "EE,F1,30,98,6A,11,4E,69,D0,DE,8A,DC,D6,8D,28,A6"
// en un array de 16 bytes
void parseKeyString(const String &keyStr, uint8_t *outArray, size_t expectedSize) {
    int index = 0;
    int start = 0;
    while (index < expectedSize && start < keyStr.length()) {
        int comma = keyStr.indexOf(',', start);
        String byteStr;
        if (comma == -1) {
            byteStr = keyStr.substring(start);
            start = keyStr.length();
        } else {
            byteStr = keyStr.substring(start, comma);
            start = comma + 1;
        }
        outArray[index] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
        index++;
    }
}

bool parseEUIString(const char* euiStr, uint64_t* eui) {
    char temp[3];
    uint8_t bytes[8];
    const char* ptr = euiStr;
    
    // Parsear 8 bytes
    for (int i = 0; i < 8; i++) {
        // Copiar dos caracteres hexadecimales
        strncpy(temp, ptr, 2);
        temp[2] = '\0';
        
        // Convertir string hex a byte
        bytes[i] = strtoul(temp, nullptr, 16);
        
        // Avanzar al siguiente par de caracteres, saltando la coma si existe
        ptr += (ptr[2] == ',') ? 3 : 2;
    }
    
    // Convertir los 8 bytes a uint64_t
    *eui = 0;
    for (int i = 0; i < 8; i++) {
        *eui = (*eui << 8) | bytes[i];
    }
    
    return true;
} 

/**
 * @brief Redondea un valor a un número específico de decimales, solo si el valor 
 *        realmente tiene más decimales que el límite indicado.
 * @param value Valor a redondear.
 * @param decimals Número máximo de decimales permitidos.
 * @return Valor redondeado o el valor original si ya tiene la precisión requerida.
 */
double roundValue(double value, int decimals) {
    double factor = pow(10.0, decimals);
    double rounded = round(value * factor) / factor;
    
    // Si la diferencia es insignificante, se entiende que el valor no tenía más decimales
    if (fabs(rounded - value) < 1e-9) {  
        return value;
    }
    return rounded;
}