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

#define RAD_TO_DEG          57.295779513f
#define DEG_TO_RAD          0.0174532925f
#define COMP_FILTER_ALPHA   0.98f           // 98% Gyro + 2% Accel

/**
  * @brief  Khởi tạo bộ lọc bù góc nghiêng với giá trị ban đầu từ gia tốc kế
  */
void Filter_Init(float initial_angle);

/**
  * @brief  Cập nhật góc nghiêng qua bộ lọc bù (Complementary Filter)
  * @param  gyro_rate: Vận tốc góc Pitch (deg/s)
  * @param  acc_x:     Gia tốc trục X (g)
  * @param  acc_z:     Gia tốc trục Z (g)
  * @param  dt:        Chu kỳ lấy mẫu (giây, ví dụ: 0.005s)
  * @retval Góc nghiêng ước lượng Theta (độ)
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
