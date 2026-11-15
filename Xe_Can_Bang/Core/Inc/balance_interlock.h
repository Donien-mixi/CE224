#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "balance_profile.h"
/* RT23 speed interlock. Supervisory only: never changes the frozen R12 force /
 * observer / allocator law and never edits a measurement.
 *
 * Why this is not a one-sample trip any more. In every RT22 log that fell, the
 * axle crossed the ordinary0.80m/s ceiling exactly while the R12 loop was
 * catching a lean or braking a released turn - the wheels are supposed to race
 * there. BalanceStop(16) then zeroed both PWM and the body fell:
 *   log RT23-8DIR preset72a t=38.94s: turn released, axle0.74->0.795m/s,
 *     fault16 at pitch2.0deg, PWM0/0, then pitch0->90deg in0.2s;
 *   log RT23-8DIR preset72b t=33.70s: same, pitch1.8deg ->105deg in0.24s;
 *   straight-run screenshot: fault16 with PWM0/0 at pitch-13.6deg.
 * A real runaway is still caught: the ceiling now follows the command
 * (max(envelope, |v_ref|+0.12) because preset72 legitimately rides0.72-0.79),
 * 200ms of continuous overspeed stops, and anything above1.45m/s stops at once. */
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
