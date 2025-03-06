/*******************************************************************************************
 * Archivo: src/main.cpp
 * Descripción: Código principal para el ESP32 que configura los sensores, radio LoRa, BLE y
 * entra en modo Deep Sleep. Se incluye inicialización de hardware y manejo de configuraciones.
 *******************************************************************************************/

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <vector>
#include <ArduinoJson.h>
#include <cmath>

// Bibliotecas específicas del proyecto
#include "config.h"
#include "PowerManager.h"
#include "MAX31865.h"
#include <RadioLib.h>
#include "RTCManager.h"
#include "sensor_types.h"
#include "SensirionI2cSht4x.h"
#include "SensorManager.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include "config_manager.h"
#include "ble_config_callbacks.h"
#include "utilities.h"
#include "ble_service.h"  
#include "deep_sleep_config.h"
#include <SensirionI2cSht3x.h>
#include "LoRaManager.h"

/*-------------------------------------------------------------------------------------------------
   Declaración de funciones
-------------------------------------------------------------------------------------------------*/
/**
 * @brief Configura el servicio BLE y sus características.
 * @param pServer Puntero al servidor BLE.
 * @return Puntero al servicio BLE creado.
 */
BLEService* setupBLEService(BLEServer* pServer);

/**
 * @brief Configura el ESP32 para entrar en deep sleep.
 */
void goToDeepSleep();

/**
 * @brief Verifica si se mantuvo presionado el botón de configuración y activa el modo BLE.
 */
void checkConfigMode();

/**
 * @brief Inicializa el bus I2C, la expansión de I/O y el PowerManager.
 */
void initHardware();

/*-------------------------------------------------------------------------------------------------
   Objetos Globales y Variables
-------------------------------------------------------------------------------------------------*/

SensirionI2cSht3x sht30Sensor;

const LoRaWANBand_t Region = US915;
const uint8_t subBand = 2;  // For US915, change this to 2, otherwise leave on 0

Preferences preferences;       // Almacenamiento de preferencias en NVS

uint32_t timeToSleep;          // Tiempo en segundos para deep sleep
String deviceId;
String stationId;
bool systemInitialized;        // Variable global para la inicialización del sistema

RTCManager rtcManager;
PCA9555 ioExpander(I2C_ADDRESS_PCA9555, I2C_SDA_PIN, I2C_SCL_PIN);
PowerManager powerManager(ioExpander);

SPIClass spi(FSPI);
SPISettings spiAdcSettings(SPI_ADC_CLOCK, MSBFIRST, SPI_MODE1);
SPISettings spiRtdSettings(SPI_RTD_CLOCK, MSBFIRST, SPI_MODE1);
SPISettings spiRadioSettings(SPI_RADIO_CLOCK, MSBFIRST, SPI_MODE0);

MAX31865_RTD rtd(MAX31865_RTD::RTD_PT100, spi, spiRtdSettings, ioExpander, PT100_CS_PIN);

SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiRadioSettings);
LoRaWANNode node(&radio, &Region, subBand);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemp(&oneWire);

RTC_DATA_ATTR uint16_t bootCount = 0;
RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin = 0;
RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
Preferences store;

/*-------------------------------------------------------------------------------------------------
   Implementación de Funciones
-------------------------------------------------------------------------------------------------*/

/**
 * @brief Entra en modo deep sleep después de apagar periféricos y poner en reposo el módulo LoRa.
 */
