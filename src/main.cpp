#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Preferences.h>
#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
#include <OneWire.h>
#include <DallasTemperature.h>
#endif
#include <vector>
#include <ArduinoJson.h>
#include <cmath>

#include "config.h"
#include "debug.h"
#include "PowerManager.h"
#include "MAX31865.h"
#include <RadioLib.h>
#include "RTCManager.h"
#include "sensor_types.h"
#include "SensorManager.h"
#include "nvs_flash.h"
#include "esp_sleep.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEAdvertising.h>
#include "config_manager.h"
#include "utilities.h"
#include <SensirionI2cSht3x.h>
#include "LoRaManager.h"
#include "BLE.h"

#include "HardwareManager.h"
#include "SleepManager.h"

//--------------------------------------------------------------------------------------------
// Variables globales
//--------------------------------------------------------------------------------------------
const LoRaWANBand_t Region = LORA_REGION;
const uint8_t subBand = LORA_SUBBAND;

Preferences preferences;
uint32_t timeToSleep;
String deviceId;
String stationId;
bool systemInitialized;

RTCManager rtcManager;
PCA9555 ioExpander(I2C_ADDRESS_PCA9555, I2C_SDA_PIN, I2C_SCL_PIN);
PowerManager powerManager(ioExpander);

SPIClass spi(FSPI);
#ifdef DEVICE_TYPE_ANALOGIC
SPISettings spiAdcSettings(SPI_ADC_CLOCK, MSBFIRST, SPI_MODE1);
#endif
SPISettings spiRtdSettings(SPI_RTD_CLOCK, MSBFIRST, SPI_MODE1);
SPISettings spiRadioSettings(SPI_RADIO_CLOCK, MSBFIRST, SPI_MODE0);

MAX31865_RTD rtd(MAX31865_RTD::RTD_PT100, spi, spiRtdSettings, ioExpander, PT100_CS_PIN);
SensirionI2cSht3x sht30Sensor;

SX1262 radio = new Module(LORA_NSS_PIN, LORA_DIO1_PIN, LORA_RST_PIN, LORA_BUSY_PIN, spi, spiRadioSettings);
LoRaWANNode node(&radio, &Region, subBand);

#if defined(DEVICE_TYPE_BASIC) || defined(DEVICE_TYPE_ANALOGIC)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature dallasTemp(&oneWire);
#endif

RTC_DATA_ATTR uint16_t bootCount = 0;
RTC_DATA_ATTR uint16_t bootCountSinceUnsuccessfulJoin = 0;
RTC_DATA_ATTR uint8_t LWsession[RADIOLIB_LORAWAN_SESSION_BUF_SIZE];
Preferences store;

//--------------------------------------------------------------------------------------------
// setup()
//--------------------------------------------------------------------------------------------
void setup() {
    DEBUG_BEGIN(SERIAL_BAUD_RATE);

    
    SleepManager::releaseHeldPins();
    pinMode(CONFIG_PIN, INPUT);

    // Inicialización del NVS y de hardware I2C/IO
    // preferences.clear();
    // nvs_flash_erase();
    // nvs_flash_init();

    // Inicialización de configuración
    if (!ConfigManager::checkInitialized()) {
        DEBUG_PRINTLN("Primera ejecución detectada. Inicializando configuración...");
        ConfigManager::initializeDefaultConfig();
    }
    ConfigManager::getSystemConfig(systemInitialized, timeToSleep, deviceId, stationId);

    // Inicialización de hardware
    if (!HardwareManager::initHardware(ioExpander, powerManager, sht30Sensor)) {
        DEBUG_PRINTLN("Error en la inicialización del hardware");
    }

    // Modo configuración BLE
    if (BLEHandler::checkConfigMode(ioExpander)) {
        return;
    }

    // Inicializar RTC
    if (!rtcManager.begin()) {
        DEBUG_PRINTLN("No se pudo encontrar RTC");
    }

    // Encender 3.3V
    powerManager.power3V3On();

    // Inicializar sensores
    SensorManager::beginSensors();
    
    // Inicializar radio LoRa
    int16_t state = radio.begin();
    if (state != RADIOLIB_ERR_NONE) {
        DEBUG_PRINTF("Error iniciando radio: %d\n", state);
    }

    // Activar LoRaWAN
    state = LoRaManager::lwActivate(node);
    if (state != RADIOLIB_LORAWAN_NEW_SESSION && state != RADIOLIB_LORAWAN_SESSION_RESTORED) {
        DEBUG_PRINTF("Error activando LoRaWAN: %d\n", state);
        SleepManager::goToDeepSleep(timeToSleep, powerManager, ioExpander, &radio, node, LWsession, spi);
        return;
    }

    // Ajustar datarate
    LoRaManager::setDatarate(node, 3);

    ioExpander.pinMode(CONFIG_LED_PIN, OUTPUT);
}

//--------------------------------------------------------------------------------------------
// loop()
//--------------------------------------------------------------------------------------------
void loop() {
    // Verificar si se mantiene presionado para modo config
    if (BLEHandler::checkConfigMode(ioExpander)) {
        return;
    }

    // Obtener todas las lecturas de sensores (normales y Modbus)
    std::vector<SensorReading> normalReadings;
    std::vector<ModbusSensorReading> modbusReadings;

    SensorManager::getAllSensorReadings(normalReadings, modbusReadings);

    LoRaManager::sendFragmentedPayload(normalReadings, modbusReadings, node, deviceId, stationId, rtcManager);

    // Dormir
    SleepManager::goToDeepSleep(timeToSleep, powerManager, ioExpander, &radio, node, LWsession, spi);
}
