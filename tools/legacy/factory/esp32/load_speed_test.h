#pragma once
/* TEMPORARY loaded-speed experiment. Delete this feature after collecting logs.
 * Still uses R12; never writes motor PWM directly. No normal MOVE8 extension. */
#include "drive8_protocol.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#define LOAD_TEST_TARGET .95f
#define LOAD_TEST_SPEED_LIMIT 1.05f
#define LOAD_TEST_TIME_MS 8000U
#define LOAD_TEST_DISTANCE 6.0f
typedef struct {
    uint32_t started_ms,ended_ms;
    float origin;
    uint16_t quiet,stopped,reserve;
    uint8_t phase,reason; /* 0 unused,1 run,2 braking,3 finished,4 rejected */
} LoadSpeedTest;
/* Reasons:1 release/lease,2 time,3 distance,4 recovery/instability,
 * 5 fault,6 start not stationary/ready,7 sustained90pct PWM. One attempt per ARM. */
static inline int LoadTest_Parse(const char *s,Drive8Packet *out){
    Drive8Packet p={0,0,0,0,0};uint32_t run;
    if(strncmp(s,"$LOADTEST,",10))return 0;
    s+=10;
    if(!Drive8_Uint(&s,&p.epoch,0)||*s++!=',')return 0;
    if(!Drive8_Uint(&s,&p.sequence,0)||*s++!=',')return 0;
    if(!Drive8_Uint(&s,&run,1)||*s||run!=1U)return 0;
    p.direction=1;p.preset=2;*out=p;return 1;
}
static inline void LoadTest_End(LoadSpeedTest *s,uint8_t reason,uint32_t now){
    if(s->phase==1){s->phase=2;s->reason=reason;s->ended_ms=now;s->stopped=0;}
}
static inline void LoadTest_Fault(LoadSpeedTest *s,uint32_t now){
    if(s->phase==1||s->phase==2){s->phase=3;s->reason=5;s->ended_ms=now;}
}
/* Retain correction authority; do not wait for saturation and a falling body.
 * Called once per200Hz tick with the previous applied motor commands. */
static inline void LoadTest_Reserve(LoadSpeedTest *s,int left,int right,uint32_t now){
    if(s->phase!=1)return;
    if(abs(left)>=2249||abs(right)>=2249){if(s->reserve<20)s->reserve++;}else s->reserve=0;
    if(s->reserve>=20)LoadTest_End(s,7,now);
}
static inline bool LoadTest_Guard(const LoadSpeedTest *s){return s->phase==1||s->phase==2;}
static inline uint16_t LoadTest_Flags(const LoadSpeedTest *s){
    return (s->phase?16384U:0U)|(s->phase==1?4096U:0U)|(s->phase==2?8192U:0U);
}
static inline void LoadTest_Update(LoadSpeedTest *s,bool requested,bool fresh,
    bool balancing,bool allow,bool ready,float pitch,float gyro,float left,float right,
    float x,float vref,bool neutral,uint32_t now){
    bool finite=isfinite(pitch)&&isfinite(gyro)&&isfinite(left)&&isfinite(right)&&isfinite(x)&&isfinite(vref);
    if(s->phase==4&&!requested){s->phase=0;s->reason=0;s->quiet=0;}
    bool quiet=finite&&balancing&&allow&&ready&&neutral&&fabsf(pitch)<=5&&fabsf(gyro)<35&&
        fabsf(left)<.12f&&fabsf(right)<.12f&&fabsf(vref)<.01f;
    if(s->phase==0){
        if(quiet){if(s->quiet<100)s->quiet++;}else s->quiet=0;
        if(requested&&fresh){
            if(!finite||!balancing||!allow||!ready||!neutral||s->quiet<100||fabsf(pitch)>5||fabsf(gyro)>=35||
               fabsf(left)>=.12f||fabsf(right)>=.12f||fabsf(vref)>=.01f){s->phase=4;s->reason=6;}
            else{s->phase=1;s->origin=x;s->started_ms=now;}
        }
    }
    if(s->phase==1){
        if(!finite||!balancing)LoadTest_Fault(s,now);
        else if(!requested||!fresh)LoadTest_End(s,1,now);
        else if(!allow||fabsf(pitch)>10||fabsf(gyro)>100||fabsf(left-right)>.30f)LoadTest_End(s,4,now);
        else if((uint32_t)(now-s->started_ms)>=LOAD_TEST_TIME_MS)LoadTest_End(s,2,now);
        else if(fabsf(x-s->origin)>=LOAD_TEST_DISTANCE)LoadTest_End(s,3,now);
    }
    if(s->phase==2){
        bool stopped=finite&&fabsf(left)<.08f&&fabsf(right)<.08f&&fabsf(vref)<.01f;
        if(stopped){if(s->stopped<20)s->stopped++;}else s->stopped=0;
        if(s->stopped>=20){s->phase=3;s->ended_ms=now;}
    }
}
