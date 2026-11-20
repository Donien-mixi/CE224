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
/* RT42: lease cua lenh dieu khien. Truoc day350ms: mot cu nghen song Wi-Fi 300-400ms
 * (nhieu AP, Bluetooth, camera2.4GHz) lam lenh bi coi la "mat" va xe phanh giua
 * luc dang chay. Do tren log RT41: khoang cach hai goi lenh p99=50ms nhung da co
 * mot lan142ms; trong phong dong co the vuot350ms. Nang len500ms chi doi 150ms
 * phanh khi mat ket noi that, trong khi cac chan an toan khac (goc, quang, interlock,
 * heartbeat10s) khong doi. */
#ifndef BALANCE_MOTION_STALE_MS
#define BALANCE_MOTION_STALE_MS 500U
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
/* RT36: bleed a static hold offset so the position term stops loading stiction.
 * Acts only after 3 s of true standstill, at 2 cm/s, within 15 cm. See the
 * comment at the use site. */
#ifndef BALANCE_MOTION_HOLD_RELAX
#define BALANCE_MOTION_HOLD_RELAX .02f
#endif
#ifndef BALANCE_MOTION_HOLD_SPAN
#define BALANCE_MOTION_HOLD_SPAN .15f
#endif
/* RT38: a direction change arrives as "neutral" for a few packets (release one
 * button, press the next). Braking on the first neutral packet turned every
 * reversal into stop-then-go. Hold the previous command for up to120ms; a new
 * direction inside the window is a continuous reversal. A real release still
 * brakes after120ms and a stale command brakes at once. */
#ifndef BALANCE_MOTION_RELEASE_HOLD_TICKS
#define BALANCE_MOTION_RELEASE_HOLD_TICKS 24U
#endif
/* RT38 probe switch:1 = old behaviour (zero the speed command when the axle
 * lags the reference by20cm) for the host test. */
#ifndef BALANCE_MOTION_LEAD_CUT
#define BALANCE_MOTION_LEAD_CUT 0
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
    uint16_t settle_ticks;   /* RT36: 3 s of true standstill before anchor bleeds */
    uint16_t release_ticks;  /* RT38: neutral packets tolerated before a brake */
    int8_t held_dir;         /* RT38: last commanded direction, kept through a gap */
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
/* RT43 — trần MAX theo duty còn dư (đường thẳng).
 *
 * Log RT42 tại nhà (148,6 s, toàn bộ chạy mốc MAX 90): duty p95 0,90 với 0-33 %
 * mẫu mỗi lượt ở >=0,96, và vượt tham chiếu tới 0,95-0,96 m/s. Hai lần ngã
 * (t=144,69 lùi thẳng; t=235,93 nhả lệnh tiến) đều có bánh ghim 2499 rồi lệch
 * trái/phải 0,91 so với 0,71 m/s, thân quật -55deg. Ở pack ~11,5-11,8 V, 0,90 m/s
 * cần 93-96 % duty nên mất toàn bộ dự trữ cho vòng cân bằng.
 *
 * Cùng cơ chế với trần cua RT40 nhưng cho đường thẳng: khi đang chạy MAX mà có
 * bánh >=96 % duty thì hạ tham chiếu tuyến tính (sàn 0,86 -> 0,774 m/s), hồi lại
 * khi <=90 %. Với pack tươi (12,2 V, 0,90 chỉ cần 88 % duty) hệ số không bao giờ
 * giảm nên MAX 90 vẫn đạt đúng mốc. Chỉ tác dụng khi KHÔNG cua (cua có trần riêng
 * của balance_turn.h) nên cua không chậm thêm.
 * b >= 0 de dat he so; b < 0 de doc. */
