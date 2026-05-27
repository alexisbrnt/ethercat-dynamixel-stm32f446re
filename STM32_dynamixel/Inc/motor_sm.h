#ifndef MOTOR_SM_H
#define MOTOR_SM_H

#include "motor_app.h"
typedef struct motor_context motor_context_t;
typedef struct motor_sm_state motor_sm_state_t;

struct motor_sm_state{
	void	(*enter)(motor_context_t *ctx);
	void	(*update)(motor_context_t *ctx);
	void	(*exit)(motor_context_t *ctx);
	const char *name;
};

struct motor_context{
	motor_command_t			*cmd;
	motor_status_t			*status;
	const motor_sm_state_t	*current_state;
	uint8_t					ping_error_count;
	uint32_t				last_reconnect_ms;
	uint8_t					read_cycle;
	uint8_t					post_comm_loss;
	uint8_t					blink_counter;
	uint8_t 				blink_led_state;
};


uint8_t motor_master_timed_out(void);
void motor_sm_init(motor_context_t *ctx, motor_command_t *cmd, motor_status_t *status,uint8_t motor_id);
void motor_sm_update(motor_context_t *ctx);
void motor_transition_to(motor_context_t *ctx, const motor_sm_state_t *new_state);
extern const motor_sm_state_t state_init;
extern const motor_sm_state_t state_operational;
extern const motor_sm_state_t state_off;
extern const motor_sm_state_t state_error;
extern const motor_sm_state_t state_sw_emergency;
extern const motor_sm_state_t state_hw_emergency;
extern const motor_sm_state_t state_comm_loss;

#if With_GRIPPER
extern const motor_sm_state_t state_gripper;
#endif

#endif /*MOTOR_SM_H*/

