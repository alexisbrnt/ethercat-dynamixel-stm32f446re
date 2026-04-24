/**
 * @file   dynamixel_2_0.h
 * @author ZiTe (honmonoh@gmail.com)
 * @brief  Robotis DYNAMIXEL Protocol 2.0 header file.
 * @remark Ref: https://emanual.robotis.com/docs/en/dxl/protocol2/
 */

#ifndef __DYNAMIXEL_2_0_H
#define __DYNAMIXEL_2_0_H


#include "main.h"
#include <stdbool.h>

#define MAX485_DE_RE_GPIO_PORT GPIOB
#define MAX485_DE_RE_GPIO_PIN GPIO_PIN_3

#define BUFFER_LENGTH 128
#define BUFFER buffer
#define BUFFER_INDEX buffer_index

#define DYNAMIXEL2_BROADCAST_ID ((uint8_t)0xFE)
extern UART_HandleTypeDef huart1;

#define VELOCITY_MODE 	1
#define POSITION_MODE	3
#define CURRENT_MODE	0
#define EXT_POS_MODE	4
#define PWM_MODE		16


typedef enum
{
  dxl_ping = 0x01,
  dxl_read = 0x02,
  dxl_write = 0x03,

  dxl_reg_write = 0x04,
  dxl_action = 0x05,

  dxl_factory_reset = 0x06,
  dxl_reboot = 0x08,
  dxl_clear = 0x10,
  dxl_control_table_backup = 0x20,

  dxl_sync_read = 0x82,
  dxl_sync_write = 0x83,
  dxl_fast_sync_read = 0x8A,

  dxl_bulk_read = 0x92,
  dxl_bulk_write = 0x93,
  dxl_fast_bulk_read = 0x9A
} dynamixel2_instruction_t;


void max485_send(uint8_t *data, uint32_t length);
void dynamixel2_write(uint8_t id, uint16_t address, uint8_t *data,
		uint16_t data_length);
bool dynamixel2_read(uint8_t id, uint16_t address, uint16_t data_length,
		uint8_t *return_data, uint16_t *return_data_length);
bool dynamixel2_ping(uint8_t id) ;
void dynamixel2_reset(uint8_t id);
void dynamixel2_set_torque_enable(uint8_t id, uint8_t enable);
uint8_t dynamixel2_get_torque_status(uint8_t id);
void dynamixel2_set_LED(uint8_t id, uint8_t enable);
void dynamixel2_change_id(int id, uint8_t val);
void dynamixel2_setOperatingMode(uint8_t id, uint8_t operating_mode);
uint8_t dynamixel2_getOperatingMode(uint8_t id);
void dynamixel2_set_goal_position(uint8_t id, int32_t position);
int32_t dynamixel2_read_present_position(uint8_t id);
void dynamixel2_set_goal_velocity(uint8_t id, int32_t velocity);
int32_t dynamixel2_read_present_velocity(uint8_t id) ;
int16_t dynamixel2_read_present_current(uint8_t id) ;
uint8_t dynamixel2_ismoving(uint8_t id);
int16_t dynamixel2_read_present_temperature(uint8_t id) ;
uint8_t dynamixel2_get_BaudRate(uint8_t id);
void dynamixel2_setBaudrate(uint8_t id, uint8_t baudrate);
uint8_t dynamixel2_getFirmwareVersion(uint8_t id);
uint8_t dynamixel2_getModelNumber(uint8_t id);
uint8_t dynamixel2_getTemplimit(uint8_t id);
void dynamixel2_setTempLimit(uint8_t id, uint8_t temp_lim);
int16_t dynamixel2_getCurrentLimit(uint8_t id);
void dynamixel2_setCurrentLimit(uint8_t id, int16_t current_lim);
void dynamixel2_set_goal_current(uint8_t id, int16_t current);
int32_t dynamixel2_getMaxPositionLimit(uint8_t id);
void dynamixel2_setMaxPositionLimit(uint8_t id, int32_t max_position_lim);
int32_t dynamixel2_getMinPositionLimit(uint8_t id);
void dynamixel2_setMinPositionLimit(uint8_t id, int32_t min_position_lim);
int32_t dynamixel2_getVelocityLimit(uint8_t id);
void dynamixel2_setVelocityLimit(uint8_t id, int32_t velocity_lim);
uint8_t dynamixel2_hardware_error(uint8_t id);
void dynamixel2_send_packet(uint8_t id, dynamixel2_instruction_t inst,
		uint8_t *params, uint16_t params_length);
void dynamixel2_clear_receive_buffer(void);
bool dynamixel2_parse_status_packet(uint8_t *packet, uint32_t packet_length,
		uint8_t *id, uint8_t *params, uint16_t *params_length, uint8_t *error,
		bool *crc_check);
bool dynamixel2_get_status_packet(uint8_t *packet, uint16_t *packet_length);
uint16_t update_crc(uint16_t crc_accum, uint8_t *data_blk_ptr,
		uint16_t data_blk_size);



#endif /* __DYNAMIXEL_2_0_H */
