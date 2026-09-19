/**
  ******************************************************************************
  * @file    buzzer_led.c
  * @brief   Hiện thực điều khiển Còi chíp (PB12) và LED PC13 phi phong bế
  ******************************************************************************
  */

#include "buzzer_led.h"

static LED_Pattern_t s_led_pattern = LED_PATTERN_OFF;
static uint32_t s_last_led_toggle = 0;
static uint32_t s_buzzer_off_time = 0;
static uint8_t  s_buzzer_active = 0;
static uint8_t  s_heartbeat_step = 0;

void BuzzerLED_Init(void)
{
    /* Đảm bảo tắt còi và tắt LED khi khởi động */
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET); // High = OFF (Active LOW)
    s_led_pattern = LED_PATTERN_FAST_BLINK;
    s_buzzer_active = 0;
}

void Buzzer_On(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
}

void Buzzer_Off(void)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
    s_buzzer_active = 0;
}

void Buzzer_Toggle(void)
{
    HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
}

void Buzzer_BeepAsync(uint16_t duration_ms)
{
    Buzzer_On();
    s_buzzer_off_time = HAL_GetTick() + duration_ms;
    s_buzzer_active = 1;
}

void LED_On(void)
{
    /* PC13 Black Pill là Active LOW: Kéo xuống 0V là sáng */
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_RESET);
}

void LED_Off(void)
{
    /* Kéo lên 3.3V là tắt */
    HAL_GPIO_WritePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin, GPIO_PIN_SET);
}

void LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
}

void LED_SetPattern(LED_Pattern_t pattern)
{
    s_led_pattern = pattern;
    if (pattern == LED_PATTERN_OFF) {
        LED_Off();
    } else if (pattern == LED_PATTERN_ON) {
        LED_On();
    }
}

void BuzzerLED_Update(uint32_t current_tick_ms)
{
    /* 1. Xử lý tắt còi khi hết thời gian beep phi phong bế */
    if (s_buzzer_active && (current_tick_ms >= s_buzzer_off_time)) {
        Buzzer_Off();
    }

    /* 2. Xử lý mẫu nhấp nháy LED */
    switch (s_led_pattern) {
        case LED_PATTERN_SLOW_BLINK:
            if (current_tick_ms - s_last_led_toggle >= 500) { // 1Hz
                LED_Toggle();
                s_last_led_toggle = current_tick_ms;
            }
            break;

        case LED_PATTERN_FAST_BLINK:
            if (current_tick_ms - s_last_led_toggle >= 100) { // 5Hz
                LED_Toggle();
                s_last_led_toggle = current_tick_ms;
            }
            break;

        case LED_PATTERN_ALARM:
            if (current_tick_ms - s_last_led_toggle >= 50) { // 10Hz
                LED_Toggle();
                s_last_led_toggle = current_tick_ms;
            }
            break;

        case LED_PATTERN_HEARTBEAT:
            // Nháy đúp: ON 50ms, OFF 100ms, ON 50ms, OFF 800ms
            if (s_heartbeat_step == 0 && (current_tick_ms - s_last_led_toggle >= 800)) {
                LED_On();
                s_last_led_toggle = current_tick_ms;
                s_heartbeat_step = 1;
            } else if (s_heartbeat_step == 1 && (current_tick_ms - s_last_led_toggle >= 50)) {
                LED_Off();
                s_last_led_toggle = current_tick_ms;
                s_heartbeat_step = 2;
            } else if (s_heartbeat_step == 2 && (current_tick_ms - s_last_led_toggle >= 100)) {
                LED_On();
                s_last_led_toggle = current_tick_ms;
                s_heartbeat_step = 3;
            } else if (s_heartbeat_step == 3 && (current_tick_ms - s_last_led_toggle >= 50)) {
                LED_Off();
                s_last_led_toggle = current_tick_ms;
                s_heartbeat_step = 0;
            }
            break;

        case LED_PATTERN_OFF:
        case LED_PATTERN_ON:
        default:
            break;
    }
}
