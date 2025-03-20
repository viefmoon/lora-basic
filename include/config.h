#ifndef CONFIG_H
#define CONFIG_H

// Descomentar solo UNO de los siguientes
// #define DEVICE_TYPE_BASIC
#define DEVICE_TYPE_ANALOGIC
// #define DEVICE_TYPE_MODBUS

// Configuración de depuración - Comentar para deshabilitar mensajes de depuración
#define DEBUG_ENABLED

#ifdef DEVICE_TYPE_BASIC

// Pines generales
#define ONE_WIRE_BUS        0
#define I2C_SDA_PIN         19
#define I2C_SCL_PIN         18
#define I2C_ADDRESS_PCA9555 0x20

// SPI
#define SPI_SCK_PIN         10
#define SPI_MISO_PIN        6
#define SPI_MOSI_PIN        7
#define SPI_RTD_CLOCK       1000000
#define SPI_RADIO_CLOCK     100000

// PT100
#define PT100_CS_PIN        P03

// Modo Config
#define CONFIG_PIN          2
#define CONFIG_TRIGGER_TIME 5000
#define CONFIG_TIMEOUT      30000
#define CONFIG_LED_PIN      P11

// LoRa
#define LORA_NSS_PIN        8
#define LORA_BUSY_PIN       4
#define LORA_RST_PIN        5
#define LORA_DIO1_PIN       3
#define MAX_LORA_PAYLOAD    200

// Serial
#define SERIAL_BAUD_RATE        921600

// Deep Sleep
#define DEFAULT_TIME_TO_SLEEP   30

// Identificadores
#define DEFAULT_DEVICE_ID   "DEV01"
#define DEFAULT_STATION_ID  "ST001"

// LoRa (OTAA)
#define DEFAULT_JOIN_EUI    "00,00,00,00,00,00,00,00"
#define DEFAULT_DEV_EUI     "1f,d4,e6,68,46,8c,e1,b7"
#define DEFAULT_APP_KEY     "1d,fb,69,80,69,d6,a0,7e,5d,bf,29,ba,6b,37,d3,04"
#define DEFAULT_NWK_KEY     "82,91,e9,55,19,ab,c0,6c,86,25,63,68,e7,f4,5a,89"

// LoRa Region y SubBand
#define LORA_REGION         US915
#define LORA_SUBBAND        2       // For US915, use 2; for other regions, use 0

#define BLE_SERVICE_UUID             "180A"
#define BLE_CHAR_SYSTEM_UUID         "2A37"
#define BLE_CHAR_SENSORS_UUID        "2A40"
#define BLE_CHAR_LORA_CONFIG_UUID    "2A41"
#define BLE_DEVICE_PREFIX            "AGRICOS_"

// Calibración batería
const double R1 = 470000.0;
const double R2 = 1500000.0;
const double conversionFactor = (R1 + R2) / R1;

// Namespaces
#define NAMESPACE_SYSTEM        "system"
#define NAMESPACE_SENSORS       "sensors"
#define NAMESPACE_LORAWAN       "lorawan"
#define NAMESPACE_LORA_SESSION  "lorasession"

// Claves
#define KEY_INITIALIZED         "initialized"
#define KEY_SLEEP_TIME          "sleep_time"
#define KEY_STATION_ID          "stationId"
#define KEY_DEVICE_ID           "deviceId"
#define KEY_VOLT                "volt"
#define KEY_SENSOR              "k"
#define KEY_SENSOR_ID           "id"
#define KEY_SENSOR_ID_TEMPERATURE_SENSOR "ts"
#define KEY_SENSOR_TYPE         "t"
#define KEY_SENSOR_ENABLE       "e"
#define KEY_LORA_JOIN_EUI       "joinEUI"
#define KEY_LORA_DEV_EUI        "devEUI"
#define KEY_LORA_NWK_KEY        "nwkKey"
#define KEY_LORA_APP_KEY        "appKey"
#define KEY_LORAWAN_SESSION     "lorawan_session"

