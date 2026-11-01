#include "phase1_catalog_control.h"
#define API __declspec(dllexport)
API void twip_catalog_control(double *in,double *state,double *result){
 float a[7],s[CAT_N+3],o[4];for(int i=0;i<7;i++)a[i]=(float)in[i];
 for(int i=0;i<CAT_N+3;i++)s[i]=(float)state[i];
 int valid=cat_step(a,s,o);for(int i=0;i<CAT_N+3;i++)result[i]=s[i];
 result[CAT_N+3]=o[0];result[CAT_N+4]=o[1];result[CAT_N+5]=valid;
}
