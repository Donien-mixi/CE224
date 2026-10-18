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
static PID_Position_t s_pid_pos;
static float          s_pitch_trim = 0.0f;
static uint8_t        s_velocity_integration_enabled;

void PID_Init(void)
{
    s_pid_angle.Kp = DEFAULT_KP_ANGLE;
    s_pid_angle.Kd = DEFAULT_KD_ANGLE;

    s_pid_vel.Kp = DEFAULT_KP_VELOCITY;
    s_pid_vel.Ki = DEFAULT_KI_VELOCITY;
    s_pid_vel.integral = 0.0f;
    
    s_pid_pos.Kp = DEFAULT_KP_POSITION;
    
    s_pitch_trim = 0.0f;
    s_velocity_integration_enabled = 0;
}

void PID_SetParams(float kp1, float kd1, float kp2, float ki2)
{
    if (!isfinite(kp1)||!isfinite(kd1)||!isfinite(kp2)||!isfinite(ki2)||
        kp1<0||kp1>300||kd1<0||kd1>40||kp2<0||kp2>100||ki2<0||ki2>2) return;
    if (kp1 >= 0.0f) s_pid_angle.Kp = kp1;
    if (kd1 >= 0.0f) s_pid_angle.Kd = kd1;
    if (kp2 >= 0.0f) s_pid_vel.Kp = kp2;
    if (ki2 >= 0.0f) s_pid_vel.Ki = ki2;
}

void PID_SetTrim(float trim)
{
    if (isfinite(trim) && fabsf(trim)<=3.0f) s_pitch_trim = trim;
}

float PID_GetTrim(void)
{
    return s_pitch_trim;
}

float PID_Position_Compute(float target_pos, float actual_pos)
{
    float error_p = target_pos - actual_pos;
    float target_vel = s_pid_pos.Kp * error_p;
    
    /* Giới hạn vận tốc bù tối đa (ví dụ +- 1.0 m/s) để tránh giật cục khi sai số lớn */
    if (target_vel > 1.0f) target_vel = 1.0f;
    if (target_vel < -1.0f) target_vel = -1.0f;
    
    return target_vel;
}

float PID_Velocity_Compute(float target_vel, float actual_vel, uint8_t is_racing, float dt)
{
    float error_v = target_vel - actual_vel;

    /* Slow bias correction starts only after capture. Hold, rather than erase,
     * the learned bias when the gate closes; do not integrate further into tilt saturation. */
    float max_tilt = is_racing ? MAX_TILT_RACE : MAX_TILT_NORMAL;
    if(s_velocity_integration_enabled) {
        float next=s_pid_vel.integral+s_pid_vel.Ki*error_v*dt;
        next=fmaxf(-MAX_INTEGRAL_VELOCITY,fminf(MAX_INTEGRAL_VELOCITY,next));
        float pending=s_pid_vel.Kp*error_v+next;
        if(fabsf(pending)<=max_tilt || pending*error_v<0)s_pid_vel.integral=next;
    }

    /* Góc nghiêng đặt Theta_target */
    float theta_target = (s_pid_vel.Kp * error_v) + s_pid_vel.integral;

    /* Giới hạn góc nghiêng theo chế độ chạy */
    if (theta_target > max_tilt) {
        theta_target = max_tilt;
    } else if (theta_target < -max_tilt) {
        theta_target = -max_tilt;
    }

    return theta_target;
}

float PID_Angle_Compute(float target_angle, float actual_angle, float gyro_rate)
{
    /* Bù góc lệch trọng tâm cơ khí tĩnh theta_trim: e = (target + trim) - actual */
    float error_theta = (target_angle + s_pitch_trim) - actual_angle;

    /* Base PWM = Kp1 * error - Kd1 * Gyro_rate (Tránh Derivative Kick) */
    float base_pwm = (s_pid_angle.Kp * error_theta) - (s_pid_angle.Kd * gyro_rate);
    return PID_ANGLE_OUTPUT_SIGN * base_pwm;
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

float PID_GetVelocityIntegral(void) { return s_pid_vel.integral; }
void PID_EnableVelocityIntegration(uint8_t enabled) { s_velocity_integration_enabled=enabled!=0; }

void PID_Reset_Integral(void)
{
    s_pid_vel.integral = 0.0f;
    s_velocity_integration_enabled = 0;
}
