#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#define BALANCE_GOVERNOR_EQUILIBRIUM 1
/* RT22: the neutral posture of this car is ~+1.5deg nose-down (measured at rest)
 * and a normal 0.8m/s forward ramp leans to+6..+9deg. A5deg absolute threshold
 * therefore fired on ordinary acceleration:70% of forward samples in log RT21
 * had pressure and v_ref froze for0.5-4.8s, while reverse (leaning the other
 * way) tripped3.7% of the time. The lean rule now guards a genuine loss of
 * headroom at9deg (11 while launching); duty>0.90 and the tracking-lag rule are
 * unchanged and remain the primary physical limits. */
/* RT37 (log (2)(5), 18/11): the car leans7-12deg in the first2s of every
 * launch. The old9deg/11deg thresholds therefore tripped pressure for25-44% of
 * that window and the ramp dropped to the0.06 crawl in bursts - the operator's
 * "ngap ngung". The lean thresholds move to10deg normal /13deg launch (the stop
 * guard is25deg, the turn taper8-12deg, so there is still margin). Sim at MAX90
 * stays11/12 (only the11.2V case), and a0.45ramp experiment made it worse
 * (9-10/12), so the ramp itself is unchanged. */
#ifndef BALANCE_GOV_LEAN_DEG
#define BALANCE_GOV_LEAN_DEG 10.0f
#endif
#ifndef BALANCE_GOV_LEAN_LAUNCH_DEG
#define BALANCE_GOV_LEAN_LAUNCH_DEG 13.0f
#endif
#ifndef BALANCE_GOV_CLEAR_DEG
#define BALANCE_GOV_CLEAR_DEG 5.0f
#endif
#ifndef BALANCE_GOV_CLEAR_GYRO
#define BALANCE_GOV_CLEAR_GYRO 30.0f
#endif
#ifndef BALANCE_GOV_LAUNCH_GYRO
#define BALANCE_GOV_LAUNCH_GYRO 45.0f
#endif
#ifndef BALANCE_GOV_CRAWL
#define BALANCE_GOV_CRAWL 0.06f
#endif
/* RT36: a commanded release brakes at up to0.85m/s^2, not0.65. Log spd90
 * (290.99s) and rev_turn (976.35s) release from0.76-0.92m/s and take1.7-1.8s /
 * ~1m to stop, felt as "does not stop when the command is released" and as
 * reverse drift. The reference ramp is still bounded by motion's0.90 decel and
 * the2.5m/s^3 jerk; only the governor's extra cap moves. */
#ifndef BALANCE_GOV_BRAKE_CAP
#define BALANCE_GOV_BRAKE_CAP .85f
#endif
#ifndef BALANCE_GOV_REST_DWELL
#define BALANCE_GOV_REST_DWELL 100U
#endif
#ifndef BALANCE_GOV_REST_SAMPLES
#define BALANCE_GOV_REST_SAMPLES 60U
#endif
/* Feedback reference shaper, NOT an invariant-set / formally certified RG.
 * No gains, measurements or operator commands are changed. Called at200Hz. */
typedef struct {
    float acceleration,duty,speed,equilibrium,rest_sum;
    uint16_t cool,saturated,rest_dwell,rest_samples;
    bool equilibrium_ready;
    bool pressure,limited;
} BalanceGovernor;
static inline void BalanceGovernor_Reset(BalanceGovernor *g){*g=(BalanceGovernor){0};}
/* RT22: a STOP must not forget the car's neutral posture. It is clamped to
 * +-2deg, re-measured at rest, and never used for control or ARM admission. */
static inline void BalanceGovernor_ResetKeepEquilibrium(BalanceGovernor *g){
 float e=g->equilibrium;bool r=g->equilibrium_ready;
 *g=(BalanceGovernor){0};g->equilibrium=e;g->equilibrium_ready=r;
}
/* Learn only the headroom predictor's neutral posture from self-balanced rest.
 * Never changes CF, IMU calibration, force gains or hard angle protection.
 * Initial0.5s dwell plus60 valid samples (RT22: was1s/100); motion freezes the
 * last accepted value. */
