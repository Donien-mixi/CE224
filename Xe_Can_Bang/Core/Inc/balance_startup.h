#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "balance_profile.h"
/* Supervisor only: never changes frozen R12 force/observer/heading law. */
typedef struct {
    float initial_angle, budget, angle_limit;
    uint32_t started_ms;
    uint16_t quiet;
    bool active;
} BalanceStartup;
static inline float BalanceStartup_Budget(uint8_t profile,float angle) {
    if(profile!=BALANCE_CONTROLLER_R12)return BALANCE_STOP_DISTANCE;
    return fminf(1.0f,fmaxf(.30f,.30f+.014f*(fabsf(angle)-10.0f)));
}
static inline void BalanceStartup_Clear(BalanceStartup *s) {
    *s=(BalanceStartup){.budget=BALANCE_STOP_DISTANCE,.angle_limit=BALANCE_STOP_ANGLE};
}
static inline void BalanceStartup_Begin(BalanceStartup *s,uint8_t profile,float pitch,uint32_t now) {
    BalanceStartup_Clear(s);s->initial_angle=pitch;s->started_ms=now;
    s->budget=BalanceStartup_Budget(profile,pitch);
    s->active=profile==BALANCE_CONTROLLER_R12&&fabsf(pitch)>20.0f;
    s->angle_limit=s->active?fminf(75.0f,fmaxf(25.0f,fabsf(pitch)+5.0f)):BALANCE_STOP_ANGLE;
}
static inline uint32_t BalanceStartup_TimeoutMs(const BalanceStartup *s) {
    return fabsf(s->initial_angle)>BALANCE_EXTREME_CAPTURE_ANGLE?BALANCE_EXTREME_CAPTURE_TIMEOUT_MS:BALANCE_CAPTURE_TIMEOUT_MS;
}
static inline uint16_t BalanceStartup_Update(BalanceStartup *s,float pitch,float gyro,float speed,uint32_t now) {
    if(!s->active)return 0;
    if(fabsf(pitch)<=8.0f&&fabsf(gyro)<25.0f&&fabsf(speed)<.15f)s->quiet++;
    else s->quiet=0;
    if(s->quiet>=40){s->active=false;s->angle_limit=BALANCE_STOP_ANGLE;return 0;}
    return (uint32_t)(now-s->started_ms)>=BalanceStartup_TimeoutMs(s)?2048U:0;
}
static inline float BalanceStartup_SpeedLimit(const BalanceStartup *s) {
    if(!s->active)return BALANCE_STOP_SPEED;
    if(fabsf(s->initial_angle)>BALANCE_EXTREME_CAPTURE_ANGLE)return BALANCE_EXTREME_CAPTURE_SPEED;
    return fabsf(s->initial_angle)>BALANCE_LARGE_CAPTURE_ANGLE?BALANCE_LARGE_CAPTURE_SPEED:BALANCE_CAPTURE_SPEED;
}
static inline void BalanceStartup_Stop(BalanceStartup *s) {
    s->active=false;s->quiet=0;s->angle_limit=BALANCE_STOP_ANGLE;
}
