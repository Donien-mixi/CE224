/**
  ******************************************************************************
  * @file    bmi160.c
  * @brief   Hiện thực hóa Driver cảm biến quán tính Bosch BMI160.
  ******************************************************************************
  */

#include "bmi160.h"
#include "filter.h"
#include <stdio.h>
#include <math.h>

/* Biến lưu trữ giá trị bù trôi tĩnh con quay hồi chuyển */
static float gyro_bias_x = 0.0f;
static float gyro_bias_y = 0.0f;
static float gyro_bias_z = 0.0f;
static uint8_t s_bmi160_i2c_addr = (0x68 << 1);

/* Hàm phụ trợ ghi 1 byte vào thanh ghi I2C */
static bool BMI160_WriteReg(uint8_t reg, uint8_t value)
{
    if (HAL_I2C_Mem_Write(&hi2c1, s_bmi160_i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, &value, 1, 2) == HAL_OK)
    {
        return true;
    }
    return false;
}

/* Hàm phụ trợ đọc dữ liệu từ thanh ghi I2C */
static bool BMI160_ReadReg(uint8_t reg, uint8_t *buffer, uint16_t len)
{
    if (HAL_I2C_Mem_Read(&hi2c1, s_bmi160_i2c_addr, reg, I2C_MEMADD_SIZE_8BIT, buffer, len, 2) == HAL_OK)
    {
        return true;
    }
    return false;
}

