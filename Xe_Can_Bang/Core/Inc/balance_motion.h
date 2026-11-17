#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#ifndef BALANCE_MOTION_SPEED
/* RT19: third preset is0.80m/s, not0.95. A measured floor slope puts PWM2499 at
 * ~0.96m/s, so0.95 needed97-99%duty and left no balance authority. */
/* RT32 (chot 17/11/2026): bo toc do cuoi cung 50 /80 /90 cm/s.
 * Do tu log that (13 226 mau RT19+ va log 37/38): v =1.0008*duty -0.0236, nen
 * 0.90 can duty0.879@12.2V,0.900@11.9V,0.955@11.2V (tien; lui +2,7%).
 * Nguong governor 0.90 => 0.90 la moc sat bien: chi dung khi pin>=12.0V.
 * 0.80 o giua can duty0.790@12.2V /0.823@11.7V /0.859@11.2V. */
#define BALANCE_MOTION_SPEED (s->speed_preset==2? .90f:s->speed_preset? .80f:.50f)
#endif
#ifndef BALANCE_MOTION_ACCEL
#define BALANCE_MOTION_ACCEL (s->speed_preset?(fabsf(old)>.50f?.40f:.70f):(fabsf(old)>.50f?.30f:.60f))
#endif
#ifndef BALANCE_MOTION_DECEL
#define BALANCE_MOTION_DECEL .90f
#endif
#ifndef BALANCE_MOTION_REVERSE_DECEL
#define BALANCE_MOTION_REVERSE_DECEL 1.0f
#endif
/* 200Hz. The observer/encoder remain in the physical frame. An abandoned
 * trajectory must never pull a blocked vehicle back toward its old target. */
typedef struct {
    float x_ref,v_ref;
    uint32_t last_word,last_ms;
    uint16_t stable,flags;
    bool ready,used,release_required;
    int8_t limit_direction,requested;
    uint16_t blocked_ticks,cancel_flags;
    bool inhibited,recovery_limited,recovery_active;
    uint8_t speed_preset;
    float cancel_origin;
    uint32_t cancel_started_ms;
    bool braking;
    bool hold_anchor; /* RT28: a disturbance brake keeps the position anchor */
    uint16_t stop_ticks;
    uint16_t post_rec_ticks; /* RT20: bounded post-REC1 softening counter */
} BalanceMotion;
static inline void BalanceMotion_Clear(BalanceMotion *s){*s=(BalanceMotion){0};}
static inline void BalanceMotion_BeginBrake(BalanceMotion *s,float position,uint32_t now){
    if(!s->braking){s->cancel_origin=position;s->cancel_started_ms=now;}
    s->braking=true;s->stop_ticks=0;s->x_ref=position;s->blocked_ticks=0;
}
/* RT28: start the REC1 braking envelope WITHOUT capturing a new position anchor.
 * A push that arrives while the vehicle stands must return to the place the last
 * command left, so the anchor may not be re-captured at the displaced point.
 * (Logs RT27 (10)(2)/(11)(2): REC1 entry moved x_ref +0.219->+0.110 and
 * -0.967->-0.943 in one tick.) */
