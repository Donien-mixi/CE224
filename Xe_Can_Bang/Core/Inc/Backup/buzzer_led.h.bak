/**
  ******************************************************************************
  * @file    buzzer_led.h
  * @brief   Điều khiển Còi chíp chủ động (PB12) và LED báo trạng thái (PC13)
  ******************************************************************************
  */

#ifndef __BUZZER_LED_H__
#define __BUZZER_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/* Kiểu mẫu nhấp nháy LED */
typedef enum {
    LED_PATTERN_OFF = 0,
    LED_PATTERN_ON,
    LED_PATTERN_SLOW_BLINK,    // Chờ dựng xe (Standby: 1Hz)
    LED_PATTERN_FAST_BLINK,    // Đang hiệu chuẩn IMU (5Hz)
    LED_PATTERN_HEARTBEAT,     // Đang cân bằng ổn định (Balancing: nháy đúp)
    LED_PATTERN_ALARM          // Ngã xe / Lỗi khẩn cấp (10Hz)
} LED_Pattern_t;

/* Khởi tạo */
void BuzzerLED_Init(void);

/* Điều khiển trực tiếp Buzzer (PB12) */
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Toggle(void);
void Buzzer_BeepAsync(uint16_t duration_ms);

/* Điều khiển trực tiếp LED (PC13 - Active LOW) */
void LED_On(void);
void LED_Off(void);
void LED_Toggle(void);
void LED_SetPattern(LED_Pattern_t pattern);

/* Hàm cập nhật không nghẽn (non-blocking) gọi trong while(1) hoặc SysTick */
void BuzzerLED_Update(uint32_t current_tick_ms);

#ifdef __cplusplus
}
#endif

#endif /* __BUZZER_LED_H__ */
