/*
 * SPI.cpp
 *
 * Created: 09/12/2017 16:55:37
 * Author : João Silva
 */ 

#include "SPI.h"

void SPI::Init(uint8_t ubLSBFirst, uint8_t ubMode, uint8_t ubPrescaler, uint8_t ubDoubleSpeed)
{
	DDRB |= (1 << DDB0) | (1 << DDB1) | (1 << DDB2);	
	DDRB &= ~(1 << DDB3);
	PORTB |= (1 << PB3);
	
	SPCR = (1 << SPE) | ((ubLSBFirst & 0x01) << DORD) | (1 << MSTR) | ((ubMode & 0x03) << CPHA) | ((ubPrescaler & 0x03) << SPR0);

	SPSR = ((ubDoubleSpeed & 0x01) << SPI2X);
}

uint8_t SPI::TransferByte(uint8_t ubData)
{
	SPDR = ubData;
	
	while(!(SPSR & (1 << SPIF)));

	return SPDR;
}