static inline void BalanceMotion_BeginBrakeHold(BalanceMotion *s,uint32_t now){
    BalanceMotion_BeginBrake(s,s->x_ref,now);s->hold_anchor=true;
}
static inline void BalanceMotion_Cancel(BalanceMotion *s,float position,float speed,uint16_t reason,uint32_t now){
    // Capture once per cancellation episode. Re-entry into REC1 cannot extend
    // this independent envelope while the moving reference follows the body.
    if((!s->cancel_flags||s->ready)&&!s->braking){s->cancel_origin=position;s->cancel_started_ms=now;}
    s->x_ref=position;s->v_ref=fmaxf(-.8f,fminf(.8f,speed));s->ready=false;s->stable=0;
    s->release_required=true;s->blocked_ticks=0;s->cancel_flags=64U|reason;
    s->braking=false;s->stop_ticks=0;
}
static inline uint16_t BalanceMotion_Fault(const BalanceMotion *s,float position,uint32_t now){
    if(!s->braking&&(!s->cancel_flags||s->ready))return 0;
    if(!isfinite(position)||!isfinite(s->cancel_origin))return 512U;
    /* RT19:0.80^2/(2*0.65)=0.49m of braking before jerk/tracking/final
     * correction. Log8 was still slowing at the old1m cut. Only ordinary
     * release braking gets1.25m; blockage/recovery cancellation remains1m. */
    if(fabsf(position-s->cancel_origin)>(s->braking?1.25f:1.0f))return 8U;
    /* RT29: a disturbance brake (REC1 entry, hold_anchor) is NOT a commanded
     * stop, so it has no completion deadline. While the operator keeps pushing,
     * the axle never sits below0.06m/s for200ms, so the brake never completes and
     * this4s timeout used to cut the motor on a vehicle that was upright and
     * balancing: log RT28 (12)(3) has three auto-cuts, all fault4096, at
     * t=11.52/85.50/156.48s, each exactly4s after a REC1 entry (7.52/81.50/
     *152.48s) and each at |pitch|<=1.1deg with |v|<=0.22m/s. The1.25m distance
     * bound above and REC1's own4s timeout still bound the recovery. */
    if(s->braking&&s->hold_anchor)return 0;
    return (uint32_t)(now-s->cancel_started_ms)>=4000U?4096U:0U;
}
static inline uint16_t BalanceMotion_Impact(const BalanceMotion *s,bool allow,float gyro,float norm2){
    if(!allow||!s->used)return 0;
    if(!isfinite(gyro)||!isfinite(norm2))return 512U;
    /* Moderate impact now requests bounded REC1 instead of cutting a motor
     * while the body is still upright. Keep this extreme-rate hard stop. */
    return fabsf(gyro)>900.0f?8192U:0U;
}
/* RT34 — trần tốc độ do đường (cornering budget).
 * `BalanceMotion8_Update` đặt giá trị này theo |w_ref| của lượt quay; 0 = không
 * giới hạn. Chỉ HẠ tham chiếu tốc độ, không bao giờ nâng, và vẫn đi qua ramp
 * BALANCE_MOTION_ACCEL/DECEL nên tốc độ đổi mượt thay vì bão hoà bánh.
 * Gọi với b >= 0 để đặt, b < 0 để đọc. */
