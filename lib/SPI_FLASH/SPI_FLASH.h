/*
 * SPI_FLASH.h
 *
 * Created: 27/11/2017 16:45:37
 * Author : joaob
 */ 


#ifndef SPI_FLASH_H_
#define SPI_FLASH_H_

#include <avr/io.h>
#include <util/atomic.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <debug_macros.h>
#include <SPI/SPI.h>

#define FLASH_CMD_READ					0x03
#define FLASH_CMD_READ_FAST				0x0B
#define FLASH_CMD_WRITE_BYTE			0x02
#define FLASH_CMD_WRITE_CONTINUOUS		0xAF
#define FLASH_CMD_WRITE_ENABLE			0x06
#define FLASH_CMD_WRITE_ENABLE_STATUS	0x50
#define FLASH_CMD_WRITE_DISABLE			0x04
#define FLASH_CMD_READ_STATUS			0x05
#define FLASH_CMD_WRITE_STATUS			0x01
#define FLASH_CMD_SECTOR_ERASE			0x20
#define FLASH_CMD_BLOCK_ERASE			0x52
#define FLASH_CMD_CHIP_ERASE			0xC7
#define FLASH_CMD_READ_ID				0xAB

#define FLASH_PROTECTION_NONE			0x0
#define FLASH_PROTECTION_LOWER_1_4		0x1
#define FLASH_PROTECTION_LOWER_1_2		0x2
#define FLASH_PROTECTION_ALL			0x3

#define FLASH_BYTE_WRITE_TIME			20
#define FLASH_SECTOR_ERASE_TIME			25000
#define FLASH_PAGE_ERASE_TIME			25000
#define FLASH_CHIP_ERASE_TIME			100000

#define FLASH_SECTOR_SIZE	((uint32_t)0x1000) // 4 KB
#define FLASH_BLOCK_SIZE	((uint32_t)0x8000) // 32 KB

#define FLASH_BLOCK_0		(FLASH_BLOCK_SIZE * 0) // Block 0
#define FLASH_BLOCK_1		(FLASH_BLOCK_SIZE * 1) // Block 1
#define FLASH_BLOCK_2		(FLASH_BLOCK_SIZE * 2) // Block 2
#define FLASH_BLOCK_3		(FLASH_BLOCK_SIZE * 3) // Block 3 - Update firmware

