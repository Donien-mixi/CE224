#pragma once
/* balance_governor.h — trần tốc độ/lực: phần inline của lớp governor.
 *
 * Governor là bộ hãm phản hồi: khi duty cạn, thân ngả nhiều hoặc bám trễ, nó hạ
 * gia tốc tham chiếu (không nâng) để xe còn dự trữ cho vòng cân bằng. Không đổi
 * luật lực R12, không sửa cảm biến. Thông số chỉnh được: xem tuning.h (nhóm 4).
 */
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "tuning.h"

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
/* Đo duty bánh (lọc 0,125) và đếm nhịp bánh bão hoà; gọi mỗi 5 ms. */
static inline void BalanceGovernor_Observe(BalanceGovernor *g,int left,int right,float speed){
    g->speed=speed;
    float duty=fmaxf(fabsf((float)left),fabsf((float)right))/2499.0f;
    g->duty+=.125f*(fminf(1.0f,duty)-g->duty);
    if(duty>=.985f){if(g->saturated<1000)g->saturated++;}else g->saturated=0;
}
/* Chỉ chặn "nợ" tham chiếu không thể tới khi bánh THẬT SỰ cạn duty (>0,95) và
 * đang có pressure; giữ >6 cm để bộ phát hiện chặn đường còn tác dụng. Bám trễ
 * đơn thuần không được dời mốc (đó là lỗi làm mất 0,79–0,84 m/s ở bản cũ). */
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
