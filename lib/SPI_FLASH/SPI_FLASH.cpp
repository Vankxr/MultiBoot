/*
 * SPI_FLASH.cpp
 *
 * Created: 27/11/2017 16:45:37
 * Author: joaob
 */ 

#include "SPI_FLASH.h"

uint8_t SPI_FLASH::Init()
{
	PORTC |= (1 << PC3);
	DDRC |= (1 << DDC3);
	
	_delay_us(FLASH_CHIP_ERASE_TIME);
	
	if(SPI_FLASH::ReadDeviceID() == 0x49 && SPI_FLASH::ReadManufacturerID() == 0xBF)
		return 1;
	
	return 0;
}

void SPI_FLASH::Read(uint32_t ulAddress, uint8_t* pubDest, uint16_t usCount)
{
	if(!usCount)
		return;
	
	ulAddress &= FLASH_MAX_ADDRESS;
	
	SPI_FLASH::BusyWait();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_READ);
		SPI::TransferByte((ulAddress >> 16) & 0xFF);
		SPI::TransferByte((ulAddress >> 8) & 0xFF);
		SPI::TransferByte(ulAddress & 0xFF);
		
		SPI::Read(pubDest, usCount);
		
		FLASH_UNSELECT();
	}
}
void SPI_FLASH::Write(uint32_t ulAddress, uint8_t* pubSrc, uint16_t usCount)
{
	if(!usCount)
		return;
	
	ulAddress &= FLASH_MAX_ADDRESS;
	
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteEnable();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		if(usCount == 1)
			SPI::TransferByte(FLASH_CMD_WRITE_BYTE);
		else
			SPI::TransferByte(FLASH_CMD_WRITE_CONTINUOUS);
		
		SPI::TransferByte((ulAddress >> 16) & 0xFF);
		SPI::TransferByte((ulAddress >> 8) & 0xFF);
		SPI::TransferByte(ulAddress & 0xFF);
				
		for(uint16_t i = 0; i < usCount; i++)
		{			
			if(i > 0)
			{				
				FLASH_SELECT();
				
				SPI::TransferByte(FLASH_CMD_WRITE_CONTINUOUS);
			}
			
			SPI::TransferByte(pubSrc[i]);
						
			FLASH_UNSELECT();
			
			_delay_us(FLASH_BYTE_WRITE_TIME);
		}
	}
	
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteDisable();
}
void SPI_FLASH::Modify(uint32_t ulAddress, uint8_t* pubSrc, uint16_t usCount)
{
	if(!usCount)
		return;
	
	ulAddress &= FLASH_MAX_ADDRESS;
	
	uint8_t buf[128];
	uint32_t sector = ulAddress & FLASH_SECTOR_MASK;
	uint16_t offset = ulAddress & ~FLASH_SECTOR_MASK;
	
	if(sector != ((ulAddress + usCount) & FLASH_SECTOR_MASK))
		return;
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		SPI_FLASH::SectorErase(FLASH_SECTOR_23);
		
		for(uint16_t i = 0; i < FLASH_SECTOR_SIZE; i += 128)
		{
			SPI_FLASH::Read(sector + i, buf, 128);
			
			if(i <= offset && i + 128 > offset)
				memcpy(buf + (offset % 128), pubSrc, usCount);
				
			SPI_FLASH::Write(FLASH_SECTOR_23 + i, buf, 128);
		}
		
		SPI_FLASH::SectorErase(sector);
		
		for(uint16_t i = 0; i < FLASH_SECTOR_SIZE; i += 128)
		{
			SPI_FLASH::Read(FLASH_SECTOR_23 + i, buf, 128);
			SPI_FLASH::Write(sector + i, buf, 128);
		}
	}
}
void SPI_FLASH::BusyWait()
{
	uint8_t buf[] = {FLASH_CMD_READ_STATUS, 0x00};
		
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();	
		
		SPI::Transfer(buf, 2, buf);
		
		FLASH_UNSELECT();
	}
	
	while(buf[1] & 1)
	{
		_delay_ms(1);
		
		buf[0] = FLASH_CMD_READ_STATUS;
		
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
		{
			FLASH_SELECT();
			
			SPI::Transfer(buf, 2, buf);
			
			FLASH_UNSELECT();
		}
	}
}
void SPI_FLASH::WriteEnable()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_WRITE_ENABLE);
		
		FLASH_UNSELECT();
	}
}
void SPI_FLASH::WriteStatusEnable()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_WRITE_ENABLE_STATUS);
		
		FLASH_UNSELECT();
	}
}
void SPI_FLASH::WriteDisable()
{
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_WRITE_DISABLE);
		
		FLASH_UNSELECT();
	}
}
void SPI_FLASH::BlockErase(uint32_t ulAddress)
{
	ulAddress &= FLASH_MAX_ADDRESS;
	
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteEnable();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_BLOCK_ERASE);
		SPI::TransferByte((ulAddress >> 16) & 0xFF);
		SPI::TransferByte((ulAddress >> 8) & 0xFF);
		SPI::TransferByte(ulAddress & 0xFF);
		
		FLASH_UNSELECT();
	}

	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteDisable();
}
void SPI_FLASH::SectorErase(uint32_t ulAddress)
{	
	ulAddress &= FLASH_MAX_ADDRESS;
	
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteEnable();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_SECTOR_ERASE);
		SPI::TransferByte((ulAddress >> 16) & 0xFF);
		SPI::TransferByte((ulAddress >> 8) & 0xFF);
		SPI::TransferByte(ulAddress & 0xFF);
		
		FLASH_UNSELECT();
	}

	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteDisable();
}
void SPI_FLASH::ChipErase()
{
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteEnable();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::TransferByte(FLASH_CMD_CHIP_ERASE);
		
		FLASH_UNSELECT();
	}

	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteDisable();
}
uint8_t SPI_FLASH::ReadDeviceID()
{
	uint8_t buf[] = {FLASH_CMD_READ_ID, 0x00, 0x00, 0x01, 0x00};
	
	SPI_FLASH::BusyWait();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::Transfer(buf, 5, buf);
			
		FLASH_UNSELECT();
	}
	
	return buf[4];
}
uint8_t SPI_FLASH::ReadManufacturerID()
{
	uint8_t buf[] = {FLASH_CMD_READ_ID, 0x00, 0x00, 0x00, 0x00};
	
	SPI_FLASH::BusyWait();
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::Transfer(buf, 5, buf);
		
		FLASH_UNSELECT();
	}
	
	return buf[4];
}
void SPI_FLASH::ProtectSectors(uint8_t ubProtect)
{
	SPI_FLASH::BusyWait();
	SPI_FLASH::WriteStatusEnable();
	
	uint8_t buf[] = {FLASH_CMD_WRITE_STATUS, (uint8_t)((ubProtect & 0x03) << 2)};
	
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		FLASH_SELECT();
		
		SPI::Transfer(buf, 2, 0);
		
		FLASH_UNSELECT();
	}
}