#include "motor_sm.h"
#include "motor_app.h"
#include "dynamixel_2_0.h"
#include "util.h"
#include "main.h"

#define Init_MAX_limit		4095
#define Init_MIN_limit		0
#define HW_EMRG_CLEAR_TEMP	60u
#define RECONNECT_PERIOD_MS 500u

#define With_GRIPPER		0

void motor_transition_to(motor_context_t *ctx,
		const motor_sm_state_t *new_state) {
	if (ctx->current_state && ctx->current_state->exit) {
		ctx->current_state->exit(ctx);
	}
	term_printf("[SM] %s -> %s\r\n",
			ctx->current_state ? ctx->current_state->name : "none",
			new_state->name);

	ctx->current_state = new_state;

	if (ctx->current_state->enter) {
		ctx->current_state->enter(ctx);
	}
}

void motor_sm_update(motor_context_t *ctx) {
	if (ctx->current_state && ctx->current_state->update)
		ctx->current_state->update(ctx);
}

static void read_motor_sensors(motor_context_t *ctx) {
	uint8_t id = ctx->status->id;

	uint16_t address = 122;
	uint16_t size = 14;
	uint8_t return_data[size];
	uint16_t return_data_length;
	dynamixel2_read(id, address, size, return_data, &return_data_length);

	ctx->status->Moving = return_data[0];
	ctx->status->present_current = return_data[4]
			+ ((return_data[5] << 8) & 0xFF00);
	ctx->status->present_velocity = return_data[6]
			+ ((return_data[7] << 8) & 0xFF00)
			+ ((return_data[8] << 16) & 0xFF0000)
			+ ((return_data[9] << 24) & 0xFF000000);
	ctx->status->present_position = return_data[10]
			+ ((return_data[11] << 8) & 0xFF00)
			+ ((return_data[12] << 16) & 0xFF0000)
			+ ((return_data[13] << 24) & 0xFF000000);

	switch (ctx->read_cycle) {
	case 0:
		ctx->status->present_temperature = dynamixel2_read_present_temperature(
				id);
		break;
	case 1:
		ctx->status->control_mode_st = dynamixel2_getOperatingMode(id);
		break;
	case 2:
		ctx->status->Hardware_error_status = dynamixel2_hardware_error(id);
		break;
	}
	ctx->read_cycle = (ctx->read_cycle + 1) % 3;
}

static void apply_motor_command(motor_context_t *ctx) {
	uint8_t id = ctx->cmd->id;
	dynamixel2_set_LED(id, ctx->cmd->LED_state);

	if (ctx->cmd->control_mode != ctx->status->control_mode_st) {
		dynamixel2_set_torque_enable(id, 0);
		dynamixel2_setOperatingMode(id, ctx->cmd->control_mode);
		ctx->status->control_mode_st = ctx->cmd->control_mode;
		dynamixel2_set_torque_enable(id, ctx->cmd->torque_enabled);
	} else {
		dynamixel2_set_torque_enable(id, ctx->cmd->torque_enabled);
	}
	switch (ctx->cmd->control_mode) {
	case POSITION_MODE:
		dynamixel2_set_goal_position(id, ctx->cmd->target_position);
		break;
	case VELOCITY_MODE:
		dynamixel2_set_goal_velocity(id, ctx->cmd->target_velocity);
		break;
	case CURRENT_MODE:
		dynamixel2_set_goal_current(id, ctx->cmd->target_current);
		break;
	default:
		break;
	}
}

static uint8_t check_safety(motor_context_t *ctx) {
	if (ctx->cmd->emergency_stop) {
		motor_transition_to(ctx, &state_sw_emergency);
		return 1;
	}

	if (motor_master_timed_out()) {
		motor_transition_to(ctx, &state_comm_loss);
		return 1;
	}

	if (ctx->status->present_temperature > 70
			|| ctx->status->Hardware_error_status != 0) {
		motor_transition_to(ctx, &state_hw_emergency);
		return 1;
	}

	return 0;
}

