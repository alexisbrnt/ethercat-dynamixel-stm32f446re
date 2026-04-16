/*
 * drv_dma.h
 */

#ifndef SRC_DRV_DRV_DMA_H_
#define SRC_DRV_DRV_DMA_H_

#include "main.h"

void dynamixel2_dma_init(void);
uint16_t dma_get_write_index(void) ;
uint16_t dma_available(void);
uint8_t dma_read_byte(void) ;
uint8_t dma_peek(uint16_t offset);
void dma_skip(uint16_t count);
void dma_clear_buffer(void);
#endif /* SRC_DRV_DRV_DMA_H_ */
