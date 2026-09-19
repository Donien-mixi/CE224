/**
  ******************************************************************************
  * @file    filter.c
  * @brief   Hiện thực hóa Bộ lọc bù Complementary Filter ước lượng góc nghiêng.
  ******************************************************************************
  */

#include "filter.h"

static float estimated_angle = 0.0f;

void Filter_Init(float initial_angle)
{
    estimated_angle = initial_angle;
}

float Filter_Complementary_Update(float gyro_rate, float acc_x, float acc_z, float dt)
{
    // 1. Tính góc nghiêng tĩnh từ gia tốc kế bằng hàm lượng giác atan2f
    // Khi xe đứng thẳng: acc_z ~ 1.0g, acc_x ~ 0.0g -> theta_acc ~ 0 độ
    float theta_acc = atan2f(acc_x, acc_z) * RAD_TO_DEG;

    // 2. Tích hợp bộ lọc bù:
    // Lấy 98% đáp ứng tức thời từ tích phân Gyro + 2% giá trị giữ chuẩn từ Accel
    estimated_angle = COMP_FILTER_ALPHA * (estimated_angle + gyro_rate * dt)
                    + (1.0f - COMP_FILTER_ALPHA) * theta_acc;

    return estimated_angle;
}

float Filter_GetAngle(void)
{
    return estimated_angle;
}
