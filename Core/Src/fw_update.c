/*
 * Firmware update
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#include "fw_update.h"

#include <string.h>

#define MAX_WRITE_SIZE 256

extern const uint32_t *_app_len;
extern const uint32_t *_app;
#define APP_LENGTH ((uint32_t) &_app_len)
#define APP_ADDRESS ((uint32_t) &_app)

volatile struct bl_params bl __attribute__((section(".noinit")));


inline static void get_data(struct w25q *mem, uint32_t address, uint16_t size,
		uint8_t *buffer)
{
	w25q_read_data(mem, address, buffer, size);
}

static void set_data(struct w25q *mem, uint32_t address, uint16_t size,
		uint8_t *buffer)
{
	uint16_t ws;

	while (size)
	{
		if (size > MAX_WRITE_SIZE)
			ws = MAX_WRITE_SIZE;
		else
			ws = size;

		w25q_write_enable(mem);
		w25q_write_data(mem, address, buffer, ws);

		size -= ws;
		address += ws;
		buffer += ws;
	}
}

inline static void erase_header(struct w25q *mem, struct fws *header)
{
	w25q_write_enable(mem);
	w25q_sector_erase(mem, FWS_HEADER_ADDR);
}

inline static void get_header(struct w25q *mem, struct fws *header)
{
	get_data(mem, FWS_HEADER_ADDR, sizeof(struct fws), (uint8_t *) header);
}

inline static void set_header(struct w25q *mem, struct fws *header)
{
	erase_header(mem, header);
	set_data(mem, FWS_HEADER_ADDR, sizeof(struct fws), (uint8_t *) header);
}

inline static void get_payload(struct w25q *mem, uint32_t address,
		uint8_t *buffer, uint16_t size)
{
	get_data(mem, FWS_PAYLOAD_ADDR + address, size, buffer);
}

static enum fws_status update(struct w25q *mem)
{
	uint8_t buffer[MAX_WRITE_SIZE];
	uint32_t checksum;
	uint32_t address;
	uint16_t ws;

	struct fws header;

	// >>>
	// Is SPI FLASH connected?
	if (w25q_get_manufacturer_id(mem) != FWS_WINBOND_MANUFACTURER_ID)
		return FWS_STATUS_ERR_NO_STORAGE;

	// >>>
	// Was new firmware downloaded?
	get_header(mem, &header);
	if (header.loaded)
		return FWS_STATUS_NO_FW;

	// >>>
	// Is size valid?
	if (header.size > APP_LENGTH)
		return FWS_STATUS_ERR_FW_SIZE;

	// >>>
	// Is checksum valid?
	checksum = FWS_CHECKSUM_INIT;
	address = 0;
	while (address < header.size)
	{
		if ((header.size - address) > MAX_WRITE_SIZE)
			ws = MAX_WRITE_SIZE;
		else
			ws = header.size - address;

		get_payload(mem, address, buffer, ws);
		address += ws;

		for (uint32_t i = 0; i < ws; i += sizeof(uint32_t))
			checksum += *((uint32_t *) &buffer[i]);
	}

	if (checksum != header.checksum)
		return FWS_STATUS_ERR_CHECKSUM_STORAGE;

	// >>>
	// Clear MCU FLASH
	uint32_t page_error;
	FLASH_EraseInitTypeDef erase_init;
	erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
	erase_init.Banks = FLASH_BANK_1;
	erase_init.PageAddress = APP_ADDRESS;
	erase_init.NbPages = APP_LENGTH / FLASH_PAGE_SIZE;

	HAL_FLASH_Unlock();
	if (HAL_FLASHEx_Erase(&erase_init, &page_error) != HAL_OK)
		return FWS_STATUS_ERR_ERASE;
	HAL_FLASH_Lock();

	// >>>
	// Load firmware to MCU FLASH
	address = 0;
	HAL_FLASH_Unlock();
	while (address < header.size)
	{
		if ((header.size - address) > MAX_WRITE_SIZE)
			ws = MAX_WRITE_SIZE;
		else
			ws = header.size - address;

		get_payload(mem, address, buffer, ws);

		for (uint32_t i = 0; i < ws; i += sizeof(uint64_t))
			HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, APP_ADDRESS +
					address + i, *((uint64_t *) &buffer[i]));
		address += ws;
	}
	HAL_FLASH_Lock();

	// >>>
	// Is loaded firmware checksum valid?
	checksum = FWS_CHECKSUM_INIT;
	for (uint32_t i = 0; i < header.size; i += sizeof(uint32_t))
		checksum += *((uint32_t *) (APP_ADDRESS + i));
	if (checksum != header.checksum)
		return FWS_STATUS_ERR_CHECKSUM_LOADED;

	// >>>
	// Update header done status
	header.loaded = 0x01;
	set_header(mem, &header);

	return FWS_STATUS_SUCCESS;
}

int fw_update(struct w25q *mem)
{
	enum fws_status status;

	status = update(mem);

	bl.status = status;
	strcpy((char *) bl.datetime, __TIMESTAMP__);

	if ((status == FWS_STATUS_NO_FW) || (status == FWS_STATUS_SUCCESS))
		return 0;
	return -1;
}
