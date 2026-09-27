/**
  ******************************************************************************
  * @file    motor.c
  * @brief   Hiện thực hóa Driver điều khiển Dual A4950 cho 2 Động cơ GA25.
  ******************************************************************************
  */

#include "motor.h"

void Motor_Init(void)
{
    // Khởi động phát xung PWM cho cả 4 kênh của Timer 1
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // PA8  - Motor Trái (AIN1)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); // PA9  - Motor Trái (AIN2)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); // PA10 - Motor Phải (BIN1)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // PA11 - Motor Phải (BIN2)

    // Đảm bảo Main Output Enable của Advanced Timer 1 luôn được kích hoạt
    __HAL_TIM_MOE_ENABLE(&htim1);

    // Khởi tạo trạng thái dừng an toàn ban đầu
    Motor_Stop();
}

void Motor_SetDuty(int16_t pwm_left, int16_t pwm_right)
{
    // 1. Giới hạn dải công suất đầu vào trong ngưỡng [-MOTOR_MAX_PWM, +MOTOR_MAX_PWM]
    if (pwm_left > MOTOR_MAX_PWM)   pwm_left = MOTOR_MAX_PWM;
    if (pwm_left < -MOTOR_MAX_PWM)  pwm_left = -MOTOR_MAX_PWM;

    if (pwm_right > MOTOR_MAX_PWM)  pwm_right = MOTOR_MAX_PWM;
    if (pwm_right < -MOTOR_MAX_PWM) pwm_right = -MOTOR_MAX_PWM;

    // 2. Điều khiển Động cơ Trái (Kênh 1 & Kênh 2 - Slow Decay)
    if (pwm_left > 0)
    {
        // Quay tiến: IN1 phát xung PWM, IN2 giữ mức 0
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t)pwm_left);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    }
    else if (pwm_left < 0)
    {
        // Quay lùi: IN1 giữ mức 0, IN2 phát xung PWM
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, (uint16_t)(-pwm_left));
    }
    else
    {
        // Thả trôi / phanh nhẹ: Cả 2 chân giữ mức 0
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    }

    // 3. Điều khiển Động cơ Phải (Kênh 3 & Kênh 4 - Slow Decay)
    if (pwm_right > 0)
    {
        // Quay tiến: IN3 phát xung PWM, IN4 giữ mức 0
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, (uint16_t)pwm_right);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    }
    else if (pwm_right < 0)
    {
        // Quay lùi: IN3 giữ mức 0, IN4 phát xung PWM
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, (uint16_t)(-pwm_right));
    }
    else
    {
        // Thả trôi / phanh nhẹ: Cả 2 chân giữ mức 0
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
    }
}

void Motor_Stop(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, 0);
}

void Motor_Brake(void)
{
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, MOTOR_MAX_PWM);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, MOTOR_MAX_PWM);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, MOTOR_MAX_PWM);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, MOTOR_MAX_PWM);
}

void Motor_SelfTest(void)
{
    __HAL_TIM_MOE_ENABLE(&htim1);

    /* 1. Quay nhẹ bánh trái trong 350ms (~40% công suất) */
    Motor_SetDuty(1000, 0);
    HAL_Delay(350);

    /* Nghỉ 100ms giữa 2 bánh */
    Motor_Stop();
    HAL_Delay(100);

    /* 2. Quay nhẹ bánh phải trong 350ms (~40% công suất) */
    Motor_SetDuty(0, 1000);
    HAL_Delay(350);

    /* 3. Dừng an toàn */
    Motor_Stop();
}
