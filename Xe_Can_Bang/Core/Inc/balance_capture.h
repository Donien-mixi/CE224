#pragma once
#include <math.h>
#include <stdbool.h>
#include "balance_profile.h"
/* Preserve R7/R9 for |pitch|<=3deg; smoothly strengthen angle P and gyro D
 * outside it. Constant-rate start lookahead is not a reachability proof. */
static inline float BalanceCapture_Gain(float pitch_deg) {
    float fraction=(fabsf(pitch_deg)-BALANCE_CAPTURE_BEGIN)/
        (BALANCE_CAPTURE_FULL-BALANCE_CAPTURE_BEGIN);
    return 1+(BALANCE_CAPTURE_MAX_GAIN-1)*fmaxf(0,fminf(1,fraction));
}
static inline bool BalanceCapture_StartAngleOK(float pitch_deg,float gyro_dps) {
    return isfinite(pitch_deg)&&isfinite(gyro_dps)&&fabsf(pitch_deg)<=BALANCE_ARM_ANGLE &&
        fabsf(pitch_deg+BALANCE_ARM_LOOKAHEAD_S*gyro_dps)<=BALANCE_ARM_ANGLE;
}
/* Evaluated while motors are braked: a settling check, not an independent
 * measurement of the physical balance angle. */
static inline bool BalanceCapture_Settled(float pitch_deg,float gyro_dps,
                                         float acc_pitch_deg,float left,float right) {
    return BalanceCapture_StartAngleOK(pitch_deg,gyro_dps) && isfinite(acc_pitch_deg) &&
        isfinite(left)&&isfinite(right) && fabsf(pitch_deg-acc_pitch_deg)<=BALANCE_ARM_ACC_AGREEMENT &&
        fabsf(left)<BALANCE_ARM_WHEEL_SPEED && fabsf(right)<BALANCE_ARM_WHEEL_SPEED;
}
