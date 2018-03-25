/*
 * UART.cpp
 *
 * Created: 25/05/2017 16:45:37
 * Author: João Silva
 */ 

#include "UART.h"

volatile uint8_t UART0::m_ubRXBuffer[UART_FIFO_SIZE + 1];
volatile uint16_t UART0::m_usRXBufferHead = 0;
volatile uint16_t UART0::m_usRXBufferTail = 0;

ISR(USART0_RX_vect)
{
	UART0::m_ubRXBuffer[UART0::m_usRXBufferHead++] = UDR0;
	
	if(UART0::m_usRXBufferHead > UART_FIFO_SIZE)
		UART0::m_usRXBufferHead = 0;
}

void UART0::Init(uint32_t ulBaud, uint8_t ubDoubleSpeed)
{
	memset((uint8_t*)UART0::m_ubRXBuffer, 0, UART_FIFO_SIZE + 1);
	
	UBRR0 = ((F_CPU + (ubDoubleSpeed ? 4UL : 8UL) * ulBaud) / ((ubDoubleSpeed ? 8UL : 16UL) * ulBaud) - 1UL);
	
	if(ubDoubleSpeed)
		UCSR0A = (1 << U2X0);
	
	UCSR0B = (1 << RXCIE0) | (1 << TXEN0) | (1 << RXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

uint8_t UART0::ReadByte()
{
	uint8_t b = UART0::m_ubRXBuffer[UART0::m_usRXBufferTail++];
	
	if(UART0::m_usRXBufferTail > UART_FIFO_SIZE)
		UART0::m_usRXBufferTail = 0;
	
	return b;
}
void UART0::WriteByte(uint8_t ubData)
{
	while(!(UCSR0A & (1 << UDRE0)));
	
	UDR0 = ubData;
}
void UART0::Printf(const char* pbFmt, ...)
{
	char msg[64];
	
	va_list args;
	va_start(args, pbFmt);
	UART0::Write((uint8_t*)msg, vsnprintf(msg, 64, pbFmt, args));
	va_end(args);
}


volatile uint8_t UART1::m_ubRXBuffer[UART_FIFO_SIZE + 1];
volatile uint16_t UART1::m_usRXBufferHead = 0;
volatile uint16_t UART1::m_usRXBufferTail = 0;

ISR(USART1_RX_vect)
{
	UART1::m_ubRXBuffer[UART1::m_usRXBufferHead++] = UDR1;
	
	if(UART1::m_usRXBufferHead > UART_FIFO_SIZE)
		UART1::m_usRXBufferHead = 0;
}

void UART1::Init(uint32_t ulBaud, uint8_t ubDoubleSpeed)
{
	memset((uint8_t*)UART1::m_ubRXBuffer, 0, UART_FIFO_SIZE + 1);
	
	UBRR1 = ((F_CPU + (ubDoubleSpeed ? 4UL : 8UL) * ulBaud) / ((ubDoubleSpeed ? 8UL : 16UL) * ulBaud) - 1UL);
	
	if(ubDoubleSpeed)
		UCSR1A = (1 << U2X1);
	
	UCSR1B = (1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1);
	UCSR1C = (1 << UCSZ11) | (1 << UCSZ10);
}

uint8_t UART1::ReadByte()
{
	uint8_t b = UART1::m_ubRXBuffer[UART1::m_usRXBufferTail++];
	
	if(UART1::m_usRXBufferTail > UART_FIFO_SIZE)
		UART1::m_usRXBufferTail = 0;
	
	return b;
}
void UART1::WriteByte(uint8_t ubData)
{
	while(!(UCSR1A & (1 << UDRE1)));
	
	UDR1 = ubData;
}
void UART1::Printf(const char* pbFmt, ...)
{
	char msg[64];
	
	va_list args;
	va_start(args, pbFmt);
	UART1::Write((uint8_t*)msg, vsnprintf(msg, 64, pbFmt, args));
	va_end(args);
}