/*
 * motor_app.c
 */
#include "motor_app.h"
#include "main.h"
#include "dynamixel_2_0.h"
#include "util.h"
#include "drv_uart.h"

motor_command_t motor_cmd;
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
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setOperatingMode(id, motor_cmd->control_mode);
		dynamixel2_set_torque_enable(id, 1);
		dynamixel2_set_goal_position(id, motor_cmd->target_position);
		motor_status->control_mode_st = dynamixel2_getOperatingMode(id);
		motor_status->baudrate = dynamixel2_get_BaudRate(id);
		motor_cmd->torque_enabled = true;

		motor_status->state = MOTOR_STATE_READY;
		//dynamixel2_set_LED(id, motor_cmd->LED_state);
		dynamixel2_set_torque_enable(id, motor_cmd->torque_enabled);
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

	if (motor_status->state == MOTOR_STATE_READY
			|| motor_status->state == MOTOR_STATE_RUNNING) {

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

		} else if (motor_cmd->control_mode == VELOCITY_MODE) {
			dynamixel2_set_goal_velocity(motor_cmd->id,
					motor_cmd->target_velocity);
		}

	}
}

void motor_status(motor_status_t *motor_status, motor_command_t *motor_cmd) {
	if (dynamixel2_ping(motor_status->id) == 0) {
		motor_status->state = MOTOR_STATE_ERROR;
		motor_status->present_position = 0;

		motor_status->present_velocity = 0;
		motor_status->present_current = 0;

		motor_status->present_temperature = 0;
		motor_status->control_mode_st = 0;

	} else {
		if (motor_cmd->torque_enabled == 0) {
			motor_status->state = MOTOR_STATE_OFF;

		} else if (motor_status->present_velocity < 10) {
			motor_status->state = MOTOR_STATE_READY;
		} else {
			motor_status->state = MOTOR_STATE_RUNNING;
		}
		motor_status->present_position = dynamixel2_read_present_position(
				motor_status->id);

		motor_status->present_velocity = dynamixel2_read_present_velocity(
				motor_status->id);

		motor_status->present_current = dynamixel2_read_present_current(
				motor_status->id);

		motor_status->present_temperature = dynamixel2_read_present_temperature(
				motor_status->id);

		motor_status->control_mode_st = dynamixel2_getOperatingMode(
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
