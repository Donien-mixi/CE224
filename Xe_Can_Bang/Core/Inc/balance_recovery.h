#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#define BALANCE_RECOVERY_LIVE_COMMANDS 1
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
/* Latest fresh commands may assist a settled REC1, with slow speed/yaw limits
 * applied by balance_motion/turn. Startup and uncatchable attitude stay gated. */
static inline unsigned BalanceRecovery_DriveMode(const BalanceRecovery *s,bool eligible,
 float pitch,float gyro,float speed,float reference){
 if(!eligible||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed)||!isfinite(reference))return 0;
 if(!s->active)return 1;
 return fabsf(pitch)<=6.0f&&fabsf(gyro)<=25.0f&&fabsf(speed)<=.35f&&fabsf(reference)<=.35f?2U:3U;
}
static inline void BalanceRecovery_Impact(BalanceRecovery *s,bool allow,bool moved,
 float gyro,float norm2,float position,float budget,uint32_t now){
 if(!allow||!moved||s->active||!isfinite(gyro)||!isfinite(norm2)||!isfinite(position)||!isfinite(budget))return;
 if(fabsf(gyro)>250.0f&&(norm2>4.0f||norm2<.16f)){
  s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
  if(s->episodes<65535U)s->episodes++;
  s->distance_limit=fminf(1.0f,fmaxf(budget,fabsf(position)+.80f));
 }
}
/* RT27: the recovery travel allowance is back to0.8m past the entry offset (the
 * RT21 value). RT26 had cut it to0.35m and that lifted fault8 (relative tracking)
 * on pushes that were still upright and recoverable - log RT26(36) was cut at
 * pitch+13.8deg with |x-x_ref|=0.378m while the vehicle was already braking home.
 * A push can cross the ordinary0.8m/s ceiling before the0.35m/s tracking
 * trigger fires (50cm/s +30cm/s). Reserve early, only for real overspeed
 * relative to the reference, without extending an already active episode. */
static inline void BalanceRecovery_SpeedReserve(BalanceRecovery *s,bool allow,
 float speed,float reference,float position,float budget,float ordinary_limit,uint32_t now){
 if(!allow||s->active||!s->primed||!isfinite(speed)||!isfinite(reference)||
    !isfinite(position)||!isfinite(budget)||!isfinite(ordinary_limit))return;
 float error=speed-reference;
 if(fabsf(speed)>ordinary_limit-.08f&&fabsf(error)>.12f&&speed*error>0){
  s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
  if(s->episodes<65535U)s->episodes++;
  s->distance_limit=fminf(1.0f,fmaxf(budget,fabsf(position)+.80f));
 }
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
        /* A short commanded transient is not a sustained loss of tracking.
         * quiet is unused while inactive: reuse it for 60ms evidence, so the
         * native simulation state remains ABI-compatible.
         * RT8: a single 150 dps gyro spike is a suspension bump, not a loss of
         * control. Log27 showed 60 dps firing four times on rough ground with
         * pitch under5deg; the threshold is now above every observed bump. */
        bool suspect=fabsf(pitch)>=tilt_trigger||fabsf(speed)>=.35f;
        if(s->primed&&maneuver&&suspect){if(s->quiet<12U)s->quiet++;}
        else s->quiet=0;
        if(s->primed&&(fabsf(gyro)>=150.0f||
            (suspect&&(!maneuver||s->quiet>=12U)))){
            s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
            if(s->episodes<65535)s->episodes++;
            s->distance_limit=fminf(1.0f,fmaxf(base_budget,fabsf(position)+.80f));
        }
    }
    if(!s->active)return 0;
    bool quiet=fabsf(pitch)<=6.0f&&fabsf(gyro)<25.0f&&fabsf(speed)<.15f&&fabsf(position)<=base_budget;
    if(quiet)s->quiet++;else s->quiet=0;
    /* RT25: leaving REC1 must not blind the next disturbance. The episode only
     * ends after300ms of quiet, so the vehicle is settled and a second push may
     * re-arm instantly. Log RT25(3) fell because REC1 exited at21.42s and the
     * push arrived0.14s later, before the500ms stable dwell could re-prime it.
     * The4000ms episode timeout still bounds a recovery that never settles. */
    if(s->quiet>=60){s->active=false;s->stable=0;s->quiet=0;s->primed=true;return 0;}
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
 if(fabsf(reference)>.55f&&fabsf(speed)>.78f&&fabsf(error)>.04f&&speed*error>0){
  s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
  if(s->episodes<65535U)s->episodes++;
  s->distance_limit=fminf(1.0f,fmaxf(budget,fabsf(position)+.80f));
 }
}
