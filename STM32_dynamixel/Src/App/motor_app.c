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
#define With_GRIPPER		1

//motor_status_t motor_status;

void motor_init(uint8_t id, motor_command_t *motor_cmd,
		motor_status_t *motor_status) {
	term_printf("\n\rDynamixel XM430-W350 init...\n\r");
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

		/*uint8_t cnt = 0;
		 uint32_t t0 = HAL_GetTick();
		 do {
		 dynamixel2_set_goal_velocity(id, 50);
		 HAL_Delay(50);
		 cnt = (abs(dynamixel2_read_present_current(id)) >= 5) ? cnt+1 : 0;
		 if (HAL_GetTick() - t0 > 10000) {  break; }
		 } while(cnt < 2);*/
		do {
			dynamixel2_set_goal_velocity(id, 50);
		} while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) != GPIO_PIN_RESET);

		dynamixel2_set_goal_velocity(id, 0);
		HAL_Delay(500);

		motor_status->Max_pos_lim = (((dynamixel2_read_present_position(id)
				% 4096) + 4096) % 4096)-20;
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setMaxPositionLimit(id, motor_status->Max_pos_lim);
		dynamixel2_set_torque_enable(id, 1);
		HAL_Delay(100);

		// Recherche limite MIN
		/*cnt = 0; t0 = HAL_GetTick();
		 do {
		 dynamixel2_set_goal_velocity(id, -50);     // ← underscore corrigé
		 HAL_Delay(50);
		 cnt = (abs(dynamixel2_read_present_current(id)) >= 7) ? cnt+1 : 0;
		 if (HAL_GetTick() - t0 > 10000) {  break; }
		 } while(cnt < 3);*/

		do {
			dynamixel2_set_goal_velocity(id, -50);
		} while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) != GPIO_PIN_RESET);
		dynamixel2_set_goal_velocity(id, 0);
		HAL_Delay(100);

		motor_status->Min_pos_lim = (((dynamixel2_read_present_position(id)
				% 4096) + 4096) % 4096)+20;
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setMinPositionLimit(id, motor_status->Min_pos_lim);
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
#endif

		motor_status->Current_lim = dynamixel2_getCurrentLimit(id);
		motor_status->Velocity_lim = dynamixel2_getVelocityLimit(id);
		motor_status->control_mode_st = dynamixel2_getOperatingMode(id);
		motor_status->baudrate = dynamixel2_get_BaudRate(id);
		motor_cmd->torque_enabled = true;

		//dynamixel2_set_LED(id, motor_cmd->LED_state);
		term_printf("Dynamixel XM430-W350 OK!\r\n");
		term_printf("\n\r");
	} else {
		motor_status->state = MOTOR_STATE_ERROR;
		term_printf("Connection to the motor failed.\r\n");
		term_printf("\n\r");
	}
}

void motor_command(motor_command_t *motor_cmd, motor_status_t *motor_status) {

	if (motor_cmd == NULL || motor_status == NULL) {
		return;
	}
	if (motor_cmd->id != motor_status->id) {
		return;
	}
	dynamixel2_set_LED(motor_cmd->id, motor_cmd->LED_state);

	if (motor_status->state == MOTOR_SW_EMERGENCY_STOP) {
		motor_cmd->torque_enabled = 0;
		dynamixel2_set_torque_enable(motor_cmd->id, motor_cmd->torque_enabled);
		return;
	}

	if (motor_status->state == MOTOR_STATE_READY
			|| motor_status->state == MOTOR_STATE_RUNNING
			|| motor_status->state == MOTOR_GRIPPER_STATE) {

		if (motor_cmd->control_mode != motor_status->control_mode_st) {
			dynamixel2_set_torque_enable(motor_cmd->id, 0);
			dynamixel2_setOperatingMode(motor_cmd->id, motor_cmd->control_mode);
			motor_status->control_mode_st = motor_cmd->control_mode;
			dynamixel2_set_torque_enable(motor_cmd->id,
					motor_cmd->torque_enabled);
		} else {
			dynamixel2_set_torque_enable(motor_cmd->id,
					motor_cmd->torque_enabled);
		}

		if (motor_cmd->control_mode == POSITION_MODE) {
			dynamixel2_set_goal_position(motor_cmd->id,
					motor_cmd->target_position);

		}
#if With_GRIPPER == 0
		else if (motor_cmd->control_mode == VELOCITY_MODE) {
			dynamixel2_set_goal_velocity(motor_cmd->id,
					motor_cmd->target_velocity);
		} else if (motor_cmd->control_mode == CURRENT_MODE) {
			dynamixel2_set_goal_current(motor_cmd->id,
					motor_cmd->target_current);
		}
#endif

	}
}

