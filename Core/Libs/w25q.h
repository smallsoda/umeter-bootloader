/*
 * W25Q Serial FLASH memory
 *
 * Dmitry Proshutinsky <dproshutinsky@gmail.com>
 * 2024
 */

#ifndef W25Q_H_
#define W25Q_H_

#include "stm32f1xx_hal.h"

#define W25Q_SECTOR_SIZE 4096

struct w25q
{
	SPI_HandleTypeDef *spi;
	GPIO_TypeDef *cs_port;
	uint16_t cs_pin;
};


void w25q_init(struct w25q *mem, SPI_HandleTypeDef *spi, GPIO_TypeDef *cs_port,
		uint16_t cs_pin);

void w25q_hw_init(struct w25q *mem);
void w25q_hw_deinit(struct w25q *mem);

void w25q_sector_erase(struct w25q *mem, uint32_t address);
void w25q_block_erase(struct w25q *mem, uint32_t address);
void w25q_chip_erase(struct w25q *mem);
void w25q_write_enable(struct w25q *mem);
void w25q_read_data(struct w25q *mem, uint32_t address, uint8_t *data,
		uint16_t size);
void w25q_write_data(struct w25q *mem, uint32_t address, uint8_t *data,
		uint16_t size);
size_t w25q_get_capacity(struct w25q *mem);
uint8_t w25q_get_manufacturer_id(struct w25q *mem);

#endif /* W25Q_H_ */
