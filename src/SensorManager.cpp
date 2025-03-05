#include "SensorManager.h"
#include <Wire.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include "ADS131M08.h"
#include "MAX31865.h"
#include "RTCManager.h"
#include "sensor_types.h"
#include "config.h"
#include <Preferences.h>
#include "config_manager.h"

// =============== Implementaciones de los métodos de SensorManager ===============

void SensorManager::beginSensors() {
    // Inicializar pines de SPI (SS) y luego SPI
    initializeSPISSPins();
    spi.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    // Encender alimentación 3.3V
    powerManager.power3V3On();

    // Inicializar RTD Y configurarlo
    rtd.begin();
    {
        bool vBias = true;
        bool autoConvert = true;
        bool oneShot = false;
        bool threeWire = false;
        uint8_t faultCycle = MAX31865_FAULT_DETECTION_NONE;
        bool faultClear = true;
        bool filter50Hz = true;
        uint16_t lowTh = 0x0000;
        uint16_t highTh = 0x7fff;
        rtd.configure(vBias, autoConvert, oneShot, threeWire, faultCycle, faultClear, filter50Hz, lowTh, highTh);
    }

    // Inicializar ADC
    adc.begin(ADC_CS_PIN, ADC_DRDY_PIN, ADC_RST_PIN);
    adc.hardReset(); //garantiza que el ADC esté en estado inicial
    delay(10);
    adc.setPowerMode(POWER_MODE_HIGH_RESOLUTION);
    adc.setOsr(OSR_16384);

    // Inicializar DS18B20
    dallasTemp.begin();
    dallasTemp.setResolution(12);
    // Lectura inicial (descartable)
    dallasTemp.requestTemperatures();
    delay(750);
    dallasTemp.getTempCByIndex(0);
}

void SensorManager::initializeSPISSPins() {
    // Inicializar SS del LORA que está conectado directamente al ESP32
    pinMode(LORA_NSS_PIN, OUTPUT);
    digitalWrite(LORA_NSS_PIN, HIGH);

    // Inicializar SS conectados al expansor I2C
    ioExpander.pinMode(PT100_CS_PIN, OUTPUT);  
    ioExpander.pinMode(ADC_CS_PIN, OUTPUT);  

    // Establecer todos los SS del expansor en HIGH
    ioExpander.digitalWrite(PT100_CS_PIN, HIGH);
    ioExpander.digitalWrite(ADC_CS_PIN, HIGH);
}

// ========== Funciones de conversión ==========

// Se crea una función auxiliar para unificar la conversión en sensores NTC (ambos 100K y 10K)
// La función recibe el voltaje medido y los parámetros de calibración (T1, R1, T2, R2, T3, R3)
static float convertNTCGeneric(float voltage, double T1, double R1, double T2, double R2, double T3, double R3) {
    // Convertir temperaturas a Kelvin
    const double T1K = T1 + 273.15;
    const double T2K = T2 + 273.15;
    const double T3K = T3 + 273.15;
    
    // El voltaje medido es diferencial
    double V_diff = voltage; // Ya viene como diferencial
    
    // Fórmula para puente balanceado (se asume que el punto medio es 3.3/2.0)
    double R_ntc = R1 * (3.3/2.0 - V_diff) / (3.3/2.0 + V_diff);
    
    // Cálculo de coeficientes Steinhart-Hart
    const double L1 = log(R1);
    const double L2 = log(R2);
    const double L3 = log(R3);
    
    const double Y1 = 1.0 / T1K;
    const double Y2 = 1.0 / T2K;
    const double Y3 = 1.0 / T3K;
    
    const double gamma2 = (Y2 - Y1) / (L2 - L1);
    const double gamma3 = (Y3 - Y1) / (L3 - L1);
    
    const double C_coef = (gamma3 - gamma2) / (L3 - L2) * (1.0 / (L1 + L2 + L3));
    const double B_coef = gamma2 - C_coef * (pow(L1,2) + L1 * L2 + pow(L2,2));
    const double A_coef = Y1 - L1 * (B_coef + C_coef * pow(L1,2));
    
    // Aplicar la ecuación Steinhart-Hart
    const double lnR = log(R_ntc);
    const double invT = A_coef + B_coef * lnR + C_coef * pow(lnR, 3);
    float temperature = (1.0f / invT) - 273.15f;
    return temperature;
}

float convertNTC100K(float voltage) {
    
    double T1, R1, T2, R2, T3, R3;
    ConfigManager::getNTC100KConfig(T1, R1, T2, R2, T3, R3);
    float temperature = convertNTCGeneric(voltage, T1, R1, T2, R2, T3, R3);
    
    return temperature;
}

