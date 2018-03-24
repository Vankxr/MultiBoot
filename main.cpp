/*
 * main.cpp
 *
 * Created: 24/03/2018 15:45:51
 * Author : joaob
 */ 

#include <avr/io.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>

// Forward declarations
inline void resetMCU() __attribute__ ((__noreturn__));

void flashProgramPage(uint32_t ulAddress, uint8_t *pubBuf, uint16_t uiSize = SPM_PAGESIZE);
void flashReadVTable(uint32_t ulAddress, uint8_t *pubBuf);

void loadFirmware(uint32_t ulAddress);

void init()	__attribute__ ((naked)) __attribute__ ((section (".init3")));
int main();
void quit()	__attribute__ ((naked)) __attribute__ ((section (".fini8")));

// Functions
void resetMCU()
{
	wdt_enable(WDTO_15MS);
	
	while(1);
}

void flashProgramPage(uint32_t ulAddress, uint8_t *pubBuf, uint16_t uiSize)
{
	if(ulAddress + SPM_PAGESIZE > FLASHEND)
		return;
	
	if(!uiSize || !pubBuf)
		return;
	
	uiSize = (uiSize > SPM_PAGESIZE) ? SPM_PAGESIZE : uiSize;

	boot_page_erase_safe(ulAddress);
	
	for(uint16_t i = 0; i < uiSize; i += 2)
		boot_page_fill_safe(ulAddress + i, pubBuf[i] | (pubBuf[i + 1] << 8));
	
	boot_page_write_safe(ulAddress);
	
	boot_spm_busy_wait();
	eeprom_busy_wait();
}
void flashReadVTable(uint32_t ulAddress, uint8_t *pubBuf)
{
	if(ulAddress + SPM_PAGESIZE > FLASHEND)
		return;
	
	if(!pubBuf)
		return;
	
	if(ulAddress > 0xFFFF)
		memcpy_PF(pubBuf, ulAddress, _VECTORS_SIZE);
	else
		memcpy_P(pubBuf, (void*)ulAddress, _VECTORS_SIZE);
}

void loadFirmware(uint32_t ulAddress)
{
	if(ulAddress < _VECTORS_SIZE)
		return;
	
	// Live patch the interrupt vector table with one residing at the specified address
	// The rest of the code can be run directly from that address
	uint8_t vtBuf[_VECTORS_SIZE];
	uint8_t pageIndex = 0;
	
	memset(vtBuf, 0, _VECTORS_SIZE);
	
	flashReadVTable(ulAddress, vtBuf);
	
	for(uint8_t i = 0; i < _VECTORS_SIZE; i += 4)
	{
		uint32_t op = ((uint32_t)vtBuf[i] << 24) | ((uint32_t)vtBuf[i + 1] << 16) | ((uint32_t)vtBuf[i + 2] << 8) | vtBuf[i + 3];

		op >>= 16;
		op &= 0x00C0;

		if(op == 0x00C0) // RJMP OP code
		{
			uint32_t destAddress = 0;
			int16_t diffAddress = 0;
			
			// Get the destination address from the RJMP instruction
			diffAddress |= vtBuf[0];
			diffAddress |= (vtBuf[i + 1] & 0x0F) << 8;
			diffAddress |= (diffAddress & 0x0800) << 1; // Propagate the last bit
			diffAddress |= (diffAddress & 0x0800) << 2;
			diffAddress |= (diffAddress & 0x0800) << 3;
			diffAddress |= (diffAddress & 0x0800) << 4;

			destAddress = ulAddress + i + 2 + diffAddress * 2; // "RJMP k" = "PC <- (k + 1)" (k is in words)
			
			// Convert the RJMP to a JMP
			vtBuf[i] = (destAddress & 0x780000) >> 15;
			vtBuf[i] |= 0x0C | ((destAddress & 0x040000) >> 18);
			vtBuf[i + 1] = 0x94 | ((destAddress & 0x020000) >> 17);
			vtBuf[i + 2] = (destAddress & 0x0001FE) >> 1;
			vtBuf[i + 3] = (destAddress & 0x01FE00) >> 9;
		}
		
		if((i + pageIndex * SPM_PAGESIZE) >= SPM_PAGESIZE)
		{
			flashProgramPage(pageIndex * SPM_PAGESIZE, vtBuf + pageIndex * SPM_PAGESIZE);
			
			pageIndex++;
		}
	}
}

// Main Program
void init()
{
	cli(); // If any ISR is called during programming we're screwed
}
int main()
{
	register uint8_t _MCUSR = MCUSR;
	
	MCUSR = 0x00;
	
	wdt_disable();
	
	if(_MCUSR & ((1 << EXTRF) | (1 << PORF))) // External & POR Reset
	{
		
	}
	else if(_MCUSR & (1 << WDRF)) // Software (WDT) Reset
	{
		
	}
	
	loadFirmware(_VECTORS_SIZE);
	
	return 0;
}
void quit()
{
	boot_rww_enable();
	
	SPL = (RAMEND & 0xFF); // Reset the stack pointer to the top of RAM
	SPH = (RAMEND >> 8);
	
	asm volatile("jmp 0x00000"); // Jump to the reset vector
}

/* INSTR TO JMP BYTE ADDR
uint8_t instr[4] = {0xFD, 0x95, 0x00, 0x00};
uint32_t addr = 0;

addr |= (instr[0] & 0xF0) << 15;
addr |= (instr[0] & 0x01) << 18;
addr |= (instr[1] & 0x01) << 17;
addr |= instr[3] << 9;
addr |= instr[2] << 1;
*/

/* IS JMP
uint8_t instr[4] = {0x0C, 0x94, 0x00, 0x00};

uint32_t op = (instr[0] << 24) | (instr[1] << 16) | (instr[2] << 8) | instr[3];

op >>= 16;
op &= 0x0C94;

op == 0x0C94;
*/