#include "driver/gpio.h"
#include "esp_sleep.h"
#include "deep_sleep_config.h"

// Lista de pines a excluir de la configuración de alta impedancia.
// Los comentarios indican su función. Puedes descomentar aquellos que no desees modificar.
const int excludePins[] = {
    // Pines de cristal de 32kHz (no usados en este proyecto)    
    // 1,    // XTAL_32K_P
    // 0,    // XTAL_32K_N

    // Pines de Strapping (para definir el modo de arranque, no modificar si son necesarios)
    2,    // GPIO2
    8,    // GPIO8
    9,    // GPIO9

    // Pines de Flash SPI (NUNCA modificarlos para no afectar la comunicación con la memoria Flash)
    12,   // SPIHD
    13,   // SPIWP
    14,   // SPICLK
    15,   // SPICS0
    16,   // SPID
    17,   // SPIQ

    // Pines para UART0 (si se usa para depuración, mantenerlos excluidos)
    20,   // U0RXD (GPIO20) - Recibe datos (UART)
    21,   // U0TXD (GPIO21) - Transmite datos (UART)

    // Pines USB para depuración y/o programación (usados con otros propósitos)
    // 18,   // USB D- (GPIO18)
    // 19,   // USB D+ (GPIO19)
};
const int numExclude = sizeof(excludePins) / sizeof(excludePins[0]);

/**
 * @brief Configura los pines no utilizados en alta impedancia para reducir el consumo durante deep sleep.
 */
void setUnusedPinsHighImpedance() {
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
