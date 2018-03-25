/*
 * UART.h
 *
 * Created: 25/05/2017 16:45:37
 * Author : João Silva
 */ 


#ifndef UART_H_
#define UART_H_

#define UART_FIFO_SIZE 127

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace UART0
{
	extern volatile uint8_t m_ubRXBuffer[UART_FIFO_SIZE + 1];
	extern volatile uint16_t m_usRXBufferHead;
	extern volatile uint16_t m_usRXBufferTail;
	
	extern void Init(uint32_t ulBaud, uint8_t ubDoubleSpeed);
	
	inline uint16_t BytesAvailable()
	{
		return (UART_FIFO_SIZE + m_usRXBufferHead - m_usRXBufferTail) % UART_FIFO_SIZE;
	}
	inline void Flush()
	{
		m_usRXBufferTail = m_usRXBufferHead;
	}
	
	extern uint8_t ReadByte();
	inline void Read(uint8_t* pubDest, uint16_t usCount)
	{
		while(usCount--)
			*(pubDest++) = ReadByte();
	}
	inline void Read(void* pDest, uint16_t usCount)
	{
		Read((uint8_t*)pDest, usCount);
	}
	
	extern void WriteByte(uint8_t ubData);
	inline void Write(uint8_t* pubSrc, uint16_t usCount)
	{
		while(usCount--)
			WriteByte(*(pubSrc++));
	}
	inline void Write(void* pSrc, uint16_t usCount)
	{
		Write((uint8_t*)pSrc, usCount);
	}
	
	extern void Printf(const char* pbFmt, ...);
}

namespace UART1
{
	extern volatile uint8_t m_ubRXBuffer[UART_FIFO_SIZE + 1];
	extern volatile uint16_t m_usRXBufferHead;
	extern volatile uint16_t m_usRXBufferTail;
	
	extern void Init(uint32_t ulBaud, uint8_t ubDoubleSpeed);
	
	inline uint16_t BytesAvailable()
	{
		return (UART_FIFO_SIZE + m_usRXBufferHead - m_usRXBufferTail) % UART_FIFO_SIZE;
	}
	inline void Flush()
	{
		m_usRXBufferTail = m_usRXBufferHead;
	}
	
	extern uint8_t ReadByte();
	inline void Read(uint8_t* pubDest, uint16_t usCount)
	{
		while(usCount--)
			*(pubDest++) = ReadByte();
	}
	inline void Read(void* pDest, uint16_t usCount)
	{
		Read((uint8_t*)pDest, usCount);
	}
	
	extern void WriteByte(uint8_t ubData);
	inline void Write(uint8_t* pubSrc, uint16_t usCount)
	{
		while(usCount--)
			WriteByte(*(pubSrc++));
	}
	inline void Write(void* pSrc, uint16_t usCount)
	{
		Write((uint8_t*)pSrc, usCount);
	}
	
	extern void Printf(const char* pbFmt, ...);
}

#endif /* UART_H_ */