// Tamaños de documentos JSON - Centralizados
#define JSON_DOC_SIZE_SMALL   300
#define JSON_DOC_SIZE_MEDIUM  1024
#define JSON_DOC_SIZE_LARGE   2048

// Batería
#define BATTERY_PIN             1

//Power Manager
#define POWER_3V3_PIN           P00
#define POWER_STABILIZE_DELAY   20

// Configuración default sensores
#define DEFAULT_SENSOR_CONFIGS { \
    {"R", "RTD1", RTD, true}, \
    {"D", "DS1", DS18B20, true}, \
    {"I2C", "SHT30", SHT30, true} \
}

#endif


#ifdef DEVICE_TYPE_ANALOGIC

// Pines generales
#define ONE_WIRE_BUS        1
#define I2C_SDA_PIN         19
#define I2C_SCL_PIN         18
#define I2C_ADDRESS_PCA9555 0x20

// SPI
#define SPI_SCK_PIN         10
#define SPI_MISO_PIN        6
#define SPI_MOSI_PIN        7
#define SPI_RTD_CLOCK       1000000
#define SPI_RADIO_CLOCK     100000

// PT100
#define PT100_CS_PIN        P03

// Modo Config
#define CONFIG_PIN          2
#define CONFIG_TRIGGER_TIME 5000
#define CONFIG_TIMEOUT      30000
#define CONFIG_LED_PIN      P11

// LoRa
#define LORA_NSS_PIN        8
#define LORA_BUSY_PIN       4
#define LORA_RST_PIN        5
#define LORA_DIO1_PIN       3
#define MAX_LORA_PAYLOAD    200

// Serial
#define SERIAL_BAUD_RATE         921600

// Deep Sleep
#define DEFAULT_TIME_TO_SLEEP   30

// Identificadores
#define DEFAULT_DEVICE_ID   "DEV01"
#define DEFAULT_STATION_ID  "ST001"

// LoRa (OTAA)
#define DEFAULT_JOIN_EUI    "00,00,00,00,00,00,00,00"
#define DEFAULT_DEV_EUI     "1f,d4,e6,68,46,8c,e1,b7"
#define DEFAULT_APP_KEY     "1d,fb,69,80,69,d6,a0,7e,5d,bf,29,ba,6b,37,d3,04"
#define DEFAULT_NWK_KEY     "82,91,e9,55,19,ab,c0,6c,86,25,63,68,e7,f4,5a,89"

// LoRa Region y SubBand
#define LORA_REGION         US915
#define LORA_SUBBAND        2       // For US915, use 2; for other regions, use 0

#define BLE_SERVICE_UUID             "180A"
#define BLE_CHAR_SYSTEM_UUID         "2A37"
#define BLE_CHAR_SENSORS_UUID        "2A40"
#define BLE_CHAR_LORA_CONFIG_UUID    "2A41"
#define BLE_CHAR_NTC100K_UUID        "2A38"
#define BLE_CHAR_NTC10K_UUID         "2A39"
#define BLE_CHAR_CONDUCTIVITY_UUID   "2A3C"
#define BLE_CHAR_PH_UUID             "2A3B"
#define BLE_DEVICE_PREFIX            "AGRICOS_"

// Calibración batería
const double R1 = 1000000.0;
const double R2 = 1500000.0;
const double conversionFactor = (R1 + R2) / R1;

// Namespaces
#define NAMESPACE_SYSTEM        "system"
#define NAMESPACE_SENSORS       "sensors"
#define NAMESPACE_LORAWAN       "lorawan"
#define NAMESPACE_LORA_SESSION  "lorasession"
#define NAMESPACE_SENSORS_MODBUS "sensors_modbus"

