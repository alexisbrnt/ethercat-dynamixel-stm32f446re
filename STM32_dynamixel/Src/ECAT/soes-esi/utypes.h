#ifndef __UTYPES_H__
#define __UTYPES_H__

#include "cc.h"

/* Object dictionary storage */

typedef struct
{
   /* Identity */

   uint32_t serial;

   /* Inputs */

   uint8_t ID_TX;
   uint8_t state;
   int32_t present_position;
   int32_t present_velocity;
   int16_t present_current;
   int16_t present_temperature;
   uint8_t baudrate;
   uint8_t operating_mode;

   /* Outputs */

   uint8_t ID_RX;
   uint8_t control_mode;
   uint8_t torque_enabled;
   uint8_t LED_STATE;
   int32_t goal_position;
   int32_t target_velocity;

} _Objects;

extern _Objects Obj;

#endif /* __UTYPES_H__ */
