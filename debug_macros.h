/*
 * debug_macros.h
 *
 * Created: 19/01/2018 00:05:12
 * Author: João Silva
 */ 

#ifdef SOFTDEBUG
	#include <UART/UART.h>
	
	#define DUART UART1

	#define DINIT() DUART::Init(19200, 1)

	#define DPRINTF_CTX(FORMAT, ...) DUART::Printf("[%s] - " FORMAT, __FUNCTION__, ##__VA_ARGS__)
	#define DPRINTFLN_CTX(FORMAT, ...) DUART::Printf("[%s] - " FORMAT "\n", __FUNCTION__, ##__VA_ARGS__)
	#define DPRINTF(FORMAT, ...) DUART::Printf(FORMAT, ##__VA_ARGS__)
	#define DPRINTFLN(FORMAT, ...) DUART::Printf(FORMAT "\n", ##__VA_ARGS__)
#else
	#define DUART

	#define DINIT()

	#define DPRINTF_CTX(...)
	#define DPRINTFLN_CTX(...)
	#define DPRINTF(...)
	#define DPRINTFLN(...)
#endif