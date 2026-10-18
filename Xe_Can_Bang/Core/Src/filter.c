/**
  ******************************************************************************
  * @file    filter.c
  * @brief   Hiện thực hóa Bộ lọc bù Complementary Filter ước lượng góc nghiêng.
  ******************************************************************************
  */

#include "filter.h"
#include "balance_profile.h"
float Filter_CalibratedRawPitch(float x,float z) {
    x=(x-BALANCE_ACC_BX)/BALANCE_ACC_SX;
    z=(z-BALANCE_ACC_BZ)/BALANCE_ACC_SZ;
    return atan2f(x,z)*RAD_TO_DEG-BALANCE_RAW_ZERO_DEG;
}

static float estimated_angle = 0.0f;
static const float filter_alpha=COMP_FILTER_ALPHA_R12;
void Filter_SelectController(uint8_t controller) {
    (void)controller; /* Fixed R12 coefficient; no legacy fallback. */
}
float Filter_GetAlpha(void) { return filter_alpha; }

void Filter_Init(float initial_angle)
{
    estimated_angle = PITCH_ACC_SIGN * initial_angle;
}

float Filter_Complementary_Update(float gyro_rate, float acc_x, float acc_z, float dt)
{
    // 1. Tính góc nghiêng tĩnh từ gia tốc kế bằng hàm lượng giác atan2f
    // Khi xe đứng thẳng: acc_z ~ 1.0g, acc_x ~ 0.0g -> theta_acc ~ 0 độ
    float theta_acc = PITCH_ACC_SIGN * Filter_CalibratedRawPitch(acc_x, acc_z);

    // 2. Tích hợp bộ lọc bù:
    // Hệ số R12 cố định: 0.998.
    estimated_angle = filter_alpha * (estimated_angle + PITCH_GYRO_SIGN * gyro_rate * dt)
                    + (1.0f - filter_alpha) * theta_acc;

    return estimated_angle;
}

float Filter_GetAngle(void)
{
    return estimated_angle;
}
