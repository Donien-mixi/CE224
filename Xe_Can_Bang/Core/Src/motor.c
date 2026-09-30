/**
  ******************************************************************************
  * @file    motor.c
  * @brief   Hiện thực hóa Driver điều khiển Dual A4950 cho 2 Động cơ GA25.
  ******************************************************************************
  */

#include "motor.h"
#include "buzzer_led.h"

void Motor_Init(void)
{
    // Khởi động phát xung PWM cho cả 4 kênh của Timer 1
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1); // PA8  - Motor Trái (AIN1)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2); // PA9  - Motor Trái (AIN2)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3); // PA10 - Motor Phải (BIN1)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // PA11 - Motor Phải (BIN2)

    // Đảm bảo Main Output Enable và Counter của Advanced Timer 1 luôn được kích hoạt
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_ENABLE(&htim1);

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
    /* Đảm bảo Timer 1 và ngõ ra MOE luôn bật */
    __HAL_TIM_MOE_ENABLE(&htim1);
    __HAL_TIM_ENABLE(&htim1);

    /* Chờ 500ms cho nguồn 12V và 5V sau khi cắm giắc pin hoàn toàn ổn định */
    HAL_Delay(500);

    /* Bíp 1 tiếng báo hiệu chuẩn bị kiểm tra động cơ */
    Buzzer_On();
    HAL_Delay(100);
    Buzzer_Off();
    HAL_Delay(300);

    /* 1. Quay Bánh Trái với công suất 64% (PWM 1600/2499) trong 600ms
     * (Mức 1600 đủ lớn để thắng ma sát tĩnh hộp số GA25 và tải trọng bánh xe) */
    Motor_SetDuty(1600, 0);
    HAL_Delay(600);

    /* Dừng bánh trái, nghỉ 200ms và bíp 1 tiếng ngắn chuyển bánh */
    Motor_Stop();
    Buzzer_On();
    HAL_Delay(50);
    Buzzer_Off();
    HAL_Delay(250);

    /* 2. Quay Bánh Phải với công suất 64% (PWM 1600/2499) trong 600ms */
    Motor_SetDuty(0, 1600);
    HAL_Delay(600);

    /* 3. Dừng an toàn cả 2 bánh */
    Motor_Stop();

    /* Bíp 2 tiếng ngắn: Hoàn thành tự kiểm tra động cơ thành công! */
    Buzzer_On();
    HAL_Delay(60);
    Buzzer_Off();
    HAL_Delay(60);
    Buzzer_On();
    HAL_Delay(60);
    Buzzer_Off();
    HAL_Delay(300);
}

void Motor_Test_Run(void)
{
    /* Đảm bảo Timer 1 hoạt động */
    Motor_Init();

    /* Báo hiệu vào chế độ TEST LIÊN TỤC: 3 tiếng bíp dài */
    for (int i = 0; i < 3; i++) {
        Buzzer_On();
        LED_On();
        HAL_Delay(150);
        Buzzer_Off();
        LED_Off();
        HAL_Delay(150);
    }
    HAL_Delay(1000);

    while (1)
    {
        /* --- GIAI ĐOẠN 1: BÁNH TRÁI TIẾN (PWM 1600 ~ 64% công suất) trong 2.0s ---
         * Lúc này đo VOM chân PA8 (AIN1) sẽ được ~2.1V DC, PA9 (AIN2) = 0V */
        Buzzer_On();
        HAL_Delay(80);
        Buzzer_Off();
        LED_On();
        Motor_SetDuty(1600, 0);
        HAL_Delay(2000);

        Motor_Stop();
        LED_Off();
        HAL_Delay(1000);

        /* --- GIAI ĐOẠN 2: BÁNH PHẢI TIẾN (PWM 1600 ~ 64% công suất) trong 2.0s ---
         * Lúc này đo VOM chân PA10 (BIN1) sẽ được ~2.1V DC, PA11 (BIN2) = 0V */
        Buzzer_On();
        HAL_Delay(80);
        Buzzer_Off();
        LED_On();
        Motor_SetDuty(0, 1600);
        HAL_Delay(2000);

        Motor_Stop();
        LED_Off();
        HAL_Delay(1000);

        /* --- GIAI ĐOẠN 3: CẢ 2 BÁNH CÙNG TIẾN (PWM 1600) trong 2.0s --- */
        Buzzer_On();
        HAL_Delay(150);
        Buzzer_Off();
        LED_On();
        Motor_SetDuty(1600, 1600);
        HAL_Delay(2000);

        Motor_Stop();
        LED_Off();
        HAL_Delay(1000);

        /* --- GIAI ĐOẠN 4: CẢ 2 BÁNH CÙNG LÙI (PWM -1600) trong 2.0s ---
         * Lúc này đo VOM chân PA9 (AIN2) và PA11 (BIN2) sẽ được ~2.1V DC */
        Buzzer_On();
        HAL_Delay(150);
        Buzzer_Off();
        LED_On();
        Motor_SetDuty(-1600, -1600);
        HAL_Delay(2000);

        Motor_Stop();
        LED_Off();
        HAL_Delay(2000);
    }
}
