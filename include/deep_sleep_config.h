#ifndef DEEP_SLEEP_CONFIG_H
#define DEEP_SLEEP_CONFIG_H

#include "driver/gpio.h"
#include "esp_sleep.h"

// Para el módulo ESP32-C3-WROOM u otro que tenga 22 pines disponibles
#define MAX_GPIO_PINS  22 

// Lista de pines a excluir de la configuración de alta impedancia.
// Los comentarios indican su función. Puedes descomentar aquellos que no desees modificar.
extern const int excludePins[];
extern const int numExclude;

/**
 * @brief Configura los pines no utilizados en alta impedancia para reducir el consumo durante deep sleep.
 */
void configurePinsForDeepSleep();

/**
 * @brief Libera el estado de retención (hold) de los pines que fueron configurados en alta impedancia.
 * Esto permite que los pines puedan ser reconfigurados adecuadamente tras salir del deep sleep.
 */
void restoreUnusedPinsState();

#endif // DEEP_SLEEP_CONFIG_H 