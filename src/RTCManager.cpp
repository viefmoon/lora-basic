#include "RTCManager.h"
#include "debug.h"

RTCManager::RTCManager() {}

bool RTCManager::begin() {
    // Inicializar el RTC
    if (!rtc.begin()) {
        return false;
    }
    
    // Configurar el RTC con la hora de compilación si no está configurado
    if (rtc.lostPower()) {
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        DEBUG_PRINTLN("RTC configurado con hora de compilación");
    }
    
    return true;
}

void RTCManager::setFallbackDateTime() {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    DEBUG_PRINTLN("RTC configurado con hora de compilación");
}

DateTime RTCManager::getCurrentTime() {
    return rtc.now();
}

void RTCManager::printDateTime() {
    DateTime currentTime = getCurrentTime();
    
    DEBUG_PRINT(currentTime.year(), DEC);
    DEBUG_PRINT('/');
    DEBUG_PRINT(currentTime.month(), DEC);
    DEBUG_PRINT('/');
    DEBUG_PRINT(currentTime.day(), DEC);
    DEBUG_PRINT(" ");
    DEBUG_PRINT(currentTime.hour(), DEC);
    DEBUG_PRINT(':');
    DEBUG_PRINT(currentTime.minute(), DEC);
    DEBUG_PRINT(':');
    DEBUG_PRINTLN(currentTime.second(), DEC);
}

uint32_t RTCManager::getEpochTime() {
    return rtc.now().unixtime();
}

bool RTCManager::setTimeFromServer(uint32_t unixTime, uint8_t fraction) {
    // Convertir el tiempo unix a DateTime
    DateTime serverTime(unixTime);
    
    // Ajustar el RTC con el tiempo del servidor
    rtc.adjust(serverTime);
    
    // Verificar si se ajustó correctamente
    if (abs((int32_t)rtc.now().unixtime() - (int32_t)unixTime) < 10) {
        DEBUG_PRINTLN("RTC actualizado exitosamente con tiempo del servidor");
        return true;
    }
    
    DEBUG_PRINTLN("Error al actualizar RTC con tiempo del servidor");
    return false;
}