static void state_init_enter(motor_context_t *ctx) {
	uint8_t id = ctx->cmd->id;
	ctx->status->id = id;
	ctx->status->state = MOTOR_STATE_INIT;

	ctx->cmd->LED_state = 0;
	ctx->cmd->target_position = 0;
	ctx->cmd->target_velocity = 0;
	ctx->cmd->target_current = 0;
	ctx->cmd->control_mode = POSITION_MODE;
	ctx->cmd->torque_enabled = 0;
	ctx->status->present_position = 0;
	ctx->status->present_velocity = 0;
	ctx->status->present_current = 0;
	ctx->status->present_temperature = 0;
	ctx->ping_error_count = 0;

	term_printf("\r\n[INIT-001] Dynamixel XM430-W350 init...\r\n");
	if (!dynamixel2_ping(id)) {
		term_printf("[INIT-001] Connection to motor failed.\r\n\r\n");
		motor_transition_to(ctx, &state_error);
		return;
	}
#if With_GRIPPER
	dynamixel2_set_torque_enable(id, 0);
	dynamixel2_setOperatingMode(id,VELOCITY_MODE);
	dynamixel2_set_torque_enable(id, 1);
	dynamixel2_set_goal_velocity(id, 0);

	do{dynamixel2_set_goal_velocity(id, 50);}
	while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)!=GPIO_PIN_RESET);
	dynamixel2_set_goal_velocity(id, 0);
	HAL_Delay(500);

	ctx->status->Max_pos_lim = (((dynamixel2_read_present_position(id)%4096)+4096)%4096)-20;
	dynamixel2_set_torque_enable(id, 0);
	dynamixel2_setMaxPositionLimit(id, ctx->status->Max_pos_lim);
	term_printf("[INIT-002] First limit saved\r\n");
	dynamixel2_set_torque_enable(id, 1);
	HAL_Delay(100);

	do{dynamixel2_set_goal_velocity(id, -50);}
	while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1)!=GPIO_PIN_RESET);
	dynamixel2_set_goal_velocity(id, 0);
	HAL_Delay(100);

	ctx->status->Min_pos_lim = (((dynamixel2_read_present_position(id)%4096)+4096)%4096)+20;
	dynamixel2_set_torque_enable(id, 0);
	dynamixel2_setMinPositionLimit(id, ctx->status->Min_pos_lim);
	term_printf("[INIT-002] Second limit saved!\r\n");

	dynamixel2_setOperatingMode(id, ctx->cmd->control_mode);
	dynamixel2_set_torque_enable(id, 1);
	int32_t mid = (ctx->status->Max_pos_lim + ctx->status->Min_pos_lim)/2;
	dynamixel2_set_goal_position(id, mid);
	HAL_Delay(500);

	ctx->cmd->torque_enabled = 0;
	dynamixel2_set_torque_enable(id, 0);
#else
	ctx->status->Max_pos_lim = Init_MAX_limit;
	ctx->status->Min_pos_lim = Init_MIN_limit;

	dynamixel2_set_torque_enable(id, 0);
	dynamixel2_setMaxPositionLimit(id, Init_MAX_limit);
	dynamixel2_setMinPositionLimit(id, Init_MIN_limit);
	dynamixel2_setOperatingMode(id, ctx->cmd->control_mode);
	dynamixel2_set_torque_enable(id, 1);
	dynamixel2_set_goal_position(id, ctx->cmd->target_position);
	term_printf("[INIT-002] Software limits saved! \r\n");
#endif
	ctx->status->Current_lim = dynamixel2_getCurrentLimit(id);
	ctx->status->Velocity_lim = dynamixel2_getVelocityLimit(id);
	ctx->status->control_mode_st = dynamixel2_getOperatingMode(id);
	ctx->status->baudrate = dynamixel2_get_BaudRate(id);
	if (ctx->post_comm_loss) {
		ctx->cmd->torque_enabled = 0;
		dynamixel2_set_torque_enable(id, 0);
	} else {
		ctx->cmd->torque_enabled = 1;
	}

	term_printf("[INIT-001] Dynamixel XM430-W350 OK!\r\n\r\n");

