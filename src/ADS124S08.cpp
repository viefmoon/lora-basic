/* --COPYRIGHT--,BSD
 * Copyright (c) 2016, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/

#include "ADS124S08.h"
#include "debug.h"

#ifdef DEVICE_TYPE_ANALOGIC

/*
 * This class is based on the ADS124S08.c provided by TI and adapted for use with
 * Particle Photon and other Arduino-like MCU's.
 *
 * Note that the confusingly named "selectDeviceCSLow"
 * actually selects the device and that "releaseChipSelect" deselects it. Not obvious
 * from the naming, but according to the datasheet CS is active LOW.
 */

/*
 * Writes the nCS pin low and waits a while for the device to finish working before
 * handing control back to the caller for a SPI transfer.
 */
void ADS124S08::selectDeviceCSLow(void){
	if (_initialized) {
		_ioExpander->digitalWrite(ADS124S08_CS_PIN, LOW);
	}
}

/*
 * Pulls the nCS pin high. Performs no waiting.
 */
void ADS124S08::releaseChipSelect(void){
	if (_initialized) {
		_ioExpander->digitalWrite(ADS124S08_CS_PIN, HIGH);
	}
}

/*
 * Constructor - solo almacena referencias a los objetos pasados
 */
ADS124S08::ADS124S08(PCA9555& ioExpander, SPIClass& spi, SPISettings& spiSettings)
{
	_ioExpander = &ioExpander;
	_spi = &spi;
	_spiSettings = spiSettings;
	_initialized = false;
	fStart = false;
}

/*
 * Inicializa el dispositivo para su uso
 * Configura los pines y registros
 */
void ADS124S08::init()
{
	if (_initialized) return;
	
	_ioExpander->pinMode(ADS124S08_START_PIN, OUTPUT);
	_ioExpander->pinMode(ADS124S08_RST_PIN, OUTPUT);
	_ioExpander->pinMode(ADS124S08_DRDY_PIN, INPUT);

	_ioExpander->digitalWrite(ADS124S08_START_PIN, LOW);
	_ioExpander->digitalWrite(ADS124S08_RST_PIN, HIGH);

	/* Default register settings */
	registers[ID_ADDR_MASK] 			= 0x08;
	registers[STATUS_ADDR_MASK] 	= 0x80;
	registers[INPMUX_ADDR_MASK]		= 0x01;
	registers[PGA_ADDR_MASK] 			= 0x00;
	registers[DATARATE_ADDR_MASK] = 0x14;
	registers[REF_ADDR_MASK] 			= 0x10;
	registers[IDACMAG_ADDR_MASK] 	= 0x00;
	registers[IDACMUX_ADDR_MASK] 	= 0xFF;
	registers[VBIAS_ADDR_MASK] 		= 0x00;
	registers[SYS_ADDR_MASK]			= 0x10;
	registers[OFCAL0_ADDR_MASK] 	= 0x00;
	registers[OFCAL1_ADDR_MASK] 	= 0x00;
	registers[OFCAL2_ADDR_MASK] 	= 0x00;
	registers[FSCAL0_ADDR_MASK] 	= 0x00;
	registers[FSCAL1_ADDR_MASK] 	= 0x00;
	registers[FSCAL2_ADDR_MASK] 	= 0x40;
	registers[GPIODAT_ADDR_MASK] 	= 0x00;
	registers[GPIOCON_ADDR_MASK]	= 0x00;
	
	_initialized = true;
}

/*
 * Resetea el ADS124S08 usando el pin RST_PIN
 * Secuencia de reset: HIGH -> LOW -> HIGH con delays entre cada cambio
 */
void ADS124S08::ADS124S08_Reset()
{
	if (!_initialized) return;
					// Esperar 100 mSec
	_ioExpander->digitalWrite(ADS124S08_RST_PIN, LOW);
	delay(1);
	_ioExpander->digitalWrite(ADS124S08_RST_PIN, HIGH);
}

void ADS124S08::begin()
{
	if (!_initialized) {
		init();
	}
	
	// Resetear el ADC antes de comenzar
	ADS124S08_Reset();
	
	_spi->begin();
	_spi->beginTransaction(_spiSettings);
	_spi->endTransaction();
}

/*
 * Reads a single register contents from the specified address
 *
 * \param regnum identifies which address to read
 *
 */
