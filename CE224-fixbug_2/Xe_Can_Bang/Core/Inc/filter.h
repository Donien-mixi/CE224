/**
  ******************************************************************************
  * @file    filter.h
  * @brief   Bộ lọc số ước lượng góc nghiêng (Complementary Filter)
  *          và lọc thông thấp vận tốc (Low-Pass Filter).
  ******************************************************************************
  */

#ifndef __FILTER_H__
#define __FILTER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>
#include <stdint.h>

#define RAD_TO_DEG          57.295779513f
#define DEG_TO_RAD          0.0174532925f
#define COMP_FILTER_ALPHA_R7 0.98f
#define COMP_FILTER_ALPHA_R12 0.998f
void Filter_SelectController(uint8_t controller);
float Filter_GetAlpha(void);
/* Forward = battery WAGO. User observed raw accel pitch falls on forward tilt.
 * Control pitch is forward-positive. Raw gyro Y is retained; verify in tilt log. */
#define PITCH_ACC_SIGN      (-1.0f)
#define PITCH_GYRO_SIGN     (1.0f)

/**
  * @brief  Khởi tạo bộ lọc bù góc nghiêng với giá trị ban đầu từ gia tốc kế
  */
void Filter_Init(float initial_angle);
float Filter_CalibratedRawPitch(float raw_x, float raw_z);

/**
  * @brief  Cập nhật góc nghiêng qua bộ lọc bù (Complementary Filter)
  * @param  gyro_rate: Vận tốc góc Pitch (deg/s)
  * @param  acc_x:     Gia tốc trục X (g)
  * @param  acc_z:     Gia tốc trục Z (g)
  * @param  dt:        Chu kỳ lấy mẫu (giây, ví dụ: 0.005s)
  * @retval Góc nghiêng ước lượng Theta trong [-180, 180] độ; dt là thời gian thực
  */
float Filter_Complementary_Update(float gyro_rate, float acc_x, float acc_z, float dt);

/**
  * @brief  Lấy góc nghiêng hiện tại
  */
float Filter_GetAngle(void);

#ifdef __cplusplus
}
#endif

#endif /* __FILTER_H__ */
