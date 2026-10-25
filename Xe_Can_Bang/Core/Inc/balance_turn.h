#pragma once
#include "balance_motion.h"
/* Extends SPEED2 motion gating; keeps measurements in the physical ARM frame.
 * Encoder-derived yaw is unwrapped. No roll/slip or global heading sensor. */
typedef struct {float psi_ref,w_ref;int8_t requested;bool used,release_required;} BalanceTurn;
static inline void BalanceTurn_Clear(BalanceTurn *t){*t=(BalanceTurn){0};}
static inline float BalanceTurn_Approach(float old,float target,float step){return old+fmaxf(-step,fminf(step,target-old));}
static inline int BalanceMotion8_Update(BalanceMotion *s,BalanceTurn *t,bool allow,uint32_t word,int turn,
 float position,float pitch,float gyro,float speed,float yaw,uint32_t now){
 if(turn<-1||turn>1||!isfinite(yaw)||!isfinite(t->psi_ref)||!isfinite(t->w_ref))return 0;
 uint32_t last=word!=s->last_word?now:s->last_ms;
 bool fresh=(word>>2)>0U&&(uint32_t)(now-last)<350U;
 bool neutral=(word&3U)==0U&&turn==0;
 if(t->used&&(!fresh||!allow||(s->cancel_flags&&!s->ready)))t->release_required=true;
 if(fresh&&neutral)t->release_required=false;
 if(!BalanceMotion_Update(s,allow&&!t->release_required,word,position,pitch,gyro,speed,now))return 0;
 bool enabled=allow&&s->ready&&!s->release_required&&!t->release_required&&fresh;
 int wanted=enabled?turn:0;
 if(wanted){t->used=true;s->used=true;s->cancel_flags=0;s->flags&=(uint16_t)~(64U|128U);s->flags|=32U;}
 /* Release/recovery cancels the abandoned yaw trajectory as well as x. */
 if(t->used&&(!allow||s->cancel_flags))t->psi_ref=yaw;
 float max_rate=(word&3U)?(s->speed_preset?.15f:.20f):.30f;
 float target=max_rate*wanted;
 if(wanted*(t->psi_ref-yaw)>.25f)target=0;
 float old=t->w_ref;
 float acceleration=(old*target<0||fabsf(target)<fabsf(old))?.80f:.50f;
 t->w_ref=BalanceTurn_Approach(old,target,acceleration*.005f);
 if(fabsf(t->w_ref)>.0001f){s->flags|=2U;if(target==0)s->flags|=4U;}
 if(t->used&&!s->cancel_flags)t->psi_ref+=.5f*(old+t->w_ref)*.005f;
 /* Hard bound on requested yaw lead. Never overwrite the sensor/state. */
 if(t->used)t->psi_ref=yaw+fmaxf(-.35f,fminf(.35f,t->psi_ref-yaw));
 if(t->release_required)s->flags|=16U;
 t->requested=(int8_t)(fresh?turn:0);
 return isfinite(t->psi_ref)&&isfinite(t->w_ref);
}
