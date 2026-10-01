/**
  ******************************************************************************
  * @file    robot_fsm.c
  * @brief   Hiện thực Máy trạng thái FSM và vòng lặp điều khiển thời gian thực 200Hz
  ******************************************************************************
  */

#include "robot_fsm.h"
#include "motor.h"
#include "encoder.h"
#include "bmi160.h"
#include "filter.h"
#include "pid.h"
#include "buzzer_led.h"
#include "esp32_comm.h"
#include "tim.h"
#include <math.h>

static Robot_Data_t s_robot = {
    .state = ROBOT_STATE_INIT,
    .pitch = 0.0f,
    .gyro_rate = 0.0f,
    .v_actual = 0.0f,
    .v_left = 0.0f,
    .v_right = 0.0f,
    .v_target = 0.0f,
    .steer_cmd = 0.0f,
    .pwm_left = 0,
    .pwm_right = 0,
    .batt_voltage = 12.0f, // Mặc định 3S LiPo ~12.0V
    .loop_count = 0
};

void Robot_Init(void)
{
    /* 1. Khởi tạo còi, LED, cơ cấu chấp hành */
    BuzzerLED_Init();
    Motor_Init();
    Motor_Stop();
    Encoder_Init();
    PID_Init();

    /* Tự kiểm tra 2 động cơ ngay khi khởi động (GĐ0 self-test):
     * PWM 1600 (~64%) mỗi bánh 600ms kèm bíp báo — xác nhận nguồn pin 12V,
     * mạch Driver A4950 và 2 động cơ hoạt động hoàn hảo. */
    Motor_SelfTest();

    /* 2. Khởi tạo cảm biến Bosch BMI160 */
    if (!BMI160_Init()) {
        s_robot.state = ROBOT_STATE_EMERGENCY;
        LED_SetPattern(LED_PATTERN_ALARM);
        Buzzer_On();
        return;
    }

    /* 3. Hiệu chuẩn bù độ trôi Gyro */
    s_robot.state = ROBOT_STATE_CALIBRATING;
    LED_SetPattern(LED_PATTERN_FAST_BLINK);
    Buzzer_BeepAsync(100);

    BMI160_Calibrate_Gyro(500);

    /* 4. Khởi tạo góc ban đầu cho bộ lọc bù */
    BMI160_Data_t init_imu;
    BMI160_Read_All(&init_imu);
    float init_pitch = atan2f(init_imu.ax, init_imu.az) * RAD_TO_DEG;
    Filter_Init(init_pitch);

    /* 5. Chuyển sang trạng thái chờ dựng xe */
    s_robot.state = ROBOT_STATE_STANDBY;
    LED_SetPattern(LED_PATTERN_SLOW_BLINK);
    Buzzer_BeepAsync(200);

    /* 6. Khởi động giao tiếp UART1 DMA với ESP32-S3 NGAY TRƯỚC KHI VÀO VÒNG LẶP CHÍNH
     * (Tránh tràn bộ đệm DMA và lỗi Overrun/Framing trong thời gian 5s chạy SelfTest và Calib) */
    ESP32_Comm_Init();
}

void Robot_RecalibrateIMU(void)
{
    /* Tạm dừng vòng điều khiển 200Hz để tránh truy cập I2C chồng chéo trong lúc hiệu chuẩn */
    HAL_TIM_Base_Stop_IT(&htim4);

    Motor_Stop();
    s_robot.pwm_left  = 0;
    s_robot.pwm_right = 0;

    s_robot.state = ROBOT_STATE_CALIBRATING;
    LED_SetPattern(LED_PATTERN_FAST_BLINK);
    Buzzer_BeepAsync(100);

    /* Lấy 500 mẫu trung bình tĩnh (~2.5s) để bù trôi Gyro */
    BMI160_Calibrate_Gyro(500);

    /* Khởi tạo lại góc ban đầu cho bộ lọc bù */
    BMI160_Data_t imu;
    if (!BMI160_Read_All(&imu)) {
        s_robot.state = ROBOT_STATE_EMERGENCY;
        LED_SetPattern(LED_PATTERN_ALARM);
        Buzzer_On();
        return; /* Không khởi động lại TIM4 -> hệ thống giữ ở trạng thái khẩn cấp */
    }

    Filter_Init(atan2f(imu.ax, imu.az) * RAD_TO_DEG);
    PID_Reset_Integral();
    Encoder_Reset();

    s_robot.state = ROBOT_STATE_STANDBY;
    LED_SetPattern(LED_PATTERN_SLOW_BLINK);
    Buzzer_BeepAsync(200);

    __HAL_TIM_SET_COUNTER(&htim4, 0);
    HAL_TIM_Base_Start_IT(&htim4);
}

