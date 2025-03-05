#ifndef RTC_MANAGER_H
#define RTC_MANAGER_H

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>

class RTCManager {
private:
    RTC_DS3231 rtc;

public:
    RTCManager();
    bool begin();
    void setFallbackDateTime();
    DateTime getCurrentTime();
    void printDateTime();
    bool hasLostPower();
    uint32_t getEpochTime();
    
    /**
     * @brief Actualiza el RTC usando el tiempo Unix recibido del servidor
     * @param unixTime Segundos desde Unix epoch (1/Jan/1970)
     * @param fraction Fracción de segundo (1/256 segundos)
     * @return true si la actualización fue exitosa
     */
    bool setTimeFromServer(uint32_t unixTime, uint8_t fraction);
};

#endif 