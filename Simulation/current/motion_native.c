static float study_speed=.72f;
#define BALANCE_MOTION_SPEED study_speed
#include "balance_turn.h"
#include "phase2_motion_control.h"
#define API __declspec(dllexport)
static BalanceMotion decode(const double *s){
 return (BalanceMotion){.x_ref=s[0],.v_ref=s[1],.last_word=s[2],.last_ms=s[3],.stable=s[4],
 .ready=s[5]!=0,.release_required=s[6]!=0,.used=s[7]!=0,.flags=s[9],.cancel_origin=s[8],
 .requested=s[10],.blocked_ticks=s[11],.cancel_flags=s[12],.inhibited=s[13]!=0,.speed_preset=s[14],.cancel_started_ms=s[15],.braking=s[23]!=0,.stop_ticks=s[24]};
}
API void twip_motion_reference(double *in,double *state,double *out){
 BalanceMotion s=decode(state);
 s.speed_preset=study_speed>.50f;
 BalanceTurn t={.psi_ref=state[16],.w_ref=state[17],.requested=state[18],.used=state[19]!=0,.release_required=state[20]!=0,.blocked_ticks=state[21],.quiet=state[22],.braking=state[25]!=0,.stop_ticks=state[26],.rate_mode=state[27]};
 int valid=BalanceMotion8_Update(&s,&t,in[0]!=0,(uint32_t)in[1],(int)in[7],in[2],in[3],in[4],in[5],in[8],in[9],(uint32_t)in[6]);
 double a[29]={s.x_ref,s.v_ref,s.last_word,s.last_ms,s.stable,s.ready,s.release_required,s.used,s.cancel_origin,
 s.flags,s.requested,s.blocked_ticks,s.cancel_flags,s.inhibited,s.speed_preset,s.cancel_started_ms,t.psi_ref,t.w_ref,t.requested,t.used,t.release_required,t.blocked_ticks,t.quiet,s.braking,s.stop_ticks,t.braking,t.stop_ticks,t.rate_mode,valid};
 for(int i=0;i<29;i++)out[i]=a[i];
}
API void twip_motion_impact(double *in,double *state,double *out){
 BalanceMotion s=decode(state);out[0]=BalanceMotion_Impact(&s,in[0]!=0,in[1],in[2]);
}
API void twip_motion_cancel_fault(double *in,double *state,double *out){
 BalanceMotion s=decode(state);out[0]=BalanceMotion_Fault(&s,in[0],(uint32_t)in[1]);
}
API void twip_motion_control(double *in,double *state,double *out){
 float a[7],s[CAT_N+3],o[4];for(int i=0;i<7;i++)a[i]=(float)in[i];
 for(int i=0;i<CAT_N+3;i++)s[i]=(float)state[i];
 int valid=motion_cat_step8_mode(a,s,o,(float)in[7],(float)in[8],(float)in[9],(float)in[10],(unsigned)in[11]);
 for(int i=0;i<CAT_N+3;i++)out[i]=s[i];
 out[CAT_N+3]=o[0];out[CAT_N+4]=o[1];out[CAT_N+5]=valid;
}
API void twip_motion_set_speed(double *in,double *out){study_speed=(float)in[0];out[0]=study_speed;}
