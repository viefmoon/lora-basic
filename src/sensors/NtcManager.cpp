#include "sensors/NtcManager.h"
#include <cmath>  // Para fabs() y otras funciones matemáticas
#include "config_manager.h"
#include "debug.h"
#include "config.h"  // Para acceder a NTC_TEMP_MIN y NTC_TEMP_MAX

#ifdef DEVICE_TYPE_ANALOGIC
#include "ADS124S08.h"
#include "AdcUtilities.h"
extern ADS124S08 ADC;

void NtcManager::calculateSteinhartHartCoeffs(double T1, double R1,
                                          double T2, double R2,
                                          double T3, double R3,
                                          double &A, double &B, double &C) 
{
    // Ecuación de Steinhart-Hart:
    // 1/T = A + B*ln(R) + C*(ln(R))^3
    
    double L1 = log(R1);
    double L2 = log(R2);
    double L3 = log(R3);
    
    double Y1 = 1.0 / T1;
    double Y2 = 1.0 / T2;
    double Y3 = 1.0 / T3;
    
    // Resolver sistema de ecuaciones para encontrar A, B, C
    double L1_3 = L1 * L1 * L1;
    double L2_3 = L2 * L2 * L2;
    double L3_3 = L3 * L3 * L3;
    
    double denominator = (L2 - L1) * (L3 - L1) * (L3 - L2);
    
    // Protección contra división por cero
    if (fabs(denominator) < 1e-10) {
        // Asignar NAN para indicar claramente el error
        A = NAN;
        B = NAN;
        C = NAN;
        return;
    }
    
    // Calcular C
    C = ((Y2 - Y1) * (L3 - L1) - (Y3 - Y1) * (L2 - L1)) / 
        ((L2_3 - L1_3) * (L3 - L1) - (L3_3 - L1_3) * (L2 - L1));
    
    // Calcular B
    B = ((Y2 - Y1) - C * (L2_3 - L1_3)) / (L2 - L1);
    
    // Calcular A
    A = Y1 - B * L1 - C * L1_3;
}

double NtcManager::steinhartHartTemperature(double resistance, double A, double B, double C) 
{
    if (resistance <= 0.0) {
        return NAN;
    }
    double lnR = log(resistance);
    double invT = A + B * lnR + C * lnR*lnR*lnR;  // 1/T en Kelvin^-1
    double tempK = 1.0 / invT;                   // Kelvin
    double tempC = tempK - 273.15;               // °C
    return tempC;
}

double NtcManager::computeNtcResistanceFromBridge(double diffVoltage)
{
    // La rama de referencia es 1.25 V. 
    // diffVoltage = (Vneg - 1.25)
    
    // Corregimos la interpretación del voltaje diferencial:
    // Para NTC, cuando la temperatura aumenta, la resistencia disminuye y el voltaje Vneg aumenta
    // Por lo tanto, diffVoltage aumenta con la temperatura
    
    // Vneg = diffVoltage + 1.25
    double Vneg = diffVoltage + 1.25;
    
    // Validación de rangos
    if (Vneg <= 0.0 || Vneg >= 2.5) {
        return -1.0;  // Indica valor inválido
    }
    
    // Fórmula corregida: Si Vneg aumenta (mayor temperatura), Rntc debe disminuir
    double Rntc = 100000.0 * ((2.5 - Vneg) / Vneg);
    
    return Rntc;
}

double NtcManager::computeNtcResistanceFromVoltageDivider(double voltage, double vRef, double rFixed, bool ntcTop)
{
    // Validación de rangos
    if (voltage <= 0.0 || voltage >= vRef) {
        return -1.0;  // Indica valor inválido
    }
    
    // Calcular resistencia del NTC
    double Rntc;
    
    if (ntcTop) {
        // NTC conectado a Vref (arriba) y resistencia fija a GND (abajo)
        // Fórmula: Rntc = rFixed * (vRef - voltage) / voltage
        Rntc = rFixed * ((vRef - voltage) / voltage);
    } else {
        // NTC conectado a GND (abajo) y resistencia fija a Vref (arriba)
        // Fórmula: Rntc = rFixed * voltage / (vRef - voltage)
        Rntc = rFixed * (voltage / (vRef - voltage));
    }
    
    return Rntc;
}

