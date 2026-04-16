#include "drv_dma.h"

#define DMA_RX_BUFFER_SIZE 256

uint8_t dma_rx_buffer[DMA_RX_BUFFER_SIZE];

static volatile uint16_t read_index = 0;
uint16_t dma_get_write_index(void) {
	return (DMA_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx));
}

 uint16_t dma_available(void) {
	uint16_t write_idx = dma_get_write_index();
	return (write_idx - read_index + DMA_RX_BUFFER_SIZE) % DMA_RX_BUFFER_SIZE;
}

uint8_t dma_read_byte(void) {
	uint8_t byte = dma_rx_buffer[read_index];
	read_index = (read_index + 1) % DMA_RX_BUFFER_SIZE;
	return byte;
}

uint8_t dma_peek(uint16_t offset) {
	return dma_rx_buffer[(read_index + offset) % DMA_RX_BUFFER_SIZE];
}

void dma_skip(uint16_t count) {
	read_index = (read_index + count) % DMA_RX_BUFFER_SIZE;
}

void dma_clear_buffer(void)
{
    read_index = dma_get_write_index();
}


void dynamixel2_dma_init(void) {
	read_index = 0;
	HAL_UART_Receive_DMA(&huart1, dma_rx_buffer, DMA_RX_BUFFER_SIZE);


	__HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT | DMA_IT_TC);
}
