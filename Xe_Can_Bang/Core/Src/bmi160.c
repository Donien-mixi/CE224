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
static BMI160_RawData_t s_last_raw;
static bool calibration_ok;
static uint16_t read_errors, retry_ok;
static uint32_t last_error, last_error_ms;
bool BMI160_CalibrationOK(void) { return calibration_ok; }
void BMI160_GetHealth(uint32_t health[4]) {
    health[0]=read_errors;health[1]=retry_ok;health[2]=last_error;health[3]=last_error_ms;
}

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
static bool BMI160_ReadTimed(uint8_t reg,uint8_t *buffer,uint16_t len,uint32_t timeout)
{
    /* Single I2C owner. HAL otherwise waits25ms for BUSY even if timeout=2.
     * Never spend five control periods waiting for a stuck bus in TIM4. */
    HAL_StatusTypeDef result;
    /* A normal STOP may still be leaving the pins when HAL returns. Give it
     * up to20us, not the HAL's25ms stuck-BUSY wait. Iteration bound also covers
     * a disabled cycle counter during a diagnostic invocation. */
    uint32_t stop_wait=DWT->CYCCNT;
    for(unsigned i=0;HAL_I2C_GetState(&hi2c1)==HAL_I2C_STATE_READY&&
        __HAL_I2C_GET_FLAG(&hi2c1,I2C_FLAG_BUSY)&&i<2048U;i++){
        if((uint32_t)(DWT->CYCCNT-stop_wait)>=SystemCoreClock/1000000U*20U)break;
    }
    if(HAL_I2C_GetState(&hi2c1)!=HAL_I2C_STATE_READY||__HAL_I2C_GET_FLAG(&hi2c1,I2C_FLAG_BUSY)) {
        last_error=0x80000000U;result=HAL_BUSY;
    }else{
        result=HAL_I2C_Mem_Read(&hi2c1,s_bmi160_i2c_addr,reg,I2C_MEMADD_SIZE_8BIT,buffer,len,timeout);
        if(result==HAL_OK)return true;
        last_error=HAL_I2C_GetError(&hi2c1);
        if(!last_error)last_error=result==HAL_TIMEOUT?HAL_I2C_ERROR_TIMEOUT:0x80000000U;
    }
    if(read_errors<UINT16_MAX)read_errors++;
    last_error_ms=HAL_GetTick();
    return false;
}
static bool BMI160_ReadReg(uint8_t reg,uint8_t *buffer,uint16_t len){return BMI160_ReadTimed(reg,buffer,len,2);}

/* Only called by Init: startup or explicit CALIB, with motors stopped and
 * TIM4 paused. Open-drain bus clear cannot repair a broken wire or lost power. */
static bool BMI160_ResetBus(void){
    HAL_I2C_DeInit(&hi2c1);
    GPIO_InitTypeDef pins={0};
    pins.Pin=GPIO_PIN_8|GPIO_PIN_9;pins.Mode=GPIO_MODE_OUTPUT_OD;
    pins.Pull=GPIO_PULLUP;pins.Speed=GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8|GPIO_PIN_9,GPIO_PIN_SET);
    HAL_GPIO_Init(GPIOB,&pins);HAL_Delay(1);
    bool free=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)==GPIO_PIN_SET;
    for(unsigned i=0;free&&i<9&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9)==GPIO_PIN_RESET;i++){
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET);HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);HAL_Delay(1);
        free=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)==GPIO_PIN_SET;
    }
    if(free){
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8|GPIO_PIN_9,GPIO_PIN_RESET);HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET);HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET);HAL_Delay(1);
    }
    free=free&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_8)==GPIO_PIN_SET&&HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_9)==GPIO_PIN_SET;
    __HAL_RCC_I2C1_FORCE_RESET();__HAL_RCC_I2C1_RELEASE_RESET();
    bool initialized=HAL_I2C_Init(&hi2c1)==HAL_OK;
    return free&&initialized;
}

bool BMI160_Init(void)
{
    uint8_t chip_id = 0;
    calibration_ok=false;
    if(!BMI160_ResetBus())return false;

    /* Thử đọc Chip ID tối đa 5 lần kết hợp tự động khôi phục bus I2C */
    for (int retry = 0; retry < 5; retry++)
    {
        // 1. Thử địa chỉ 0x68 trước (khi SDO nối GND)
        s_bmi160_i2c_addr = (0x68 << 1);
        HAL_Delay(25);
        chip_id=0;
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
        if(!BMI160_ResetBus())return false;
    }

    if (chip_id != 0xD1)
    {
        return false; // Lỗi tiếp xúc hoặc chưa cấp nguồn cảm biến
    }

    // 2. Reset mềm chip để đưa về trạng thái sạch sẽ
    if(!BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_SOFT_RESET))return false;
    HAL_Delay(15);

    // 3. Đánh thức Accelerometer vào Normal Mode
    if(!BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_ACC_MODE_NORMAL))return false;
    HAL_Delay(5);

    // Bosch SensorAPI uses80ms for gyro suspend->normal; verify PMU afterward.
    if(!BMI160_WriteReg(BMI160_REG_CMD, BMI160_CMD_GYR_MODE_NORMAL))return false;
    HAL_Delay(85);

    // 5. Cấu hình Dải đo:
    // Gyro: +-2000 deg/s (Ghi 0x00 vào 0x43)
    if(!BMI160_WriteReg(BMI160_REG_GYR_RANGE, 0x00))return false;

    // Accel: +-8g (Ghi 0x08 vào 0x41)
    if(!BMI160_WriteReg(BMI160_REG_ACC_RANGE, 0x08))return false;

    // 0x28 = ODR 100Hz (0x08), normal bandwidth (0x20). Log 27/11: ODR 200 làm pitch/pwm dao động gấp đôi, REC1 16% -> quay lại 100Hz.
    if(!BMI160_WriteReg(BMI160_REG_ACC_CONF, 0x28)||!BMI160_WriteReg(BMI160_REG_GYR_CONF, 0x28))return false;
    uint8_t cfg[4],pmu;
    return BMI160_ReadReg(BMI160_REG_ACC_CONF,cfg,4)&&cfg[0]==0x28&&cfg[1]==8&&cfg[2]==0x28&&cfg[3]==0&&
        BMI160_ReadReg(0x03,&pmu,1)&&(pmu&0x3c)==0x14;
}

