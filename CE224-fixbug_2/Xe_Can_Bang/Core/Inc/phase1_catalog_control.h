#ifndef PHASE1_CATALOG_CONTROL_H
#define PHASE1_CATALOG_CONTROL_H
/* Causal 200Hz control law, HAL independent. Units: m, s, rad, N, Nm.
 * Candidate only. No plant truth, measured VM or motor current is available. */
#include <math.h>
#include "phase1_catalog_params.h"
static inline float cat_clip(float a,float b){return fmaxf(-b,fminf(b,a));}
static inline void cat_reset(float *s,float phi){
 for(int i=0;i<CAT_N+3;i++)s[i]=0;
 s[2]=phi/CAT_PARAM[10];s[5]=phi;s[CAT_N+1]=s[2];
 const int d=(CAT_N-8)/2;for(int i=8+d;i<CAT_N;i++)s[i]=phi;
}
static inline void cat_allocate(float total,float yaw,const float *v,float *out){
 const float track=CAT_PARAM[0],capacity=CAT_PARAM[2]*CAT_PARAM[3];float loss[2];
 for(int j=0;j<2;j++){
  float request=total*.5f+(j==0?-yaw:yaw)/track;
  int direction=v[j]<0?1:0;if(fabsf(v[j])<.002f)direction=request<0?1:0;
  float smooth=fabsf(v[j])/(fabsf(v[j])+.005f);
  float sign=smooth*tanhf(v[j]/.01f)+(1-smooth)*tanhf(request/.01f);
  float friction=smooth*CAT_FRICTION[j*2+direction]+(1-smooth)*CAT_START[j*2+direction];
  loss[j]=CAT_PARAM[2]/CAT_VNL[j*2+direction]*v[j]+friction*sign;
 }
 float low[2]={-capacity-loss[0],-capacity-loss[1]};
 float high[2]={capacity-loss[0],capacity-loss[1]};
 total=fmaxf(low[0]+low[1],fminf(high[0]+high[1],total));float average=total*.5f;
 float dlo=fmaxf(low[0]-average,average-high[1]),dhi=fminf(high[0]-average,average-low[1]);
 float diff=fmaxf(dlo,fminf(dhi,-yaw/track));
 out[0]=roundf(2499*(average+diff+loss[0])/capacity);
 out[1]=roundf(2499*(average-diff+loss[1])/capacity);
 out[2]=-diff*track;out[3]=capacity*(out[0]+out[1])/2499-loss[0]-loss[1];
}
static inline int cat_step(const float *in,float *s,float *out){
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
#endif
