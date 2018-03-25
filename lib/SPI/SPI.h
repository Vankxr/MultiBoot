/*
 * SPI.h
 *
 * Created: 09/12/2017 16:55:37
 * Author : João Silva
 */ 


#ifndef SPI_H_
#define SPI_H_

#include <avr/io.h>
#include <stdint.h>

namespace SPI
{
	extern void Init(uint8_t ubLSBFirst = 0, uint8_t ubMode = 0, uint8_t ubPrescaler = 0, uint8_t ubDoubleSpeed = 1);
	
	extern uint8_t TransferByte(uint8_t ubData = 0x00);
	inline void Transfer(uint8_t* pubSrc, uint16_t usCount, uint8_t* pubDest = 0)
	{
		if(pubSrc)
		{
			for(uint16_t i = 0; i < usCount; i++)
			{
				if(pubDest)
					*(pubDest + i) = TransferByte(*(pubSrc + i));
				else
					TransferByte(*(pubSrc + i));
			}
		}
		else if(pubDest)
		{
			for(uint16_t i = 0; i < usCount; i++)
				*(pubDest + i) = TransferByte();
		}
	}
	inline void Read(uint8_t* pubDest, uint16_t usCount)
	{
		Transfer(0, usCount, pubDest);
	}
}

#endif /* SPI_H_ */