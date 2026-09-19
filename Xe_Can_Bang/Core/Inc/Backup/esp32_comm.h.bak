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
    float   steer_cmd;      // Lực lái rẽ (-1000 đến +1000)
    uint8_t is_racing;      // 1: Chế độ đua (góc nghiêng tối đa 15 deg), 0: Thường (8 deg)
    uint8_t trigger_calib;  // Cờ yêu cầu hiệu chuẩn IMU
    uint8_t emergency_stop; // Cờ dừng khẩn cấp
    uint32_t last_cmd_time; // Thời điểm nhận gói tin cuối (dùng cho Failsafe timeout)
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
  * @brief  Lấy con trỏ cấu trúc lệnh hiện tại
  */
ESP32_Command_t* ESP32_Comm_GetCommand(void);

/**
  * @brief  Gửi gói tin Telemetry lên ESP32-S3 qua DMA không nghẽn
  */
bool ESP32_Comm_SendTelemetry(float pitch, float gyro, float v_act, float v_tgt,
                              int16_t pwm_l, int16_t pwm_r, uint8_t state, float batt_volt);

#ifdef __cplusplus
}
#endif

#endif /* __ESP32_COMM_H__ */
