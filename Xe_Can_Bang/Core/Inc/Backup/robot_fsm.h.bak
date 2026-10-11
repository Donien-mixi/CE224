/**
  ******************************************************************************
  * @file    robot_fsm.h
  * @brief   Máy trạng thái hữu hạn FSM và vòng điều khiển trung tâm 200Hz
  ******************************************************************************
  */

#ifndef __ROBOT_FSM_H__
#define __ROBOT_FSM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Các trạng thái hoạt động của xe cân bằng */
typedef enum {
    ROBOT_STATE_INIT = 0,       // Khởi tạo phần cứng
    ROBOT_STATE_CALIBRATING,    // Lấy 500 mẫu hiệu chuẩn Gyro tĩnh
    ROBOT_STATE_STANDBY,        // Chờ người dùng dựng xe (|pitch| < 2.5 độ)
    ROBOT_STATE_BALANCING,      // Cân bằng chế độ thường (tối đa +-8 độ)
    ROBOT_STATE_RACING,         // Cân bằng chế độ đua tốc độ cao (tối đa +-15 độ)
    ROBOT_STATE_FALLEN,         // Xe bị ngã (|pitch| > 45 độ) -> Cắt PWM
    ROBOT_STATE_EMERGENCY       // Lỗi khẩn cấp (I2C đứt, quá dòng, mất cảm biến)
} Robot_State_t;

/* Cấu trúc dữ liệu trạng thái toàn cục của robot */
typedef struct {
    Robot_State_t state;
    float   pitch;              // Góc nghiêng Pitch hiện tại (độ)
    float   gyro_rate;          // Vận tốc góc Pitch (deg/s)
    float   v_actual;           // Vận tốc tịnh tiến thực tế trung bình (m/s)
    float   v_target;           // Vận tốc đặt (m/s)
    float   steer_cmd;          // Lực rẽ
    int16_t pwm_left;           // PWM bánh trái (-2499 đến +2499)
    int16_t pwm_right;          // PWM bánh phải (-2499 đến +2499)
    float   batt_voltage;       // Điện áp pin ước tính hoặc đo qua ADC (V)
    uint32_t loop_count;        // Đếm chu kỳ 200Hz
} Robot_Data_t;

/**
  * @brief  Khởi tạo toàn bộ hệ thống robot, cảm biến, thuật toán
  */
void Robot_Init(void);

/**
  * @brief  Vòng lặp điều khiển thời gian thực 200Hz (gọi trong ngắt TIM4 5ms)
  */
void Robot_ControlLoop_200Hz(void);

/**
  * @brief  Lấy con trỏ cấu trúc dữ liệu của robot
  */
Robot_Data_t* Robot_GetData(void);

#ifdef __cplusplus
}
#endif

#endif /* __ROBOT_FSM_H__ */
