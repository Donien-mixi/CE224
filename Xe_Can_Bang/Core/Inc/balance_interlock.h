#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "balance_profile.h"
/* Giám sát tốc độ. Chỉ lớp giám sát: không đổi luật lực R12, observer, phân bổ
 * hay số đo.
 *
 * Vì sao không cắt theo một mẫu: trục vượt trần 0,80 m/s đúng lúc vòng R12 đang
 * bắt nghiêng hoặc đang phanh cua — bánh phải quay nhanh ở đó. Cắt cả hai PWM
 * lúc ấy chính là làm xe ngã. Vì vậy trần đi theo lệnh (max(envelope,
 * |v_ref|+0,12)), cần 200 ms vượt liên tục mới cắt, và trên 1,45 m/s cắt ngay. */
typedef struct {
    uint16_t ticks;
} BalanceInterlock;
static inline void BalanceInterlock_Clear(BalanceInterlock *g) {
    g->ticks = 0U;
}
static inline float BalanceInterlock_Ceiling(float envelope, float v_ref) {
    if (!isfinite(envelope) || !isfinite(v_ref)) return 0.0f;
    float track = fabsf(v_ref) + BALANCE_SPEED_TRACK_MARGIN_MPS;
    return envelope > track ? envelope : track;
}
/* Call once per200Hz tick. true = the supervisor must stop with fault16. */
static inline bool BalanceInterlock_Step(BalanceInterlock *g, float speed,
    float wheel_left, float wheel_right, float envelope, float v_ref, bool max_envelope) {
    if (!isfinite(speed) || !isfinite(wheel_left) || !isfinite(wheel_right)) return true;
    bool over = fabsf(speed) > BalanceInterlock_Ceiling(envelope, v_ref);
    if (max_envelope && (fabsf(wheel_left) > 1.20f || fabsf(wheel_right) > 1.20f)) over = true;
    if (over) { if (g->ticks < 60000U) g->ticks++; }
    else g->ticks = 0U;
    return fabsf(speed) > BALANCE_SPEED_OVERRUN_MPS || g->ticks >= BALANCE_SPEED_TRIP_TICKS;
}