float convertNTC10K(float voltage) {
    
    double T1, R1, T2, R2, T3, R3;
    ConfigManager::getNTC10KConfig(T1, R1, T2, R2, T3, R3);
    float temperature = convertNTCGeneric(voltage, T1, R1, T2, R2, T3, R3);
    
    return temperature;
}

float convertHDS10(float voltage) {
    
    const float V_diff = voltage;
    const float a = -147.445f;
    const float b = 13.088f;
    const float c = 100.302f;
    
    float humidity = a * pow(V_diff, 2) + b * V_diff + c;
    humidity = constrain(humidity, 0.0f, 100.0f);
    
    return humidity;
}

float convertSoilMoisture(float voltage) {
    
    const float SOIL_MOISTURE_FULL_SCALE_V = 1.17f; //voltaje máximo de la sonda
    const float slope = 100.0f / SOIL_MOISTURE_FULL_SCALE_V;
    float humidity = voltage * slope;
    humidity = constrain(humidity, 0.0f, 100.0f);
    
    return humidity;
}

float convertConductivity(float voltage, float solutionTemp) {
    
    float calTemp, coefComp, V1, T1, V2, T2, V3, T3;
    ConfigManager::getConductivityConfig(calTemp, coefComp, V1, T1, V2, T2, V3, T3);

    // Si solutionTemp es NAN, usar la temperatura de calibración como valor por defecto
    if (isnan(solutionTemp)) {
        solutionTemp = calTemp;
    }

    // Matriz para resolver el sistema de ecuaciones
    // Basado en 3 puntos de calibración
    const double det = V1*V1*(V2 - V3) - V1*(V2*V2 - V3*V3) + (V2*V2*V3 - V2*V3*V3);
    
    // Calcular coeficientes solo si el determinante no es cero
    if(fabs(det) > 1e-6) {
        const double a = (T1*(V2 - V3) - T2*(V1 - V3) + T3*(V1 - V2)) / det;
        const double b = (T1*(V3*V3 - V2*V2) + T2*(V1*V1 - V3*V3) + T3*(V2*V2 - V1*V1)) / det;
        const double c = (T1*(V2*V2*V3 - V2*V3*V3) - T2*(V1*V1*V3 - V1*V3*V3) + T3*(V1*V1*V2 - V1*V2*V2)) / det;
        
        // Aplicar compensación de temperatura usando el parámetro recibido
        const double compensation = 1.0 + coefComp * (solutionTemp - calTemp);
        double compensatedVoltage = voltage / compensation;
        double conduc = a * (compensatedVoltage * compensatedVoltage) 
                    + b * compensatedVoltage 
                    + c;

        return fmax(conduc, 0.0);
    }
    else {
        return NAN;
    }
}

float convertBattery(float voltage) {
    //divisor de voltaje
    const double R1 = 470000.0; //resistencias fija
    const double R2 = 1500000.0;
    const double conversionFactor = (R1 + R2) / R1;
    float batteryVoltage = (float)(voltage * conversionFactor);
    
    return batteryVoltage;
}

float convertPH(float voltage, float solutionTemp) {
    
    float V1, T1, V2, T2, V3, T3, TEMP_CAL;
    ConfigManager::getPHConfig(V1, T1, V2, T2, V3, T3, TEMP_CAL);

    // Si solutionTemp es NAN, usar la temperatura de calibración como valor por defecto
    if (isnan(solutionTemp)) {
        solutionTemp = TEMP_CAL;
    }

    // Datos de calibración (pH, voltaje)
    const double pH_calib[] = {T1, T2, T3};
    const double V_calib[] = {V1, V2, V3};
    const int n = 3; // Número de puntos de calibración

    // Calcular sumatorias necesarias para mínimos cuadrados
    double sum_pH = 0.0;
    double sum_V = 0.0;
    double sum_pHV = 0.0;
    double sum_pH2 = 0.0;

    for (int i = 0; i < n; i++) {
        sum_pH += pH_calib[i];
        sum_V += V_calib[i];
        sum_pHV += pH_calib[i] * V_calib[i];
        sum_pH2 += pH_calib[i] * pH_calib[i];
    }

    // Calcular la pendiente S usando mínimos cuadrados
    double S_CAL = ((n * sum_pHV) - (sum_pH * sum_V)) / ((n * sum_pH2) - (sum_pH * sum_pH));

    // Calcular el offset E0 usando mínimos cuadrados
    double E0 = ((sum_V) + (S_CAL * sum_pH)) / n;

    // Ajustar la pendiente según la temperatura actual usando la ecuación de Nernst
    const double tempK = (solutionTemp + 273.15);
    const double tempCalK = (TEMP_CAL + 273.15);
    const double S_T = S_CAL * (tempK / tempCalK);

    // Calcular pH usando la ecuación de Nernst ajustada: pH = (E0 - E) / S(T)
    double pH = ((E0 + voltage) / S_T);
    // Limitar el pH a un rango físicamente posible (0-14)
    pH = constrain(pH, 0.0, 14.0);

    return pH;
}