void Robot_ControlLoop_200Hz(void)
{
    s_robot.loop_count++;

    /* Nếu hệ thống đang gặp lỗi khẩn cấp, cắt PWM và thoát ngay để không làm nghẽn vi điều khiển */
    if (s_robot.state == ROBOT_STATE_EMERGENCY) {
        Motor_Stop();
        s_robot.pwm_left = 0;
        s_robot.pwm_right = 0;
        return;
    }

    /* Task 1: Đọc dữ liệu IMU Bosch BMI160 qua I2C1 */
    BMI160_Data_t imu;
    if (!BMI160_Read_All(&imu)) {
        s_robot.state = ROBOT_STATE_EMERGENCY;
        LED_SetPattern(LED_PATTERN_ALARM);
        Buzzer_On();
        Motor_Stop();
        s_robot.pwm_left = 0;
        s_robot.pwm_right = 0;
        return;
    }

    /* Task 2: Lọc bù góc nghiêng Pitch & lấy Gyro rate */
    s_robot.pitch = Filter_Complementary_Update(imu.gy, imu.ax, imu.az, 0.005f);
    s_robot.gyro_rate = imu.gy;

    /* Task 3: Đọc vận tốc Encoder bánh xe & lọc LPF */
    Encoder_Update(0.005f);
    s_robot.v_left = Encoder_GetLeftVelocity();
    s_robot.v_right = Encoder_GetRightVelocity();
    s_robot.v_actual = Encoder_GetAverageVelocity();

    /* Task 4: Lấy lệnh điều khiển từ ESP32-S3 */
    ESP32_Command_t* cmd = ESP32_Comm_GetCommand();
    if (cmd->emergency_stop) {
        s_robot.state = ROBOT_STATE_FALLEN;
        cmd->emergency_stop = 0;
        cmd->bench_test = 0;
    }
    /* Luôn đồng bộ lệnh vận tốc đặt và lái vào cấu trúc Telemetry thời gian thực */
    s_robot.v_target = cmd->v_target;
    s_robot.steer_cmd = cmd->steer_cmd;

    /* Xử lý bật / tắt CHẾ ĐỘ TEST BÀN (BENCH TEST) */
    if (cmd->bench_test) {
        if (s_robot.state != ROBOT_STATE_BENCH_TEST) {
            s_robot.state = ROBOT_STATE_BENCH_TEST;
            LED_SetPattern(LED_PATTERN_FAST_BLINK);
            Buzzer_BeepAsync(60);
        }
    } else if (s_robot.state == ROBOT_STATE_BENCH_TEST) {
        /* Khi tắt Bench Test, đưa về STANDBY an toàn và tắt motor */
        s_robot.state = ROBOT_STATE_STANDBY;
        Motor_Stop();
        s_robot.pwm_left = 0;
        s_robot.pwm_right = 0;
        LED_SetPattern(LED_PATTERN_SLOW_BLINK);
    }

    /* Task 4.1: THỰC THI CHẾ ĐỘ TEST BÀN (Bỏ qua bảo vệ góc nghiêng) */
    if (s_robot.state == ROBOT_STATE_BENCH_TEST) {
        int16_t pwm_base = 0;
        if (fabsf(s_robot.v_target) > 0.01f) {
            /* Quy đổi v_target (-1.5 đến +1.5 m/s) ra PWM trực tiếp:
             * Tại v = 0.60 m/s -> PWM = 0.60 * 1500 = 900 + deadband 250 = 1150 (46% công suất) */
            if (s_robot.v_target > 0.0f) {
                pwm_base = (int16_t)(s_robot.v_target * 1500.0f) + MOTOR_DEADBAND;
            } else {
                pwm_base = (int16_t)(s_robot.v_target * 1500.0f) - MOTOR_DEADBAND;
            }
        }

        int16_t steer = (int16_t)s_robot.steer_cmd;
        int16_t pwm_l = pwm_base + steer;
        int16_t pwm_r = pwm_base - steer;

        if (pwm_l > MOTOR_MAX_PWM) pwm_l = MOTOR_MAX_PWM;
        if (pwm_l < -MOTOR_MAX_PWM) pwm_l = -MOTOR_MAX_PWM;
        if (pwm_r > MOTOR_MAX_PWM) pwm_r = MOTOR_MAX_PWM;
        if (pwm_r < -MOTOR_MAX_PWM) pwm_r = -MOTOR_MAX_PWM;

        s_robot.pwm_left = pwm_l;
        s_robot.pwm_right = pwm_r;

        Motor_SetDuty(pwm_l, pwm_r);
        return; // Thoát Task điều khiển, bỏ qua Task 5-7
    }

    /* Task 5: Bảo vệ chống ngã - Cắt điện ngay lập tức nếu góc ngả > 45 độ */
    if (fabsf(s_robot.pitch) > 45.0f) {
        Motor_Stop();
        PID_Reset_Integral();
        s_robot.pwm_left = 0;
        s_robot.pwm_right = 0;
        if (s_robot.state != ROBOT_STATE_FALLEN) {
            s_robot.state = ROBOT_STATE_FALLEN;
            LED_SetPattern(LED_PATTERN_ALARM);
            Buzzer_BeepAsync(300);
        }
        return;
    }

    /* Task 6: Máy trạng thái chuyển đổi giữa STANDBY / FALLEN và BALANCING */
    if (s_robot.state == ROBOT_STATE_STANDBY || s_robot.state == ROBOT_STATE_FALLEN) {
        if (fabsf(s_robot.pitch) < 15.0f) {
            /* Dựng xe đứng thẳng trong phạm vi +-15 độ -> Tự động kích hoạt cân bằng! */
            s_robot.state = cmd->is_racing ? ROBOT_STATE_RACING : ROBOT_STATE_BALANCING;
            PID_Reset_Integral();
            Encoder_Reset();
            LED_SetPattern(LED_PATTERN_HEARTBEAT);
            Buzzer_BeepAsync(80);
        } else {
            Motor_Stop();
            s_robot.pwm_left = 0;
            s_robot.pwm_right = 0;
            return;
        }
    }

    /* Task 7: Tính toán Cascade PID khi đang cân bằng */
    if (s_robot.state == ROBOT_STATE_BALANCING || s_robot.state == ROBOT_STATE_RACING) {
        s_robot.v_target = cmd->v_target;
        s_robot.steer_cmd = cmd->steer_cmd;

        uint8_t is_race = (s_robot.state == ROBOT_STATE_RACING) ? 1 : 0;

        /* Vòng ngoài: Vận tốc PI sinh ra góc nghiêng đặt */
        float theta_target = PID_Velocity_Compute(s_robot.v_target, s_robot.v_actual, is_race, 0.005f);

        /* Vòng trong: Góc nghiêng PD sinh ra Base PWM */
        float pwm_base = PID_Angle_Compute(theta_target, s_robot.pitch, s_robot.gyro_rate);

        /* Bù vi sai lái thích ứng vận tốc */
        float steer_eff = PID_Adaptive_Steering(s_robot.steer_cmd, s_robot.v_actual);

        /* Trộn kênh hai bánh */
        float pwm_l = pwm_base + steer_eff;
        float pwm_r = pwm_base - steer_eff;

        /* Bù ma sát vùng chết hộp số GA25 và kẹp biên cực đại 2499 */
        int16_t out_l = PID_Apply_Deadband(pwm_l, MOTOR_DEADBAND, MOTOR_MAX_PWM);
        int16_t out_r = PID_Apply_Deadband(pwm_r, MOTOR_DEADBAND, MOTOR_MAX_PWM);

        s_robot.pwm_left = out_l;
        s_robot.pwm_right = out_r;

        /* Xuất xung ra Dual A4950 */
        Motor_SetDuty(out_l, out_r);
    }
}

Robot_Data_t* Robot_GetData(void)
{
    return &s_robot;
}
