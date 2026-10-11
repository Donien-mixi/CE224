/**
  ******************************************************************************
  * @file    bmi160.h
  * @brief   Driver cảm biến quán tính 6 trục cao cấp Bosch BMI160 (GY-BMI160).
  *          Giao tiếp qua I2C1 Fast Mode 400kHz (PB8: SCL, PB9: SDA).
  *          Đọc đồng thời 12 bytes Gia tốc và Con quay hồi chuyển trong < 350µs.
  ******************************************************************************
  */

#ifndef __BMI160_H__
#define __BMI160_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include <stdint.h>
#include <stdbool.h>

/* I2C Address Defines -------------------------------------------------------*/
#define BMI160_I2C_ADDR             (0x68 << 1) // 0xD0 (Ghi), 0xD1 (Đọc)

/* Register Addresses --------------------------------------------------------*/
#define BMI160_REG_CHIP_ID          0x00        // Giá trị trả về phải là 0xD1
#define BMI160_REG_DATA_0           0x0C        // Bắt đầu 12 bytes dữ liệu (Gx LSB)
#define BMI160_REG_ACC_CONF         0x40        // Cấu hình ODR và bộ lọc số Accel
#define BMI160_REG_ACC_RANGE        0x41        // Cấu hình dải đo Accel
#define BMI160_REG_GYR_CONF         0x42        // Cấu hình ODR và bộ lọc số Gyro
#define BMI160_REG_GYR_RANGE        0x43        // Cấu hình dải đo Gyro
#define BMI160_REG_CMD              0x7E        // Thanh ghi lệnh chế độ nguồn

/* Command Register Codes ---------------------------------------------------*/
#define BMI160_CMD_ACC_MODE_NORMAL  0x11        // Chuyển Accel sang Normal Mode
#define BMI160_CMD_GYR_MODE_NORMAL  0x15        // Chuyển Gyro sang Normal Mode
#define BMI160_CMD_SOFT_RESET       0xB6        // Reset mềm toàn bộ chip

/* Scale Factors -------------------------------------------------------------*/
// Dải đo Gyro: +-2000 deg/s -> 16.4 LSB / (deg/s) -> 1 / 16.4 = 0.061035156f
#define BMI160_GYRO_SCALE_2000DPS   (1.0f / 16.4f)

// Dải đo Accel: +-8g -> 4096 LSB / g -> 1 / 4096 = 0.00024414f
#define BMI160_ACCEL_SCALE_8G       (1.0f / 4096.0f)

/* Structs -------------------------------------------------------------------*/
typedef struct {
    int16_t gx_raw;
    int16_t gy_raw;
    int16_t gz_raw;
    int16_t ax_raw;
    int16_t ay_raw;
    int16_t az_raw;
} BMI160_RawData_t;

typedef struct {
    float gx;   // Vận tốc góc Roll (deg/s)
    float gy;   // Vận tốc góc Pitch (deg/s - Trục ngã của xe)
    float gz;   // Vận tốc góc Yaw (deg/s - Trục quay bẻ lái)
    float ax;   // Gia tốc trục X (g)
    float ay;   // Gia tốc trục Y (g)
    float az;   // Gia tốc trục Z (g)
} BMI160_Data_t;

/* Function Prototypes -------------------------------------------------------*/

/**
  * @brief  Khởi tạo, đánh thức và cấu hình dải đo cho Bosch BMI160
  * @retval true nếu tìm thấy Chip ID 0xD1 và khởi tạo thành công, false nếu lỗi I2C
  */
bool BMI160_Init(void);

/**
  * @brief  Đọc Burst 12 bytes liên tục từ thanh ghi 0x0C đến 0x17
  * @param  data: Con trỏ tới cấu trúc chứa dữ liệu đơn vị vật lý (deg/s và g)
  * @retval true nếu đọc thành công, false nếu I2C timeout
  */
bool BMI160_Read_All(BMI160_Data_t *data);

/**
  * @brief  Hiệu chuẩn bù độ trôi tĩnh con quay hồi chuyển (Zero-rate Bias)
  * @param  samples: Số mẫu lấy trung bình (khuyến nghị 500 mẫu trong 2.5s khi xe nằm yên)
  */
void BMI160_Calibrate_Gyro(uint16_t samples);

#ifdef __cplusplus
}
#endif

#endif /* __BMI160_H__ */
