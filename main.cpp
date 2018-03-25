/*
 * main.cpp
 *
 * Created: 24/03/2018 15:45:51
 * Author : joaob
 */ 

#include <main.h>

// Default boot config
boot_cfg_t g_bootConfigDefault = {
	BOOT_MAGIC,
	1,
	BOOT_MODE_NORMAL,
	BOOT_LOAD_STATUS_OFF,
	255,
	0,
	0,
	0,
	1,
	{
		0x00100,
		0x00000,
		0x00000,
		0x00000,
		0x00000
	},
	0x00000000,
	0,
	0
};

// Variables
uint8_t g_ubMCUSR = 0;
uint8_t g_ubSPIFlashOK = 0;

// Functions
void resetMCU()
{
	wdt_enable(WDTO_15MS);
	
	while(1);
}

void calcCRC16(boot_cfg_t* pConfig)
{
	for(uint16_t i = 0; i < sizeof(boot_cfg_t) - sizeof(uint16_t); i++)
		pConfig->m_usCRC = _crc16_update(pConfig->m_usCRC, ((uint8_t*)pConfig)[i]);
}
uint8_t validateConfig(boot_cfg_t* pConfig)
{	
	if(pConfig->m_ubMagic != BOOT_MAGIC)
	{
		DPRINTFLN_CTX("Boot Config magic does not match [%02X]", pConfig->m_ubMagic);
		
		return 0;
	}
	
	if(pConfig->m_ubNormalROM >= MAX_ROMS)
	{
		DPRINTFLN_CTX("Normal ROM index exceeds MAX_ROMS [%d]", pConfig->m_ubCurrentROM);
		
		return 0;
	}
	
	if(pConfig->m_ubLoadStatus == BOOT_LOAD_STATUS_ON && pConfig->m_ubLoadROM >= MAX_ROMS)
	{
		DPRINTFLN_CTX("Load ROM index exceeds MAX_ROMS [%d]", pConfig->m_ubLoadROM);
		
		return 0;
	}
	
	if((pConfig->m_ubMode == BOOT_MODE_PIN || pConfig->m_ubMode == BOOT_MODE_PIN_RESET) && pConfig->m_ubPinROM >= MAX_ROMS)
	{
		DPRINTFLN_CTX("Pin ROM index exceeds MAX_ROMS [%d]", pConfig->m_ubPinROM);
		
		return 0;
	}
	
	uint16_t crc = 0;
	
	for(uint16_t i = 0; i < sizeof(boot_cfg_t); i++)
		crc = _crc16_update(crc, ((uint8_t*)pConfig)[i]);
	
	if(crc)
	{
		DPRINTFLN_CTX("CRC does not match [%02X]", crc);
		
		return 0;
	}
	
	return 1;
}

void flashProgramPage(uint32_t ulAddress, uint8_t *pubBuf, uint16_t uiSize)
{
	if(!uiSize || !pubBuf)
	{
		DPRINTFLN_CTX("Buffer pointer or size invalid [0x%04X] [%d]", pubBuf, uiSize);
		
		return;
	}
	
	if(ulAddress + uiSize > FLASHEND)
	{
		DPRINTFLN_CTX("Data size exceeds flash size [0x%08X] [%d]", ulAddress, uiSize);
		
		return;
	}
	
	uiSize = (uiSize > SPM_PAGESIZE) ? SPM_PAGESIZE : uiSize;

	boot_page_erase_safe(ulAddress);
	DPRINTFLN_CTX("Erased page at address [0x%08X]", ulAddress);
	
	for(uint16_t i = 0; i < uiSize; i += 2)
		boot_page_fill_safe(ulAddress + i, pubBuf[i] | (pubBuf[i + 1] << 8));
	
	boot_page_write_safe(ulAddress);
	DPRINTFLN_CTX("Written page at address [0x%08X]", ulAddress);
	
	boot_spm_busy_wait();
	eeprom_busy_wait();
	DPRINTFLN_CTX("EEPROM & SPM not busy, OK!");
}

