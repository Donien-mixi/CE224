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
/* STOP/re-ARM giữ tư thế trung tính đã học (kẹp ±2°); chỉ dùng cho bộ dự đoán
 * headroom, không phải điểm đặt điều khiển. */
static inline void BalanceGovernor_ResetKeepEquilibrium(BalanceGovernor *g){
 float e=g->equilibrium;bool r=g->equilibrium_ready;
 *g=(BalanceGovernor){0};g->equilibrium=e;g->equilibrium_ready=r;
}
/* Học tư thế trung tính của bộ dự đoán headroom khi xe tự cân bằng đứng yên.
 * Không đổi CF, hiệu chuẩn IMU, gain lực hay chốt góc; chờ 0,5 s + 60 mẫu hợp
 * lệ, motion đóng băng giá trị cuối. */
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
    /* Giữ ngưỡng cũ; quy tâm dự đoán về tư thế đo được (không giả định 0°) vì tư
 * thế yên thay đổi ±2°. */
    pitch-=g->equilibrium;
    bool quick_launch=wanted!=0&&fabsf(old)<.50f&&old*target>=0;
    int direction=old>.02f?1:old<-.02f?-1:wanted;
    float predicted=direction*(pitch+.10f*gyro);
    bool accelerating=wanted!=0&&old*target>=0&&fabsf(target)>fabsf(old)+.00001f;
    /* Bám trễ là bình thường khi tham chiếu còn tăng tốc; chỉ coi là thiếu dự trữ
 * khi đang giữ/phanh. Ngưỡng nghiêng 10°/13° tính từ tư thế đã học. */
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
    /* Pressure hạ gia tốc tham chiếu xuống mức bò thay vì đóng băng: lệnh vẫn được
 * thi hành trong lúc thân ngả nhiều, còn neo vẫn chặn "nợ" không tới được. */
    /* Chống bế tắc: tham chiếu bò dưới 0,2 m/s mà xe đứng thì không bao giờ thoả
 * điều kiện nhả (|old−speed|<0,08). Cho bò chậm để phá ma sát tĩnh; bộ phát
 * hiện chặn đường vẫn huỷ tường thật sau đủ bằng chứng. */
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
    /* Nhả lệnh: không dùng gia tốc xuất phát đã tích để tăng tiếp tham chiếu — chỉ
 * bỏ gia tốc hướng ra xa 0. */
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