#if With_GRIPPER
	motor_transition_to(ctx, &state_gripper);
#else
	motor_transition_to(ctx, &state_operational);
#endif
}

const motor_sm_state_t state_init = { .enter = state_init_enter, .update = NULL,
		.exit = NULL, .name = "INIT", };

static void state_operational_update(motor_context_t *ctx) {
	if (check_safety(ctx))
		return;

	if (ctx->post_comm_loss) {
		dynamixel2_set_torque_enable(ctx->cmd->id, 0);
		ctx->status->state = MOTOR_STATE_READY;
		if (!ctx->cmd->torque_enabled) {
			ctx->post_comm_loss = 0;
			term_printf("[SAF-003] Safe reset confirmed by master.\r\n");
		}
		return;
	}

	if (!dynamixel2_ping(ctx->status->id)) {
		if (++ctx->ping_error_count >= 3) {
			ctx->ping_error_count = 0;
			motor_transition_to(ctx, &state_error);
		}
		return;
	}
	ctx->ping_error_count = 0;

	if (!ctx->cmd->torque_enabled) {
		motor_transition_to(ctx, &state_off);
		return;
	}

	read_motor_sensors(ctx);
	apply_motor_command(ctx);

	ctx->status->state =
			ctx->status->Moving ? MOTOR_STATE_RUNNING : MOTOR_STATE_READY;
}

const motor_sm_state_t state_operational = { .enter = NULL, .update =
		state_operational_update, .exit = NULL, .name = "OPERATIONAL" };

static void state_off_enter(motor_context_t *ctx) {
	ctx->status->state = MOTOR_STATE_OFF;
	dynamixel2_set_torque_enable(ctx->cmd->id, 0);
}

static void state_off_update(motor_context_t *ctx) {
	if (ctx->cmd->emergency_stop) {
		motor_transition_to(ctx, &state_sw_emergency);
		return;
	}

	if (ctx->cmd->torque_enabled) {
		motor_transition_to(ctx, &state_operational);
	}
}

const motor_sm_state_t state_off = { .enter = state_off_enter, .update =
		state_off_update, .exit = NULL, .name = "OFF", };

static void state_error_enter(motor_context_t *ctx) {
	ctx->status->state = MOTOR_STATE_ERROR;
	ctx->status->present_position = 0;
	ctx->status->present_velocity = 0;
	ctx->status->present_current = 0;
	ctx->status->present_temperature = 0;
	ctx->status->control_mode_st = 0;
	ctx->last_reconnect_ms = 0;
	//RELAY_Off();
}

static void state_error_update(motor_context_t *ctx) {
	uint32_t now = HAL_GetTick();
	if ((now - ctx->last_reconnect_ms) < RECONNECT_PERIOD_MS)
		return;
	ctx->last_reconnect_ms = now;
	term_printf("[SAF-004] Attempting motor reconnection...\r\n");

	//RELAY_On();
	if (dynamixel2_ping(ctx->status->id)) {
		term_printf("[SAF-004] Motor found ! Reinitializing...\r\n");
		motor_transition_to(ctx, &state_init);
	} else {
		term_printf("[SAF-004] Motor not responding\r\n");
	}
}

const motor_sm_state_t state_error = { .enter = state_error_enter, .update =
		state_error_update, .exit = NULL, .name = "ERROR", };

static void state_sw_emergency_enter(motor_context_t *ctx) {
	ctx->status->state = MOTOR_SW_EMERGENCY_STOP;
	ctx->cmd->torque_enabled = 0;
	dynamixel2_set_torque_enable(ctx->cmd->id, 0);
	ctx->post_comm_loss = 1;
	term_printf("[SAF-002] MAster initiated Emergency STOP\r\n");
}