// Claves
#define KEY_INITIALIZED         "initialized"
#define KEY_SLEEP_TIME          "sleep_time"
#define KEY_STATION_ID          "stationId"
#define KEY_DEVICE_ID           "deviceId"
#define KEY_VOLT                "volt"
#define KEY_SENSOR              "k"
#define KEY_SENSOR_ID           "id"
#define KEY_SENSOR_ID_TEMPERATURE_SENSOR "ts"
#define KEY_SENSOR_TYPE         "t"
#define KEY_SENSOR_ENABLE       "e"
#define KEY_LORA_JOIN_EUI       "joinEUI"
#define KEY_LORA_DEV_EUI        "devEUI"
#define KEY_LORA_NWK_KEY        "nwkKey"
#define KEY_LORA_APP_KEY        "appKey"
#define KEY_LORAWAN_SESSION     "lorawan_session"

// Claves para Modbus
#define KEY_MODBUS_SENSOR_ID    "id"
#define KEY_MODBUS_SENSOR_TYPE  "t"
#define KEY_MODBUS_SENSOR_ADDR  "a"
#define KEY_MODBUS_SENSOR_ENABLE "e"

// Configuración Modbus
#define MODBUS_BAUDRATE         9600
#define MODBUS_SERIAL_CONFIG    SERIAL_8N1
#define MODBUS_RESPONSE_TIMEOUT 300  // Tiempo de espera para respuesta en ms
#define MODBUS_MAX_RETRY        3     // Número máximo de intentos de lectura Modbus


// Tamaños de documentos JSON - Centralizados
#define JSON_DOC_SIZE_SMALL   300
#define JSON_DOC_SIZE_MEDIUM  1024
#define JSON_DOC_SIZE_LARGE   2048

// Batería
#define POWER_3V3_PIN           P00
#define POWER_12V_PIN           P01
#define POWER_2V5_PIN           P02
#define POWER_STABILIZE_DELAY   1

// ADS124S08
#define ADS124S08_CS_PIN        P05
#define ADS124S08_DRDY_PIN      P06
#define ADS124S08_RST_PIN       P13
#define ADS124S08_START_PIN     P14
#define SPI_ADC_CLOCK           1000000

// FlowSensor
#define FLOW_SENSOR_PIN         0

// Namespaces analógicos
#define NAMESPACE_NTC100K   "ntc_100k"
#define NAMESPACE_NTC10K    "ntc_10k"
#define NAMESPACE_COND      "cond"
#define NAMESPACE_PH        "ph"

// Calibración NTC 100K
#define DEFAULT_T1_100K     25.0
#define DEFAULT_R1_100K     100000.0
#define DEFAULT_T2_100K     35.0
#define DEFAULT_R2_100K     64770.0
#define DEFAULT_T3_100K     45.0
#define DEFAULT_R3_100K     42530.0

// Calibración NTC 10K
#define DEFAULT_T1_10K      25.0
#define DEFAULT_R1_10K      10000.0
#define DEFAULT_T2_10K      35.0
#define DEFAULT_R2_10K      6477.0
#define DEFAULT_T3_10K      45.0
#define DEFAULT_R3_10K      4253.0

// Calibración Conductividad
#define CONDUCTIVITY_DEFAULT_V1    0.010314f
#define CONDUCTIVITY_DEFAULT_T1    84.0f
#define CONDUCTIVITY_DEFAULT_V2    0.175384f
#define CONDUCTIVITY_DEFAULT_T2    1413.0f
#define CONDUCTIVITY_DEFAULT_V3    1.407745f
#define CONDUCTIVITY_DEFAULT_T3    12880.0f
#define TEMP_COEF_COMPENSATION     0.02f
#define CONDUCTIVITY_DEFAULT_TEMP  24.22f

// Calibración pH
#define PH_DEFAULT_V1          0.4425
#define PH_DEFAULT_T1          4.01
#define PH_DEFAULT_V2          0.001
#define PH_DEFAULT_T2          6.86
#define PH_DEFAULT_V3         -0.32155
#define PH_DEFAULT_T3          9.18
#define PH_DEFAULT_TEMP        25.0

