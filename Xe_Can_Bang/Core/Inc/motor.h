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
#define MOTOR_MAX_PWM 2499 // Giá trị ARR cực đại của Timer 1 (100% công suất)
/* Ngưỡng bứt phá ma sát tĩnh ĐO THỰC TẾ (Motor_Find_Deadband) ở chế độ Slow Decay:
 *   Bánh TRÁI ~240 PWM, Bánh PHẢI ~240 PWM. Bù riêng từng bánh để tránh lệch hướng. */
#define MOTOR_DEADBAND_L 240 // Bù vùng chết bánh TRÁI (đo thực tế Slow Decay)
#define MOTOR_DEADBAND_R 240 // Bù vùng chết bánh PHẢI (đo thực tế Slow Decay)

/* Function Prototypes -------------------------------------------------------*/

/**
 * @brief  Khởi động 4 kênh PWM Timer 1 điều khiển Driver Dual A4950
 */
void Motor_Init(void);

/**
 * @brief  Cài đặt tốc độ và chiều quay cho 2 động cơ (Chế độ Slow Decay)
 * @param  pwm_left:  Giá trị PWM bánh trái (-2499 đến +2499)
 *                    > 0: Quay tiến (IN1 = 1, IN2 = PWM đảo)
 *                    < 0: Quay lùi  (IN1 = PWM đảo, IN2 = 1)
 *                    = 0: Phanh nhẹ (IN1 = 1, IN2 = 1)
 * @param  pwm_right: Giá trị PWM bánh phải (-2499 đến +2499)
 *                    > 0: Quay tiến (IN3 = 1, IN4 = PWM đảo)
 *                    < 0: Quay lùi  (IN3 = PWM đảo, IN4 = 1)
 *                    = 0: Phanh nhẹ (IN3 = 1, IN4 = 1)
 */
void Motor_SetDuty(int16_t pwm_left, int16_t pwm_right);

/**
 * @brief  Dừng khẩn cấp cả 2 động cơ (Cắt xung PWM về 0)
 */
void Motor_Stop(void);
void Motor_Coast(void); /* Encoder hand-turn only; both bridge inputs LOW. */

/**
 * @brief  Hãm phanh cứng cả 2 động cơ (Kéo cả 2 chân IN lên mức 1)
 */
void Motor_Brake(void);

/**
 * @brief  Tự kiểm tra quay thử 2 động cơ khi khởi động (Self-Test)
 */
void Motor_SelfTest(void);

/**
 * @brief  Đo ngưỡng ma sát tĩnh (Deadband) của 2 động cơ
 */
void Motor_Find_Deadband(void);

/**
 * @brief  Chế độ kiểm tra liên tục 2 động cơ và hỗ trợ đo điện áp VOM
 *         (Chạy vòng lặp vô hạn: Trái -> Phải -> Tiến -> Lùi)
 */
void Motor_Test_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* __MOTOR_H__ */
