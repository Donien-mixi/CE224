/**
  ******************************************************************************
  * @file    encoder.h
  * @brief   Driver đọc phản hồi vị trí và vận tốc từ 2 Hall Encoder động cơ GA25.
  *          Sử dụng Timer 2 (Bánh Trái) và Timer 3 (Bánh Phải) ở Mode TI12 (x4).
  *          Độ phân giải: 1320 xung/vòng bánh xe, đường kính bánh xe 65mm.
  ******************************************************************************
  */

#ifndef __ENCODER_H__
#define __ENCODER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define ENCODER_PPR                 1320.0f         // 11 xung đĩa từ * tỷ số 30 * 4 cạnh xung
#define WHEEL_DIAMETER_M            0.065f          // Đường kính bánh xe cao su (65mm = 0.065m)
#define WHEEL_CIRCUMFERENCE_M       0.204203522f    // Chu vi bánh: PI * D = 3.14159265 * 0.065
#define METERS_PER_TICK             (WHEEL_CIRCUMFERENCE_M / ENCODER_PPR) // ~0.0001547m/xung
#define ENCODER_LPF_BETA            0.75f           // Hệ số lọc thông thấp vận tốc

/* Structs -------------------------------------------------------------------*/
typedef struct {
    int32_t total_ticks;        // Tổng số xung tích lũy (quãng đường)
    int16_t delta_ticks;        // Số xung đếm được trong chu kỳ vừa qua
    float   velocity_raw;       // Vận tốc tức thời (m/s)
    float   velocity_filtered;  // Vận tốc sau khi qua bộ lọc thông thấp (m/s)
} Encoder_Data_t;

/* Function Prototypes -------------------------------------------------------*/

/**
  * @brief  Khởi động chế độ Encoder x4 trên Timer 2 và Timer 3
  */
void Encoder_Init(void);

/**
  * @brief  Cập nhật số xung, tính toán và lọc vận tốc mỗi chu kỳ ngắt
  * @param  dt: Chu kỳ lấy mẫu tính bằng giây (ví dụ: 0.005s cho chu kỳ 200Hz)
  */
void Encoder_Update(float dt);

/**
  * @brief  Lấy vận tốc lọc của bánh trái (m/s)
  */
float Encoder_GetLeftVelocity(void);

/**
  * @brief  Lấy vận tốc lọc của bánh phải (m/s)
  */
float Encoder_GetRightVelocity(void);

/**
  * @brief  Lấy vận tốc tịnh tiến trung bình của robot (m/s)
  */
float Encoder_GetAverageVelocity(void);

/**
  * @brief  Reset biến đếm và thanh ghi Encoder về 0
  */
void Encoder_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* __ENCODER_H__ */