static inline float BalanceMotion_corner_budget(float b){
    static float budget=0.0f;
    if(b>=0.0f)budget=b;
    return budget;
}
static inline int BalanceMotion_Update(BalanceMotion *s,bool allow,uint32_t word,
    float position,float pitch,float gyro,float speed,uint32_t now){
    if(!isfinite(position)||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed))return 0;
    if(word!=s->last_word){s->last_word=word;s->last_ms=now;}
    unsigned code=word&3U;if(code>2U)return 0;
    int dir=code==1?1:code==2?-1:0;
    bool fresh=(word>>2)>0U&&(uint32_t)(now-s->last_ms)<350U;
    if(!fresh&&dir!=0)s->release_required=true;
    /* Inhibit (startup/recovery) only pauses the trajectory: the reference is
     * held on the body and v_ref ramps to zero, so the SAME held press resumes
     * when the inhibit ends. The gesture is never cancelled by a recovery. */
    s->inhibited=!allow;
    /* A press/direction change starts here, not at an earlier gesture's anchor.
     * Neutral abandons position debt immediately; only velocity is ramped down. */
    if(allow&&fresh&&s->ready&&!s->release_required&&dir!=0&&dir!=s->requested){
        s->x_ref=position;s->braking=false;s->hold_anchor=false;s->stop_ticks=0;s->blocked_ticks=0;
        /* Re-presses start at the measured velocity, not at the unfinished
         * ramp of the preceding gesture. Avoid a force step on rapid reversal. */
#ifndef BALANCE_MOTION_FILTER
        s->v_ref=fmaxf(s->speed_preset==2?-1.05f:-.8f,fminf(s->speed_preset==2?1.05f:.8f,speed));
#endif
    }
    if(s->used&&!s->cancel_flags&&((fresh&&dir==0)||!fresh)&&s->requested!=0){
        BalanceMotion_BeginBrake(s,position,now);
    }
    // Encoder tracking detects a possible blockage, not wall distance. A wheel
    // spinning against a wall may evade this; violent IMU motion has its own cut.
    float error=s->x_ref-position;
    /* RT8: log27 showed reverse settling 9.6cm behind the reference, which the
     * old18cm/10cm pair treated as a permanent blockage. The run-up budget is
     * now20cm and the blockage evidence30cm, so a reversal transient no longer
     * throttles the whole reverse leg while a real wall still cancels. */
    bool lead=dir!=0&&s->used&&!s->cancel_flags&&!s->braking&&dir*error>.30f;
    /* A stationary balancing/turning axle can have a small steady offset.
     * Low speed is blockage evidence only during commanded translation;
     * the30cm commanded lead and ordinary distance guards still apply. An
     * overshoot or a pure turn is not evidence of a blocked translation. */
    /* RT18: tiny references and inverse launch need longer evidence, not a
     * disabled blockage detector. Below0.15m/s (or while reversing) require
     * 1.5s; below0.30m/s require1s. Higher-reference stalls keep300ms.
     * >30cm lead still cancels in100ms at ANY reference. Even a governor-held
     * 0.04m/s against a real obstacle cannot avoid cancellation indefinitely. */
    bool stuck=dir!=0&&s->used&&!s->cancel_flags&&!s->braking&&
        dir*error>.06f&&fabsf(speed)<.05f;
    if(allow&&(lead||stuck)){
        s->blocked_ticks+=(uint16_t)(lead?3:1);
        unsigned evidence=lead||fabsf(s->v_ref)>=.30f?60U:dir*s->v_ref>=.15f?200U:300U;
        if(s->blocked_ticks>=evidence)BalanceMotion_Cancel(s,position,speed,128U,now);
    }else s->blocked_ticks=0;
    if(fresh&&dir==0)s->release_required=false;
    // During inhibition the trajectory simply waits on the measured position.
    if(s->cancel_flags&&!s->ready&&(!allow||fabsf(s->v_ref)>.0001f))s->x_ref=position;
    if(allow&&!s->release_required&&!s->cancel_flags){
        /* First motion admission follows physical settling, not a return to
         * the ARM position. Log12 held-quiet at-11cm for57s could never pass
         * the old8cm test. Do not rebase the hold anchor or change the force
         * while idle: the existing NEW gesture captures its own start once.
         * Reject a queued held press before first readiness. FSM distance and
         * startup/angle/speed guards remain independent. */
        bool anchor_ok=s->used?fabsf(position-s->x_ref)<.08f:dir==0;
        bool quiet=fabsf(pitch)<=5&&fabsf(gyro)<25&&fabsf(speed)<(s->cancel_flags?.10f:.15f)&&fabsf(s->v_ref)<.01f&&anchor_ok;
        if(quiet){if(s->stable<100U)s->stable++;}else s->stable=0;
        if(s->stable>=100U)s->ready=true;
    }
    int wanted=allow&&s->ready&&!s->release_required&&fresh?dir:0;
    if(wanted!=0&&s->braking){s->x_ref=position;s->braking=false;s->hold_anchor=false;s->stop_ticks=0;}
    if(wanted!=0)s->cancel_flags=0;
    bool limited=wanted*(s->x_ref-position)>.20f;
    float speed_limit=s->recovery_limited?fminf(BALANCE_MOTION_SPEED,s->recovery_active?.25f:.50f):BALANCE_MOTION_SPEED;
    float target=limited?0:speed_limit*wanted;
    /* RT34: giới hạn do đường. Bánh ngoài khi cua cần v + |w|*0,131 mà động cơ chỉ
     * tới ~0,95 m/s (pack 11,7 V), nên ở MAX 90 mà cua gắt thì bánh ngoài bão hoà
     * 100%, yaw vọt quá tham chiếu rồi mới hồi (log 38: vọt 37%). Hạ tham chiếu
     * xuống mức bánh còn làm được để cua êm. */
    float cb=BalanceMotion_corner_budget(-1.0f);
    if(cb>0.0f){if(target>cb)target=cb;else if(target<-cb)target=-cb;}
    if(wanted!=0)s->used=true;
    float old=s->v_ref;
    float rate=old*target<0?BALANCE_MOTION_REVERSE_DECEL:
        fabsf(target)<fabsf(old)?BALANCE_MOTION_DECEL:BALANCE_MOTION_ACCEL;
    if(s->recovery_limited&&old*target>=0&&fabsf(target)>fabsf(old))rate=fminf(rate,.30f);
    #ifdef BALANCE_MOTION_FILTER
    s->v_ref=BALANCE_MOTION_FILTER(old,target,rate,wanted,pitch,gyro);
    #else
    s->v_ref+=fmaxf(-rate*.005f,fminf(rate*.005f,target-s->v_ref));
    #endif
    if(s->used&&!s->cancel_flags){
        if(s->braking){
            /* Drop the old destination immediately, then stop moving the
             * anchor as soon as the velocity ramp reaches zero. Continuing
             * to follow the axle indefinitely defeats position feedback in
             * the presence of a small IMU/COM bias (log25). */
            if(fabsf(old)>.0001f&&!s->hold_anchor)s->x_ref=position;
            /* Encoder quantisation and bias cause small residual velocities.
             * Capture the new hold anchor before demanding mathematically zero
             * speed; the unchanged position loop removes that final drift. */
            bool stopped=fabsf(speed)<.06f&&fabsf(s->v_ref)<.01f&&fabsf(pitch)<=6&&fabsf(gyro)<25;
            if(stopped){if(s->stop_ticks<20U)s->stop_ticks++;}else s->stop_ticks=0;
            if(s->stop_ticks>=20U){s->braking=false;s->v_ref=0;if(!s->hold_anchor)s->x_ref=position;s->hold_anchor=false;}
        }else if(allow){
            /* RT19: the brake dwell above fires while the body can still roll a
             * few centimetres (log(13)(2): 9cm backwards after a0.66m/s stop).
             * Freezing the anchor at that instant left a permanent position
             * error, and the force loop then pushed continuously (~+240 PWM)
             * without moving. Capture the new hold anchor once,200ms after the
             * brake silence, when the vehicle is genuinely still. Only the
             *20..40 window can re-anchor, so a slow drift during a normal hold
             * is still corrected by position feedback. */
            if(!s->hold_anchor&&s->stop_ticks>=20U&&s->stop_ticks<40U&&fabsf(s->v_ref)<.0001f&&
               fabsf(speed)<.02f&&fabsf(pitch)<=6&&fabsf(gyro)<20){
                s->stop_ticks++;
                if(s->stop_ticks>=40U)s->x_ref=position;
            }
            s->x_ref+=.5f*(old+s->v_ref)*.005f;
        }
        else if(!s->recovery_active)s->x_ref=position;
    }
    #ifdef BALANCE_MOTION_ANCHOR
    if(allow&&wanted&&!s->braking&&!s->cancel_flags)BALANCE_MOTION_ANCHOR(s->x_ref,position,wanted);
    #endif
    s->requested=(int8_t)(fresh?dir:0);
    s->flags=(s->ready&&!s->release_required?1U:0U)|(fabsf(s->v_ref)>.0001f?2U:0U)|
      (target==0&&fabsf(s->v_ref)>.0001f?4U:0U)|(limited?8U:0U)|
      (s->release_required?16U:0U)|(s->used?32U:0U)|s->cancel_flags|(s->speed_preset?256U:0U)|(s->braking?1028U:0U)|(s->recovery_limited?32768U:0U);
    return isfinite(s->x_ref)&&isfinite(s->v_ref);
}
