/*******************************************************************************************
 * Archivo: include/debug.h
 * Descripción: Sistema de depuración configurable para mensajes por Serial
 *******************************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include "config.h"

// Si DEBUG_ENABLED está definido en config.h, las macros de depuración estarán activas
// Si no está definido, las macros se compilarán como código vacío

#ifdef DEBUG_ENABLED
    #define DEBUG_BEGIN(baud)     Serial.begin(baud)
    #define DEBUG_PRINT(...)      Serial.print(__VA_ARGS__)
    #define DEBUG_PRINTLN(...)    Serial.println(__VA_ARGS__)
    #define DEBUG_PRINTF(...)     Serial.printf(__VA_ARGS__)
    #define DEBUG_FLUSH()         Serial.flush()
    #define DEBUG_END()           Serial.end()
#else
    #define DEBUG_BEGIN(baud)     Serial.begin(baud)  // Mantenemos Serial.begin por compatibilidad
    #define DEBUG_PRINT(...)      {}
    #define DEBUG_PRINTLN(...)    {}
    #define DEBUG_PRINTF(...)     {}
    #define DEBUG_FLUSH()         Serial.flush()      // Mantenemos Serial.flush por seguridad
    #define DEBUG_END()           Serial.end()        // Mantenemos Serial.end por seguridad
#endif

#endif // DEBUG_H