void goToDeepSleep() {
    
    // Guardar sesión en RTC y otras rutinas de apagado
    uint8_t *persist = node.getBufferSession();
    memcpy(LWsession, persist, RADIOLIB_LORAWAN_SESSION_BUF_SIZE);
    
    // Poner el PCA9555 en modo sleep
    ioExpander.sleep();
    
    // Apagar todos los reguladores
    powerManager.allPowerOff();
    
    // Deshabilitar I2C y SPI
    Wire.end();
    spi.end();
    
    // Flush Serial antes de dormir
    Serial.flush();
    Serial.end();
    
    // Apagar módulos
    LoRaManager::prepareForSleep(&radio);
    btStop();
    
    // Configurar el temporizador y GPIO para despertar
    esp_sleep_enable_timer_wakeup(timeToSleep * 1000000ULL);
    gpio_wakeup_enable((gpio_num_t)CONFIG_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
    esp_deep_sleep_enable_gpio_wakeup(BIT(CONFIG_PIN), ESP_GPIO_WAKEUP_GPIO_LOW);
    setUnusedPinsHighImpedance();
    
    // Entrar en deep sleep
    esp_deep_sleep_start();
}

/**
 * @brief Comprueba si se ha activado el modo configuración mediante un pin.
 *        Si se mantiene presionado el botón de configuración durante el tiempo definido, activa BLE.
 */
void checkConfigMode() {
    if (digitalRead(CONFIG_PIN) == LOW) {
        Serial.println("Modo configuración activado");
        unsigned long startTime = millis();
        while (digitalRead(CONFIG_PIN) == LOW) {
            if (millis() - startTime >= CONFIG_TRIGGER_TIME) {

                // Inicializar BLE y crear servicio de configuración usando la nueva función modularizada
                LoRaConfig loraConfig = ConfigManager::getLoRaConfig();
                String bleName = "SENSOR_DEV" + String(loraConfig.devEUI);
                BLEDevice::init(bleName.c_str());

                BLEServer* pServer = BLEDevice::createServer();
                pServer->setCallbacks(new MyBLEServerCallbacks());  // Añadido el callback del servidor
                BLEService* pService = setupBLEService(pServer);

                // Configurar publicidad BLE
                BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
                pAdvertising->addServiceUUID(pService->getUUID());
                pAdvertising->setScanResponse(true);
                pAdvertising->setMinPreferred(0x06);
                pAdvertising->setMinPreferred(0x12);
                pAdvertising->start();

                // Bucle de parpadeo del LED de configuración
                while (true) {
                    ioExpander.digitalWrite(CONFIG_LED_PIN, HIGH);
                    delay(500);
                    ioExpander.digitalWrite(CONFIG_LED_PIN, LOW);
                    delay(500);
                }
            }
        }
    }
}

/**
 * @brief Inicializa configuraciones básicas de hardware:
 *        - Inicia el bus I2C.
 *        - Inicializa el expansor de I/O (PCA9555).
 *        - Configura el PowerManager.
 */
void initHardware() {
    // Inicializar I2C con pines definidos
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

    // Inicializar SHT30 para reset
    sht30Sensor.begin(Wire, SHT30_I2C_ADDR_44);
    sht30Sensor.stopMeasurement();
    delay(1);
    sht30Sensor.softReset();
    delay(100);

    if (!ioExpander.begin()) {
        Serial.println("Error al inicializar PCA9555");
    }

    // Inicializar PowerManager
    if (!powerManager.begin()) {
        Serial.println("Error al inicializar PowerManager");
    }
}

/*-------------------------------------------------------------------------------------------------
   Función setup()
   Inicializa la comunicación serial, hardware, configuración de sensores y radio,
   y recupera configuraciones previas del sistema.
-------------------------------------------------------------------------------------------------*/
void setup() {
    Serial.begin(115200);
    
    // Liberar el hold de los pines no excluidos si se está saliendo de deep sleep.
    restoreUnusedPinsState();
    
    pinMode(CONFIG_PIN, INPUT);

    // // Inicialización del NVS y de hardware I2C/IO
    // preferences.clear();
    // nvs_flash_erase();
    // nvs_flash_init();

    if (!ConfigManager::checkInitialized()) {
        Serial.println("Primera ejecución detectada. Inicializando configuración...");
        ConfigManager::initializeDefaultConfig();
    }
    
    ConfigManager::getSystemConfig(systemInitialized, timeToSleep, deviceId, stationId);
    initHardware();

    checkConfigMode();

    if (!rtcManager.begin()) {
        Serial.println("No se pudo encontrar el RTC");
    }

    powerManager.power3V3On();
    SensorManager::beginSensors();
    
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Error iniciando radio: %d\n", state);
    }

    // Activar el nodo usando la nueva función
    state = LoRaManager::lwActivate(node);
    if (state != RADIOLIB_LORAWAN_NEW_SESSION && state != RADIOLIB_LORAWAN_SESSION_RESTORED) {
        Serial.printf("Error en la activación LoRaWAN: %d\n", state);
        goToDeepSleep();
        return;
    }
    // Configurar datarate
    LoRaManager::setDatarate(node, 3);

    // Configurar pin para LED
    ioExpander.pinMode(CONFIG_LED_PIN, OUTPUT);
}

/*-------------------------------------------------------------------------------------------------
   Función loop()
   Ejecuta el ciclo principal: comprobación del modo configuración, lecturas de sensores,
   impresión de resultados de depuración y finalmente entra en deep sleep.
-------------------------------------------------------------------------------------------------*/
void loop() {
    // Comprobar constantemente si se solicita el modo configuración
    checkConfigMode();
    
    // Obtener directamente los sensores habilitados usando la nueva función
    auto enabledSensors = ConfigManager::getEnabledSensorConfigs();
    
    // Array para almacenar las lecturas de sensores
    std::vector<SensorReading> readings;
    
    // Obtener lecturas de los sensores habilitados
    for (const auto& sensor : enabledSensors) {
        readings.push_back(SensorManager::getSensorReading(sensor));
    }
    
    // Enviar el payload fragmentado
    LoRaManager::sendFragmentedPayload(readings, node, deviceId, stationId, rtcManager);
    
    // Entrar en modo deep sleep tras finalizar las tareas del ciclo
    goToDeepSleep();
}