static inline void BalanceGovernor_ObserveRest(BalanceGovernor *g,bool eligible,
 float pitch,float gyro,float speed,float yaw_rate){
 if(!eligible||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed)||!isfinite(yaw_rate)||
    fabsf(pitch)>8||fabsf(gyro)>60||fabsf(speed)>.10f||fabsf(yaw_rate)>.4f){
  g->rest_dwell=g->rest_samples=0;g->rest_sum=0;return;
 }
 if(g->rest_dwell<BALANCE_GOV_REST_DWELL){g->rest_dwell++;return;}
 if(fabsf(pitch)>6||fabsf(gyro)>25||fabsf(speed)>.04f||fabsf(yaw_rate)>.15f)return;
 g->rest_sum+=pitch;g->rest_samples++;
 if(g->rest_samples>=BALANCE_GOV_REST_SAMPLES){
  float mean=g->rest_sum/g->rest_samples;
  g->equilibrium=fabsf(mean)<.50f?0:fmaxf(-2.0f,fminf(2.0f,mean));
  g->equilibrium_ready=true;g->rest_samples=0;g->rest_sum=0;
 }
}
static inline bool BalanceGovernor_ReserveNeeded(float pitch,float gyro){
    return fabsf(pitch)>5.0f||fabsf(gyro)>60.0f;
}
static inline void BalanceGovernor_Observe(BalanceGovernor *g,int left,int right,float speed){
    g->speed=speed;
    float duty=fmaxf(fabsf((float)left),fabsf((float)right))/2499.0f;
    g->duty+=.125f*(fminf(1.0f,duty)-g->duty);
    if(duty>=.985f){if(g->saturated<1000)g->saturated++;}else g->saturated=0;
}
/* RT36: the anchor may only bound debt when the wheels are genuinely out of
 * headroom. Log 18/11 (idle/fwd_turn/rev_turn/spd90) shows the old rule clamping
 * `x_ref` at +10 cm on a benign steady lag (duty 0.83, lean predicted 0.6 deg):
 * forward then held 0.79-0.84 m/s against a 0.90 reference while reverse, which
 * tracked with lag 0.015, reached 0.90. The same clamp held the left turn at
 * 0.61 m/s. Pressure from a tracking lag alone therefore no longer moves the
 * anchor; only a saturated duty (>0.95, i.e. the 0.90 preset at a weak pack)
 * may. */