double NtcManager::readNtc100kTemperature(const char* configKey) {
    // Obtener calibración NTC100K de la configuración
    double t1=25.0, r1=100000.0, t2=35.0, r2=64770.0, t3=45.0, r3=42530.0;
    ConfigManager::getNTC100KConfig(t1, r1, t2, r2, t3, r3);

    // Pasar °C a Kelvin
    double T1K = t1 + 273.15;
    double T2K = t2 + 273.15;
    double T3K = t3 + 273.15;

    // Calcular coeficientes Steinhart-Hart
    double A=0, B=0, C=0;
    calculateSteinhartHartCoeffs(T1K, r1, T2K, r2, T3K, r3, A, B, C);
    
    // Elegir canal según sensorId: "NTC1" => AIN1+/AIN0-, "NTC2" => AIN3+/AIN2-
    uint8_t muxConfig = 0; 
    if (strcmp(configKey, "0") == 0) {
        DEBUG_PRINTLN("NTC100K 0");
        muxConfig = ADS_P_AIN1 | ADS_N_AIN0;  // AIN1+ / AIN0-
    } else if (strcmp(configKey, "1") == 0) {
        DEBUG_PRINTLN("NTC100K 1");
        muxConfig = ADS_P_AIN3 | ADS_N_AIN2;  // AIN3+ / AIN2-
    } else {
        // Si no coincide con "NTC1" ni "NTC2", retornamos NAN
        return NAN;
    }

    // Medir voltaje diferencial
    float diffVoltage = AdcUtilities::measureAdcDifferential(muxConfig);
    if (isnan(diffVoltage)) {
        return NAN;
    }

    // Calcular la resistencia NTC en ohms
    double Rntc = computeNtcResistanceFromBridge(diffVoltage);
    if (Rntc <= 0.0) {
        return NAN;
    }

    // Usar Steinhart-Hart para calcular la temperatura en °C
    double tempC = steinhartHartTemperature(Rntc, A, B, C);
    
    // Validar que el valor de temperatura está dentro de los límites aceptables
    if (isnan(tempC) || tempC < NTC_TEMP_MIN || tempC > NTC_TEMP_MAX) {
        return NAN;
    }
    
    return tempC;
}

double NtcManager::readNtc10kTemperature() {
    // Obtener calibración NTC10K de la configuración
    // Usando valores por defecto para un NTC10K común
    double t1=25.0, r1=10000.0, t2=50.0, r2=3893.0, t3=85.0, r3=1218.0;
    ConfigManager::getNTC10KConfig(t1, r1, t2, r2, t3, r3);

    // Pasar °C a Kelvin
    double T1K = t1 + 273.15;
    double T2K = t2 + 273.15;
    double T3K = t3 + 273.15;

    // Calcular coeficientes Steinhart-Hart
    double A=0, B=0, C=0;
    calculateSteinhartHartCoeffs(T1K, r1, T2K, r2, T3K, r3, A, B, C);

    // NTC3 está en el canal AIN11 con AINCOM
    uint8_t muxConfig = ADS_P_AIN11 | ADS_N_AIN8;
    
    // Medir voltaje single-ended
    float voltage = AdcUtilities::measureAdcDifferential(muxConfig);
    if (isnan(voltage)) {
        return NAN;
    }

    // Calcular la resistencia NTC
    // El NTC en NTC3 está conectado entre 2.5V y el punto medio con resistencia de 10k a GND
    double vRef = 2.5; // Voltaje de referencia
    double rFixed = 10000.0; // Resistencia fija (10k)
    bool ntcTop = true; // NTC está conectado a Vref (arriba)
    
    double Rntc = computeNtcResistanceFromVoltageDivider(voltage, vRef, rFixed, ntcTop);
    if (Rntc <= 0.0) {
        return NAN;
    }

    // Usar Steinhart-Hart para calcular la temperatura en °C
    double tempC = steinhartHartTemperature(Rntc, A, B, C);
    
    // Validar que el valor de temperatura está dentro de los límites aceptables
    if (isnan(tempC) || tempC < NTC_TEMP_MIN || tempC > NTC_TEMP_MAX) {
        return NAN;
    }
    
    return tempC;
}

#endif // DEVICE_TYPE_ANALOGIC 