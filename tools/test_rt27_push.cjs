// RT27 push policy: back to the RT21-ANTISTALL reference behaviour. A push must
// NOT make the velocity reference follow the body (RT25/RT26 did that: v_ref:=speed
// meant the R12 law kept driving the vehicle in the direction it was already
// rolling, so a light push ran away to0.87m/s and fell).
// Replays the recorded window before the RT26 fall and checks the reference.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt27_push');
fs.mkdirSync(out,{recursive:true});
const POS="0.3068f,0.3067f,0.3059f,0.3042f,0.3016f,0.2983f,0.2940f,0.2891f,0.2830f,0.2763f,0.2688f,0.2604f,0.2512f,0.2412f,0.2304f,0.2189f,0.2065f,0.1935f,0.1797f,0.1652f,0.1500f,0.1341f,0.1176f,0.1011f,0.0839f,0.0666f,0.0492f,0.0318f,0.0143f,-0.0031f,-0.0206f,-0.0379f,-0.0551f,-0.0722f,-0.0895f,-0.1062f,-0.1233f,-0.1405f,-0.1553f",PITCH="-14.0941f,-14.4925f,-15.1662f,-15.7439f,-16.1826f,-16.5375f,-16.8755f,-17.0927f,-17.2291f,-17.2826f,-17.2567f,-17.1910f,-17.0861f,-16.9635f,-16.7774f,-16.5235f,-16.3487f,-15.9752f,-15.7063f,-15.3573f,-15.0131f,-14.5777f,-14.0981f,-13.6934f,-13.4353f,-13.3737f,-13.6983f,-14.3296f,-15.2569f,-16.6271f,-18.3574f,-20.6854f,-23.7025f,-27.3335f,-31.4335f,-36.5791f,-43.0630f,-49.3234f,-53.5264f",GYRO="-6.4584f,-28.5316f,-37.1291f,-29.8730f,-24.8121f,-22.4340f,-21.0316f,-15.3609f,-12.1901f,-5.5438f,0.1879f,1.0416f,-0.4828f,-0.7877f,5.6757f,7.0782f,4.5172f,19.9440f,11.5294f,18.0538f,14.0294f,22.4440f,20.3099f,11.7123f,5.3709f,-9.2023f,-29.5072f,-41.3365f,-61.5804f,-82.6779f,-105.7877f,-140.3609f,-170.1779f,-204.9340f,-221.7023f,-311.5194f,-326.8243f,-253.5926f,-158.1048f",SPEED="0.0052f,-0.0040f,-0.0310f,-0.0696f,-0.1129f,-0.1512f,-0.1991f,-0.2326f,-0.2813f,-0.3197f,-0.3598f,-0.4035f,-0.4442f,-0.4857f,-0.5261f,-0.5607f,-0.6035f,-0.6392f,-0.6756f,-0.7154f,-0.7482f,-0.7817f,-0.8133f,-0.8245f,-0.8472f,-0.8578f,-0.8652f,-0.8661f,-0.8712f,-0.8686f,-0.8724f,-0.8655f,-0.8650f,-0.8554f,-0.8598f,-0.8395f,-0.8544f,-0.8566f,-0.7504f",THETA="-14.3277f,-15.0810f,-15.8682f,-16.3739f,-16.8070f,-17.1939f,-17.5754f,-17.7691f,-17.9238f,-17.9353f,-17.8830f,-17.8779f,-17.8745f,-17.8131f,-17.5699f,-17.3673f,-17.2746f,-16.7499f,-16.6877f,-16.2902f,-16.0940f,-15.6222f,-15.2831f,-15.2577f,-15.1627f,-15.5194f,-16.2636f,-17.0995f,-18.5270f,-20.2757f,-22.5942f,-25.6661f,-29.1728f,-33.5678f,-38.0242f,-45.4458f,-51.6282f,-56.5798f,0.0000f",SEQ="397,398,398,399,400,401,402,403,403,404,405,406,407,408,408,409,410,411,412,413,413,414,415,416,417,418,418,419,420,421,422,423,423,424,425,426,427,428,0",PHASE="2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0";
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#include "balance_governor.h"
static BalanceGovernor gov;
float BalanceMotion_FilterHook(float o,float t,float r,int w,float p,float g){return BalanceGovernor_Step(&gov,o,t,r,w,p,g);}
int BalanceMotion_GovernorActive(void){return 1;}
void BalanceMotion_AnchorHook(float *r,float p,int d){BalanceGovernor_Anchor(&gov,r,p,d);}
#include "phase1_catalog_params.h"
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static uint32_t ms;static int seq;
#define N 39
static const float POS[N]={0.3068f,0.3067f,0.3059f,0.3042f,0.3016f,0.2983f,0.2940f,0.2891f,0.2830f,0.2763f,0.2688f,0.2604f,0.2512f,0.2412f,0.2304f,0.2189f,0.2065f,0.1935f,0.1797f,0.1652f,0.1500f,0.1341f,0.1176f,0.1011f,0.0839f,0.0666f,0.0492f,0.0318f,0.0143f,-0.0031f,-0.0206f,-0.0379f,-0.0551f,-0.0722f,-0.0895f,-0.1062f,-0.1233f,-0.1405f,-0.1553f};
static const float PITCH[N]={-14.0941f,-14.4925f,-15.1662f,-15.7439f,-16.1826f,-16.5375f,-16.8755f,-17.0927f,-17.2291f,-17.2826f,-17.2567f,-17.1910f,-17.0861f,-16.9635f,-16.7774f,-16.5235f,-16.3487f,-15.9752f,-15.7063f,-15.3573f,-15.0131f,-14.5777f,-14.0981f,-13.6934f,-13.4353f,-13.3737f,-13.6983f,-14.3296f,-15.2569f,-16.6271f,-18.3574f,-20.6854f,-23.7025f,-27.3335f,-31.4335f,-36.5791f,-43.0630f,-49.3234f,-53.5264f};
static const float GYRO[N]={-6.4584f,-28.5316f,-37.1291f,-29.8730f,-24.8121f,-22.4340f,-21.0316f,-15.3609f,-12.1901f,-5.5438f,0.1879f,1.0416f,-0.4828f,-0.7877f,5.6757f,7.0782f,4.5172f,19.9440f,11.5294f,18.0538f,14.0294f,22.4440f,20.3099f,11.7123f,5.3709f,-9.2023f,-29.5072f,-41.3365f,-61.5804f,-82.6779f,-105.7877f,-140.3609f,-170.1779f,-204.9340f,-221.7023f,-311.5194f,-326.8243f,-253.5926f,-158.1048f};
static const float SPEED[N]={0.0052f,-0.0040f,-0.0310f,-0.0696f,-0.1129f,-0.1512f,-0.1991f,-0.2326f,-0.2813f,-0.3197f,-0.3598f,-0.4035f,-0.4442f,-0.4857f,-0.5261f,-0.5607f,-0.6035f,-0.6392f,-0.6756f,-0.7154f,-0.7482f,-0.7817f,-0.8133f,-0.8245f,-0.8472f,-0.8578f,-0.8652f,-0.8661f,-0.8712f,-0.8686f,-0.8724f,-0.8655f,-0.8650f,-0.8554f,-0.8598f,-0.8395f,-0.8544f,-0.8566f,-0.7504f};
static const float THETA[N]={-14.3277f,-15.0810f,-15.8682f,-16.3739f,-16.8070f,-17.1939f,-17.5754f,-17.7691f,-17.9238f,-17.9353f,-17.8830f,-17.8779f,-17.8745f,-17.8131f,-17.5699f,-17.3673f,-17.2746f,-16.7499f,-16.6877f,-16.2902f,-16.0940f,-15.6222f,-15.2831f,-15.2577f,-15.1627f,-15.5194f,-16.2636f,-17.0995f,-18.5270f,-20.2757f,-22.5942f,-25.6661f,-29.1728f,-33.5678f,-38.0242f,-45.4458f,-51.6282f,-56.5798f,0.0000f};
static const int SEQ[N]={397,398,398,399,400,401,402,403,403,404,405,406,407,408,408,409,410,411,412,413,413,414,415,416,417,418,418,419,420,421,422,423,423,424,425,426,427,428,0};
static const int PHASE[N]={2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0};
/* Same expression as motion_cat_step8_mode_core. */
static float request(float x,float xref,float v,float vref,float theta_deg,float gyro_dps){
 float th=theta_deg*(float)M_PI/180.0f, om=gyro_dps*(float)M_PI/180.0f;
 float r=CAT_K[0]*(x-xref)+CAT_K[1]*(v-vref)+CAT_K[2]*th+CAT_K[3]*om;
 return fmaxf(-CAT_PARAM[4],fminf(CAT_PARAM[4],r));
}
int main(void){
 setbuf(stdout,NULL);
 BalanceGovernor_Reset(&gov);BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);
 s.ready=true;ms=1000;seq=0;
 float worst_vref=0,min_force=1e9f,max_gap=0;
 for(int i=0;i<N;i++){
  ms+=20;seq++;
  unsigned perm=PHASE[i]==0?1U:3U;
  assert(BalanceMotion8_Update(&s,&t,perm,(uint32_t)SEQ[i]<<2,0,POS[i],PITCH[i],GYRO[i],SPEED[i],0.0f,0.0f,ms));
  if(fabsf(s.v_ref)>worst_vref)worst_vref=fabsf(s.v_ref);
  /* RT26 policy fed v_ref:=speed; the RT21 policy leaves the reference alone. */
  float f26=request(POS[i],0.0f,SPEED[i],SPEED[i],THETA[i],GYRO[i]);
  float f27=request(POS[i],0.0f,SPEED[i],0.0f,THETA[i],GYRO[i]);
  if(f26-f27>max_gap)max_gap=f26-f27;
  if(i>=N-8&&f27<min_force)min_force=f27;
  if(i%5==0)printf("  %2d pitch%+7.2f v%+.2f  v_ref%+.3f  F_rt26%+6.2f  F_rt27%+6.2f\n",
    i,PITCH[i],SPEED[i],s.v_ref,f26,f27);
 }
 printf("max |v_ref| with a neutral command: %.3f m/s (RT26 followed the body)\n",worst_vref);
 printf("worst gap: the RT21 references brake %.2f N harder than the RT26 catch did\n",max_gap);
 printf("strongest braking with the RT27 references: %.2f N\n",min_force);
 /* 1. The reference must stay at rest: this is the whole point. */
 assert(worst_vref<=0.02f);
 /* 2. In the last part of the push the RT21 references ask for real braking,
  *    while the RT26 catch asked for ~no damping (positive = still driving). */
 assert(min_force<=-5.0f);
 assert(max_gap>=4.0f);
 /* 3. No reference following means the anchor is untouched too. */
 assert(fabsf(s.x_ref)<=0.05f);
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'push.c');
fs.writeFileSync(file,src.replace('0.3068f,0.3067f,0.3059f,0.3042f,0.3016f,0.2983f,0.2940f,0.2891f,0.2830f,0.2763f,0.2688f,0.2604f,0.2512f,0.2412f,0.2304f,0.2189f,0.2065f,0.1935f,0.1797f,0.1652f,0.1500f,0.1341f,0.1176f,0.1011f,0.0839f,0.0666f,0.0492f,0.0318f,0.0143f,-0.0031f,-0.0206f,-0.0379f,-0.0551f,-0.0722f,-0.0895f,-0.1062f,-0.1233f,-0.1405f,-0.1553f',POS).replace('-14.0941f,-14.4925f,-15.1662f,-15.7439f,-16.1826f,-16.5375f,-16.8755f,-17.0927f,-17.2291f,-17.2826f,-17.2567f,-17.1910f,-17.0861f,-16.9635f,-16.7774f,-16.5235f,-16.3487f,-15.9752f,-15.7063f,-15.3573f,-15.0131f,-14.5777f,-14.0981f,-13.6934f,-13.4353f,-13.3737f,-13.6983f,-14.3296f,-15.2569f,-16.6271f,-18.3574f,-20.6854f,-23.7025f,-27.3335f,-31.4335f,-36.5791f,-43.0630f,-49.3234f,-53.5264f',PITCH).replace('-6.4584f,-28.5316f,-37.1291f,-29.8730f,-24.8121f,-22.4340f,-21.0316f,-15.3609f,-12.1901f,-5.5438f,0.1879f,1.0416f,-0.4828f,-0.7877f,5.6757f,7.0782f,4.5172f,19.9440f,11.5294f,18.0538f,14.0294f,22.4440f,20.3099f,11.7123f,5.3709f,-9.2023f,-29.5072f,-41.3365f,-61.5804f,-82.6779f,-105.7877f,-140.3609f,-170.1779f,-204.9340f,-221.7023f,-311.5194f,-326.8243f,-253.5926f,-158.1048f',GYRO)
 .replace('0.0052f,-0.0040f,-0.0310f,-0.0696f,-0.1129f,-0.1512f,-0.1991f,-0.2326f,-0.2813f,-0.3197f,-0.3598f,-0.4035f,-0.4442f,-0.4857f,-0.5261f,-0.5607f,-0.6035f,-0.6392f,-0.6756f,-0.7154f,-0.7482f,-0.7817f,-0.8133f,-0.8245f,-0.8472f,-0.8578f,-0.8652f,-0.8661f,-0.8712f,-0.8686f,-0.8724f,-0.8655f,-0.8650f,-0.8554f,-0.8598f,-0.8395f,-0.8544f,-0.8566f,-0.7504f',SPEED).replace('-14.3277f,-15.0810f,-15.8682f,-16.3739f,-16.8070f,-17.1939f,-17.5754f,-17.7691f,-17.9238f,-17.9353f,-17.8830f,-17.8779f,-17.8745f,-17.8131f,-17.5699f,-17.3673f,-17.2746f,-16.7499f,-16.6877f,-16.2902f,-16.0940f,-15.6222f,-15.2831f,-15.2577f,-15.1627f,-15.5194f,-16.2636f,-17.0995f,-18.5270f,-20.2757f,-22.5942f,-25.6661f,-29.1728f,-33.5678f,-38.0242f,-45.4458f,-51.6282f,-56.5798f,0.0000f',THETA).replace('397,398,398,399,400,401,402,403,403,404,405,406,407,408,408,409,410,411,412,413,413,414,415,416,417,418,418,419,420,421,422,423,423,424,425,426,427,428,0',SEQ).replace('2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,0',PHASE));
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'push.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
console.log('PASS RT27 push: neutral command keeps v_ref=0, so the law brakes instead of chasing the push.');