char ADS124S08::regRead(unsigned int regnum)
{
	if (!_initialized) return 0;
	
	int i;
	uint8_t ulDataTx[3];
	uint8_t ulDataRx[3];
	ulDataTx[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
	ulDataTx[1] = 0x00;
	ulDataTx[2] = 0x00;
	selectDeviceCSLow();

	_spi->beginTransaction(_spiSettings);

	for(i = 0; i < 3; i++)
		ulDataRx[i] = _spi->transfer(ulDataTx[i]);
	if(regnum < NUM_REGISTERS)
			registers[regnum] = ulDataRx[2];

	_spi->endTransaction();

	releaseChipSelect();
	return ulDataRx[2];
}

/*
 * Reads a group of registers starting at the specified address
 *
 * \param regnum is addr_mask 8-bit mask of the register from which we start reading
 * \param count The number of registers we wish to read
 * \param *location pointer to the location in memory to write the data
 *
 */
void ADS124S08::readRegs(unsigned int regnum, unsigned int count, uint8_t *data)
{
	if (!_initialized) return;
	
	int i;
	uint8_t ulDataTx[2];
	ulDataTx[0] = REGRD_OPCODE_MASK + (regnum & 0x1f);
	ulDataTx[1] = count-1;
	selectDeviceCSLow();

	_spi->beginTransaction(_spiSettings);
	
	_spi->transfer(ulDataTx[0]);
	_spi->transfer(ulDataTx[1]);
	for(i = 0; i < count; i++)
	{
		data[i] = _spi->transfer(0);
		if(regnum+i < NUM_REGISTERS)
			registers[regnum+i] = data[i];
	}
	
	_spi->endTransaction();
	
	releaseChipSelect();
}

/*
 * Writes a single of register with the specified data
 *
 * \param regnum addr_mask 8-bit mask of the register to which we start writing
 * \param data to be written
 *
 */
void ADS124S08::regWrite(unsigned int regnum, unsigned char data)
{
	if (!_initialized) return;
	
	uint8_t ulDataTx[3];
	ulDataTx[0] = REGWR_OPCODE_MASK + (regnum & 0x1f);
	ulDataTx[1] = 0x00;
	ulDataTx[2] = data;
	selectDeviceCSLow();
	
	_spi->beginTransaction(_spiSettings);
	
	_spi->transfer(ulDataTx[0]);
	_spi->transfer(ulDataTx[1]);
	_spi->transfer(ulDataTx[2]);
	
	_spi->endTransaction();
	
	releaseChipSelect();
	return;
}

/*
 * Writes a group of registers starting at the specified address
 *
 * \param regnum is addr_mask 8-bit mask of the register from which we start writing
 * \param count The number of registers we wish to write
 * \param *location pointer to the location in memory to read the data
 *
 */
void ADS124S08::writeRegs(unsigned int regnum, unsigned int howmuch, unsigned char *data)
{
	if (!_initialized) return;
	
	unsigned int i;
	uint8_t ulDataTx[2];
	ulDataTx[0] = REGWR_OPCODE_MASK + (regnum & 0x1f);
	ulDataTx[1] = howmuch-1;
	selectDeviceCSLow();
	
	_spi->beginTransaction(_spiSettings);
	
	_spi->transfer(ulDataTx[0]);
	_spi->transfer(ulDataTx[1]);
	for(i=0; i < howmuch; i++)
	{
		_spi->transfer(data[i]);
		if(regnum+i < NUM_REGISTERS)
			registers[regnum+i] = data[i];
	}
	
	_spi->endTransaction();
	
	releaseChipSelect();
	return;
}

/*
 * Sends a command to the ADS124S08
 *
 * \param op_code is the command being issued
 *
 */
void ADS124S08::sendCommand(uint8_t op_code)
{
	if (!_initialized) return;
	
	selectDeviceCSLow();
	
	_spi->beginTransaction(_spiSettings);
	_spi->transfer(op_code);
	_spi->endTransaction();

	releaseChipSelect();
	return;
}

/*
 * Sends a STOP/START command sequence to the ADS124S08 to restart conversions (SYNC)
 *
 */
void ADS124S08::reStart(void)
{
	if (!_initialized) return;
	
	sendCommand(STOP_OPCODE_MASK);
	sendCommand(START_OPCODE_MASK);
	return;
}

/*
 * Sets the GPIO hardware START pin high (red LED)
 *
 */
void ADS124S08::assertStart()
{
	if (!_initialized) return;
	
	fStart = true;
	_ioExpander->digitalWrite(ADS124S08_START_PIN, HIGH);
}

/*
 * Sets the GPIO hardware START pin low
 *
 */
void ADS124S08::deassertStart()
{
	if (!_initialized) return;
	
	fStart = false;
	_ioExpander->digitalWrite(ADS124S08_START_PIN, LOW);
}

/*
 * Reads data using the RDATA command
 * Espera a que el pin DRDY esté en LOW (activo bajo) para indicar que los datos están listos
 * antes de enviar el comando RDATA y leer los datos del ADC.
 */
int ADS124S08::rData(uint8_t *dStatus, uint8_t *dData, uint8_t *dCRC)
{
	if (!_initialized) return -1;
	
	// Esperar a que el pin DRDY esté en LOW (datos disponibles)
	// DRDY es activo bajo, por lo que cuando está en LOW, los datos están listos
	uint32_t timeout = millis() + 1000; // Timeout de 1 segundo
	while (_ioExpander->digitalRead(ADS124S08_DRDY_PIN) == HIGH) {
		// Si se supera el timeout, retornar error
		if (millis() > timeout) {
			DEBUG_PRINTLN("Error: Timeout esperando datos del ADC en rData");
			return -1;
		}
		delay(1); // Pequeña pausa para no saturar el CPU
	}
	
	int result = -1;
	selectDeviceCSLow();

	_spi->beginTransaction(_spiSettings);

	// according to datasheet chapter 9.5.4.2 Read Data by RDATA Command
	sendCommand(RDATA_OPCODE_MASK);

	// if the Status byte is set - grab it
	uint8_t shouldWeReceiveTheStatusByte = (registers[SYS_ADDR_MASK] & 0x01) == DATA_MODE_STATUS;
	if(shouldWeReceiveTheStatusByte)
	{
		dStatus[0] = _spi->transfer(0x00);
	}

	// get the conversion data (3 bytes)
	uint8_t data[3];
	data[0] = _spi->transfer(0x00);
	data[1] = _spi->transfer(0x00);
	data[2] = _spi->transfer(0x00);
	
	// Armar el entero de 24 bits con extensión de signo
	uint32_t raw = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
	
	// Extender el signo si el bit 23 es 1
	if (raw & 0x800000) {
		raw |= 0xFF000000;  // Extensión de signo
	}
	
	result = (int)raw;

	// is CRC enabled?
	uint8_t isCrcEnabled = (registers[SYS_ADDR_MASK] & 0x02) == DATA_MODE_CRC;
	if(isCrcEnabled)
	{
		dCRC[0] = _spi->transfer(0x00);
	}

	_spi->endTransaction();

	releaseChipSelect();
	return result;
}

/*
 *
 * Read the last conversion result
 * Espera a que el pin DRDY esté en LOW (activo bajo) para indicar que los datos están listos
 * antes de leer los datos del ADC.
 *
 */
int ADS124S08::dataRead(uint8_t *dStatus, uint8_t *dData, uint8_t *dCRC)
{
	if (!_initialized) return -1;
	
	// Esperar a que el pin DRDY esté en LOW (datos disponibles)
	// DRDY es activo bajo, por lo que cuando está en LOW, los datos están listos
	uint32_t timeout = millis() + 1000; // Timeout de 1 segundo
	while (_ioExpander->digitalRead(ADS124S08_DRDY_PIN) == HIGH) {
		// Si se supera el timeout, retornar error
		if (millis() > timeout) {
			DEBUG_PRINTLN("Error: Timeout esperando datos del ADC");
			return -1;
		}
		delay(1); // Pequeña pausa para no saturar el CPU
	}
	
	uint8_t xcrc;
	uint8_t xstatus;
	int iData = 0;
	selectDeviceCSLow();
	
	_spi->beginTransaction(_spiSettings);
	
	if((registers[SYS_ADDR_MASK] & 0x01) == DATA_MODE_STATUS)
	{
		xstatus = _spi->transfer(0x00);
		dStatus[0] = (uint8_t)xstatus;
	}

	// get the conversion data (3 bytes)
	uint8_t data[3];
	data[0] = _spi->transfer(0x00);
	data[1] = _spi->transfer(0x00);
	data[2] = _spi->transfer(0x00);

	// Armar el entero de 24 bits con extensión de signo
	uint32_t raw = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | data[2];
	
	// Extender el signo si el bit 23 es 1
	if (raw & 0x800000) {
		raw |= 0xFF000000;  // Extensión de signo
	}
	
	iData = (int)raw;
	
	if((registers[SYS_ADDR_MASK] & 0x02) == DATA_MODE_CRC)
	{
		xcrc = _spi->transfer(0x00);
		dCRC[0] = (uint8_t)xcrc;
	}
	
	_spi->endTransaction();
	
	releaseChipSelect();
	return iData;
}

#endif // DEVICE_TYPE_ANALOGIC