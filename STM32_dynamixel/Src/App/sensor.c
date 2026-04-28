/*
 * sensor.c
 */
#include "main.h"
volatile uint8_t emergency_state = 0;
uint8_t button_update(void)
{
    static uint32_t press_tick = 0;
    static uint8_t  btn_prev   = GPIO_PIN_SET;

    GPIO_PinState btn_now = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

    if (btn_prev == GPIO_PIN_SET && btn_now == GPIO_PIN_RESET) {
        press_tick = HAL_GetTick();
    }

    if (btn_prev == GPIO_PIN_RESET && btn_now == GPIO_PIN_SET) {
        if ((HAL_GetTick() - press_tick) >= 50) {
            emergency_state ^= 1;
        }
    }

    btn_prev = btn_now;
    return emergency_state;
}