bool BMI160_Init(void)
{
    uint8_t chip_id = 0;

    /* Thử đọc Chip ID tối đa 5 lần kết hợp tự động khôi phục bus I2C */
    for (int retry = 0; retry < 5; retry++)
    {
        // 1. Thử địa chỉ 0x68 trước (khi SDO nối GND)
        s_bmi160_i2c_addr = (0x68 << 1);
        HAL_Delay(25);
        BMI160_ReadReg(BMI160_REG_CHIP_ID, &chip_id, 1);

        // 2. Nếu không thấy 0xD1, thử tiếp địa chỉ 0x69 (khi SDO nối 3.3V)
        if (chip_id != 0xD1)
        {
            s_bmi160_i2c_addr = (0x69 << 1);
            BMI160_ReadReg(BMI160_REG_CHIP_ID, &chip_id, 1);
        }

        if (chip_id == 0xD1)
        {
            break; // Đã tìm thấy cảm biến Bosch BMI160 thành công!
        }

        // Nếu bus I2C bị nghẽn (cờ BUSY), thực hiện Reset mềm ngoại vi I2C1
        __HAL_RCC_I2C1_FORCE_RESET();
        HAL_Delay(5);
        __HAL_RCC_I2C1_RELEASE_RESET();
        HAL_I2C_Init(&hi2c1);
    }

    if (chip_id != 0xD1)
    {
        return false; // Lỗi tiếp xúc hoặc chưa cấp nguồn cảm biến
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

void BMI160_Test_Run(void)
{
    printf("\r\n=======================================================\r\n");
    printf("   CHUONG TRINH TEST BOSCH BMI160 TREN STM32 (115200)  \r\n");
    printf("   Cau hinh 6 chan: 3V3, GND, PB8(SCL), PB9(SDA), CS, SA0\r\n");
    printf("   Luu y: HOAN TOAN BO TRONG CHAN VIN                  \r\n");
    printf("=======================================================\r\n");

    // 1. Quét bus I2C1 tìm thiết bị
    printf("[1] Dang quet bus I2C1 (PB8-SCL, PB9-SDA)...\r\n");
    uint8_t found_addr = 0;
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (HAL_I2C_IsDeviceReady(&hi2c1, (addr << 1), 2, 5) == HAL_OK) {
            printf("  -> Tim thay thiet bi I2C tai dia chi: 0x%02X", addr);
            if (addr == 0x68) {
                printf(" [BOSCH BMI160 - SA0 noi GND]\r\n");
                found_addr = addr;
            } else if (addr == 0x69) {
                printf(" [BOSCH BMI160 - SA0 noi 3.3V]\r\n");
                found_addr = addr;
            } else {
                printf("\r\n");
            }
        }
    }

    if (found_addr == 0) {
        printf("\r\n[LOI] Khong tim thay thiet bi I2C nao!\r\n");
        printf("Kiem tra: 3V3->3.3V (BO TRONG VIN), GND, PB8->SCL, PB9->SDA, CS->3.3V, SA0->GND.\r\n");
        return;
    }

    // 2. Đọc thanh ghi CHIP_ID (0x00)
    printf("\r\n[2] Doc thanh ghi CHIP_ID (0x00)...\r\n");
    uint8_t chip_id = 0;
    s_bmi160_i2c_addr = (found_addr << 1);
    if (BMI160_ReadReg(BMI160_REG_CHIP_ID, &chip_id, 1)) {
        printf("  -> Gia tri Chip ID: 0x%02X\r\n", chip_id);
        if (chip_id == 0xD1) {
            printf("  -> [PASS] Chinh xac la cam bien Bosch BMI160!\r\n");
        } else {
            printf("  -> [CANH BAO] Chip ID khac 0xD1!\r\n");
        }
    }

    // 3. Khởi tạo chip
    printf("\r\n[3] Khoi tao BMI160 (ODR 200Hz, Accel +-8g, Gyro +-2000 dps)...\r\n");
    if (BMI160_Init()) {
        printf("  -> [PASS] Khoi tao thanh cong!\r\n");
    } else {
        printf("  -> [LOI] Khoi tao that bai!\r\n");
        return;
    }

    // 4. Hiệu chuẩn bù trôi tĩnh Gyro
    printf("\r\n[4] Hieu chuan Gyro Zero-Rate Bias (Dat yen xe trong 2s)...\r\n");
    HAL_Delay(500);
    BMI160_Calibrate_Gyro(400);
    printf("  -> [PASS] Hieu chuan xong!\r\n");
    printf("     Bias X = %+6.2f deg/s | Bias Y = %+6.2f deg/s | Bias Z = %+6.2f deg/s\r\n",
           gyro_bias_x, gyro_bias_y, gyro_bias_z);

    // 5. Khởi tạo bộ lọc bù
    BMI160_Data_t imu_init;
    BMI160_Read_All(&imu_init);
    float init_pitch = atan2f(imu_init.ax, imu_init.az) * RAD_TO_DEG;
    Filter_Init(init_pitch);
    printf("  -> Goc Pitch ban dau tu Accel: %.2f do\r\n", init_pitch);

    // 6. Vòng lặp đọc dữ liệu liên tục
    printf("\r\n[5] BAT DAU DOC DU LIEU REAL-TIME (Chu ky 100ms - 10Hz):\r\n");
    printf("Dinh dang: Accel [g] (X, Y, Z) | Gyro [deg/s] (X, Y, Z) | Pitch, Roll, Pitch_Filt\r\n");
    printf("---------------------------------------------------------------------------------\r\n");

    BMI160_Data_t imu;
    uint32_t count = 0;
    while (1) {
        if (BMI160_Read_All(&imu)) {
            count++;
            float pitch_acc = atan2f(imu.ax, imu.az) * RAD_TO_DEG;
            float roll_acc  = atan2f(imu.ay, imu.az) * RAD_TO_DEG;
            float pitch_filt = Filter_Complementary_Update(imu.gy, imu.ax, imu.az, 0.1f);

            printf("#%04lu | Acc[g]: %+5.2f %+5.2f %+5.2f | Gyro[dps]: %+6.1f %+6.1f %+6.1f | Pit: %+5.1f | Rol: %+5.1f | Filt: %+5.1f\r\n",
                   (unsigned long)count, imu.ax, imu.ay, imu.az, imu.gx, imu.gy, imu.gz, pitch_acc, roll_acc, pitch_filt);

            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        } else {
            printf("[CANH BAO] Mat ket noi I2C voi BMI160!\r\n");
        }
        HAL_Delay(100);
    }
}
