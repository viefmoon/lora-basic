#ifndef _MAX31865_H
#define _MAX31865_H

#include <stdint.h>
#include <SPI.h>
#include "clsPCA9555.h"


#define MAX31865_FAULT_HIGH_THRESHOLD  ( 1 << 7 )
#define MAX31865_FAULT_LOW_THRESHOLD   ( 1 << 6 )
#define MAX31865_FAULT_REFIN           ( 1 << 5 )
#define MAX31865_FAULT_REFIN_FORCE     ( 1 << 4 )
#define MAX31865_FAULT_RTDIN_FORCE     ( 1 << 3 )
#define MAX31865_FAULT_VOLTAGE         ( 1 << 2 )

#define MAX31865_FAULT_DETECTION_NONE      ( 0x00 << 2 )
#define MAX31865_FAULT_DETECTION_AUTO      ( 0x01 << 2 )
#define MAX31865_FAULT_DETECTION_MANUAL_1  ( 0x02 << 2 )
#define MAX31865_FAULT_DETECTION_MANUAL_2  ( 0x03 << 2 )



/* RTD data, RTD current, and measurement reference
   voltage. The ITS-90 standard is used; other RTDs
   may have coefficients defined by the DIN 43760 or
   the U.S. Industrial (American) standard. */
#define RTD_A_ITS90         3.9080e-3
#define RTD_A_USINDUSTRIAL  3.9692e-3
#define RTD_A_DIN43760      3.9848e-3
#define RTD_B_ITS90         -5.870e-7
#define RTD_B_USINDUSTRIAL  -5.8495e-7
#define RTD_B_DIN43760      -5.8019e-7
/* RTD coefficient C is required only for temperatures
   below 0 deg. C.  The selected RTD coefficient set
   is specified below. */
#define SELECT_RTD_HELPER(x) x
#define SELECT_RTD(x) SELECT_RTD_HELPER(x)
#define RTD_A         SELECT_RTD(RTD_A_ITS90)
#define RTD_B         SELECT_RTD(RTD_B_ITS90)
/*
 * The reference resistor on the hardware; see the MAX31865 datasheet
 * for details.  The values 400 and 4000 Ohm are recommended values for
 * the PT100 and PT1000.
 */
#define RTD_RREF_PT100         430 /* Ohm */
#define RTD_RREF_PT1000       4000 /* Ohm */
/*
 * The RTD resistance at 0 degrees Celcius.  For the PT100, this is 100 Ohm;
 * for the PT1000, it is 1000 Ohm.
 */
#define RTD_RESISTANCE_PT100   100 /* Ohm */
#define RTD_RESISTANCE_PT1000 1000 /* Ohm */

#define RTD_ADC_RESOLUTION  ( 1u << 15 ) /* 15 bits */


/* See the main (MAX31865.cpp) file for documentation of the class methods. */
class MAX31865_RTD
{
public:
  enum ptd_type { RTD_PT100, RTD_PT1000 };

  /**
   * @brief Constructor para usar PCA9555 como CS.
   * @param type         Tipo de RTD (PT100/PT1000)
   * @param spi          Referencia al objeto SPI que usarás (por ej. spiCustom)
   * @param spiSettings  Configuración de SPI (frecuencia, MSB/LSB, modo, etc.)
   * @param pca          Referencia a tu PCA9555
   * @param pcaPinCS     Pin del PCA que actúa como CS
   */
  MAX31865_RTD(
      ptd_type type,
      SPIClass& spi,
      SPISettings& spiSettings,
      PCA9555& pca,
      uint8_t pcaPinCS
  );

    /**
   * @brief Constructor alternativo, usando pin nativo de microcontrolador como CS.
   */
  MAX31865_RTD(
      ptd_type type,
      SPIClass& spi,
      SPISettings& spiSettings,
      uint8_t csPin
  );

  void configure( bool v_bias, bool conversion_mode, bool one_shot, bool three_wire,
                  uint8_t fault_cycle, bool fault_clear, bool filter_50hz,
                  uint16_t low_threshold, uint16_t high_threshold );
  uint8_t read_all( );
  double temperature( ) const;
  uint8_t status( ) const { return( measured_status ); }
  uint16_t low_threshold( ) const { return( measured_low_threshold ); }
  uint16_t high_threshold( ) const  { return( measured_high_threshold ); }
  uint16_t raw_resistance( ) const { return( measured_resistance ); }
  double resistance( ) const
  {
    const double rtd_rref =
      ( this->type == RTD_PT100 ) ? (double)RTD_RREF_PT100 : (double)RTD_RREF_PT1000;
    return( (double)raw_resistance( ) * rtd_rref / (double)RTD_ADC_RESOLUTION );
  }

  // Nuevo método para medición única
  double singleMeasurement(uint16_t conversionDelayMs = 100);

  // Agregar el método begin
  bool begin();

private:
  void reconfigure();
  void setCSLow();
  void setCSHigh();

  /* SPI */
  SPIClass* _spi;           ///< Puntero a la clase SPI
  SPISettings* _spiSettings;   ///< Puntero a SPISettings

  /* PCA9555 / Pin CS */
  PCA9555* _pca = nullptr; 
  uint8_t  _pcaPinCS = 0xFF;
  bool     _usePCA   = false;

  uint8_t  _csPinMCU = 0xFF;     ///< CS pin nativo (si no usamos PCA)

  /* Config RTD */
  ptd_type type;
  uint8_t  configuration_control_bits;
  uint16_t configuration_low_threshold;
  uint16_t configuration_high_threshold;

  /* Valores leídos del dispositivo */
  uint8_t  measured_configuration = 0;
  uint16_t measured_resistance    = 0;
  uint16_t measured_high_threshold= 0;
  uint16_t measured_low_threshold = 0;
  uint8_t  measured_status        = 0;
};

#endif /* _MAX31865_H */
