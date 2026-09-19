/**
  ******************************************************************************
  * @file    bmi160.c
  * @brief   Hiện thực hóa Driver cảm biến quán tính Bosch BMI160.
  ******************************************************************************
  */

#include "bmi160.h"

/* Biến lưu trữ giá trị bù trôi tĩnh con quay hồi chuyển */
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;

/* Hàm phụ trợ ghi 1 byte vào thanh ghi I2C */
static bool BMI160_WriteReg(uint8_t reg, uint8_t value)
{
    if (HAL_I2C_Mem_Write(&hi2c1, BMI160_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 10) == HAL_OK)
    {
        return true;
    }
    return false;
}

/* Hàm phụ trợ đọc dữ liệu từ thanh ghi I2C */
static bool BMI160_ReadReg(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (HAL_I2C_Mem_Read(&hi2c1, BMI160_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buffer, len, 10) == HAL_OK)
    {
        return true;
    }
    return false;
}

bool BMI160_Init(void)
{
    uint8_t chip_id = 0;

    // 1. Đọc và xác thực mã Chip ID (Bắt buộc phải là 0xD1)
    HAL_Delay(20);
    BMI160_ReadReg(BMI160_REG_CHIP_ID, &chip_id, 1);
    if (chip_id != 0xD1)
    {
        return false; // Lỗi tiếp xúc hoặc sai địa chỉ I2C
    }

    // 2. Reset mềm chip để đưa về trạng thái sạch sẽ
    BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_SOFT_RESET);
    HAL_Delay(15);

    // 3. Đánh thức Accelerometer vào Normal Mode
    BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_ACC_MODE_NORMAL);
    HAL_Delay(5);

    // 4. Đánh thức Gyroscope vào Normal Mode (Cần tối thiểu 50ms để bộ thạch anh ổn định)
    BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_GYR_MODE_NORMAL);
    HAL_Delay(55);

    // 5. Cấu hình Dải đo:
    // Gyro: +-2000 deg/s (Ghi 0x00 vào 0x43)
    BMI160_WriteReg(BMI160_REG_GYR_RANGE, 0x00);

    // Accel: +-8g (Ghi 0x08 vào 0x41)
    BMI160_WriteReg(BMI160_REG_ACC_RANGE, 0x08);

    // 6. Cấu hình ODR 200Hz và bộ lọc số triệt tiêu rung cao tần:
    // 0x28 = ODR 200Hz (0x08) kết hợp bộ lọc chuẩn Normal (0x20)
    BMI160_WriteReg(BMI160_REG_ACC_CONF, 0x28);
    BMI160_WriteReg(BMI160_REG_GYR_CONF, 0x28);

    return true;
}

bool BMI160_Read_All(BMI160_Data_t *data)
{
    uint8_t raw_buf[12];

    // Đọc Burst 12 bytes liên tục: 6 bytes Gyro (Gx, Gy, Gz) + 6 bytes Accel (Ax, Ay, Az)
    if (!BMI160_ReadReg(BMI160_REG_DATA_0, raw_buf, 12))
    {
        return false;
    }

    // Ghép 2 bytes (LSB trước, MSB sau) thành số nguyên có dấu 16-bit
    int16_t gx_raw = (int16_t)(((uint16_t)raw_buf[1]  << 8) | raw_buf[0]);
    int16_t gy_raw = (int16_t)(((uint16_t)raw_buf[3]  << 8) | raw_buf[2]);
    int16_t gz_raw = (int16_t)(((uint16_t)raw_buf[5]  << 8) | raw_buf[4]);

    int16_t ax_raw = (int16_t)(((uint16_t)raw_buf[7]  << 8) | raw_buf[6]);
    int16_t ay_raw = (int16_t)(((uint16_t)raw_buf[9]  << 8) | raw_buf[8]);
    int16_t az_raw = (int16_t)(((uint16_t)raw_buf[11] << 8) | raw_buf[10]);

    // Chuyển đổi sang đơn vị vật lý và trừ giá trị lệch tĩnh Gyro Bias
    data->gx = ((float)gx_raw * BMI160_GYRO_SCALE_2000DPS) - gyro_bias_x;
    data->gy = ((float)gy_raw * BMI160_GYRO_SCALE_2000DPS) - gyro_bias_y;
    data->gz = ((float)gz_raw * BMI160_GYRO_SCALE_2000DPS) - gyro_bias_z;

    data->ax = (float)ax_raw * BMI160_ACCEL_SCALE_8G;
    data->ay = (float)ay_raw * BMI160_ACCEL_SCALE_8G;
    data->az = (float)az_raw * BMI160_ACCEL_SCALE_8G;

    return true;
}

void BMI160_Calibrate_Gyro(uint16_t samples)
{
    float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
    uint8_t raw[12];

    gyro_bias_x = 0.0f;
    gyro_bias_y = 0.0f;
    gyro_bias_z = 0.0f;

    for (uint16_t i = 0; i < samples; i++)
    {
        if (BMI160_ReadReg(BMI160_REG_DATA_0, raw, 12))
        {
            int16_t gx = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
            int16_t gy = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
            int16_t gz = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);

            sum_x += (float)gx * BMI160_GYRO_SCALE_2000DPS;
            sum_y += (float)gy * BMI160_GYRO_SCALE_2000DPS;
            sum_z += (float)gz * BMI160_GYRO_SCALE_2000DPS;
        }
        HAL_Delay(5); // Lấy mẫu mỗi 5ms
    }

    gyro_bias_x = sum_x / (float)samples;
    gyro_bias_y = sum_y / (float)samples;
    gyro_bias_z = sum_z / (float)samples;
}
