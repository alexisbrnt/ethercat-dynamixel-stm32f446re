/*
 * motor_app.h
 */

#ifndef SRC_APP_MOTOR_APP_H
#define SRC_APP_MOTOR_APP_H
#include "main.h"
#include "dynamixel_2_0.h"
#include "util.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum
{
	MOTOR_STATE_IDLE = 0,
	MOTOR_STATE_INIT,
	MOTOR_STATE_READY,
	MOTOR_STATE_RUNNING,
	MOTOR_STATE_ERROR,
	MOTOR_STATE_OFF,
	MOTOR_SW_EMERGENCY_STOP,
	MOTOR_GRIPPER_STATE,
	MOTOR_HW_EMERGENCY_STOP,
} motor_state_t;

typedef struct{
	uint8_t id;
	int32_t target_position;
	int32_t target_velocity;
	int16_t target_current;
	uint8_t control_mode;
	uint8_t torque_enabled;
	uint8_t LED_state;
	uint8_t reboot;
	uint8_t emergency_stop;
} motor_command_t;

typedef struct{
	uint8_t id;
	int32_t present_position;
	int32_t present_velocity;
	int16_t present_current;
	int16_t present_temperature;
	motor_state_t state;
	uint8_t control_mode_st;
	uint8_t baudrate;
	int32_t Max_pos_lim;
	int32_t Min_pos_lim;
	int32_t Velocity_lim;
	int16_t Current_lim;
	uint8_t Hardware_error_status;
	uint8_t Moving;
	uint8_t torque_status;

} motor_status_t;



void motor_init(uint8_t id, motor_command_t *motor_cmd, motor_status_t *motor_status);
void motor_command(motor_command_t *motor_cmd, motor_status_t *motor_status);
void motor_status(motor_status_t *motor_status,motor_command_t *motor_cmd);

void motor_LED(motor_command_t *motor_cmd, uint8_t led_state);
void motor_set_target_position(motor_command_t *motor_cmd, int32_t position);
void motor_set_control_mode(motor_command_t *motor_cmd, int8_t control);
void motor_set_target_velocity(motor_command_t *motor_cmd, int32_t velocity);
void motor_enable_torque(motor_command_t *motor_cmd, uint8_t enable);

int32_t motor_get_present_position(motor_status_t *motor_status);
motor_state_t motor_get_state(motor_status_t* motor_status);
#endif /* SRC_APP_MOTOR_APP_H */
