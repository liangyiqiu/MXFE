/* Copyright (c) 2015-2017 Henrik Brix Andersen <henrik@brixandersen.dk>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#define PAGE_SIZE		256

/*
 * Byte Positions.
 */
#define BYTE1				0 /* Byte 1 position */
#define BYTE2				1 /* Byte 2 position */
#define BYTE3				2 /* Byte 3 position */
#define BYTE4				3 /* Byte 4 position */
#define BYTE5				4 /* Byte 5 position */

#define READ_WRITE_EXTRA_BYTES		4 /* Read/Write extra bytes */
#define	READ_WRITE_EXTRA_BYTES_4BYTE_MODE	5 /**< Command extra bytes */

#define RD_ID_SIZE					4

#define ISSI_ID_BYTE0			0x9D
#define MICRON_ID_BYTE0			0x20

#define ENTER_4B_ADDR_MODE		0xb7 /* Enter 4Byte Mode command */
#define EXIT_4B_ADDR_MODE		0xe9 /* Exit 4Byte Mode command */
#define EXIT_4B_ADDR_MODE_ISSI	0x29
#define	WRITE_ENABLE			0x06 /* Write Enable command */

#define ENTER_4B	1
#define EXIT_4B		0

#define	FLASH_16_MB	0x18
#define FLASH_MAKE		0
#define	FLASH_SIZE		2

#include <stdio.h>
#include <string.h>

#include <mb_interface.h>
#include <xil_cache.h>
#include <xil_printf.h>
#include <xspi.h>
#include <xparameters.h>

#include "elf32.h"
#include "eb-config.h"

static XSpi Spi;
u8 ReadBuffer[EFFECTIVE_READ_BUFFER_SIZE + SPI_VALID_DATA_OFFSET];
u8 WriteBuffer[260];
u8 FlashID[3];
int Status;

/*
 * Reduce code size on Microblaze (no interrupts used)
 */
#ifdef __MICROBLAZE__
void _interrupt_handler() {}
void _exception_handler() {}
void _hw_exception_handler() {}
#endif

/**
 * Simple wrapper function for reading a number of bytes from SPI flash.
 */
int spi_flash_read(XSpi *InstancePtr, u32 FlashAddress, u8 *RecvBuffer, unsigned int ByteCount)
{
	RecvBuffer[0] = SPI_READ_OPERATION;
	RecvBuffer[1] = (u8) (FlashAddress >> 24);
	RecvBuffer[2] = (u8) (FlashAddress >> 16);
	RecvBuffer[3] = (u8) (FlashAddress >> 8);
	RecvBuffer[4] = (u8) FlashAddress;

	return XSpi_Transfer(InstancePtr, RecvBuffer, RecvBuffer, ByteCount + SPI_VALID_DATA_OFFSET);
}

