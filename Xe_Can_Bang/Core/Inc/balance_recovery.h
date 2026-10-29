#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "balance_tuning.h"
/* Temporary disturbance envelope only. Never changes the R12 force/yaw law.
 * Call at200Hz. Inhibit during startup/first ARM/non-balancing states. */
typedef struct {
    uint32_t started_ms;
    uint16_t stable,quiet,episodes;
    bool primed,active;
    float distance_limit;
} BalanceRecovery;
static inline void BalanceRecovery_Clear(BalanceRecovery *s) {
    *s=(BalanceRecovery){0};
}
static inline uint16_t BalanceRecovery_UpdateMode(BalanceRecovery *s,bool allow,
    float pitch,float gyro,float speed,float position,float base_budget,uint32_t now,bool maneuver) {
    if(!allow){BalanceRecovery_Clear(s);return 0;}
    if(!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed)||!isfinite(position)||!isfinite(base_budget))return 512U;
    if(!s->active){
        bool stable=fabsf(pitch)<=8.0f&&fabsf(gyro)<25.0f&&fabsf(speed)<.15f;
        if(stable){if(s->stable<100)s->stable++;}else s->stable=0;
        if(s->stable>=100)s->primed=true;
        /* Log26: commanded reverse reaches8deg at modest gyro/speed error.
         * This is also a normal balance transient, not by itself an impact.
         * Only an acknowledged, fresh maneuver gets12deg; idle remains8deg.
         * Gyro/speed triggers and all recovery/interlock limits are unchanged. */
        float tilt_trigger=maneuver?12.0f:8.0f;
        if(s->primed&&(fabsf(pitch)>=tilt_trigger||fabsf(gyro)>=60.0f||fabsf(speed)>=.35f)){
            s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
            if(s->episodes<65535)s->episodes++;
            s->distance_limit=fminf(1.0f,fmaxf(base_budget,fabsf(position)+.80f));
        }
    }
    if(!s->active)return 0;
    bool quiet=fabsf(pitch)<=6.0f&&fabsf(gyro)<25.0f&&fabsf(speed)<.15f&&fabsf(position)<=base_budget;
    if(quiet)s->quiet++;else s->quiet=0;
    if(s->quiet>=60){s->active=false;s->stable=0;s->quiet=0;return 0;}
    return (uint32_t)(now-s->started_ms)>=4000U?4096U:0;
}
static inline uint16_t BalanceRecovery_Update(BalanceRecovery *s,bool allow,
    float pitch,float gyro,float speed,float position,float base_budget,uint32_t now) {
    return BalanceRecovery_UpdateMode(s,allow,pitch,gyro,speed,position,base_budget,now,false);
}
static inline float BalanceRecovery_Angle(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,50.0f):ordinary;
}
static inline float BalanceRecovery_Speed(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,1.20f):ordinary;
}
static inline float BalanceRecovery_Distance(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,s->distance_limit):ordinary;
}

/* Enter the existing bounded recovery before the ordinary speed interlock.
 * Both velocities are measured/reference values; no sensor is rewritten. */
static inline void BalanceRecovery_Reserve(BalanceRecovery *s,bool allow,float speed,float reference,
 float position,float budget,uint32_t now){
 if(!allow||s->active||!s->primed||!isfinite(speed)||!isfinite(reference)||
    !isfinite(position)||!isfinite(budget))return;
 float error=speed-reference;
 if(fabsf(reference)>BAL_TUNE_RESERVE_REFERENCE_MPS&&
    fabsf(speed)>BAL_TUNE_RESERVE_SPEED_MPS&&
    fabsf(error)>BAL_TUNE_RESERVE_ERROR_MPS&&speed*error>0){
  s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
  if(s->episodes<65535U)s->episodes++;
  s->distance_limit=fminf(1.0f,fmaxf(budget,fabsf(position)+.80f));
 }
}