// Añadir variable estática para almacenar las lecturas del ADC
static AdcOutput lastAdcReading = {0};
static bool adcReadingValid = false;

bool SensorManager::updateADCReadings(uint32_t timeout_ms) {
    unsigned long startTime = millis();
    adcReadingValid = false;
    
    // Esperar hasta que haya datos disponibles o se alcance el timeout
    while ((millis() - startTime) < timeout_ms) {
        if (adc.isDataReady()) {
            lastAdcReading = adc.readAdcFloat();
            adcReadingValid = true;
            
            // Se han quitado los prints de debug
            return true;
        }
        yield(); // Permitir que se ejecuten otras tareas en vez de busy-wait
    }
    
    // Si hubo timeout, se asignan 0 a todas las lecturas
    for (int i = 0; i < 8; i++) {
        lastAdcReading.ch[i].f = 0.0f;
    }
    return false;
}

float SensorManager::readAnalogSensor(const SensorConfig &cfg) {
    // Añadir logs de diagnóstico
    float rawValue = lastAdcReading.ch[cfg.channel].f;
    
    // Añadir validación de canal
    if (cfg.channel < 0 || cfg.channel >= 8) {
        return NAN;
    }

    switch(cfg.type) {
        case N100K:
            return convertNTC100K(rawValue);
            
        case N10K:
            return convertNTC10K(rawValue);
            
        case PH: {
            float solutionTemp = NAN;
            if(strlen(cfg.tempSensorId) > 0) {
                auto sensors = ConfigManager::getAllSensorConfigs();
                for(const auto& sensor : sensors) {
                    if(strcmp(sensor.sensorId, cfg.tempSensorId) == 0) {
                        solutionTemp = getSensorReading(sensor).value;
                        break;
                    }
                }
            }
            return convertPH(rawValue, solutionTemp);
        }
        case COND: {
            float solutionTemp = NAN;
            if(strlen(cfg.tempSensorId) > 0) {
                auto sensors = ConfigManager::getAllSensorConfigs();
                for(const auto& sensor : sensors) {
                    if(strcmp(sensor.sensorId, cfg.tempSensorId) == 0) {
                        solutionTemp = getSensorReading(sensor).value;
                        break;
                    }
                }
            }
            return convertConductivity(rawValue, solutionTemp);
        }
        case SOILH:
            return convertSoilMoisture(rawValue);

        case CONDH: 
            return convertHDS10(rawValue);
        default:
            return NAN;
    }
}

float SensorManager::readRtdSensor() {
    uint8_t status = rtd.read_all();
    if (status == 0) {
        return rtd.temperature(); // °C
    } else {
        return NAN; 
    }
}

float SensorManager::readDallasSensor() {
    dallasTemp.requestTemperatures();
    float temp = dallasTemp.getTempCByIndex(0);
    if (temp == DEVICE_DISCONNECTED_C) {
        return NAN;
    }
    return temp;
}

float SensorManager::readSensorValue(const SensorConfig &cfg) {
    switch(cfg.type) {
        case N100K:
        case N10K:
        case WNTC10K:
        case PH:
        case COND: 
        case SOILH:
        case CONDH:
            return readAnalogSensor(cfg);
        case RTD:
            return readRtdSensor();
        case DS18B20:
            return readDallasSensor();
        default:
            return 0.0;
    }
}

SensorReading SensorManager::getSensorReading(const SensorConfig &cfg) {
    SensorReading reading;
    strncpy(reading.sensorId, cfg.sensorId, sizeof(reading.sensorId) - 1);
    reading.type = cfg.type;

    // Realizar la lectura del sensor
    reading.value = readSensorValue(cfg);

    return reading;
}

float SensorManager::readBatteryVoltage() {
    // Asegurarse de tener una lectura ADC actualizada
    if (!adcReadingValid) {
        updateADCReadings(1000);
    }
    // Se utiliza un canal fijo para la lectura de voltaje, definido como BATTERY_ADC_CHANNEL
    float rawValue = lastAdcReading.ch[BATTERY_ADC_CHANNEL].f;
    return convertBattery(rawValue);
}