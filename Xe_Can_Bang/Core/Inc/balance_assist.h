#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "balance_profile.h"

/* Encoder-distance feedback, not an absolute heading sensor. Positive delta
 * means corrected right wheel has travelled farther: add to L, subtract R.
 * Applied AFTER direction-specific deadband; preserve common PWM exactly. */
typedef struct { float filtered_difference, correction; } BalanceHeading;
static inline void BalanceHeading_Reset(BalanceHeading *h) {
    h->filtered_difference=0;h->correction=0;
}
static inline bool BalanceHeading_Apply(BalanceHeading *h,float delta_m,float rate_mps,
                                        int16_t *left,int16_t *right,int max_pwm) {
    if(!isfinite(delta_m)||!isfinite(rate_mps))return false;
    h->filtered_difference=BALANCE_HEADING_BETA*h->filtered_difference+
        (1-BALANCE_HEADING_BETA)*rate_mps;
    float u=BALANCE_HEADING_KP*delta_m+BALANCE_HEADING_KD*h->filtered_difference;
    float cap=BALANCE_HEADING_LIMIT;
    cap=fminf(cap,max_pwm-fabsf((float)*left));cap=fminf(cap,max_pwm-fabsf((float)*right));
    cap=fmaxf(0,fminf(cap,fminf(fabsf((float)*left),fabsf((float)*right))-1));
    int correction=(int)lroundf(fmaxf(-cap,fminf(cap,u)));
    if((*left<=0&&*right>=0)||(*left>=0&&*right<=0))correction=0;
    *left+=(int16_t)correction;*right-=(int16_t)correction;
    h->correction=(float)correction;
    return true;
}