uint8_t bootROM(uint32_t ulAddress)
{
	if(ulAddress < (((_VECTORS_SIZE / SPM_PAGESIZE) * SPM_PAGESIZE) + SPM_PAGESIZE)) // The (original) page-boundary IVT space is required to be volatile (i.e. each firmware must have its own IVT copy, never flash to 0x00000)
	{
		DPRINTFLN_CTX("Address is lower than IVT size [0x%04X]", ulAddress);
		
		return 0;
	}
		
	if(ulAddress + _VECTORS_SIZE > FLASHEND)
	{
		DPRINTFLN_CTX("Data size exceeds flash size [0x%04X]", ulAddress);
		
		return 0;
	}
	
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
		
	DPRINTFLN_CTX("IVT Loaded into internal buffer [0x%04X] [%u]", ulAddress, _VECTORS_SIZE);
	
	for(uint16_t i = 0; i < _VECTORS_SIZE; i += 4) // Each vector is 4 bytes in size (2 words)
	{
		uint32_t op = ((uint32_t)ivtBuf[i] << 24) | ((uint32_t)ivtBuf[i + 1] << 16) | ((uint32_t)ivtBuf[i + 2] << 8) | ivtBuf[i + 3];

		DPRINTFLN_CTX("Original bytecode [%02X] [%02X] [%02X] [%02X]", ivtBuf[i], ivtBuf[i + 1], ivtBuf[i + 2], ivtBuf[i + 3]);

		op >>= 16; // Lower byte would be masked anyways, so discard it
		op &= 0x00F0; // Mask the offset and leave only the OP code

		DPRINTFLN_CTX("OP code at vector [0x%04X] [%u]", op, i / 4);

		if(op == 0x00C0) // RJMP OP code
		{
			DPRINTFLN_CTX("Found RJMP at vector [%u]", i / 4);
			
			uint32_t destAddr = 0; // Destination (byte) address
			int16_t diffAddr = 0; // Difference (word) address
			
			// Get the destination address from the RJMP instruction
			diffAddr |= ivtBuf[i];
			diffAddr |= (ivtBuf[i + 1] & 0x0F) << 8;
			diffAddr |= (diffAddr & 0x0800) << 1; // Propagate the last bit for signed operations (2's complement)
			diffAddr |= (diffAddr & 0x0800) << 2;
			diffAddr |= (diffAddr & 0x0800) << 3;
			diffAddr |= (diffAddr & 0x0800) << 4;

			DPRINTFLN_CTX("RJMP diff byte address [0x%04X]", diffAddr * 2);

			destAddr = ulAddress + i + 2 + diffAddr * 2; // "RJMP k" = "PC <- (k + 1)" (k is in words)
			
			DPRINTFLN_CTX("JMP absolute byte address [0x%08X]", destAddr);
			
			// Convert the RJMP into a JMP to the absolute destination address (implicit byte-address to word-address conversion)
			ivtBuf[i] = (destAddr & 0x780000) >> 15;
			ivtBuf[i] |= 0x0C | ((destAddr & 0x040000) >> 18);
			ivtBuf[i + 1] = 0x94 | ((destAddr & 0x020000) >> 17);
			ivtBuf[i + 2] = (destAddr & 0x0001FE) >> 1;
			ivtBuf[i + 3] = (destAddr & 0x01FE00) >> 9;
			
			DPRINTFLN_CTX("Patched bytecode [%02X] [%02X] [%02X] [%02X]", ivtBuf[i], ivtBuf[i + 1], ivtBuf[i + 2], ivtBuf[i + 3]);
		}
		
		if((i / SPM_PAGESIZE) > pageIndex) // If we have already modified one flash page, write it and increment the counter
		{			
			flashProgramPage(pageIndex * SPM_PAGESIZE, ivtBuf + pageIndex * SPM_PAGESIZE);
			
			DPRINTFLN_CTX("Wrote flash page at [0x%08X] [%u]", pageIndex * SPM_PAGESIZE, SPM_PAGESIZE);
			
			pageIndex++;
		}
	}
	
	// Write the remaining bytes from the patched IVT
	flashProgramPage(pageIndex * SPM_PAGESIZE, ivtBuf + pageIndex * SPM_PAGESIZE, _VECTORS_SIZE - pageIndex * SPM_PAGESIZE);
	
	DPRINTFLN_CTX("Wrote flash page at [0x%08X] [%d]", pageIndex * SPM_PAGESIZE, _VECTORS_SIZE - pageIndex * SPM_PAGESIZE);
	
	return 1;
}
uint8_t loadROM(uint32_t ulIntAddress, uint32_t ulExtAddress, uint32_t ulSize)
{
	if(ulIntAddress + ulSize > FLASHEND)
	{
		DPRINTFLN_CTX("Data size exceeds internal flash size [0x%08X] [%lu]", ulIntAddress, ulSize);
		
		return 0;
	}
	
	if(ulExtAddress + ulSize > FLASH_MAX_ADDRESS)
	{
		DPRINTFLN_CTX("Data size exceeds external flash size [0x%08X] [%lu]", ulIntAddress, ulSize);
		
		return 0;
	}
	
	if(!g_ubSPIFlashOK)
	{
		DPRINTFLN_CTX("SPI Flash init NOK");
		
		return 0;
	}
		
	_delay_ms(10);
		
	while(ulSize > 0)
	{
		static uint16_t currentPage;
		static uint8_t buf[SPM_PAGESIZE];
			
		uint16_t dataSize = (ulSize > SPM_PAGESIZE) ? SPM_PAGESIZE : ulSize;
			
		SPI_FLASH::Read(ulExtAddress + currentPage, buf, dataSize);
		flashProgramPage(ulIntAddress + currentPage, buf, dataSize);
		
		currentPage += dataSize;
		ulSize -= dataSize;
		
		_delay_ms(5);
	}
	
	DPRINTFLN_CTX("Copied firmware from external flash to internal flash [0x%08X] [0x%08X] [%lu]", ulExtAddress, ulIntAddress, ulSize);

	return 1;
}

