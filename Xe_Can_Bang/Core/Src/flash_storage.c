/**
  ******************************************************************************
  * @file    flash_storage.c
  * @brief   Lưu thông số tune vào Sector 7 (0x08060000, 128 KB) của STM32F411
  ******************************************************************************
  */

#include "flash_storage.h"
#include "main.h"
#include "pid.h"
#include <string.h>

#define FLASH_STORAGE_ADDR   0x08060000UL
#define FLASH_STORAGE_MAGIC  0x54574950UL   /* "TWIP" */

/* Kích thước là bội số của 4 byte (ghi theo word) */
typedef struct {
    uint32_t magic;
    float    kp1;
    float    kd1;
    float    kp2;
    float    ki2;
    float    trim;
    float    k_yaw;
    uint32_t checksum;   /* XOR toàn bộ các word phía trước */
} Stored_Params_t;

static uint32_t Checksum(const Stored_Params_t *p)
{
    const uint32_t *w = (const uint32_t *)p;
    uint32_t x = 0xA5A5A5A5UL;
    for (uint32_t i = 0; i < (sizeof(Stored_Params_t) / 4U) - 1U; i++) {
        x ^= w[i];
    }
    return x;
}

bool FlashStorage_Load(void)
{
    const Stored_Params_t *p = (const Stored_Params_t *)FLASH_STORAGE_ADDR;

    if (p->magic != FLASH_STORAGE_MAGIC || p->checksum != Checksum(p)) {
        return false;
    }
    /* Từ chối giá trị rác (NaN / âm) dù checksum đúng */
    if (!(p->kp1 >= 0.0f) || !(p->kd1 >= 0.0f) || !(p->kp2 >= 0.0f) || !(p->ki2 >= 0.0f)) {
        return false;
    }

    PID_SetParams(p->kp1, p->kd1, p->kp2, p->ki2);
    PID_SetTrim(p->trim);
    PID_SetYawGain(p->k_yaw);
    return true;
}

bool FlashStorage_Save(void)
{
    Stored_Params_t d;
    PID_GetParams(&d.kp1, &d.kd1, &d.kp2, &d.ki2);
    d.magic = FLASH_STORAGE_MAGIC;
    d.trim  = PID_GetTrim();
    d.k_yaw = PID_GetYawGain();
    d.checksum = Checksum(&d);

    bool ok = true;

    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,   /* VDD 2.7-3.6V -> ghi theo word */
        .Sector       = FLASH_SECTOR_7,
        .NbSectors    = 1
    };
    uint32_t sector_error = 0;
    if (HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK) {
        ok = false;
    }

    if (ok) {
        const uint32_t *w = (const uint32_t *)&d;
        for (uint32_t i = 0; i < sizeof(d) / 4U; i++) {
            if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                  FLASH_STORAGE_ADDR + i * 4U, w[i]) != HAL_OK) {
                ok = false;
                break;
            }
        }
    }
    HAL_FLASH_Lock();

    /* Xác minh bằng cách đọc lại */
    return ok && (memcmp((const void *)FLASH_STORAGE_ADDR, &d, sizeof(d)) == 0);
}
