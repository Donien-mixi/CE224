#pragma once
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#ifndef BALANCE_MOTION_SPEED
#define BALANCE_MOTION_SPEED (s->speed_preset? .60f:.40f)
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
    bool inhibited;
    uint8_t speed_preset;
    float cancel_origin;
    uint32_t cancel_started_ms;
} BalanceMotion;
static inline void BalanceMotion_Clear(BalanceMotion *s){*s=(BalanceMotion){0};}
static inline void BalanceMotion_Cancel(BalanceMotion *s,float position,float speed,uint16_t reason,uint32_t now){
    // Capture once per cancellation episode. Re-entry into REC1 cannot extend
    // this independent envelope while the moving reference follows the body.
    if(!s->cancel_flags||s->ready){s->cancel_origin=position;s->cancel_started_ms=now;}
    s->x_ref=position;s->v_ref=fmaxf(-.8f,fminf(.8f,speed));s->ready=false;s->stable=0;
    s->release_required=true;s->blocked_ticks=0;s->cancel_flags=64U|reason;
}
static inline uint16_t BalanceMotion_Fault(const BalanceMotion *s,float position,uint32_t now){
    if(!s->cancel_flags||s->ready)return 0;
    if(!isfinite(position)||!isfinite(s->cancel_origin))return 512U;
    if(fabsf(position-s->cancel_origin)>1.0f)return 8U;
    return (uint32_t)(now-s->cancel_started_ms)>=4000U?4096U:0U;
}
static inline uint16_t BalanceMotion_Impact(const BalanceMotion *s,bool allow,float gyro,float norm2){
    if(!allow||!s->used)return 0;
    if(!isfinite(gyro)||!isfinite(norm2))return 512U;
    return fabsf(gyro)>900.0f||(fabsf(gyro)>250.0f&&(norm2>4.0f||norm2<.16f))?8192U:0U;
}
static inline int BalanceMotion_Update(BalanceMotion *s,bool allow,uint32_t word,
    float position,float pitch,float gyro,float speed,uint32_t now){
    if(!isfinite(position)||!isfinite(pitch)||!isfinite(gyro)||!isfinite(speed))return 0;
    if(word!=s->last_word){s->last_word=word;s->last_ms=now;}
    unsigned code=word&3U;if(code>2U)return 0;
    int dir=code==1?1:code==2?-1:0;
    bool fresh=(word>>2)>0U&&(uint32_t)(now-s->last_ms)<350U;
    if(!fresh&&dir!=0)s->release_required=true;
    if(!allow&&s->used&&!s->inhibited)BalanceMotion_Cancel(s,position,speed,0,now);
    s->inhibited=!allow;
    // Encoder tracking detects a possible blockage, not wall distance. A wheel
    // spinning against a wall may evade this; violent IMU motion has its own cut.
    float error=s->x_ref-position;
    bool lead=s->used&&!s->cancel_flags&&fabsf(error)>.12f;
    bool stuck=s->used&&!s->cancel_flags&&fabsf(error)>.06f&&fabsf(speed)<.05f;
    if(allow&&(lead||stuck)){
        s->blocked_ticks+=(uint16_t)(lead?3:1);
        if(s->blocked_ticks>=60U)BalanceMotion_Cancel(s,position,speed,128U,now);
    }else s->blocked_ticks=0;
    if(fresh&&dir==0)s->release_required=false;
    if(!allow){s->ready=false;s->stable=0;if(s->used)s->release_required=true;}
    // During cancellation let balance catch the body without chasing an obsolete
    // position. Retain velocity damping. Freeze a new anchor only after quiet.
    if(s->cancel_flags&&!s->ready)s->x_ref=position;
    if(allow&&!s->release_required){
        bool quiet=fabsf(pitch)<=5&&fabsf(gyro)<25&&fabsf(speed)<(s->cancel_flags?.10f:.15f)&&fabsf(s->v_ref)<.01f&&fabsf(position-s->x_ref)<.08f;
        if(quiet){if(s->stable<100U)s->stable++;}else s->stable=0;
        if(s->stable>=100U)s->ready=true;
    }
    int wanted=allow&&s->ready&&!s->release_required&&fresh?dir:0;
    if(wanted!=0)s->cancel_flags=0;
    bool limited=wanted*(s->x_ref-position)>.10f;
    float target=limited?0:BALANCE_MOTION_SPEED*wanted;
    if(wanted!=0)s->used=true;
    float old=s->v_ref;
    float rate=(old*target<0||fabsf(target)<fabsf(old))?.30f:.20f;
    s->v_ref+=fmaxf(-rate*.005f,fminf(rate*.005f,target-s->v_ref));
    if(s->used&&!s->cancel_flags)s->x_ref+=.5f*(old+s->v_ref)*.005f;
    s->requested=(int8_t)(fresh?dir:0);
    s->flags=(s->ready&&!s->release_required?1U:0U)|(fabsf(s->v_ref)>.0001f?2U:0U)|
      (target==0&&fabsf(s->v_ref)>.0001f?4U:0U)|(limited?8U:0U)|
      (s->release_required?16U:0U)|(s->used?32U:0U)|s->cancel_flags|(s->speed_preset?256U:0U);
    return isfinite(s->x_ref)&&isfinite(s->v_ref);
}
