#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
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
static inline uint16_t BalanceRecovery_Update(BalanceRecovery *s,bool allow,
    float pitch,float gyro,float speed,float position,float base_budget,uint32_t now) {
    if(!allow){BalanceRecovery_Clear(s);return 0;}
    if(!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed)||!isfinite(position)||!isfinite(base_budget))return 512U;
    if(!s->active){
        bool stable=fabsf(pitch)<=8.0f&&fabsf(gyro)<25.0f&&fabsf(speed)<.15f;
        if(stable){if(s->stable<100)s->stable++;}else s->stable=0;
        if(s->stable>=100)s->primed=true;
        if(s->primed&&(fabsf(pitch)>=8.0f||fabsf(gyro)>=60.0f||fabsf(speed)>=.35f)){
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
static inline float BalanceRecovery_Angle(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,50.0f):ordinary;
}
static inline float BalanceRecovery_Speed(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,1.20f):ordinary;
}
static inline float BalanceRecovery_Distance(const BalanceRecovery *s,float ordinary) {
    return s->active?fmaxf(ordinary,s->distance_limit):ordinary;
}