// Main Program
void init()
{
	cli(); // Disable interrupts
	
	g_ubMCUSR = MCUSR; // Read reset flags
	MCUSR = 0x00;
	 
	wdt_disable(); // Disable the watchdog to prevent unwanted resets
	 
	 _delay_ms(10);
	 
	// Move the IVT to the Bootloader section
	MCUCR |= (1 << IVCE);
	MCUCR = (MCUCR & ~(1 << IVCE)) | (1 << IVSEL);
	
	DINIT(); // Debug init
	SPI::Init(0, 0, 0, 1); // MSB First, Mode 0, 4 MHz, 2x speed
	//g_ubSPIFlashOK = SPI_FLASH::Init();
	
	sei(); // Enable interrupts now that the IVT is in a safe place
	
	_delay_ms(100);
	
	DPRINTFLN("\r\n\r\n> MultiBoot v1.0");
}
int main()
{	
	boot_cfg_t bootConfig;
	
	memset(&bootConfig, 0, sizeof(boot_cfg_t));
	
	DPRINTFLN_CTX("Reading boot config at EEPROM address [0x%04X]", BOOT_CONFIG_EE_ADDRESS);

	eeprom_busy_wait();
	eeprom_read_block(&bootConfig, BOOT_CONFIG_EE_ADDRESS, sizeof(boot_cfg_t));
	
	DPRINTFLN_CTX("Validating boot config");
	
	if(!validateConfig(&bootConfig))
	{
		DPRINTFLN_CTX("Boot config not valid, waiting 5 seconds before rebooting");
		
		_delay_ms(5000);
		
		resetMCU();
	}
	
	DPRINTFLN_CTX("Current ROM [%d]", bootConfig.m_ubCurrentROM);
	
	if(bootConfig.m_ubLoadStatus == BOOT_LOAD_STATUS_ON)
	{
		DPRINTFLN_CTX("Going to load ROM [%d]", bootConfig.m_ubLoadROM);
		
		if(loadROM(bootConfig.m_ulROMAddress[bootConfig.m_ubLoadROM], bootConfig.m_ulLoadROMFlashAddress, bootConfig.m_ulLoadROMSize))
			bootConfig.m_ubLoadStatus = BOOT_LOAD_STATUS_OFF;
	}
	
	if(bootConfig.m_ubMode == BOOT_MODE_NORMAL && bootConfig.m_ubNormalROM != bootConfig.m_ubCurrentROM)
	{
		DPRINTFLN_CTX("Going to boot ROM [%d]", bootConfig.m_ubNormalROM);
		
		if(bootROM(bootConfig.m_ulROMAddress[bootConfig.m_ubNormalROM]))
			bootConfig.m_ubCurrentROM = bootConfig.m_ubNormalROM;
	}
	
	calcCRC16(&bootConfig);
	
	eeprom_busy_wait();
	eeprom_update_block(&bootConfig, BOOT_CONFIG_EE_ADDRESS, sizeof(boot_cfg_t));
	
	/*	
	if(g_ubMCUSR & ((1 << EXTRF) | (1 << PORF))) // External & POR Reset
	{
		
	}
	else if(g_ubMCUSR & (1 << WDRF)) // Software (WDT) Reset
	{
		
	}
	*/
	
	return 0;
}
void quit()
{
	cli(); // Disable interrupts
	
	boot_rww_enable(); // Re-enable the RWW flash sectors
	
	 // Move the IVT back to the Application section
	MCUCR |= (1 << IVCE);
	MCUCR &= ~((1 << IVCE) | (1 << IVSEL));
	
	SPL = (RAMEND & 0xFF); // Reset the stack pointer to the top of RAM
	SPH = (RAMEND >> 8);
	
	asm volatile("jmp 0x00000"); // Jump to the Application reset vector
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