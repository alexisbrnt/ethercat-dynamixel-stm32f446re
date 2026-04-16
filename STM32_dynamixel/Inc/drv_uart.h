 /*
 * drv_uart.h
 */

#ifndef SRC_DRV_DRV_UART_H_
#define SRC_DRV_DRV_UART_H
#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
#define NB_CAR_TO_RECEIVE		1		// nombre de caractères à recevoir pour déclencher une interruption


void num2str(char *s, unsigned int number, unsigned int base, unsigned int size, int sp);
unsigned int str2num(char *s, unsigned base);
void term_printf(const char* fmt, ...);
void sendFrame(unsigned char* s, int size);

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;


#endif /* SRC_DRV_DRV_UART_H_ */