// Claves NTC100K
#define KEY_NTC100K_T1         "n100k_t1"
#define KEY_NTC100K_R1         "n100k_r1"
#define KEY_NTC100K_T2         "n100k_t2"
#define KEY_NTC100K_R2         "n100k_r2"
#define KEY_NTC100K_T3         "n100k_t3"
#define KEY_NTC100K_R3         "n100k_r3"

// Claves NTC10K
#define KEY_NTC10K_T1          "n10k_t1"
#define KEY_NTC10K_R1          "n10k_r1"
#define KEY_NTC10K_T2          "n10k_t2"
#define KEY_NTC10K_R2          "n10k_r2"
#define KEY_NTC10K_T3          "n10k_t3"
#define KEY_NTC10K_R3          "n10k_r3"

// Claves Conductividad
#define KEY_CONDUCT_CT         "c_ct"
#define KEY_CONDUCT_CC         "c_cc"
#define KEY_CONDUCT_V1         "c_v1"
#define KEY_CONDUCT_T1         "c_t1"
#define KEY_CONDUCT_V2         "c_v2"
#define KEY_CONDUCT_T2         "c_t2"
#define KEY_CONDUCT_V3         "c_v3"
#define KEY_CONDUCT_T3         "c_t3"

// Claves pH
#define KEY_PH_V1              "ph_v1"
#define KEY_PH_T1              "ph_t1"
#define KEY_PH_V2              "ph_v2"
#define KEY_PH_T2              "ph_t2"
#define KEY_PH_V3              "ph_v3"
#define KEY_PH_T3              "ph_t3"
#define KEY_PH_CT              "ph_ct"

// Configuración default sensores
#define  DEFAULT_SENSOR_CONFIGS { \
    {"0", "NTC1",  N100K, true}, \
    {"1", "NTC2",  N100K, true}, \
    {"2", "NTC3",  N10K, true}, \
    {"3", "HDS10",  HDS10, true}, \
    {"4", "COND",  COND, true}, \
    {"5", "SM1",   SOILH, true}, \
    {"8", "PH",    PH, true}, \
    {"R", "RTD1",  RTD, true}, \
    {"D", "DS1",   DS18B20, true}, \
    {"I2C", "SHT30", SHT30, true} \
}

// Sin sensores Modbus registrados
#define DEFAULT_MODBUS_SENSOR_CONFIGS { }

// Límites de temperatura NTC para evitar lecturas erróneas cuando esta desconectado
#define NTC_TEMP_MIN           -20.0   // Temperatura mínima válida en °C
#define NTC_TEMP_MAX            100.0   // Temperatura máxima válida en °C

#endif


#ifdef DEVICE_TYPE_MODBUS

// Pines generales
#define I2C_SDA_PIN         19
#define I2C_SCL_PIN         18
#define I2C_ADDRESS_PCA9555 0x20

// SPI
#define SPI_SCK_PIN         10
#define SPI_MISO_PIN        6
#define SPI_MOSI_PIN        7
#define SPI_RTD_CLOCK       1000000
#define SPI_RADIO_CLOCK     100000

// PT100
#define PT100_CS_PIN        P03

// FlowSensor
#define FLOW_SENSOR_PIN     0

// Batería
#define BATTERY_PIN         1

// Modo Config
#define CONFIG_PIN          2
#define CONFIG_TRIGGER_TIME 5000
#define CONFIG_TIMEOUT      30000
#define CONFIG_LED_PIN      P11

// LoRa
#define LORA_NSS_PIN        8
#define LORA_BUSY_PIN       4
#define LORA_RST_PIN        5
#define LORA_DIO1_PIN       3
#define MAX_LORA_PAYLOAD    200

// Serial
#define SERIAL_BAUD_RATE         9600

// Deep Sleep
#define DEFAULT_TIME_TO_SLEEP   30