void motor_status(motor_status_t *motor_status, motor_command_t *motor_cmd) {
	if (motor_cmd->emergency_stop == 1) {
		motor_status->state = MOTOR_SW_EMERGENCY_STOP;
	}

	else if (dynamixel2_ping(motor_status->id) == 0) {
		motor_status->state = MOTOR_STATE_ERROR;
		motor_status->present_position = 0;

		motor_status->present_velocity = 0;
		motor_status->present_current = 0;

		motor_status->present_temperature = 0;
		motor_status->control_mode_st = 0;

	}
	else if (motor_status->present_temperature > 70 || motor_status->Hardware_error_status !=0){
		motor_status->state = MOTOR_HW_EMERGENCY_STOP;
	}
	else {
		if (motor_cmd->torque_enabled == 0) {
			motor_status->state = MOTOR_STATE_OFF;

		} else if (motor_status->Max_pos_lim < 4095
				&& motor_status->Min_pos_lim > 0) {
			motor_status->state = MOTOR_GRIPPER_STATE;

		} else if (motor_status->Moving == 0) {
			motor_status->state = MOTOR_STATE_READY;
		} else {
			motor_status->state = MOTOR_STATE_RUNNING;
		}

		uint16_t address = 122;
		uint16_t size = 14;
		uint8_t return_data[size];
		uint16_t return_data_length;
		dynamixel2_read(motor_status->id, address, size, return_data,
				&return_data_length);

		motor_status->Moving = return_data[0];
		motor_status->present_current = return_data[4]
				+ ((return_data[5] << 8) & 0xFF00);
		motor_status->present_velocity = return_data[6]
				+ ((return_data[7] << 8) & 0xFF00)
				+ ((return_data[8] << 16) & 0xFF0000)
				+ ((return_data[9] << 24) & 0xFF000000);
		motor_status->present_position = return_data[10]
				+ ((return_data[11] << 8) & 0xFF00)
				+ ((return_data[12] << 16) & 0xFF0000)
				+ ((return_data[13] << 24) & 0xFF000000);

		motor_status->present_temperature = dynamixel2_read_present_temperature(
				motor_status->id);

		motor_status->control_mode_st = dynamixel2_getOperatingMode(
				motor_status->id);
		motor_status->Hardware_error_status = dynamixel2_hardware_error(
				motor_status->id);

	}

}

void motor_set_target_position(motor_command_t *motor_cmd, int32_t position) {
	motor_cmd->target_position = position;
}

void motor_set_control_mode(motor_command_t *motor_cmd, int8_t control) {
	motor_cmd->control_mode = control;
}

void motor_LED(motor_command_t *motor_cmd, uint8_t led_state) {
	motor_cmd->LED_state = led_state;
}

void motor_set_target_velocity(motor_command_t *motor_cmd, int32_t velocity) {
	motor_cmd->target_velocity = velocity;
}

void motor_enable_torque(motor_command_t *motor_cmd, uint8_t enable) {
	dynamixel2_set_torque_enable(motor_cmd->id, enable);
	motor_cmd->torque_enabled = enable;

}

int32_t motor_get_present_position(motor_status_t *motor_status) {
	return motor_status->present_position;
}

motor_state_t motor_get_state(motor_status_t *motor_status) {
	return motor_status->state;
}
