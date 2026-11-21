#pragma once
/* balance_recovery.h — REC1: phần KHAI BÁO + THÔNG SỐ.
 * Thân hàm nằm ở Core/Src/balance_recovery.c.
 *
 * REC1 là gì: khi xe bị đẩy, bị đập hoặc mất bám, lớp này bật một "cuộc lấy lại"
 * có giới hạn (nới góc lên 50°, tốc độ 1,20 m/s, đi thêm 1 m), rồi tự thoát khi
 * xe đã yên 300 ms. Nó không đổi luật lực R12.
 */
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "balance_interlock.h"

#define BALANCE_RECOVERY_LIVE_COMMANDS 1

/* ---- Thông số chỉnh được ---- */

/* Ngưỡng gyro để coi là bị đập (°/s). Dưới ngưỡng này chỉ là xóc ổ gà. */
#ifndef BALANCE_RECOVERY_GYRO_DPS
#define BALANCE_RECOVERY_GYRO_DPS 150.0f
#endif

/* Ngưỡng nghiêng khi ĐANG giữ lệnh. Đo trên log: một cú phanh bình thường ngả
 * tới 15,1°, nên để 16° để không vào REC1 mỗi lần phanh. */
#ifndef BALANCE_RECOVERY_TILT_MANEUVER
#define BALANCE_RECOVERY_TILT_MANEUVER 16.0f
#endif

/* Ngưỡng nghiêng khi KHÔNG có lệnh (bị đẩy lúc đứng yên): nhạy hơn. */
#ifndef BALANCE_RECOVERY_TILT_IDLE
#define BALANCE_RECOVERY_TILT_IDLE 8.0f
#endif

/* Hạn một cuộc REC1 (ms). Đủ dài cho một lần bị giữ tay; 50°/1,0 m/interlock
 * vẫn là các chốt độc lập. */
#ifndef BALANCE_RECOVERY_TIMEOUT_MS
#define BALANCE_RECOVERY_TIMEOUT_MS 8000U
#endif

/* Bằng chứng vượt tốc độ phải duy trì bao nhiêu nhịp 5 ms (20 = 100 ms). */
#ifndef BALANCE_RECOVERY_OVERSPEED_TICKS
#define BALANCE_RECOVERY_OVERSPEED_TICKS 20U
#endif

/* Chỉ dùng cho test: >0 để hạ ngưỡng vượt tốc độ xuống dưới trần (mô phỏng luật cũ). */
#ifndef BALANCE_RECOVERY_OVERSPEED_SLACK
#define BALANCE_RECOVERY_OVERSPEED_SLACK 0.0f
#endif

/* Chỉ dùng cho test: 1 = vào REC1 ngay khi có lệnh (mô phỏng luật trước RT38). */
#ifndef BALANCE_RECOVERY_IMMEDIATE_MANEUVER
#define BALANCE_RECOVERY_IMMEDIATE_MANEUVER 0
#endif

/* ---- Trạng thái một cuộc REC1 ---- */
typedef struct {
    uint32_t started_ms;   /* lúc bắt đầu, để tính hạn 8 s */
    uint16_t stable;       /* số nhịp xe đã yên (đủ 100 thì "primed") */
    uint16_t quiet;        /* số nhịp đã yên trong REC1 (đủ 60 thì thoát) */
    uint16_t episodes;     /* số lần đã vào REC1 từ lúc ARM */
    uint16_t overspeed;    /* số nhịp vượt trần liên tiếp */
    bool primed;           /* đã có mốc an toàn để bật REC1 */
    bool active;           /* đang trong REC1 */
    float distance_limit;  /* quãng được đi trong cuộc này (m) */
} BalanceRecovery;

static inline void BalanceRecovery_Clear(BalanceRecovery *s) { *s = (BalanceRecovery){0}; }

/* ---- Hàm ---- */
void     BalanceRecovery_Impact(BalanceRecovery *s, bool allow, bool moved,
                                float gyro, float norm2, float position, float budget, uint32_t now);
bool     BalanceRecovery_OverSpeed(BalanceRecovery *s, bool allow, float speed,
                                   float reference, float position, float budget,
                                   float envelope, uint32_t now);
uint16_t BalanceRecovery_UpdateMode(BalanceRecovery *s, bool allow, float pitch, float gyro,
                                    float speed, float position, float base_budget,
                                    uint32_t now, bool maneuver);
uint16_t BalanceRecovery_Update(BalanceRecovery *s, bool allow, float pitch, float gyro,
                                float speed, float position, float base_budget, uint32_t now);
unsigned BalanceRecovery_DriveMode(const BalanceRecovery *s, bool eligible,
                                   float pitch, float gyro, float speed, float reference);
float    BalanceRecovery_Angle(const BalanceRecovery *s, float ordinary);
float    BalanceRecovery_Speed(const BalanceRecovery *s, float ordinary);
float    BalanceRecovery_Distance(const BalanceRecovery *s, float ordinary);