// Identificadores
#define DEFAULT_DEVICE_ID   "DEV01"
#define DEFAULT_STATION_ID  "ST001"

// LoRa (OTAA)
#define DEFAULT_JOIN_EUI    "00,00,00,00,00,00,00,00"
#define DEFAULT_DEV_EUI     "1f,d4,e6,68,46,8c,e1,b7"
#define DEFAULT_APP_KEY     "1d,fb,69,80,69,d6,a0,7e,5d,bf,29,ba,6b,37,d3,04"
#define DEFAULT_NWK_KEY     "82,91,e9,55,19,ab,c0,6c,86,25,63,68,e7,f4,5a,89"

// LoRa Region y SubBand
#define LORA_REGION         US915
#define LORA_SUBBAND        2

#define BLE_SERVICE_UUID             "180A"
#define BLE_CHAR_SYSTEM_UUID         "2A37"
#define BLE_CHAR_SENSORS_UUID        "2A40"
#define BLE_CHAR_LORA_CONFIG_UUID    "2A41"
#define BLE_DEVICE_PREFIX            "AGRICOS_"

// Calibración batería
const double R1 = 470000.0;
const double R2 = 1500000.0;
const double conversionFactor = (R1 + R2) / R1;

// Namespaces
#define NAMESPACE_SYSTEM                "system"
#define NAMESPACE_SENSORS               "sensors"
#define NAMESPACE_LORAWAN               "lorawan"
#define NAMESPACE_LORA_SESSION          "lorasession"
#define NAMESPACE_SENSORS_MODBUS        "sensors_modbus"

// Claves
#define KEY_INITIALIZED                  "initialized"
#define KEY_SLEEP_TIME                   "sleep_time"
#define KEY_STATION_ID                   "stationId"
#define KEY_DEVICE_ID                    "deviceId"
#define KEY_VOLT                         "volt"
#define KEY_SENSOR                       "k"
#define KEY_SENSOR_ID                    "id"
#define KEY_SENSOR_ID_TEMPERATURE_SENSOR "ts"
#define KEY_SENSOR_TYPE                  "t"
#define KEY_SENSOR_ENABLE                "e"
#define KEY_LORA_JOIN_EUI                "joinEUI"
#define KEY_LORA_DEV_EUI                 "devEUI"
#define KEY_LORA_NWK_KEY                 "nwkKey"
#define KEY_LORA_APP_KEY                 "appKey"
#define KEY_LORAWAN_SESSION              "lorawan_session"

// Claves para Modbus
#define KEY_MODBUS_SENSOR_ID    "id"
#define KEY_MODBUS_SENSOR_TYPE  "t"
#define KEY_MODBUS_SENSOR_ADDR  "a"
#define KEY_MODBUS_SENSOR_ENABLE "e"

// Configuración Modbus
#define MODBUS_BAUDRATE         9600
#define MODBUS_SERIAL_CONFIG    SERIAL_8N1
#define MODBUS_RESPONSE_TIMEOUT 300  // Tiempo de espera para respuesta en ms
#define MODBUS_MAX_RETRY        3     // Número máximo de intentos de lectura Modbus

// Tamaños JSON
#define JSON_DOC_SIZE_SMALL   300
#define JSON_DOC_SIZE_MEDIUM  1024
#define JSON_DOC_SIZE_LARGE   2048

// Power management
#define POWER_3V3_PIN           P00
#define POWER_12V_PIN           P01
#define POWER_STABILIZE_DELAY   20

// Configuración default sensores
#define DEFAULT_SENSOR_CONFIGS { \
    {"R", "RTD1", RTD, 0, "", true}, \
    {"D", "DS1", DS18B20, 0, "", true}, \
    {"I2C", "SHT30", SHT30, 0, "", true} \
}

// Sin sensores Modbus registrados
#define DEFAULT_MODBUS_SENSOR_CONFIGS { }

#endif

#endif // CONFIG_H