static void state_sw_emergency_update(motor_context_t *ctx) {
	if (!ctx->cmd->emergency_stop) {
		//dynamixel2_reboot(ctx->status->id);
		term_printf("[SAF-007] Motor reboot after emergency\r\n");
		//HAL_Delay(500);
		motor_transition_to(ctx, &state_operational);
	}
}

const motor_sm_state_t state_sw_emergency = { .enter = state_sw_emergency_enter,
		.update = state_sw_emergency_update, .exit = NULL, .name =
				"SW_EMERGENCY", };

static void state_hw_emergency_enter(motor_context_t *ctx) {
	ctx->status->state = MOTOR_HW_EMERGENCY_STOP;
	ctx->cmd->torque_enabled = 0;
	dynamixel2_set_torque_enable(ctx->cmd->id, 0);
	term_printf("[SAF-001] Autonomous Emergency Stop\r\n");
	term_printf("	temperature	 : %u\r\n", ctx->status->present_temperature);
	term_printf("	hw error	: %u\r\n", ctx->status->Hardware_error_status);
}

static void state_hw_emergency_update(motor_context_t *ctx) {
	ctx->status->present_temperature = dynamixel2_read_present_temperature(
			ctx->status->id);
	ctx->status->Hardware_error_status = dynamixel2_hardware_error(
			ctx->status->id);

	if (ctx->status->present_temperature < HW_EMRG_CLEAR_TEMP
			&& ctx->status->Hardware_error_status == 0) {
		term_printf("[SAF-001] HW emergency cleared, reinitializating?\r\n");
		dynamixel2_reboot(ctx->status->id);
		HAL_Delay(500);
		motor_transition_to(ctx, &state_init);
	}
}

const motor_sm_state_t state_hw_emergency = { .enter = state_hw_emergency_enter,
		.update = state_hw_emergency_update, .exit = NULL, .name =
				"HW_EMERGENCY", };

static void state_comm_loss_enter(motor_context_t *ctx) {
	ctx->status->state = MOTOR_COMM_LOSS;
	ctx->cmd->torque_enabled = 0;
	ctx->cmd->target_velocity = 0;
	ctx->cmd->target_current = 0;
	dynamixel2_set_torque_enable(ctx->cmd->id, 0);
	ctx->post_comm_loss = 1;
	term_printf("[SAF-003] Master timeout! Motor disabled.\r\n");
}

static void state_comm_loss_update(motor_context_t *ctx) {
	if (!motor_master_timed_out()) {
		term_printf("[SAF-003] Master reconnected.\r\n");
		motor_transition_to(ctx, &state_operational);
	}
}

const motor_sm_state_t state_comm_loss = { .enter = state_comm_loss_enter,
		.update = state_comm_loss_update, .exit = NULL, .name = "COMM_LOSS", };

#if With_GRIPPER
static void state_gripper_enter(motor_context_t *ctx){
	ctx->status->state = MOTOR_GRIPPER_STATE;
}
static void state_gripper_update(motor_context_t *ctx){
	if(check_safety(ctx))return;
	read_motor_sensors(ctx);
	apply_motor_command(ctx);
	ctx->status->state = MOTOR_GRIPPER_STATE;
}

const motor_sm_state_t state_gripper = {
	.enter = state_gripper_enter,
	.update = state_gripper_update,
	.exit = NULL,
	.name = "GRIPPER",
};
#endif

void motor_sm_init(motor_context_t *ctx, motor_command_t *cmd,
		motor_status_t *status, uint8_t motor_id) {
	ctx->cmd = cmd;
	ctx->status = status;
	ctx->current_state = NULL;
	ctx->read_cycle = 0;
	ctx->ping_error_count = 0;
	ctx->last_reconnect_ms = 0;
	ctx->post_comm_loss = 0;
	ctx->cmd->id = motor_id;
	ctx->status->id = motor_id;

	motor_transition_to(ctx, &state_init);
}

