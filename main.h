/*
 * main.h
 *
 * Created: 25/03/2018 16:20:55
 *  Author: joaob
 */ 


#ifndef MAIN_H_
#define MAIN_H_

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
#include <debug_macros.h>
#include <SPI/SPI.h>
#include <SPI_FLASH/SPI_FLASH.h>

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
	uint8_t m_ubNormalROM;
	uint8_t m_ubPinROM;
	uint8_t m_ubLoadROM;
	uint8_t m_ubROMCount;
	uint32_t m_ulROMAddress[MAX_ROMS];
	uint32_t m_ulLoadROMFlashAddress;
	uint32_t m_ulLoadROMSize;
	uint16_t m_usCRC;
};

// Functions
inline void resetMCU() __attribute__ ((__noreturn__));

void calcCRC16(boot_cfg_t* pConfig);
uint8_t validateConfig(boot_cfg_t* pConfig);

void flashProgramPage(uint32_t ulAddress, uint8_t *pubBuf, uint16_t uiSize = SPM_PAGESIZE);

uint8_t bootROM(uint32_t ulAddress);
uint8_t loadROM(uint32_t ulIntAddress, uint32_t ulExtAddress, uint32_t ulSize);


void init()	__attribute__ ((naked)) __attribute__ ((section (".init3")));
int main();
void quit()	__attribute__ ((naked)) __attribute__ ((section (".fini8")));

#endif /* MAIN_H_ */