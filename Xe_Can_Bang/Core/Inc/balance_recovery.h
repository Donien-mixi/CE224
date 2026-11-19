#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include "balance_interlock.h"
#define BALANCE_RECOVERY_LIVE_COMMANDS 1
/* RT38: a commanded reversal spikes the gyro; while a fresh maneuver is held
 * this needs60ms of evidence, with no command it still trips immediately. */
#ifndef BALANCE_RECOVERY_GYRO_DPS
#define BALANCE_RECOVERY_GYRO_DPS 150.0f
#endif
/* Probe switch:1 restores the immediate trigger for the host test. */
#ifndef BALANCE_RECOVERY_IMMEDIATE_MANEUVER
#define BALANCE_RECOVERY_IMMEDIATE_MANEUVER 0
#endif
/* Temporary disturbance envelope only. Never changes the R12 force/yaw law.
 * Call at200Hz. Inhibit during startup/first ARM/non-balancing states. */
typedef struct {
    uint32_t started_ms;
    uint16_t stable,quiet,episodes;
    uint16_t overspeed;   /* RT39:100ms evidence for the overspeed reserve */
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
/* RT39: the overspeed reserve compares the axle with the CEILING the speed
 * interlock protects, not with the instantaneous velocity reference.
 *
 * Log RT38 (39) - preset80, 84s, all four turn directions - has12 REC1 episodes
 * for25.3s (30% of the run). Six of them were entered inside a COMMANDED
 * manoeuvre at0.77-0.79m/s against a0.749 corner reference (error0.04) or right
 * after the reference was released (error0.12): the old0.04/0.12 margins put the
 * guard below the normal operating band, so every corner entry and every release
 * read as an uncommanded overspeed. REC1 then braked0.78->0 for1.9-2.8s, in all
 * four directions, which is exactly the hesitation the driver reported. Above the
 * real ceiling (max(envelope,|v_ref|+0.12)) the axle appears in only TWO samples
 * of that whole log, both already inside an active REC1.
 *
 * The guard keeps its job - enter the bounded recovery (angle50deg, speed1.20,
 * +0.8m) BEFORE BalanceStop(16) cuts the motors on an upright vehicle - because
 * its100ms evidence is shorter than the interlock's200ms trip, and because REC1
 * raises the interlock ceiling to1.20 on the same tick. It needs no lean gate: a
 * straight runaway on a flat body is exactly the case the interlock gate missed. */
#ifndef BALANCE_RECOVERY_OVERSPEED_TICKS
#define BALANCE_RECOVERY_OVERSPEED_TICKS 20U /* 100ms at200Hz, trip is200ms */
#endif
/* Probe hook only: a positive slack restores the pre-RT39 behaviour of guarding
 * a band BELOW the ceiling, so a host test can prove it catches that regression. */
#ifndef BALANCE_RECOVERY_OVERSPEED_SLACK
#define BALANCE_RECOVERY_OVERSPEED_SLACK 0.0f
#endif
static inline bool BalanceRecovery_OverSpeed(BalanceRecovery *s,bool allow,float speed,
 float reference,float position,float budget,float envelope,uint32_t now){
 if(!isfinite(speed)||!isfinite(reference)||!isfinite(position)||!isfinite(budget)||
    !isfinite(envelope)){s->overspeed=0U;return false;}
 if(!allow||s->active){s->overspeed=0U;return false;}
 float ceiling=BalanceInterlock_Ceiling(envelope,reference)-BALANCE_RECOVERY_OVERSPEED_SLACK;
 float error=speed-reference;
 /* Same sign as the reference: an axle coasting the wrong way is a settling
  * transient the R12 loop corrects, not a runaway. */
 if(fabsf(speed)>ceiling&&speed*error>0.0f){
  if(s->overspeed<60000U)s->overspeed++;
 }else s->overspeed=0U;
 if(s->overspeed<BALANCE_RECOVERY_OVERSPEED_TICKS)return false;
 s->overspeed=0U;s->active=true;s->primed=false;s->quiet=0;s->started_ms=now;
 if(s->episodes<65535U)s->episodes++;
 s->distance_limit=fminf(1.0f,fmaxf(budget,fabsf(position)+.80f));
 return true;
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
        /* RT38: a commanded reversal produces a real150-250dps gyro spike at
         * the axle. Treating that as an impact put the car into REC1 for1.2s
         * right after every direction change (log (2)(5):52% phase2 in the
         * second after -0.9 -> +0.9m/s, speed never returned to0.5 within1.2s).
         * While a fresh maneuver is held, the gyro spike now needs the same
         *60ms evidence as the lean path; with no command it still trips at
         * once, so a real push is unchanged. */
        bool gyro_spike=fabsf(gyro)>=BALANCE_RECOVERY_GYRO_DPS;
        if(s->primed&&maneuver&&(suspect||gyro_spike)){if(s->quiet<12U)s->quiet++;}
        else s->quiet=0;
#if BALANCE_RECOVERY_IMMEDIATE_MANEUVER
        bool trigger=s->primed&&(gyro_spike||suspect); /* probe: old behaviour */
#else
        bool trigger=s->primed&&(!maneuver?(gyro_spike||suspect):
                                  ((suspect||gyro_spike)&&s->quiet>=12U));
#endif
        if(trigger){
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

/* RT39: the separate `BalanceRecovery_Reserve` (|v_ref|>0.55, |v|>0.78,
 * |v-v_ref|>0.04, gated on |pitch|>5 or |gyro|>60) is removed. It was the same
 * guard as above with a smaller margin and a lean gate, and the lean gate could
 * not stop a straight-line runaway on a flat body. One ceiling-based guard now
 * covers both, so there is a single place where "is this a disturbance" is
 * decided. */
