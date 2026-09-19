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
#include <math.h>

static Robot_Data_t s_robot = {
    .state = ROBOT_STATE_INIT,
    .pitch = 0.0f,
    .gyro_rate = 0.0f,
    .v_actual = 0.0f,
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
    ESP32_Comm_Init();

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
}

void Robot_ControlLoop_200Hz(void)
{
    s_robot.loop_count++;

    /* Task 1: Đọc dữ liệu IMU Bosch BMI160 qua I2C1 */
    BMI160_Data_t imu;
    if (!BMI160_Read_All(&imu)) {
        s_robot.state = ROBOT_STATE_EMERGENCY;
        Motor_Stop();
        return;
    }

    /* Task 2: Lọc bù góc nghiêng Pitch & lấy Gyro rate */
    s_robot.pitch = Filter_Complementary_Update(imu.gy, imu.ax, imu.az, 0.005f);
    s_robot.gyro_rate = imu.gy;

    /* Task 3: Đọc vận tốc Encoder bánh xe & lọc LPF */
    Encoder_Update(0.005f);
    s_robot.v_actual = Encoder_GetAverageVelocity();

    /* Task 4: Lấy lệnh điều khiển từ ESP32-S3 */
    ESP32_Command_t* cmd = ESP32_Comm_GetCommand();
    if (cmd->emergency_stop) {
        s_robot.state = ROBOT_STATE_FALLEN;
        cmd->emergency_stop = 0;
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
        if (fabsf(s_robot.pitch) < 2.5f) {
            /* Dựng xe đứng thẳng -> Tự động kích hoạt cân bằng! */
            s_robot.state = cmd->is_racing ? ROBOT_STATE_RACING : ROBOT_STATE_BALANCING;
            PID_Reset_Integral();
            Encoder_Reset();
            LED_SetPattern(LED_PATTERN_HEARTBEAT);
            Buzzer_BeepAsync(80);
        } else {
            Motor_Stop();
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
