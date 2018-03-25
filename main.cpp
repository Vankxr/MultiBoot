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
#include <util/crc16.h>
#include <stdint.h>
#include <string.h>

#define MAX_ROMS 5
#define BOOT_MAGIC 0x5B

#define BOOT_CONFIG_EE_ADDRESS ((void*)0xC00)

// Structs & Enums
enum boot_mode_t
{
	BOOT_MODE_NORMAL = 0,
	BOOT_MODE_PIN,
	BOOT_MODE_PIN_RESET,
};
enum boot_load_status_t
{
	BOOT_LOAD_STATUS_OFF = 0,
	BOOT_LOAD_STATUS_ON,
};

struct boot_cfg_t
{
	uint8_t m_ubMagic;
	uint8_t m_ubVersion;
	boot_mode_t m_ubMode;
	boot_load_status_t m_ubLoadStatus;
	uint8_t m_ubCurrentROM;
	uint8_t m_ubPinROM;
	uint8_t m_ubLoadROM;
	uint8_t m_ubROMCount;
	uint32_t m_ulROMAddress[MAX_ROMS];
	uint32_t m_ulLoadROMFlashAddress;
	uint32_t m_ulLoadROMSize;
	uint8_t m_ubCRC;
};

// Variables
register uint8_t g_MCUSR asm("r2");

// Forward declarations
inline void resetMCU() __attribute__ ((__noreturn__));

uint8_t validateConfig(boot_cfg_t* pConfig);

void flashProgramPage(uint32_t ulAddress, uint8_t *pubBuf, uint16_t uiSize = SPM_PAGESIZE);

void loadROM(uint32_t ulAddress);


void init()	__attribute__ ((naked)) __attribute__ ((section (".init3")));
int main();
void quit()	__attribute__ ((naked)) __attribute__ ((section (".fini8")));

// Functions
void resetMCU()
{
	wdt_enable(WDTO_15MS);
	
	while(1);
}

uint8_t validateConfig(boot_cfg_t* pConfig)
{	
	if(pConfig->m_ubMagic != BOOT_MAGIC)
		return 0;
	
	if(pConfig->m_ubCurrentROM >= MAX_ROMS)
		return 0;
	
	if(pConfig->m_ubLoadStatus == BOOT_LOAD_STATUS_ON && pConfig->m_ubLoadROM >= MAX_ROMS)
		return 0;
	
	if((pConfig->m_ubMode == BOOT_MODE_PIN || pConfig->m_ubMode == BOOT_MODE_PIN_RESET) && pConfig->m_ubPinROM >= MAX_ROMS)
		return 0;
	
	uint16_t crc;
	
	for(uint16_t i = 0; i < sizeof(boot_cfg_t); i++)
		crc = _crc16_update(crc, ((uint8_t*)pConfig)[i]);
	
	if(crc)
		return 0;
	
	return 1;
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

void loadROM(uint32_t ulAddress)
{
	if(ulAddress < _VECTORS_SIZE) // The (original) IVT space is required to be volatile (i.e. each firmware must have its own IVT copy, never flash to 0x00000)
		return;
		
	if(ulAddress + _VECTORS_SIZE > FLASHEND)
		return;
	
	// Live patch the interrupt vector table with one residing at the specified address
	// The rest of the code can be run directly from that address
	uint8_t ivtBuf[_VECTORS_SIZE]; // Interrupt vector table
	uint16_t pageIndex = 0; // Pages written (in case the VTable is bigger than one flash page)
	
	memset(ivtBuf, 0, _VECTORS_SIZE); // Probably not needed
	
	// Load the IVT from the desired address
	if(ulAddress > 0xFFFF) // If the address is larger than 2 bytes we need a ELPM instead of a LPM
		memcpy_PF(ivtBuf, ulAddress, _VECTORS_SIZE);
	else
		memcpy_P(ivtBuf, (void*)ulAddress, _VECTORS_SIZE);
	
	for(uint8_t i = 0; i < _VECTORS_SIZE; i += 4) // Each vector is 4 bytes in size (2 words)
	{
		uint32_t op = ((uint32_t)ivtBuf[i] << 24) | ((uint32_t)ivtBuf[i + 1] << 16) | ((uint32_t)ivtBuf[i + 2] << 8) | ivtBuf[i + 3];

		op >>= 16; // Lower byte would be masked anyways, so discard it
		op &= 0x00C0; // Mask the offset and leave only the OP code

		if(op == 0x00C0) // RJMP OP code
		{
			uint32_t destAddr = 0; // Destination (byte) address
			int16_t diffAddr = 0; // Difference (word) address
			
			// Get the destination address from the RJMP instruction
			diffAddr |= ivtBuf[0];
			diffAddr |= (ivtBuf[i + 1] & 0x0F) << 8;
			diffAddr |= (diffAddr & 0x0800) << 1; // Propagate the last bit for signed operations (2's complement)
			diffAddr |= (diffAddr & 0x0800) << 2;
			diffAddr |= (diffAddr & 0x0800) << 3;
			diffAddr |= (diffAddr & 0x0800) << 4;

			destAddr = ulAddress + i + 2 + diffAddr * 2; // "RJMP k" = "PC <- (k + 1)" (k is in words)
			
			// Convert the RJMP into a JMP to the absolute destination address (implicit byte-address to word-address conversion)
			ivtBuf[i] = (destAddr & 0x780000) >> 15;
			ivtBuf[i] |= 0x0C | ((destAddr & 0x040000) >> 18);
			ivtBuf[i + 1] = 0x94 | ((destAddr & 0x020000) >> 17);
			ivtBuf[i + 2] = (destAddr & 0x0001FE) >> 1;
			ivtBuf[i + 3] = (destAddr & 0x01FE00) >> 9;
		}
		
		if((i + pageIndex * SPM_PAGESIZE) >= SPM_PAGESIZE) // If we have already modified one flash page, write it and increment the counter
		{
			flashProgramPage(pageIndex * SPM_PAGESIZE, ivtBuf + pageIndex * SPM_PAGESIZE);
			
			pageIndex++;
		}
	}
}


// Main Program
void init()
{
	cli(); // If any ISR is called during programming we're screwed
	
	g_MCUSR = MCUSR;
	 
	 MCUSR = 0x00;
	 
	 wdt_disable();
}
int main()
{
	boot_cfg_t bootConfig;
	
	memset(&bootConfig, 0, sizeof(boot_cfg_t));
	
	eeprom_busy_wait();
	eeprom_read_block(&bootConfig, BOOT_CONFIG_EE_ADDRESS, sizeof(boot_cfg_t));
	
	if(!validateConfig(&bootConfig))
		return 1;
		
	if(g_MCUSR & ((1 << EXTRF) | (1 << PORF))) // External & POR Reset
	{
		
	}
	else if(g_MCUSR & (1 << WDRF)) // Software (WDT) Reset
	{
		
	}
	
	loadROM(_VECTORS_SIZE);
	
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