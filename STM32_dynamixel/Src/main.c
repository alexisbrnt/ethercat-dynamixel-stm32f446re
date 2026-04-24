/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "drv_gpio.h"
#include "drv_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os2.h"
#include "ECAT/soes/ecat_slv.h"
#include "ECAT/soes/esc.h"
#include "ECAT/soes-esi/utypes.h"

#define TEST	0
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
_Objects Obj;
motor_command_t motor_cmd1;
motor_status_t motor_status1;
SemaphoreHandle_t mutex_motor;
DMA_HandleTypeDef hdma_usart1_rx;
extern uint8_t rec_buf1[];

static esc_cfg_t config = { .user_arg = "/dev/lan9252", .use_interrupt = 0,
		.watchdog_cnt = 500, .set_defaults_hook = NULL, .pre_state_change_hook =
		NULL, .post_state_change_hook = NULL, .application_hook = NULL,
		.safeoutput_override = NULL, .pre_object_download_hook = NULL,
		.post_object_download_hook = NULL, .rxpdo_override = NULL,
		.txpdo_override = NULL, .esc_hw_interrupt_enable = NULL,
		.esc_hw_interrupt_disable = NULL, .esc_hw_eep_handler = NULL,
		.esc_check_dc_handler = NULL, };

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define ID_1			1
#define BUFFER_LENGTH	128
void MX_DMA_Init(void);

void cb_get_inputs() {
	if (xSemaphoreTake(mutex_motor, pdMS_TO_TICKS(1)) == pdTRUE) {
		Obj.ID_TX = motor_status1.id;
		Obj.state = (uint8_t) motor_status1.state;
		Obj.present_position = motor_status1.present_position;
		Obj.present_velocity = motor_status1.present_velocity;
		Obj.present_current = motor_status1.present_current;
		Obj.present_temperature = motor_status1.present_temperature;
		Obj.baudrate = motor_status1.baudrate;
		Obj.operating_mode = motor_status1.control_mode_st;
		Obj.Max_pos_lim = motor_status1.Max_pos_lim;
		Obj.Min_pos_lim = motor_status1.Min_pos_lim;
		Obj.Velocity_lim = motor_status1.Velocity_lim;
		Obj.Current_lim = motor_status1.Current_lim;
		Obj.Hardware_error_status = motor_status1.Hardware_error_status;
		Obj.Moving = motor_status1.Moving;
		Obj.torque_status = motor_status1.torque_status;
		xSemaphoreGive(mutex_motor);
	}
}

void cb_set_outputs() {
	motor_cmd1.id = Obj.ID_RX;
	motor_cmd1.control_mode = Obj.control_mode;
	motor_cmd1.torque_enabled = Obj.torque_enabled;
	motor_cmd1.LED_state = Obj.LED_STATE;
	motor_cmd1.target_position = Obj.goal_position;
	motor_cmd1.target_velocity = Obj.target_velocity;
	motor_cmd1.target_current = Obj.target_current;
	motor_cmd1.reboot = Obj.Reboot;
	motor_cmd1.emergency_stop = Obj.Emergency_stop;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */

volatile uint8_t ecat_operational = 0;

static void task_EtherCAT(void *pvParameters) {
	while (!ecat_operational) {
		ecat_slv();
		if ((ESCvar.App.state & APPSTATE_OUTPUT) > 0) {
			ecat_operational = 1;
			term_printf("EtherCAT OP reached!\r\n");
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	TickType_t xLastWakeTime = xTaskGetTickCount();
	for (;;) {
		ecat_slv();
		vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
	}
}

static void task_motor_cmd(void *pvParameters) {
	while (!ecat_operational) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}
	for (;;) {
		if (xSemaphoreTake(mutex_motor, pdMS_TO_TICKS(200))) {
			motor_command(&motor_cmd1, &motor_status1);
			xSemaphoreGive(mutex_motor);
		}
		vTaskDelay(pdMS_TO_TICKS(5));
	}
}

static void task_motor_status(void *pvParameters) {

	while (!ecat_operational) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}

	for (;;) {
		if (xSemaphoreTake(mutex_motor, pdMS_TO_TICKS(5))) {
			motor_status(&motor_status1, &motor_cmd1);

			xSemaphoreGive(mutex_motor);
		}
		vTaskDelay(pdMS_TO_TICKS(5));
	}
}

int main(void) {

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration--------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* USER CODE BEGIN Init */

	/* USER CODE END Init */

	/* Configure the system clock */
	SystemClock_Config();

	/* USER CODE BEGIN SysInit */

	/* USER CODE END SysInit */

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_DMA_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();

	dynamixel2_dma_init();
	/* USER CODE BEGIN 2 */
	term_printf("\n\r===== Boot =====\r\n");
	ecat_slv_init(&config);

	motor_init(ID_1, &motor_cmd1, &motor_status1);
#if TEST == 0
	/* USER CODE END 2 */
	mutex_motor = xSemaphoreCreateMutex();
	osKernelInitialize();
	xTaskCreate(task_EtherCAT, "ECAT", 1024, NULL, 32, NULL);
	xTaskCreate(task_motor_cmd, "MOT_CMD", 1024, NULL, 24, NULL);
	xTaskCreate(task_motor_status, "MOT_RD", 1024, NULL, 20, NULL);

	osKernelStart();
	/* USER CODE BEGIN WHILE */
#else

	while (1) {
		GPIO_PinState state_close = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
		GPIO_PinState state_open = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0);

		if (state_open == GPIO_PIN_RESET) {
			term_printf("open\r\n");
		} else if (state_close == GPIO_PIN_RESET){
			term_printf("close\r\n");
		}

	}

#endif

}
/* USER CODE END 3 */

void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/* HSI 16 MHz -> PLL -> 180 MHz */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLM = 8; /* HSI/8 = 2 MHz */
	RCC_OscInitStruct.PLL.PLLN = 180; /* 2*180 = 360 MHz VCO */
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2; /* 360/2 = 180 MHz SYSCLK */
	RCC_OscInitStruct.PLL.PLLQ = 8; /* pour USB si besoin */
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	if (HAL_PWREx_EnableOverDrive() != HAL_OK) {
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; /* HCLK = 180 MHz */
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4; /* APB1 = 45 MHz */
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; /* APB2 = 90 MHz */

	/* IMPORTANT : Flash latency 5 WS pour 180 MHz */
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN SPI1_Init 2 */

/* USER CODE END SPI1_Init 2 */

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
	/* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
