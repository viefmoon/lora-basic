#include "driver/gpio.h"
#include "esp_sleep.h"
#include "deep_sleep_config.h"
#include "config.h"

// Lista de pines a excluir de la configuración de alta impedancia.
// Los comentarios indican su función. Puedes descomentar aquellos que no desees modificar.
const int excludePins[] = {
    // Pines de Strapping
    CONFIG_PIN,    //Usado para configurar y despertar de deepSleep
    LORA_NSS_PIN,    // Pin NSS del módulo LoRa (debe mantenerse en alto durante deep sleep)
    // 8,    // GPIO8
    // 9,    // GPIO9

    // Pines de Flash SPI (NUNCA modificarlos para no afectar la comunicación con la memoria Flash)
    12,   // SPIHD
    13,   // SPIWP
    14,   // SPICLK
    15,   // SPICS0
    16,   // SPID
    17,   // SPIQ
};
const int numExclude = sizeof(excludePins) / sizeof(excludePins[0]);

/**
 * @brief Configura los pines no utilizados en alta impedancia para reducir el consumo durante deep sleep.
 */
void configurePinsForDeepSleep() {
    // Configurar explícitamente LORA_NSS_PIN como salida en alto para mantener el chip select del módulo LoRa desactivado
    gpio_reset_pin((gpio_num_t)LORA_NSS_PIN);
    gpio_set_direction((gpio_num_t)LORA_NSS_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level((gpio_num_t)LORA_NSS_PIN, 1);  // Establecer en alto
    gpio_hold_en((gpio_num_t)LORA_NSS_PIN);       // Mantener este estado durante el deep sleep
    
    for (int pin = 0; pin < MAX_GPIO_PINS; ++pin) {
        bool excluded = false;
        for (int j = 0; j < numExclude; j++) {
            if (pin == excludePins[j]) {
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            // Reinicia la configuración del pin para tenerlo en un estado conocido.
            gpio_reset_pin((gpio_num_t)pin);
            // Configura el pin como entrada sin resistencia interna para que quede flotante (alta impedancia).
            gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);
            // Activa la retención para mantener el estado durante el deep sleep.
            gpio_hold_en((gpio_num_t)pin);
        }
    }
    // Habilita el wakeup por GPIO, si se utiliza como fuente de despertar.
    esp_sleep_enable_gpio_wakeup();
}

/**
 * @brief Libera el estado de retención (hold) de los pines que fueron configurados en alta impedancia.
 * Esto permite que los pines puedan ser reconfigurados adecuadamente tras salir del deep sleep.
 */
void restoreUnusedPinsState() {
    // Liberar específicamente el pin NSS de LoRa
    gpio_hold_dis((gpio_num_t)LORA_NSS_PIN);
    
    for (int pin = 0; pin < MAX_GPIO_PINS; ++pin) {
        bool excluded = false;
        for (int j = 0; j < numExclude; j++) {
            if (pin == excludePins[j]) {
                excluded = true;
                break;
            }
        }
        if (!excluded) {
            gpio_hold_dis((gpio_num_t)pin);
        }
    }
}
