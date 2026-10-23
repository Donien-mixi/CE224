#pragma once
#include "phase1_catalog_control.h"
/* Original R12 observer, allocation and yaw; reference affects force request only. */
static inline int motion_cat_step_core(const float *in,float *s,float *out,float x_ref,float v_ref){
 float z[CAT_N],err[4];
 for(int i=0;i<CAT_N;i++){
  float v=CAT_B[i]*s[CAT_N+2];for(int j=0;j<CAT_N;j++)v+=CAT_A[i*CAT_N+j]*s[j];z[i]=v;
 }
 float measurement[4]={in[3]-CAT_PARAM[1]*s[CAT_N+1],.5f*(in[4]+in[5]),in[1],in[2]};
 for(int i=0;i<4;i++){err[i]=measurement[i];for(int j=0;j<CAT_N;j++)err[i]-=CAT_H[i*CAT_N+j]*z[j];}
 for(int i=0;i<CAT_N;i++){for(int j=0;j<4;j++)z[i]+=CAT_L[i*4+j]*err[j];s[i]=z[i];}
 float theta0=in[0]+.03f*in[1],theta=theta0+CAT_PARAM[11]*(z[2]-theta0);
 float omega=in[1]+CAT_PARAM[11]*(z[3]-in[1]);
 float x[4]={in[3]+CAT_PARAM[1]*(theta-s[CAT_N+1]),.5f*(in[4]+in[5])+CAT_PARAM[1]*omega,theta,omega};
 float request=0;for(int i=0;i<4;i++)request+=CAT_K[i]*x[i];
 request-=CAT_K[0]*x_ref+CAT_K[1]*v_ref;
 float desired=cat_clip(request,CAT_PARAM[4]);
 float psi=in[6]/CAT_PARAM[0],rate=(in[5]-in[4])/CAT_PARAM[0];
 float ci=cat_clip(s[CAT_N]+CAT_PARAM[7]*psi*.005f,CAT_PARAM[8]);
 float unsat=-CAT_PARAM[5]*psi-CAT_PARAM[6]*rate-ci;
 cat_allocate(desired,cat_clip(unsat,CAT_PARAM[9]),in+4,out);
 if(fabsf(unsat-out[2])<.0001f||(unsat-out[2])*psi>0)s[CAT_N]=ci;
 else cat_allocate(desired,cat_clip(-CAT_PARAM[5]*psi-CAT_PARAM[6]*rate-s[CAT_N],CAT_PARAM[9]),in+4,out);
 s[CAT_N+2]=out[3];
 for(int i=0;i<CAT_N+3;i++)if(!isfinite(s[i]))return 0;
 return isfinite(out[0])&&isfinite(out[1])&&fabsf(out[0])<=2499&&fabsf(out[1])<=2499;
}
static inline int motion_cat_step(const float *in,float *s,float *out,float x_ref,float v_ref){
 if(x_ref==0.0f&&v_ref==0.0f)return cat_step(in,s,out);
 return motion_cat_step_core(in,s,out,x_ref,v_ref);
}

static inline int motion_cat_step8_mode_core(const float *in,float *s,float *out,float x_ref,float v_ref,float psi_ref,float w_ref,unsigned yaw_mode){
 float z[CAT_N],err[4];
 for(int i=0;i<CAT_N;i++){
  float v=CAT_B[i]*s[CAT_N+2];for(int j=0;j<CAT_N;j++)v+=CAT_A[i*CAT_N+j]*s[j];z[i]=v;
 }
 float measurement[4]={in[3]-CAT_PARAM[1]*s[CAT_N+1],.5f*(in[4]+in[5]),in[1],in[2]};
 for(int i=0;i<4;i++){err[i]=measurement[i];for(int j=0;j<CAT_N;j++)err[i]-=CAT_H[i*CAT_N+j]*z[j];}
 for(int i=0;i<CAT_N;i++){for(int j=0;j<4;j++)z[i]+=CAT_L[i*4+j]*err[j];s[i]=z[i];}
 float theta0=in[0]+.03f*in[1],theta=theta0+CAT_PARAM[11]*(z[2]-theta0);
 float omega=in[1]+CAT_PARAM[11]*(z[3]-in[1]);
 float x[4]={in[3]+CAT_PARAM[1]*(theta-s[CAT_N+1]),.5f*(in[4]+in[5])+CAT_PARAM[1]*omega,theta,omega};
 float request=0;for(int i=0;i<4;i++)request+=CAT_K[i]*x[i];
 request-=CAT_K[0]*x_ref+CAT_K[1]*v_ref;
 float desired=cat_clip(request,CAT_PARAM[4]);
 float psi=yaw_mode?0:in[6]/CAT_PARAM[0]-psi_ref,rate=(in[5]-in[4])/CAT_PARAM[0]-w_ref;
 /* Rate PI during a held gesture compensates directional load without storing
  * heading debt. Braking drops that gesture's integral; quiet hold uses R12. */
 if(yaw_mode==2U)s[CAT_N]=0;
 float integral_error=yaw_mode==1U?rate:psi;
 float ci=cat_clip(s[CAT_N]+CAT_PARAM[7]*integral_error*.005f,CAT_PARAM[8]);
 float unsat=-CAT_PARAM[5]*psi-CAT_PARAM[6]*rate-ci;
 cat_allocate(desired,cat_clip(unsat,CAT_PARAM[9]),in+4,out);
 if(fabsf(unsat-out[2])<.0001f||(unsat-out[2])*integral_error>0)s[CAT_N]=ci;
 else cat_allocate(desired,cat_clip(-CAT_PARAM[5]*psi-CAT_PARAM[6]*rate-s[CAT_N],CAT_PARAM[9]),in+4,out);
 s[CAT_N+2]=out[3];
 for(int i=0;i<CAT_N+3;i++)if(!isfinite(s[i]))return 0;
 return isfinite(out[0])&&isfinite(out[1])&&fabsf(out[0])<=2499&&fabsf(out[1])<=2499;
}
static inline int motion_cat_step8_core(const float *in,float *s,float *out,float x_ref,float v_ref,float psi_ref,float w_ref){
 return motion_cat_step8_mode_core(in,s,out,x_ref,v_ref,psi_ref,w_ref,0U);
}
static inline int motion_cat_step8(const float *in,float *s,float *out,float x_ref,float v_ref,float psi_ref,float w_ref){
 for(int i=0;i<7;i++)if(!isfinite(in[i]))return 0;
 if(!isfinite(x_ref)||!isfinite(v_ref)||!isfinite(psi_ref)||!isfinite(w_ref))return 0;
 if(psi_ref==0.0f&&w_ref==0.0f)return motion_cat_step(in,s,out,x_ref,v_ref);
 return motion_cat_step8_core(in,s,out,x_ref,v_ref,psi_ref,w_ref);
}
static inline int motion_cat_step8_mode(const float *in,float *s,float *out,float x_ref,float v_ref,float psi_ref,float w_ref,unsigned yaw_mode){
 if(yaw_mode>2U)return 0;
 if(!yaw_mode)return motion_cat_step8(in,s,out,x_ref,v_ref,psi_ref,w_ref);
 for(int i=0;i<7;i++)if(!isfinite(in[i]))return 0;
 if(!isfinite(x_ref)||!isfinite(v_ref)||!isfinite(psi_ref)||!isfinite(w_ref))return 0;
 return motion_cat_step8_mode_core(in,s,out,x_ref,v_ref,psi_ref,w_ref,yaw_mode);
}
