/**
  ******************************************************************************
  * @file    esp32_comm.h
  * @brief   Giao tiếp UART1 DMA hai chiều giữa STM32F411 và ESP32-S3 N16R8
  ******************************************************************************
  */

#ifndef __ESP32_COMM_H__
#define __ESP32_COMM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/* Kích thước bộ đệm truyền nhận DMA */
#define ESP32_RX_BUF_SIZE   256
#define ESP32_TX_BUF_SIZE   256

/* Cấu trúc dữ liệu nhận từ ESP32 */
typedef struct {
    float   v_target;       // Vận tốc đặt (m/s)
    float   steer_cmd;      // Lệnh lái rẽ (thang PWM, mặc định 0)
    uint8_t is_racing;      // 1: Chế độ đua (góc nghiêng tối đa 15 deg), 0: Thường (8 deg)
    uint8_t trigger_calib;  // Cờ yêu cầu hiệu chuẩn IMU
    uint8_t emergency_stop; // Cờ dừng khẩn cấp
    uint8_t arm_request, diag_request, pulse_request;
    volatile uint8_t profile_request; // reserved legacy mailbox; cleared, no switching
    uint8_t arm_profile; // must be12; stale ARM for another law is rejected
    int8_t pulse_direction;
    uint8_t bench_test;     // 1: Chế độ test quay bánh trực tiếp trên bàn, 0: Cân bằng tự động
    uint32_t last_cmd_time; // Thời điểm nhận gói tin cuối (dùng cho Failsafe timeout)
    uint32_t arm_drive_epoch;
    volatile uint32_t drive_epoch;
    volatile uint32_t drive_word; // atomic sequence/direction publication
    volatile int8_t drive_turn; // -1:right,0:straight,+1:left; published with drive_word
    volatile uint8_t drive_speed_preset; //0:0.4m/s,1:0.6m/s; published with word while IRQs are masked
} ESP32_Command_t;

/**
  * @brief  Khởi tạo ngoại vi UART1 DMA và cấu trúc truyền nhận
  */
void ESP32_Comm_Init(void);

/**
  * @brief  Xử lý giải mã luồng byte từ DMA Circular Buffer (gọi trong while(1))
  */
void ESP32_Comm_Process(void);

/**
  * @brief  Xóa/đồng bộ bộ đệm RX (dùng sau khi chạy tác vụ blocking như $CALIB)
  */
void ESP32_Comm_FlushRx(void);
bool ESP32_Comm_SendBytes(const void *data, uint16_t length);

/**
  * @brief  Lấy con trỏ cấu trúc lệnh hiện tại
  */
ESP32_Command_t* ESP32_Comm_GetCommand(void);

/**
  * @brief  Gửi gói tin Telemetry lên ESP32-S3 qua DMA không nghẽn
  */
bool ESP32_Comm_SendTelemetry(float pitch, float gyro, float v_act, float v_tgt,
                              int16_t pwm_l, int16_t pwm_r, uint8_t state, float batt_volt,
                              float v_l, float v_r);

#ifdef __cplusplus
}
#endif

#endif /* __ESP32_COMM_H__ */
