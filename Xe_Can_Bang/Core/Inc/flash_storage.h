/**
  ******************************************************************************
  * @file    flash_storage.h
  * @brief   Lưu / nạp thông số PID + θ_trim + K_yaw vào Flash nội (Sector 7)
  ******************************************************************************
  */

#ifndef __FLASH_STORAGE_H__
#define __FLASH_STORAGE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
  * @brief  Nạp thông số đã lưu vào module PID (gọi sau PID_Init)
  * @retval true nếu Flash có dữ liệu hợp lệ, false nếu dùng mặc định
  */
bool FlashStorage_Load(void);

/**
  * @brief  Ghi thông số hiện tại vào Sector 7 (xóa cả sector, mất ~1-2 s, CPU đứng).
  *         CHỈ gọi khi xe không cân bằng và đã tắt ngắt điều khiển.
  * @retval true nếu ghi + xác minh thành công
  */
bool FlashStorage_Save(void);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_STORAGE_H__ */
