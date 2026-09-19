/**
  ******************************************************************************
  * @file    pid.h
  * @brief   Bộ điều khiển Cascade PID 2 vòng lồng nhau (Góc PD + Vận tốc PI)
  *          kèm bù vùng chết ma sát hộp số (Deadband) và bẻ lái thích ứng vận tốc.
  ******************************************************************************
  */

#ifndef __PID_H__
#define __PID_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <math.h>

/* Thông số mặc định ban đầu */
#define DEFAULT_KP_ANGLE        350.0f  // Hệ số tỉ lệ vòng góc
#define DEFAULT_KD_ANGLE        8.5f    // Hệ số vi sai vòng góc
#define DEFAULT_KP_VELOCITY     1.2f    // Hệ số tỉ lệ vòng vận tốc
#define DEFAULT_KI_VELOCITY     0.15f   // Hệ số tích phân vòng vận tốc (tự học trọng tâm)

#define MAX_TILT_NORMAL         8.0f    // Giới hạn góc ngả chế độ thường (độ)
#define MAX_TILT_RACE           15.0f   // Giới hạn góc ngả chế độ đua (độ)
#define MAX_INTEGRAL_VELOCITY   10.0f   // Kẹp tích phân chống bão hòa Anti-windup (độ)

typedef struct {
    float Kp;
    float Kd;
} PID_Angle_t;

typedef struct {
    float Kp;
    float Ki;
    float integral;
} PID_Velocity_t;

/**
  * @brief  Khởi tạo các tham số PID ban đầu
  */
void PID_Init(void);

/**
  * @brief  Cập nhật tham số PID trực tiếp từ lệnh tune ($PID,kp1,kd1,kp2,ki2*)
  */
void PID_SetParams(float kp1, float kd1, float kp2, float ki2);

/**
  * @brief  Tính toán vòng ngoài (Velocity Loop PI) sinh ra góc nghiêng đặt
  * @param  target_vel: Vận tốc đặt từ Joystick (m/s)
  * @param  actual_vel: Vận tốc đo được từ Encoder (m/s)
  * @param  is_racing:  1 nếu đang ở chế độ đua, 0 nếu chế độ thường
  * @param  dt:         Chu kỳ lấy mẫu (giây)
  * @retval Góc nghiêng đặt Theta_target (độ)
  */
float PID_Velocity_Compute(float target_vel, float actual_vel, uint8_t is_racing, float dt);

/**
  * @brief  Tính toán vòng trong (Angle Loop PD) sinh ra Base PWM
  * @param  target_angle: Góc đặt từ vòng vận tốc (độ)
  * @param  actual_angle: Góc nghiêng thực từ bộ lọc bù (độ)
  * @param  gyro_rate:    Vận tốc góc Pitch từ Gyro (deg/s)
  * @retval Xung PWM cơ sở (Base PWM)
  */
float PID_Angle_Compute(float target_angle, float actual_angle, float gyro_rate);

/**
  * @brief  Bù vi sai lái thích ứng theo vận tốc (tránh lật xe khi vào cua tốc độ cao)
  * @param  steer_cmd:  Lệnh rẽ từ Joystick
  * @param  actual_vel: Vận tốc hiện tại (m/s)
  * @retval Giá trị PWM vi sai bù vào 2 bánh
  */
float PID_Adaptive_Steering(float steer_cmd, float actual_vel);

/**
  * @brief  Bù vùng chết ma sát tĩnh hộp số GA25 (Deadband Compensation)
  */
int16_t PID_Apply_Deadband(float pwm_val, int16_t deadband, int16_t max_pwm);

/**
  * @brief  Reset khâu tích phân về 0 khi xe ngã hoặc đứng chờ
  */
void PID_Reset_Integral(void);

#ifdef __cplusplus
}
#endif

#endif /* __PID_H__ */
