#include "MAX31865.h"
#include <math.h>   // Para sqrt

/**
 * @brief Constructor con PCA9555.
 */
MAX31865_RTD::MAX31865_RTD(
    ptd_type type,
    SPIClass& spi,
    SPISettings& spiSettings,
    PCA9555& pca,
    uint8_t pcaPinCS
)
{
  _spi = &spi;
  _spiSettings = &spiSettings;
  this->type = type;
  this->_usePCA = true;
  this->_pca = &pca;
  this->_pcaPinCS = pcaPinCS;
  this->_csPinMCU = 0xFF;
}

/**
 * @brief Constructor con pin nativo de MCU.
 */
MAX31865_RTD::MAX31865_RTD(
    ptd_type type,
    SPIClass& spi,
    SPISettings& spiSettings,
    uint8_t csPin
)
{
  _spi = &spi;
  _spiSettings = &spiSettings;
  this->type = type;
  this->_usePCA = false;
  this->_pca = nullptr;
  this->_pcaPinCS = 0xFF;
  this->_csPinMCU = csPin;
}

// -----------------------------------------------------------------------
void MAX31865_RTD::configure(bool v_bias, bool conversion_mode, bool one_shot,
                             bool three_wire, uint8_t fault_cycle, bool fault_clear,
                             bool filter_50hz,
                             uint16_t low_threshold,
                             uint16_t high_threshold)
{
  uint8_t control_bits = 0;
  // Arma la máscara
  control_bits |= (v_bias ? 0x80 : 0);
  control_bits |= (conversion_mode ? 0x40 : 0);
  control_bits |= (one_shot ? 0x20 : 0);
  control_bits |= (three_wire ? 0x10 : 0);
  control_bits |= (fault_cycle & 0b00001100);
  control_bits |= (fault_clear ? 0x02 : 0);
  control_bits |= (filter_50hz ? 0x01 : 0);

  this->configuration_control_bits   = control_bits;
  this->configuration_low_threshold  = low_threshold;
  this->configuration_high_threshold = high_threshold;

  // Llama a reconfigure() para escribir valores
  reconfigure();
}

// -----------------------------------------------------------------------
void MAX31865_RTD::reconfigure()
{
  // Inicia transacción SPI
  _spi->beginTransaction(*_spiSettings);

  // Escribe config
  setCSLow();
  _spi->transfer(0x80); // Dirección de escritura del registro config
  _spi->transfer(this->configuration_control_bits);
  setCSHigh();

  // Umbrales
  setCSLow();
  _spi->transfer(0x83); // Registro de umbrales
  _spi->transfer( (this->configuration_high_threshold >> 8) & 0xFF );
  _spi->transfer(  this->configuration_high_threshold       & 0xFF );
  _spi->transfer( (this->configuration_low_threshold  >> 8) & 0xFF );
  _spi->transfer(  this->configuration_low_threshold        & 0xFF );
  setCSHigh();

  // Cierra transacción
  _spi->endTransaction();
}

// -----------------------------------------------------------------------
uint8_t MAX31865_RTD::read_all()
{
  uint16_t combined_bytes = 0;

  _spi->beginTransaction(*_spiSettings);

  setCSLow();
  // Indica que leeremos desde la dirección 0 (se manda 0x00)
  _spi->transfer(0x00);

  // Orden de lectura: Config, RTD, High Fault Th, Low Fault Th, Status
  measured_configuration = _spi->transfer(0x00);

  combined_bytes  = ((uint16_t)_spi->transfer(0x00) << 8);
  combined_bytes |=          _spi->transfer(0x00);
  measured_resistance = (combined_bytes >> 1);

  combined_bytes  = ((uint16_t)_spi->transfer(0x00) << 8);
  combined_bytes |=          _spi->transfer(0x00);
  measured_high_threshold = (combined_bytes >> 1);

  combined_bytes  = ((uint16_t)_spi->transfer(0x00) << 8);
  combined_bytes |=          _spi->transfer(0x00);
  measured_low_threshold = (combined_bytes >> 1);

  measured_status = _spi->transfer(0x00);

  delayMicroseconds(20);
  setCSHigh();
  _spi->endTransaction();

  // Reconfigura si resistencia=0 o hay falla
  if ((measured_resistance == 0) || (measured_status != 0)) {
    reconfigure();
  }
  return measured_status;
}

// -----------------------------------------------------------------------
double MAX31865_RTD::temperature() const
{
  // Callendar-Van Dusen
  static const double a2 = 2.0 * RTD_B;
  static const double b_sq = (RTD_A * RTD_A);
  double rtd_resistance = (type == RTD_PT100) ? RTD_RESISTANCE_PT100 : RTD_RESISTANCE_PT1000;

  double c = 1.0 - (resistance() / rtd_resistance);
  double D = b_sq - 2.0 * a2 * c;
  double tempC = (-RTD_A + sqrt(D)) / a2;

  return tempC;
}

void MAX31865_RTD::setCSLow() {
  if (_usePCA && _pca) {
    _pca->digitalWrite(_pcaPinCS, LOW);
    delayMicroseconds(5);
  } else {
    digitalWrite(_csPinMCU, LOW);
  }
}

void MAX31865_RTD::setCSHigh() {
  if (_usePCA && _pca) {
    _pca->digitalWrite(_pcaPinCS, HIGH);
  } else {
    digitalWrite(_csPinMCU, HIGH);
  }
}

double MAX31865_RTD::singleMeasurement(uint16_t conversionDelayMs) {
    // Activar BIAS
    uint8_t originalConfig = configuration_control_bits;
    configuration_control_bits |= 0x80;  // Set VBIAS on
    reconfigure();
    
    delay(10);  // Esperar a que VBIAS se estabilice
    
    // Iniciar conversión única
    configuration_control_bits |= 0x20;  // Set one-shot bit
    reconfigure();
    
    // Esperar a que la conversión termine
    delay(conversionDelayMs);
    
    // Leer resultado
    read_all();
    
    // Desactivar BIAS y one-shot
    configuration_control_bits = originalConfig;
    reconfigure();
    
    return temperature();
}

// Nuevo método begin para inicializar los pines
bool MAX31865_RTD::begin() {
    if (_usePCA && _pca) {
        _pca->pinMode(_pcaPinCS, OUTPUT);
        _pca->digitalWrite(_pcaPinCS, HIGH);
    } else {
        pinMode(_csPinMCU, OUTPUT);
        digitalWrite(_csPinMCU, HIGH);
    }
    return true;
}
