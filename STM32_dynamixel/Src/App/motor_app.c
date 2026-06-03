/*
 * motor_app.c
 */
#include "motor_app.h"
#include "main.h"
#include "dynamixel_2_0.h"
#include "util.h"
#include "drv_uart.h"

motor_command_t motor_cmd;

#define Init_MAX_limit 		4095
#define Init_MIN_limit 		0
#define With_GRIPPER		0

uint8_t prev_emergency = 0;
uint8_t error_count = 0;
uint32_t last_reconnect_attempt = 0;
static uint32_t last_master_rx_tick = 0;

void motor_init(uint8_t id, motor_command_t *motor_cmd,
		motor_status_t *motor_status) {
	term_printf("\n\r[INIT-001]Dynamixel XM430-W350 init...\n\r");
	motor_cmd->id = id;
	motor_status->id = id;
	motor_cmd->LED_state = false;
	motor_cmd->target_position = 0;
	motor_cmd->target_velocity = 0;
	motor_cmd->control_mode = POSITION_MODE;
	motor_status->present_position = 0;
	motor_status->present_velocity = 0;
	motor_status->present_current = 0;
	motor_status->present_temperature = 0;
	motor_cmd->torque_enabled = false;
	motor_status->state = MOTOR_STATE_INIT;

	if (dynamixel2_ping(id)) {

#if With_GRIPPER
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setOperatingMode(id, VELOCITY_MODE);
		dynamixel2_set_torque_enable(id, 1);
		dynamixel2_set_goal_velocity(id, 0);

		do {
			dynamixel2_set_goal_velocity(id, 50);
		} while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != GPIO_PIN_RESET);

		dynamixel2_set_goal_velocity(id, 0);
		HAL_Delay(500);

		motor_status->Max_pos_lim = (((dynamixel2_read_present_position(id)
				% 4096) + 4096) % 4096) - 20;
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setMaxPositionLimit(id, motor_status->Max_pos_lim);
		term_printf("[INIT-002]first limit position saved !\r\n");
		dynamixel2_set_torque_enable(id, 1);
		HAL_Delay(100);


		do {
			dynamixel2_set_goal_velocity(id, -50);
		} while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) != GPIO_PIN_RESET);
		dynamixel2_set_goal_velocity(id, 0);
		HAL_Delay(100);

		motor_status->Min_pos_lim = (((dynamixel2_read_present_position(id)
				% 4096) + 4096) % 4096) + 20;
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setMinPositionLimit(id, motor_status->Min_pos_lim);
		term_printf("[INIT-002]second limit position saved !\r\n");
		dynamixel2_setOperatingMode(id, motor_cmd->control_mode);
		dynamixel2_set_torque_enable(id, 1);
		int32_t moyenne =
				(motor_status->Max_pos_lim + motor_status->Min_pos_lim) / 2;
		dynamixel2_set_goal_position(id, moyenne);
		HAL_Delay(500);

		dynamixel2_set_torque_enable(id, motor_cmd->torque_enabled);
		motor_status->state = MOTOR_GRIPPER_STATE;

#else
		motor_status->Max_pos_lim = Init_MAX_limit;
		motor_status->Min_pos_lim = Init_MIN_limit;

		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setMaxPositionLimit(id, Init_MAX_limit);
		dynamixel2_setMinPositionLimit(id, Init_MIN_limit);
		dynamixel2_setOperatingMode(id, motor_cmd->control_mode);
		dynamixel2_set_torque_enable(id, 1);
		dynamixel2_set_goal_position(id, motor_cmd->target_position);
		motor_status->state = MOTOR_STATE_READY;
		term_printf("[INIT-002]Software limits saved !\r\n");

#endif

		motor_status->Current_lim = dynamixel2_getCurrentLimit(id);
		motor_status->Velocity_lim = dynamixel2_getVelocityLimit(id);
		motor_status->control_mode_st = dynamixel2_getOperatingMode(id);
		motor_status->baudrate = dynamixel2_get_BaudRate(id);
		motor_cmd->torque_enabled = true;

		term_printf("[INIT-001]Dynamixel XM430-W350 OK!\r\n");
		term_printf("\n\r");
	} else {
		motor_status->state = MOTOR_STATE_ERROR;
		term_printf("[INIT-001]Connection to the motor failed.\r\n");
		term_printf("\n\r");
	}
}

void motor_update_master_watchdog(void) {
	last_master_rx_tick = HAL_GetTick();
}

uint8_t motor_master_timed_out(void)
{
    return (HAL_GetTick() - last_master_rx_tick) > COMM_TIMEOUT_MS;
}




