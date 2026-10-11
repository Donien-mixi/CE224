/**
  ******************************************************************************
  * @file    pid.c
  * @brief   Hiện thực thuật toán Cascade PID 2 vòng lồng nhau (Góc PD + Vận tốc PI)
  *          kèm bù vùng chết ma sát hộp số (Deadband) và bẻ lái thích ứng vận tốc.
  ******************************************************************************
  */

#include "pid.h"

/* Biến toàn cục quản lý cấu trúc PID */
static PID_Angle_t    s_pid_angle;
static PID_Velocity_t s_pid_vel;

void PID_Init(void)
{
    s_pid_angle.Kp = DEFAULT_KP_ANGLE;
    s_pid_angle.Kd = DEFAULT_KD_ANGLE;

    s_pid_vel.Kp = DEFAULT_KP_VELOCITY;
    s_pid_vel.Ki = DEFAULT_KI_VELOCITY;
    s_pid_vel.integral = 0.0f;
}

void PID_SetParams(float kp1, float kd1, float kp2, float ki2)
{
    if (kp1 >= 0.0f) s_pid_angle.Kp = kp1;
    if (kd1 >= 0.0f) s_pid_angle.Kd = kd1;
    if (kp2 >= 0.0f) s_pid_vel.Kp = kp2;
    if (ki2 >= 0.0f) s_pid_vel.Ki = ki2;
}

float PID_Velocity_Compute(float target_vel, float actual_vel, uint8_t is_racing, float dt)
{
    float error_v = target_vel - actual_vel;

    /* Tích lũy khâu tích phân (Anti-windup qua kẹp biên) */
    s_pid_vel.integral += s_pid_vel.Ki * error_v * dt;

    if (s_pid_vel.integral > MAX_INTEGRAL_VELOCITY) {
        s_pid_vel.integral = MAX_INTEGRAL_VELOCITY;
    } else if (s_pid_vel.integral < -MAX_INTEGRAL_VELOCITY) {
        s_pid_vel.integral = -MAX_INTEGRAL_VELOCITY;
    }

    /* Góc nghiêng đặt Theta_target */
    float theta_target = (s_pid_vel.Kp * error_v) + s_pid_vel.integral;

    /* Giới hạn góc nghiêng theo chế độ chạy */
    float max_tilt = is_racing ? MAX_TILT_RACE : MAX_TILT_NORMAL;
    if (theta_target > max_tilt) {
        theta_target = max_tilt;
    } else if (theta_target < -max_tilt) {
        theta_target = -max_tilt;
    }

    return theta_target;
}

float PID_Angle_Compute(float target_angle, float actual_angle, float gyro_rate)
{
    float error_theta = target_angle - actual_angle;

    /* Base PWM = Kp1 * error - Kd1 * Gyro_rate (Tránh Derivative Kick) */
    float base_pwm = (s_pid_angle.Kp * error_theta) - (s_pid_angle.Kd * gyro_rate);
    return base_pwm;
}

float PID_Adaptive_Steering(float steer_cmd, float actual_vel)
{
    /* Suy giảm phi tuyến theo vận tốc: 1 / (1 + beta * |v|) */
    const float beta = 1.2f;
    float attenuation = 1.0f / (1.0f + beta * fabsf(actual_vel));
    return steer_cmd * attenuation;
}

int16_t PID_Apply_Deadband(float pwm_val, int16_t deadband, int16_t max_pwm)
{
    if (pwm_val > 1.0f) {
        float out = pwm_val + (float)deadband;
        if (out > (float)max_pwm) out = (float)max_pwm;
        return (int16_t)out;
    } else if (pwm_val < -1.0f) {
        float out = pwm_val - (float)deadband;
        if (out < -(float)max_pwm) out = -(float)max_pwm;
        return (int16_t)out;
    } else {
        return 0;
    }
}

void PID_Reset_Integral(void)
{
    s_pid_vel.integral = 0.0f;
}