int flash_read_id()
{
	int Status;
		int i;

		/* Read ID in Auto mode.*/
		WriteBuffer[0] = 0x9f;
		WriteBuffer[1] = 0xff;		/* 4 dummy bytes */
		WriteBuffer[2] = 0xff;
		WriteBuffer[3] = 0xff;
		WriteBuffer[4] = 0xff;

		Status = XSpi_Transfer(&Spi, WriteBuffer, ReadBuffer, 5);
		if (Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		for(i = 0; i < 3; i++)
			FlashID[i] = ReadBuffer[i + 1];

		xil_printf("FlashID=0x%x 0x%x 0x%x\n\r", ReadBuffer[1], ReadBuffer[2],
				ReadBuffer[3]);
		return XST_SUCCESS;
}

int FlashWriteEnable(XSpi *Spi)
{
	int Status;
	u8 *NULLPtr = NULL;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = WRITE_ENABLE;

	Status = XSpi_Transfer(Spi, WriteBuffer, NULLPtr, 1);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int FlashEnterExit4BAddMode(XSpi *Spi, unsigned int Enable)
{
	int Status;
	u8 *NULLPtr = NULL;

	if((FlashID[FLASH_MAKE] == MICRON_ID_BYTE0) ||
		(FlashID[FLASH_MAKE] == ISSI_ID_BYTE0)) {

		Status = FlashWriteEnable(Spi);
		if(Status != XST_SUCCESS) {
			return XST_FAILURE;
		}
	}

	if (Enable) {
		WriteBuffer[BYTE1] = ENTER_4B_ADDR_MODE;
	} else {
		if (FlashID[FLASH_MAKE] == ISSI_ID_BYTE0)
			WriteBuffer[BYTE1] = EXIT_4B_ADDR_MODE_ISSI;
		else
			WriteBuffer[BYTE1] = EXIT_4B_ADDR_MODE;
	}

	Status = XSpi_Transfer(Spi, WriteBuffer, NULLPtr, 1);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int main()
{
	elf32_hdr hdr;
	elf32_phdr phdr;
	elf32_shdr shdr;
	u32 addr;
	int ret, i, j;

	/*
	 * Disable caches
	 */
#if (XPAR_MICROBLAZE_USE_DCACHE == 1)
	Xil_DCacheInvalidate()
	Xil_DCacheDisable();
#endif
#if (XPAR_MICROBLAZE_USE_ICACHE == 1)
	Xil_ICacheInvalidate();
	Xil_ICacheDisable();
#endif

	print("\r\nSPI ELF Bootloader\r\n");

	/*
	 * Initialize the SPI controller in polled mode
	 * and enable the flash slave select
	 */
	ret = XSpi_Initialize(&Spi, SPI_DEVICE_ID);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_IS_STARTED) {
			if (XSpi_Stop(&Spi) == XST_DEVICE_BUSY) {
				print("SPI device is busy\r\n");
				return -1;
			}
		} else if (ret == XST_DEVICE_NOT_FOUND) {
			print("SPI device not found\r\n");
			return -1;
		} else {
			print("Unknown error\r\n");
			return -1;
		}
	}

	ret = XSpi_SetOptions(&Spi, XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_BUSY) {
			print("SPI device is busy\r\n");
			return -1;
		} else if (ret == XST_SPI_SLAVE_ONLY) {
			print("SPI device is slave-only\r\n");
			return -1;
		} else {
			print("Unknown error\r\n");
			return -1;
		}
	}

	ret = XSpi_SetSlaveSelect(&Spi, SPI_FLASH_SLAVE_SELECT);
	if (ret != XST_SUCCESS) {
		if (ret == XST_DEVICE_BUSY) {
			print("SPI device is busy\r\n");
			return ret;
		} else if (ret == XST_SPI_TOO_MANY_SLAVES) {
			print("Too many SPI slave devices\r\n");
			return ret;
		} else {
			print("Unknown error\r\n");
			return ret;
		}
	}

	XSpi_Start(&Spi);
	XSpi_IntrGlobalDisable(&Spi);

	ret=flash_read_id();
	if(FlashID[FLASH_SIZE] > FLASH_16_MB) {
			Status = FlashEnterExit4BAddMode(&Spi, ENTER_4B);
			if(Status != XST_SUCCESS) {
				return XST_FAILURE;
			}
		}

	print("Copying ELF image from SPI flash @ 0x");
	putnum(ELF_IMAGE_BASEADDR);
	print(" to RAM\r\n");

	/*
	 * Read ELF header
	 */
	ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR, ReadBuffer, sizeof(hdr)+SPI_VALID_DATA_OFFSET);
	if (ret == XST_SUCCESS) {
		memcpy(&hdr, ReadBuffer + SPI_FLASH_NDUMMY_BYTES+1, sizeof(hdr));

#ifdef DEBUG_ELF_LOADER
		print("hdr.ident:\r\n");
		for (i = 0; i < HDR_IDENT_NBYTES; i++) {
			putnum(hdr.ident[i]);
			print("\r\n");
		}
		print("hdr.shnum:\r\n");
		putnum(hdr.shnum);
		print("\r\n");
		xil_printf("hdr.shoff=0x%x \n\r", hdr.shoff);
#endif

	} else {
		print("Failed to read ELF header");
		return -1;
	}

	/*
	 * Validate ELF header
	 */
	if (hdr.ident[0] != HDR_IDENT_MAGIC_0 ||
			hdr.ident[1] != HDR_IDENT_MAGIC_1 ||
			hdr.ident[2] != HDR_IDENT_MAGIC_2 ||
			hdr.ident[3] != HDR_IDENT_MAGIC_3) {
		print("Invalid ELF header");
		return -1;
	}


	/**
	 * Read ELF section headers
	 */
	for (i = 0; i < hdr.shnum; i++)
	{
		ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + hdr.shoff + i * hdr.shentsize, ReadBuffer, sizeof(shdr));
		if (ret == XST_SUCCESS)
		{
			memcpy(&shdr, ReadBuffer + SPI_FLASH_NDUMMY_BYTES+1, sizeof(shdr));
			if (shdr.flags != 0)
			{
				/*
				 * Copy program segment from flash to RAM
				 */
				xil_printf("Section: %d Address: 0x%x Offset: 0x%x Size: 0x%x\n\r",i, shdr.addr, shdr.offset,shdr.size);

				for (addr = 0; addr < shdr.size; addr += EFFECTIVE_READ_BUFFER_SIZE)
				{
					if (addr + EFFECTIVE_READ_BUFFER_SIZE > shdr.size)
					{
						ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + shdr.offset + addr,
								ReadBuffer, shdr.size - addr);

					}
					else
					{
						ret = spi_flash_read(&Spi, ELF_IMAGE_BASEADDR + shdr.offset + addr,
								ReadBuffer, EFFECTIVE_READ_BUFFER_SIZE);
#ifdef DEBUG_ELF_LOADER
						if (addr == 0)
						{
							print("segment start:\r\n");
							for (j = 0; j < EFFECTIVE_READ_BUFFER_SIZE; j++)
							{
								putnum(ReadBuffer[SPI_FLASH_NDUMMY_BYTES+1 + j]);
								print(" ");
							}
							print("\r\n");
						}
#endif
					}
					if (ret == XST_SUCCESS)
					{
						if (addr + EFFECTIVE_READ_BUFFER_SIZE > shdr.size) {
							memcpy((void*)shdr.addr + addr, ReadBuffer + SPI_FLASH_NDUMMY_BYTES+1, shdr.size - addr);
						} else {
							memcpy((void*)shdr.addr + addr, ReadBuffer + SPI_FLASH_NDUMMY_BYTES+1, EFFECTIVE_READ_BUFFER_SIZE);
						}
#ifdef DEBUG_ELF_LOADER
						if (addr % 1024 == 0) {
							print(".");
						}
#endif
					} else {
						print("Failed to read ELF program segment");
						return -1;
					}
				}
			}
		}
		else
		{
			print("Failed to read ELF program header");
			return -1;
		}
	}

	/**
	 * Jump to ELF entry address
	 */
	print("\r\nTransferring execution to program @ 0x");
	putnum(hdr.entry);
	print("\r\n");

	((void (*)())hdr.entry)();

//	void (*laddr)();
//	laddr=(void (*)())hdr.entry;
//	(*laddr)();


	// Never reached
	return 0;
}