bool BMI160_Read_All(BMI160_Data_t *data)
{
    uint8_t raw_buf[12];
    uint32_t started=DWT->CYCCNT;

    // Đọc Burst 12 bytes liên tục: 6 bytes Gyro (Gx, Gy, Gz) + 6 bytes Accel (Ax, Ay, Az)
    if (!BMI160_ReadReg(BMI160_REG_DATA_0, raw_buf, 12))
    {
        /* One immediate retry ONLY for a short address/data NACK. No cached
         * sample, synthetic zero, bus reset or calibration inside the IRQ. */
        if(last_error!=HAL_I2C_ERROR_AF||
           (uint32_t)(DWT->CYCCNT-started)>SystemCoreClock/1000000U*750U||
           !BMI160_ReadTimed(BMI160_REG_DATA_0,raw_buf,12,1))return false;
        if(retry_ok<UINT16_MAX)retry_ok++;
    }

    // Ghép 2 bytes (LSB trước, MSB sau) thành số nguyên có dấu 16-bit
    int16_t gx_raw = (int16_t)(((uint16_t)raw_buf[1]  << 8) | raw_buf[0]);
    int16_t gy_raw = (int16_t)(((uint16_t)raw_buf[3]  << 8) | raw_buf[2]);
    int16_t gz_raw = (int16_t)(((uint16_t)raw_buf[5]  << 8) | raw_buf[4]);

    int16_t ax_raw = (int16_t)(((uint16_t)raw_buf[7]  << 8) | raw_buf[6]);
    int16_t ay_raw = (int16_t)(((uint16_t)raw_buf[9]  << 8) | raw_buf[8]);
    int16_t az_raw = (int16_t)(((uint16_t)raw_buf[11] << 8) | raw_buf[10]);
    s_last_raw = (BMI160_RawData_t){gx_raw, gy_raw, gz_raw, ax_raw, ay_raw, az_raw};

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
    uint16_t valid = 0;
    float sq[3]={0}; bool moved=false; calibration_ok=false;
    float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
    uint8_t raw[12];

    /* Hiệu chuẩn có giao dịch: lần đang động hoặc lỗi không được công bố bias giả
     * 6 °/s; ARM vẫn bị khoá khi hiệu chuẩn lỗi. */

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
            float rates[3]={gx*BMI160_GYRO_SCALE_2000DPS,gy*BMI160_GYRO_SCALE_2000DPS,gz*BMI160_GYRO_SCALE_2000DPS};
            for(unsigned j=0;j<3;j++) {sq[j]+=rates[j]*rates[j];if(fabsf(rates[j])>3.0f)moved=true;}
            valid++;
        }
        HAL_Delay(10); // ODR 100 Hz: tránh đếm trùng mẫu.
    }

    if (valid) {
        float means[3]={sum_x/valid,sum_y/valid,sum_z/valid};
        calibration_ok=valid==samples && samples>=200 && !moved;
        for(unsigned j=0;j<3;j++) if(sq[j]/valid-means[j]*means[j]>.04f)calibration_ok=false;
        if(calibration_ok){gyro_bias_x=means[0];gyro_bias_y=means[1];gyro_bias_z=means[2];}
    }
}

void BMI160_GetLastRaw(BMI160_RawData_t *raw) { *raw = s_last_raw; }
void BMI160_GetBias(float bias[3]) {
    bias[0] = gyro_bias_x; bias[1] = gyro_bias_y; bias[2] = gyro_bias_z;
}
bool BMI160_GetConfig(uint8_t config[6]) {
    config[0] = s_bmi160_i2c_addr >> 1;
    return BMI160_ReadReg(BMI160_REG_CHIP_ID, config + 1, 1) &&
           BMI160_ReadReg(BMI160_REG_ACC_CONF, config + 2, 4);
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
    printf("\r\n[3] Khoi tao BMI160 (ODR 100Hz, Accel +-8g, Gyro +-2000 dps)...\r\n");
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
