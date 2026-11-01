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

static float WrapAngle(float angle) {
    angle = fmodf(angle, 360.0f);
    if (angle > 180.0f) angle -= 360.0f;
    if (angle < -180.0f) angle += 360.0f;
    return angle;
}

static float estimated_angle = 0.0f;
static const float filter_alpha=COMP_FILTER_ALPHA_R12;
void Filter_SelectController(uint8_t controller) {
    (void)controller; /* Fixed R12 coefficient; no legacy fallback. */
}
float Filter_GetAlpha(void) { return filter_alpha; }

void Filter_Init(float initial_angle)
{
    estimated_angle = WrapAngle(PITCH_ACC_SIGN * initial_angle);
}

float Filter_Complementary_Update(float gyro_rate, float acc_x, float acc_z, float dt)
{
    // 1. Tính góc nghiêng tĩnh từ gia tốc kế bằng hàm lượng giác atan2f
    // Khi xe đứng thẳng: acc_z ~ 1.0g, acc_x ~ 0.0g -> theta_acc ~ 0 độ
    float theta_acc = PITCH_ACC_SIGN * Filter_CalibratedRawPitch(acc_x, acc_z);

    if (!isfinite(gyro_rate) || !isfinite(theta_acc) || !isfinite(dt) || dt <= 0.0f)
        return estimated_angle;
    /* Keep the R12 response at 5 ms; use elapsed time when ticks are delayed.
     * Blend the shortest angular error across +/-180, not opposite headings. */
    float alpha = dt == 0.005f ? filter_alpha : powf(filter_alpha, dt / 0.005f);
    float predicted = WrapAngle(estimated_angle + PITCH_GYRO_SIGN * gyro_rate * dt);
    estimated_angle = WrapAngle(predicted + (1.0f-alpha) * WrapAngle(theta_acc-predicted));

    return estimated_angle;
}

float Filter_GetAngle(void)
{
    return estimated_angle;
}