#ifndef BALANCE_MOTION_MAXHEAD_HI
#define BALANCE_MOTION_MAXHEAD_HI 0.96f
#endif
#ifndef BALANCE_MOTION_MAXHEAD_LO
#define BALANCE_MOTION_MAXHEAD_LO 0.90f
#endif
#ifndef BALANCE_MOTION_MAXHEAD_FLOOR
#define BALANCE_MOTION_MAXHEAD_FLOOR 0.86f
#endif
static inline float BalanceMotion_maxhead(float b){static float scale=1.0f;if(b>=0.0f)scale=b;return scale;}
static inline float BalanceMotion_maxhead_next(float duty,bool active){
 float sc=BalanceMotion_maxhead(-1.0f);
 if(!active)return 1.0f;
 if(duty>=BALANCE_MOTION_MAXHEAD_HI){if(sc>BALANCE_MOTION_MAXHEAD_FLOOR)sc-=0.004f;}
 else if(duty<=BALANCE_MOTION_MAXHEAD_LO){if(sc<1.0f)sc+=0.002f;}
 return sc;
}
static inline int BalanceMotion_Update(BalanceMotion *s,bool allow,uint32_t word,
    float position,float pitch,float gyro,float speed,uint32_t now){
    if(!isfinite(position)||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed))return 0;
    if(word!=s->last_word){s->last_word=word;s->last_ms=now;}
    unsigned code=word&3U;if(code>2U)return 0;
    int dir=code==1?1:code==2?-1:0;
    bool fresh=(word>>2)>0U&&(uint32_t)(now-s->last_ms)<BALANCE_MOTION_STALE_MS;
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
    /* RT38: neutral for a few packets is a button change, not a stop. See the
     * constants above. A stale command (no packet for350ms) still brakes now.
     * held_dir is used instead of requested because requested already returns
     * to0 on the first neutral packet. */
    bool neutral_now=fresh&&dir==0;
    bool stale_now=!fresh&&dir!=0;
    if(s->used&&!s->cancel_flags&&s->held_dir!=0&&(neutral_now||stale_now)){
        if(stale_now){s->release_ticks=0;s->held_dir=0;BalanceMotion_BeginBrake(s,position,now);}
        else{
            if(s->release_ticks<60000U)s->release_ticks++;
            if(s->release_ticks>=BALANCE_MOTION_RELEASE_HOLD_TICKS){
                s->held_dir=0;BalanceMotion_BeginBrake(s,position,now);
            }
        }
    }else if(fresh&&dir!=0)s->release_ticks=0;
    if(dir!=0)s->held_dir=(int8_t)dir;
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
    /* RT38: during the120ms button-change window keep driving the previous
     * direction, so a reversal ramps through zero instead of restarting. */
    if(wanted==0&&allow&&s->ready&&!s->release_required&&neutral_now&&
       s->release_ticks>0&&s->release_ticks<BALANCE_MOTION_RELEASE_HOLD_TICKS)
        wanted=s->held_dir;
    if(wanted!=0&&s->braking){s->x_ref=position;s->braking=false;s->hold_anchor=false;s->stop_ticks=0;}
    if(wanted!=0)s->cancel_flags=0;
    bool limited=wanted*(s->x_ref-position)>.20f;
    float speed_limit=s->recovery_limited?fminf(BALANCE_MOTION_SPEED,s->recovery_active?.25f:.50f):BALANCE_MOTION_SPEED;
    /* RT43: at MAX, if a wheel is already at the duty ceiling the pack cannot
     * deliver 0.90 - aim lower instead of saturating and losing the axle. */
    if(s->speed_preset==2)speed_limit*=BalanceMotion_maxhead(-1.0f);
#if BALANCE_MOTION_LEAD_CUT
    float target=limited?0:speed_limit*wanted;
#else
    float target=speed_limit*wanted;
    if(limited){
        /* RT38: a moving axle that lags the reference by20cm used to have its
         * speed command zeroed and re-ramped, which is the0.9m/s stutter. The
         * reference is anchored at the axle instead and the command is kept;
         * a stopped axle (blocked) still loses the command so the wall
         * detector can cancel as before. */
        if(fabsf(speed)>=.30f)s->x_ref=position+wanted*.20f;
        else target=0;
    }
#endif
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
            /* RT36: standing on a stiction wall. Log idle 80-140 s held x at
             * -7 cm against a 0 anchor with +300 PWM, and the same loading is
             * felt as drift after a stop. After3 s of true standstill the anchor
             * bleeds toward the measured position at2 cm/s, so the loop stops
             * pressing a friction wall. Never while moving, braking, cancelling
             * or inside a recovery; bounded to15 cm so a real displacement is
             * still corrected first. */
            if(!s->braking&&!s->cancel_flags&&!s->recovery_active&&!s->recovery_limited&&
               fabsf(s->v_ref)<.005f&&fabsf(speed)<.02f){
                if(s->settle_ticks<60000U)s->settle_ticks++;
            }else s->settle_ticks=0;
            if(s->settle_ticks>=600U){
                float err=s->x_ref-position;
                if(fabsf(err)<=BALANCE_MOTION_HOLD_SPAN){
                    float step=BALANCE_MOTION_HOLD_RELAX*.005f;
                    s->x_ref=fabsf(err)<=step?position:s->x_ref-copysignf(step,err);
                }
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
