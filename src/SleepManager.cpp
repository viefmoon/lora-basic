/*******************************************************************************************
 * Archivo: src/SleepManager.cpp
 * Descripción: Implementación de la gestión del modo deep sleep para el ESP32.
 *******************************************************************************************/

#include "SleepManager.h"
#include "debug.h"
#include "LoRaManager.h"

void SleepManager::goToDeepSleep(uint32_t timeToSleep, 
                               PowerManager& powerManager,
                               PCA9555& ioExpander,
                               SX1262* radio,
                               LoRaWANNode& node,
                               uint8_t* LWsession,
                               SPIClass& spi) {
    // Guardar sesión en RTC y otras rutinas de apagado
    uint8_t *persist = node.getBufferSession();
    memcpy(LWsession, persist, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
    
    // Apagar todos los reguladores
    powerManager.allPowerOff();
    
    // Flush Serial antes de dormir
    DEBUG_FLUSH();
    DEBUG_END();
    
    // Apagar módulos
    LoRaManager::prepareForSleep(radio);
    btStop();

    // Poner el PCA9555 en modo sleep
    ioExpander.sleep();

    // Deshabilitar I2C y SPI
    Wire.end();
    spi.end();
    
    // Configurar el temporizador y GPIO para despertar
    esp_sleep_enable_timer_wakeup(timeToSleep * 1000000ULL);
    gpio_wakeup_enable((gpio_num_t)CONFIG_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_deep_sleep_enable_gpio_wakeup(BIT(CONFIG_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
    
    // Configurar pines para deep sleep
    configurePinsForDeepSleep();
    
    esp_deep_sleep_start();
}

/**
 * @brief Configura los pines no utilizados en alta impedancia para reducir el consumo durante deep sleep.
 */
void SleepManager::configurePinsForDeepSleep() {
    // Configurar pines específicos del módulo LoRa como ANALOG
    pinMode(FLOW_SENSOR_PIN, ANALOG);
    pinMode(BATTERY_PIN, ANALOG);

    pinMode(LORA_RST_PIN, ANALOG);
    pinMode(LORA_BUSY_PIN, ANALOG);
    pinMode(LORA_DIO1_PIN, ANALOG);
    pinMode(SPI_SCK_PIN, ANALOG);
    pinMode(SPI_MISO_PIN, ANALOG);
    pinMode(SPI_MOSI_PIN, ANALOG);

    pinMode(20, ANALOG); //Serial RX
    pinMode(21, ANALOG); //Serial TX

    pinMode(I2C_SDA_PIN, ANALOG); //I2C SDA
    pinMode(I2C_SCL_PIN, ANALOG); //I2C SCL

    pinMode(9, ANALOG); //LORA NSS

    // Configurar explícitamente LORA_NSS_PIN como salida en alto para mantener el chip select del módulo LoRa desactivado
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);
    gpio_hold_en((gpio_num_t)LORA_NSS_PIN);
}

/**
 * @brief Libera el estado de retención (hold) de los pines que fueron configurados para deep sleep.
 * Esto permite que los pines puedan ser reconfigurados adecuadamente tras salir del deep sleep.
 */
void SleepManager::releaseHeldPins() {
    // Liberar específicamente el pin NSS de LoRa
    gpio_hold_dis((gpio_num_t)LORA_NSS_PIN);
    
    // Liberar otros pines si se ha aplicado retención
    // Nota: Los pines configurados como ANALOG no necesitan liberación específica
    // ya que no se les aplica retención (gpio_hold_en)
}