#ifndef BALANCE_GOV_ANCHOR_DUTY
#define BALANCE_GOV_ANCHOR_DUTY 0.95f
#endif
static inline void BalanceGovernor_Anchor(const BalanceGovernor *g,float *reference,float position,int direction){
    /* Bound only unreachable forward reference debt. Keep >6cm so the
     * existing low-speed blockage detector still operates against a wall. */
    if(g->pressure&&g->duty>BALANCE_GOV_ANCHOR_DUTY&&direction*(*reference-position)>.10f)*reference=position+direction*.10f;
}
static inline float BalanceGovernor_Step(BalanceGovernor *g,float old,float target,
    float rate,int wanted,float pitch,float gyro){
    if(!isfinite(old)||!isfinite(target)||!isfinite(pitch)||!isfinite(gyro)||
       !isfinite(rate)||!isfinite(g->acceleration)||!isfinite(g->duty)||!isfinite(g->speed)||!isfinite(g->equilibrium))return NAN;
    float requested=fabsf(target);
    /* Keep RT16 thresholds; center the predictor on measured quiet posture,
     * not an assumed perfect0deg. Logs10/11 show quiet posture varying~2deg.
     * Default0 is exactly the previous predictor until rest is qualified. */
    pitch-=g->equilibrium;
    bool quick_launch=wanted!=0&&fabsf(old)<.50f&&old*target>=0;
    int direction=old>.02f?1:old<-.02f?-1:wanted;
    float predicted=direction*(pitch+.10f*gyro);
    bool accelerating=wanted!=0&&old*target>=0&&fabsf(target)>fabsf(old)+.00001f;
    /* RT21-A: tracking lag is expected while the reference is still
     * accelerating. Only treat it as missing headroom while holding/braking.
     * RT22: lean term is9deg normal /11deg launch, centred on the learned
     * posture; duty>0.90 and the lag rule are unchanged. */
    if(g->duty>.90f||predicted>(quick_launch?BALANCE_GOV_LEAN_LAUNCH_DEG:BALANCE_GOV_LEAN_DEG)||
       direction*(old-g->speed)>(accelerating?.30f:.12f)){g->pressure=true;g->cool=0;}
    /* At launch, a normal balancing correction can trip the lean predictor.
     * Require100ms of recovered margin below0.5m/s instead of waiting300ms
     * for an almost motionless body. High-speed reserve is unchanged. */
    else if(g->duty<.85f&&
       (quick_launch?(predicted<BALANCE_GOV_LEAN_LAUNCH_DEG-2.0f&&
                      fabsf(pitch)<BALANCE_GOV_LEAN_LAUNCH_DEG&&fabsf(gyro)<BALANCE_GOV_LAUNCH_GYRO):
                          (fabsf(pitch)<BALANCE_GOV_CLEAR_DEG&&fabsf(gyro)<BALANCE_GOV_CLEAR_GYRO))&&
       fabsf(old-g->speed)<.08f){
        if(g->cool<60)g->cool++;
        if(g->cool>=(quick_launch?20:60))g->pressure=false;
    }else g->cool=0;
    g->limited=accelerating&&g->pressure;
    bool deadlock=false;
    bool retreat=wanted!=0&&old*target>0&&fabsf(old)>.15f&&
        (g->saturated>=8||predicted>BALANCE_GOV_LEAN_DEG+2.5f);
    /* RT22: pressure limits the commanded acceleration to a crawl instead of
     * freezing the reference. The old freeze waited for |ref-speed|<0.08, so a
     * held forward command stalled0.5-4.8s; a crawl keeps obeying the command
     * while the lean is large, and the anchor still bounds unreachable debt. */
    /* RT21-A anti-deadlock: a crawling reference below0.2m/s with a
     * stationary vehicle can never satisfy the clear test (|old-speed|<.08).
     * Allow a slow crawl so static friction can be broken; the motion blockage
     * detector still cancels a real wall after its evidence window. */
    if(g->limited&&wanted!=0&&fabsf(g->speed)<.05f&&fabsf(old)<.15f){
        target=direction*.15f;
        deadlock=true;
    }
    if(retreat){target=old-direction*.01f;g->limited=true;}
    bool braking=old*target<0||fabsf(target)<fabsf(old)-.00001f;
    float cap=braking?fminf(rate,BALANCE_GOV_BRAKE_CAP):
        fminf(rate,requested<=.50001f?.60f:fabsf(old)>.50f?.22f:.50f);
    /* The stiction-breaking crawl above keeps its normal rate: crawling to0.15
     * at0.06m/s^2 would take1.8s and defeat its purpose. */
    if(g->limited&&!deadlock)cap=fminf(cap,BALANCE_GOV_CRAWL);
    if(retreat)cap=.10f;
    float jerk=(braking||g->limited)?2.5f:2.0f;
    /* RT14: neutral must never spend a stored launch acceleration increasing
     * the velocity reference. Keep the balance loop untouched; discard only
     * acceleration pointing away from zero when the operator releases. */
    if(wanted==0&&old*g->acceleration>0)g->acceleration=0;
    /* A new reversal must brake now, not first spend the old launch
     * acceleration making the previous direction faster for~200ms. */
    if(old*target<0&&g->acceleration*(target-old)<0)g->acceleration=0;
    /* Start removing acceleration BEFORE reaching the velocity target. Merely
     * clamping (target-old)/dt with a jerk bound creates a limit cycle. */
    float attainable=fminf(cap,sqrtf(2.0f*jerk*fabsf(target-old)));
    float desired=fmaxf(-attainable,fminf(attainable,(target-old)/.005f));
    g->acceleration+=fmaxf(-jerk*.005f,fminf(jerk*.005f,desired-g->acceleration));
    float next=old+g->acceleration*.005f;
    if(wanted==0&&fabsf(next)>fabsf(old)){next=old;g->acceleration=0;}
    /* Capture a fixed target crossing to remove discrete-time chatter.
     * Pressure holds (target==old) still let acceleration slew down. */
    if(target!=old&&(target-old)*(target-next)<=0){
        next=target;g->acceleration=0;
    }
    return next;
}
