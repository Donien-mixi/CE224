/**
  ******************************************************************************
  * @file    motor.h
  * @brief   Driver điều khiển 2 Động cơ GA25-370 qua Mạch công suất Dual A4950.
  *          Sử dụng Timer 1 phát xung PWM 20kHz đối xứng tâm (Center-aligned),
  *          vận hành ở chế độ Slow Decay giúp mô-men xoắn tuyến tính và êm ái.
  ******************************************************************************
  */

#ifndef __MOTOR_H__
#define __MOTOR_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include <stdint.h>

/* Defines -------------------------------------------------------------------*/
#define MOTOR_MAX_PWM       2499    // Giá trị ARR cực đại của Timer 1 (100% công suất)
#define MOTOR_DEADBAND      250     // Giá trị bù ma sát tĩnh hộp số GA25 (~10% PWM)

/* Function Prototypes -------------------------------------------------------*/

/**
  * @brief  Khởi động 4 kênh PWM Timer 1 điều khiển Driver Dual A4950
  */
void Motor_Init(void);

/**
  * @brief  Cài đặt tốc độ và chiều quay cho 2 động cơ (Chế độ Slow Decay)
  * @param  pwm_left:  Giá trị PWM bánh trái (-2499 đến +2499)
  *                    > 0: Quay tiến (IN1 = PWM, IN2 = GND)
  *                    < 0: Quay lùi  (IN1 = GND, IN2 = PWM)
  *                    = 0: Phanh nhẹ (IN1 = GND, IN2 = GND)
  * @param  pwm_right: Giá trị PWM bánh phải (-2499 đến +2499)
  *                    > 0: Quay tiến (IN3 = PWM, IN4 = GND)
  *                    < 0: Quay lùi  (IN3 = GND, IN4 = PWM)
  *                    = 0: Phanh nhẹ (IN3 = GND, IN4 = GND)
  */
void Motor_SetDuty(int16_t pwm_left, int16_t pwm_right);

/**
  * @brief  Dừng khẩn cấp cả 2 động cơ (Cắt xung PWM về 0)
  */
void Motor_Stop(void);

/**
  * @brief  Hãm phanh cứng cả 2 động cơ (Kéo cả 2 chân IN lên mức 1)
  */
void Motor_Brake(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_H__ */