#define FLASH_SECTOR_0		(FLASH_SECTOR_SIZE *  0) // Block 0, Sector 0
#define FLASH_SECTOR_1		(FLASH_SECTOR_SIZE *  1) // Block 0, Sector 1
#define FLASH_SECTOR_2		(FLASH_SECTOR_SIZE *  2) // Block 0, Sector 2
#define FLASH_SECTOR_3		(FLASH_SECTOR_SIZE *  3) // Block 0, Sector 3
#define FLASH_SECTOR_4		(FLASH_SECTOR_SIZE *  4) // Block 0, Sector 4
#define FLASH_SECTOR_5		(FLASH_SECTOR_SIZE *  5) // Block 0, Sector 5
#define FLASH_SECTOR_6		(FLASH_SECTOR_SIZE *  6) // Block 0, Sector 6
#define FLASH_SECTOR_7		(FLASH_SECTOR_SIZE *  7) // Block 0, Sector 7
#define FLASH_SECTOR_8		(FLASH_SECTOR_SIZE *  8) // Block 1, Sector 0
#define FLASH_SECTOR_9		(FLASH_SECTOR_SIZE *  9) // Block 1, Sector 1
#define FLASH_SECTOR_10		(FLASH_SECTOR_SIZE * 10) // Block 1, Sector 2
#define FLASH_SECTOR_11		(FLASH_SECTOR_SIZE * 11) // Block 1, Sector 3
#define FLASH_SECTOR_12		(FLASH_SECTOR_SIZE * 12) // Block 1, Sector 4
#define FLASH_SECTOR_13		(FLASH_SECTOR_SIZE * 13) // Block 1, Sector 5
#define FLASH_SECTOR_14		(FLASH_SECTOR_SIZE * 14) // Block 1, Sector 6
#define FLASH_SECTOR_15		(FLASH_SECTOR_SIZE * 15) // Block 1, Sector 7
#define FLASH_SECTOR_16		(FLASH_SECTOR_SIZE * 16) // Block 2, Sector 0
#define FLASH_SECTOR_17		(FLASH_SECTOR_SIZE * 17) // Block 2, Sector 1
#define FLASH_SECTOR_18		(FLASH_SECTOR_SIZE * 18) // Block 2, Sector 2
#define FLASH_SECTOR_19		(FLASH_SECTOR_SIZE * 19) // Block 2, Sector 3
#define FLASH_SECTOR_20		(FLASH_SECTOR_SIZE * 20) // Block 2, Sector 4
#define FLASH_SECTOR_21		(FLASH_SECTOR_SIZE * 21) // Block 2, Sector 5
#define FLASH_SECTOR_22		(FLASH_SECTOR_SIZE * 22) // Block 2, Sector 6
#define FLASH_SECTOR_23		(FLASH_SECTOR_SIZE * 23) // Block 2, Sector 7 - Sector buffer
#define FLASH_SECTOR_24		(FLASH_SECTOR_SIZE * 24) // Block 3, Sector 0 - Update firmware
#define FLASH_SECTOR_25		(FLASH_SECTOR_SIZE * 25) // Block 3, Sector 1 - Update firmware
#define FLASH_SECTOR_26		(FLASH_SECTOR_SIZE * 26) // Block 3, Sector 2 - Update firmware
#define FLASH_SECTOR_27		(FLASH_SECTOR_SIZE * 27) // Block 3, Sector 3 - Update firmware
#define FLASH_SECTOR_28		(FLASH_SECTOR_SIZE * 28) // Block 3, Sector 4 - Update firmware
#define FLASH_SECTOR_29		(FLASH_SECTOR_SIZE * 29) // Block 3, Sector 5 - Update firmware
#define FLASH_SECTOR_30		(FLASH_SECTOR_SIZE * 30) // Block 3, Sector 6 - Update firmware
#define FLASH_SECTOR_31		(FLASH_SECTOR_SIZE * 31) // Block 3, Sector 7 - Update firmware

#define FLASH_MAX_ADDRESS	(FLASH_SECTOR_SIZE * 32 - 1) // 0x1FFFF for 1 Mbit
#define FLASH_SECTOR_MASK	(FLASH_MAX_ADDRESS & ~(FLASH_SECTOR_SIZE - 1))
#define FLASH_BLOCK_MASK	(FLASH_MAX_ADDRESS & ~(FLASH_BLOCK_SIZE - 1))

#define FLASH_SELECT() PORTC &= ~(1 << PC3)
#define FLASH_UNSELECT() PORTC |= (1 << PC3)

namespace SPI_FLASH
{	
	extern uint8_t Init();
	
	extern void Read(uint32_t ulAddress, uint8_t* pubDest, uint16_t usCount);
	extern void Write(uint32_t ulAddress, uint8_t* pubSrc, uint16_t usCount);
	extern void Modify(uint32_t ulAddress, uint8_t* pubSrc, uint16_t usCount);
	
	inline uint8_t ReadByte(uint32_t ulAddress)
	{
		uint8_t ret = 0xFF;
		
		Read(ulAddress, &ret, 1);
		
		return ret;
	}
	inline void WriteByte(uint32_t ulAddress, uint8_t ubData)
	{
		Write(ulAddress, &ubData, 1);
	}
	inline void ModifyByte(uint32_t ulAddress, uint8_t ubData)
	{
		Modify(ulAddress, &ubData, 1);
	}
	
	extern void BusyWait();
	extern void WriteStatusEnable();
	extern void WriteEnable();
	extern void WriteDisable();
	extern void BlockErase(uint32_t ulAddress);
	extern void SectorErase(uint32_t ulAddress);
	extern void ChipErase();
	extern uint8_t ReadDeviceID();
	extern uint8_t ReadManufacturerID();
	extern void ProtectSectors(uint8_t ubProtect);
}

#endif /* SPI_FLASH_H_ */