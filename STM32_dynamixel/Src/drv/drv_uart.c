/*
 * drv_uart.c
 */
#include "drv_uart.h"
#include "math.h"
#include "util.h"
#include "dynamixel_2_0.h"

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
uint8_t rec_buf1[NB_CAR_TO_RECEIVE+1]="";
uint8_t rec_buf2[NB_CAR_TO_RECEIVE+1]="";


void MX_DMA_Init(void)
{
    __HAL_RCC_DMA2_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 3000000;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_8;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  //HAL_UART_RxCpltCallback(&huart1);  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

//===============================================================================
// PUT CHAR
//===============================================================================
int put_char(char ch){
	HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xFFFF);
	return 0;
}
//===============================================================================
// PUT STRING
//===============================================================================
void put_string(char* s){
	while (*s != '\0'){
		put_char(*s);
		s++;
	}
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{

    if (huart->Instance == USART1)
    {

        //dynamixel2_receive_callback(rec_buf1 [0]);
        HAL_UART_Receive_IT(&huart1, (uint8_t *)rec_buf1, NB_CAR_TO_RECEIVE);
    }

    else if (huart->Instance == USART2)
    {
    	HAL_UART_Receive_IT(&huart2, (uint8_t *)rec_buf2, NB_CAR_TO_RECEIVE);
    }
}


//================================================================
//				TERM_PRINTF
//================================================================

void term_printf(const char* fmt, ...)
{
	__gnuc_va_list         ap;
	char          *p;
	char           ch;
	unsigned long  ul;
	unsigned long long ull;
	unsigned long  size;
	unsigned int   sp;
	char           s[60];
	int first=0;

	va_start(ap, fmt);

	while (*fmt != '\0') {
		if (*fmt =='%') {
			size=0; sp=1;
			if (*++fmt=='0') {fmt++; sp=0;}	// parse %04d --> sp=0
			ch=*fmt;
			if ((ch>'0') && (ch<='9')) {	// parse %4d --> size=4
				char tmp[10];
				int i=0;
				while ((ch>='0') && (ch<='9')) {
					tmp[i++]=ch;
					ch=*++fmt;
				}
				tmp[i]='\0';
				size=str2num(tmp,10);
			}
			switch (ch) {
				case '%':
					put_char('%');
					break;
				case 'c':
					ch = va_arg(ap, int);
					put_char(ch);
					break;
				case 's':
					p = va_arg(ap, char *);
					put_string(p);
					break;
				case 'd':
					ul = va_arg(ap, long);
					if ((long)ul < 0) {
						put_char('-');
						ul = -(long)ul;
						//size--;
					}
					num2str(s, ul, 10, size, sp);
					put_string(s);
					break;
				case 'u':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 10, size, sp);
					put_string(s);
					break;
				case 'o':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 8, size, sp);
					put_string(s);
					break;
				case 'p':
					put_char('0');
					put_char('x');
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string(s);
					break;
				case 'x':
					ul = va_arg(ap, unsigned int);
					num2str(s, ul, 16, size, sp);
					put_string(s);
					break;
				case 'f':
					if(first==0){ ull = va_arg(ap, long long unsigned int); first = 1;}
					ull = va_arg(ap, long long unsigned int);
					int sign = ( ull & 0x80000000 ) >> 31;
					int m = (ull & 0x000FFFFF) ; // should be 0x007FFFFF
					float mf = (float)m ;
					mf = mf / pow(2.0,20.0);
					mf = mf + 1.0;
					int e = ( ull & 0x78000000 ) >> 23 ; // should be int e = ( ul & 0x7F800000 ) >> 23;
					e = e | (( ull & 0x000F00000 ) >> 20);
					e = e - 127;
					float f = mf*myPow(2.0,e);
					if(sign==1){ put_char('-'); }
					float2str((char*)s, f, 5);
					put_string((char*)s);
					break;

				default:
					put_char(*fmt);
			}
		} else put_char(*fmt);
		fmt++;
	}
	va_end(ap);
}
