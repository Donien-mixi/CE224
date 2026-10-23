#pragma once
#include "balance_motion.h"
#ifndef BALANCE_TURN_PURE_RATE
#define BALANCE_TURN_PURE_RATE (s->speed_preset?2.2f:1.2f)
#define BALANCE_TURN_DIAGONAL_RATE (s->speed_preset?.55f:.80f)
#define BALANCE_TURN_ACCEL 1.5f
#define BALANCE_TURN_DECEL 6.0f
#endif
/* Extends SPEED2 motion gating; keeps measurements in the physical ARM frame.
 * Encoder-derived yaw is unwrapped. No roll/slip or global heading sensor. */
typedef struct {float psi_ref,w_ref;int8_t requested;bool used,release_required;uint16_t blocked_ticks,quiet;
 bool braking;uint16_t stop_ticks;uint8_t rate_mode;} BalanceTurn;
static inline void BalanceTurn_Clear(BalanceTurn *t){*t=(BalanceTurn){0};}
static inline bool BalanceMotion8_Maneuver(const BalanceMotion *s,const BalanceTurn *t,uint32_t now){
 return s->ready&&!s->cancel_flags&&!s->release_required&&!t->release_required&&
    (uint32_t)(now-s->last_ms)<350U&&
    (s->requested!=0||t->requested!=0||s->braking||t->braking);
}
static inline float BalanceTurn_Approach(float old,float target,float step){return old+fmaxf(-step,fminf(step,target-old));}
static inline int BalanceMotion8_Update(BalanceMotion *s,BalanceTurn *t,bool allow,uint32_t word,int turn,
 float position,float pitch,float gyro,float speed,float yaw,float yaw_rate,uint32_t now){
 if(turn<-1||turn>1||!isfinite(yaw_rate)||!isfinite(yaw)||!isfinite(t->psi_ref)||!isfinite(t->w_ref))return 0;
 uint32_t last=word!=s->last_word?now:s->last_ms;
 bool fresh=(word>>2)>0U&&(uint32_t)(now-last)<350U;
 bool neutral=(word&3U)==0U&&turn==0;
 bool linear_neutral=(word&3U)==0U;
 /* Capture ONCE when a pure-turn gesture starts. Following the axle every
  * tick erased longitudinal position feedback (log25: left wheel stopped,
  * right wheel drove an arc). Keep the R12 balance loop and a fixed center. */
 if(allow&&fresh&&turn&&linear_neutral&&s->ready&&!s->release_required&&
    !t->release_required&&(t->requested==0||s->requested!=0)){
  s->x_ref=position;s->blocked_ticks=0;
  if(fabsf(s->v_ref)<.01f&&fabsf(speed)<.06f){s->braking=false;s->stop_ticks=0;}
 }
 if(allow&&fresh&&neutral&&t->requested!=0&&!s->cancel_flags){
  BalanceMotion_BeginBrake(s,position,now);
 }
 if(t->used&&(!fresh||!allow||(s->cancel_flags&&!s->ready)))t->release_required=true;
 if(fresh&&neutral)t->release_required=false;
 if(!BalanceMotion_Update(s,allow&&!t->release_required,word,position,pitch,gyro,speed,now))return 0;
 bool enabled=allow&&s->ready&&!s->release_required&&!t->release_required&&fresh;
 int wanted=enabled?turn:0;
 /* Held steering commands angular velocity, never a stored total angle.
  * On release brake angular velocity and then freeze a NEW heading anchor. */
 if(wanted){t->braking=false;t->stop_ticks=0;t->psi_ref=yaw;}
 else if(t->used&&t->requested!=0){t->braking=true;t->stop_ticks=0;t->psi_ref=yaw;}
 /* A blocked turn must not keep applying torque indefinitely. Cancel both
  * trajectories through the existing bounded recovery/release mechanism. */
 if(wanted&&fabsf(t->w_ref)>.35f&&fabsf(yaw_rate)<.08f){
  if(t->blocked_ticks<200U)t->blocked_ticks++;
 }else t->blocked_ticks=0;
 if(t->blocked_ticks>=200U){
  BalanceMotion_Cancel(s,position,speed,128U,now);
  s->flags=(s->flags&~1U)|s->cancel_flags|16U;
  t->release_required=true;t->blocked_ticks=0;wanted=0;
 }
 if(wanted){t->used=true;s->used=true;s->cancel_flags=0;s->flags&=(uint16_t)~(64U|128U);s->flags|=32U;}
 /* Release/recovery cancels the abandoned yaw trajectory as well as x. */
 if(t->used&&(!allow||s->cancel_flags)){t->psi_ref=yaw;t->braking=true;}
 float requested_rate=(word&3U)?BALANCE_TURN_DIAGONAL_RATE:BALANCE_TURN_PURE_RATE;
 float max_rate=requested_rate;
 float wheel_cap=fmaxf(.10f,2.0f*(.80f-fabsf(speed))/.262f);
 float lateral_cap=fabsf(speed)>.1f?.4f/fabsf(speed):max_rate;
 max_rate=fminf(max_rate,fminf(wheel_cap,lateral_cap));
 float target=max_rate*wanted;
 float lead=wanted*(t->psi_ref-yaw);
 float tracking=fmaxf(0,fminf(1,(.50f-lead)/.25f));
 float balance=fmaxf(0,fminf(1,(12.0f-fabsf(pitch))/6.0f));
 /* A drifting turn center calls for less turn authority, not a false linear
  * blockage cancellation (log26). R12 position feedback keeps its fixed anchor;
  * ordinary/recovery position budgets still provide the independent bound. */
 float center=linear_neutral?fmaxf(.20f,fminf(1,(.16f-fabsf(s->x_ref-position))/.10f)):1.0f;
 target*=tracking*balance*center;
 if(wanted&&(max_rate<requested_rate-.001f||tracking<.999f||balance<.999f||center<.999f))s->flags|=512U;
 float old=t->w_ref;
 float acceleration=(old*target<0||fabsf(target)<fabsf(old))?BALANCE_TURN_DECEL:BALANCE_TURN_ACCEL;
 /* Neutral removes the commanded turn now. Physical deceleration is provided
  * by bounded yaw-rate feedback, not by finishing a stored angular ramp. */
 t->w_ref=wanted?BalanceTurn_Approach(old,target,acceleration*.005f):0;
 if(fabsf(t->w_ref)>.0001f){s->flags|=2U;if(target==0)s->flags|=4U;}
 if(t->used&&(wanted||t->braking))t->psi_ref=yaw;
 if(t->braking){
  bool stopped=fabsf(yaw_rate)<.08f&&fabsf(t->w_ref)<.01f;
  if(stopped){if(t->stop_ticks<20U)t->stop_ticks++;}else t->stop_ticks=0;
  if(t->stop_ticks>=20U){t->braking=false;t->w_ref=0;t->psi_ref=yaw;}
 }
 t->rate_mode=wanted?1U:t->braking?2U:0U;
 if(t->rate_mode)s->flags|=2048U;
 if(t->braking)s->flags|=4U;
 /* Hard bound on requested yaw lead. Never overwrite the sensor/state. */
 if(t->used)t->psi_ref=yaw+fmaxf(-.60f,fminf(.60f,t->psi_ref-yaw));
 if(t->release_required)s->flags|=16U;
 /* End an abort envelope only after physically settling, even if no neutral
  * packet arrives. Keep both command gates closed: never resume stale motion.
  * Merely waiting or receiving PING cannot end an unsettled abort. */
 if((t->release_required||s->release_required)&&allow&&s->cancel_flags&&fabsf(pitch)<=5&&fabsf(gyro)<25&&
    fabsf(speed)<.10f&&fabsf(yaw_rate)<.15f&&fabsf(s->v_ref)<.01f&&fabsf(t->w_ref)<.01f){
  if(t->quiet<100U)t->quiet++;
 }else t->quiet=0;
 if(t->quiet>=100U){
  s->cancel_flags=0;s->cancel_started_ms=0;s->ready=false;s->stable=0;
  s->flags=(s->flags&~(1U|64U|128U))|16U;
 }
 t->requested=(int8_t)(fresh?turn:0);
 return isfinite(t->psi_ref)&&isfinite(t->w_ref);
}
