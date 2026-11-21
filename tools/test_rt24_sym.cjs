// RT24: turn speed must be symmetric while the vehicle pivots. Replays the pitch
// trace recorded on the car (log RT23-oncar, preset80, pure turns) through the
// real BalanceMotion8_Update. The lean taper must key on the lean BEYOND the
// learned neutral posture; the RT23 rule keyed on absolute pitch and cut the left
// turn to 0.65-0.91 of the request because a left pivot rides at +6.5deg.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt24_sym');
fs.mkdirSync(out,{recursive:true});
const LEFT_TRACE="-0.435f,-0.745f,-1.243f,-2.014f,-1.868f,-1.026f,0.976f,2.601f,3.650f,3.794f,3.624f,3.083f,2.457f,2.210f,2.571f,3.514f,4.106f,4.258f,4.280f,4.186f,4.116f,4.556f,4.844f,4.745f,4.544f,4.675f,4.739f,4.741f,5.259f,5.243f,4.662f,3.902f,3.372f,3.605f,4.202f,4.911f,5.676f,6.195f,6.055f,5.389f,5.072f,4.917f,5.328f,6.731f,7.956f,8.218f,7.804f,7.335f,6.849f,6.700f,6.854f,7.105f,7.336f,7.487f,7.296f,6.954f,6.545f,6.338f,6.694f,6.282f,5.857f,5.825f,5.908f,6.041f,6.191f,5.985f,5.903f,6.023f,6.168f,6.184f,6.235f,6.527f,6.708f,7.277f,7.440f,7.625f,7.695f,7.794f,7.883f,7.937f,7.868f,7.400f,6.831f,6.433f,6.516f,7.165f,7.862f,8.266f,8.529f,8.571f,8.531f,7.769f,6.339f,5.005f,4.313f,4.390f,5.098f,5.928f,6.734f,7.119f,7.263f,7.361f,7.008f,6.362f,5.843f,5.693f,5.742f,6.349f,7.238f,7.564f,7.576f,7.471f,7.193f,6.592f,6.340f,6.516f,6.972f";
const RIGHT_TRACE="2.107f,2.371f,2.218f,0.744f,-0.427f,-1.265f,-1.232f,-0.864f,0.184f,1.826f,3.697f,5.270f,4.733f,3.368f,1.739f,0.446f,0.061f,0.756f,1.772f,1.901f,1.808f,1.789f,1.750f,1.309f,0.589f,0.405f,0.919f,1.446f,1.704f,1.648f,1.644f,1.323f,0.570f,-0.290f,-0.311f,0.086f,0.248f,-0.173f,-0.919f,-1.592f,-2.115f,-1.907f,-1.722f,-1.869f,-2.256f,-2.700f,-3.307f,-3.792f,-3.363f,-2.226f,-1.483f,-1.426f,-1.863f,-2.188f,-2.533f,-2.645f,-2.173f,-1.224f,-0.482f,-0.530f,-1.091f,-1.783f,-2.571f,-2.669f,-2.512f,-2.192f,-1.847f,-1.947f,-2.478f,-3.797f,-4.077f,-3.903f,-3.703f,-4.027f,-4.036f,-3.823f,-3.889f,-4.200f,-4.204f,-3.462f,-2.741f,-2.498f,-2.627f,-2.631f,-2.787f,-3.189f,-2.745f,-2.245f,-1.663f,-1.692f,-2.302f,-2.396f";
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"
static BalanceGovernor gov;
float BalanceMotion_FilterHook(float o,float t,float r,int w,float p,float g){return BalanceGovernor_Step(&gov,o,t,r,w,p,g);}
int BalanceMotion_GovernorActive(void){return 1;}
void BalanceMotion_AnchorHook(float *r,float p,int d){BalanceGovernor_Anchor(&gov,r,p,d);}
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static uint32_t ms;static int seq;
/* Learned neutral posture from the car's own BINFO (fields24/25 =1.074deg,1). */
#define EQ 1.074f
static const float LEFT[117]={-0.435f,-0.745f,-1.243f,-2.014f,-1.868f,-1.026f,0.976f,2.601f,3.650f,3.794f,3.624f,3.083f,2.457f,2.210f,2.571f,3.514f,4.106f,4.258f,4.280f,4.186f,4.116f,4.556f,4.844f,4.745f,4.544f,4.675f,4.739f,4.741f,5.259f,5.243f,4.662f,3.902f,3.372f,3.605f,4.202f,4.911f,5.676f,6.195f,6.055f,5.389f,5.072f,4.917f,5.328f,6.731f,7.956f,8.218f,7.804f,7.335f,6.849f,6.700f,6.854f,7.105f,7.336f,7.487f,7.296f,6.954f,6.545f,6.338f,6.694f,6.282f,5.857f,5.825f,5.908f,6.041f,6.191f,5.985f,5.903f,6.023f,6.168f,6.184f,6.235f,6.527f,6.708f,7.277f,7.440f,7.625f,7.695f,7.794f,7.883f,7.937f,7.868f,7.400f,6.831f,6.433f,6.516f,7.165f,7.862f,8.266f,8.529f,8.571f,8.531f,7.769f,6.339f,5.005f,4.313f,4.390f,5.098f,5.928f,6.734f,7.119f,7.263f,7.361f,7.008f,6.362f,5.843f,5.693f,5.742f,6.349f,7.238f,7.564f,7.576f,7.471f,7.193f,6.592f,6.340f,6.516f,6.972f};
static const float RIGHT[92]={2.107f,2.371f,2.218f,0.744f,-0.427f,-1.265f,-1.232f,-0.864f,0.184f,1.826f,3.697f,5.270f,4.733f,3.368f,1.739f,0.446f,0.061f,0.756f,1.772f,1.901f,1.808f,1.789f,1.750f,1.309f,0.589f,0.405f,0.919f,1.446f,1.704f,1.648f,1.644f,1.323f,0.570f,-0.290f,-0.311f,0.086f,0.248f,-0.173f,-0.919f,-1.592f,-2.115f,-1.907f,-1.722f,-1.869f,-2.256f,-2.700f,-3.307f,-3.792f,-3.363f,-2.226f,-1.483f,-1.426f,-1.863f,-2.188f,-2.533f,-2.645f,-2.173f,-1.224f,-0.482f,-0.530f,-1.091f,-1.783f,-2.571f,-2.669f,-2.512f,-2.192f,-1.847f,-1.947f,-2.478f,-3.797f,-4.077f,-3.903f,-3.703f,-4.027f,-4.036f,-3.823f,-3.889f,-4.200f,-4.204f,-3.462f,-2.741f,-2.498f,-2.627f,-2.631f,-2.787f,-3.189f,-2.745f,-2.245f,-1.663f,-1.692f,-2.302f,-2.396f};
/* RT34: n*STEP - from co the toi 281 mau (117*4 - 187); mang cu 117 bi tran. */
static float samples[600];
static void init(void){BalanceGovernor_Reset(&gov);BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);s.ready=true;s.speed_preset=2;ms=1000;seq=0;}
/* Records |w_ref| of the last60% of the gesture, sample by sample.
 * The stored trace is decimated x4, so each entry is held for STEP 5ms ticks:
 * a full2.40rad/s ramp needs0.53s and must fit inside the replay. */
#define STEP 4
static float replay(const float *pitch,int n,int turn_cmd,float *out_min){
 init();
 int k=0,ticks=n*STEP,from=(int)(ticks*0.4);
 for(int i=0;i<n;i++)for(int rep=0;rep<STEP;rep++){
  ms+=5;seq++;
#ifdef NO_LEAN_REF
  t.lean_ref=0.0f;                 /* RT23 behaviour: absolute pitch */
#else
  t.lean_ref=EQ;                   /* RT24: lean beyond the neutral posture */
#endif
  assert(BalanceMotion8_Update(&s,&t,1U,seq*4,turn_cmd,s.x_ref,pitch[i],0,0.05f,0,t.w_ref,ms));
  if(k>=0&&i*STEP+rep>=from&&k<(int)(sizeof(samples)/sizeof(samples[0])))samples[k++]=fabsf(t.w_ref);
 }
 float lo=1e9f;
 for(int i=0;i<k;i++)if(samples[i]<lo)lo=samples[i];
 for(int i=0;i<k;i++)for(int j=i+1;j<k;j++)if(samples[j]<samples[i]){float x=samples[i];samples[i]=samples[j];samples[j]=x;}
 *out_min=lo;
 return samples[k/2];
}
int main(void){
 setbuf(stdout,NULL);
 float loL,loR;
 float medL=replay(LEFT,117,1,&loL);
 float medR=replay(RIGHT,92,-1,&loR);
 printf("left pivot : w_ref median %.2f min %.2f\n",medL,loL);
 printf("right pivot: w_ref median %.2f min %.2f\n",medR,loR);
 /* Full requested2.40rad/s in BOTH directions, sampled from the real lean. */
 assert(medL>=2.30f);
 assert(medR>=2.30f);
 assert(fabsf(medL-medR)<=0.12f);
 /* The left pivot's own lean must not produce a deep rate dip either. */
 assert(loL>=2.30f);
 assert(loR>=2.30f);
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'turn.c');
fs.writeFileSync(file,src.replace('-0.435f,-0.745f,-1.243f,-2.014f,-1.868f,-1.026f,0.976f,2.601f,3.650f,3.794f,3.624f,3.083f,2.457f,2.210f,2.571f,3.514f,4.106f,4.258f,4.280f,4.186f,4.116f,4.556f,4.844f,4.745f,4.544f,4.675f,4.739f,4.741f,5.259f,5.243f,4.662f,3.902f,3.372f,3.605f,4.202f,4.911f,5.676f,6.195f,6.055f,5.389f,5.072f,4.917f,5.328f,6.731f,7.956f,8.218f,7.804f,7.335f,6.849f,6.700f,6.854f,7.105f,7.336f,7.487f,7.296f,6.954f,6.545f,6.338f,6.694f,6.282f,5.857f,5.825f,5.908f,6.041f,6.191f,5.985f,5.903f,6.023f,6.168f,6.184f,6.235f,6.527f,6.708f,7.277f,7.440f,7.625f,7.695f,7.794f,7.883f,7.937f,7.868f,7.400f,6.831f,6.433f,6.516f,7.165f,7.862f,8.266f,8.529f,8.571f,8.531f,7.769f,6.339f,5.005f,4.313f,4.390f,5.098f,5.928f,6.734f,7.119f,7.263f,7.361f,7.008f,6.362f,5.843f,5.693f,5.742f,6.349f,7.238f,7.564f,7.576f,7.471f,7.193f,6.592f,6.340f,6.516f,6.972f',LEFT_TRACE).replace('2.107f,2.371f,2.218f,0.744f,-0.427f,-1.265f,-1.232f,-0.864f,0.184f,1.826f,3.697f,5.270f,4.733f,3.368f,1.739f,0.446f,0.061f,0.756f,1.772f,1.901f,1.808f,1.789f,1.750f,1.309f,0.589f,0.405f,0.919f,1.446f,1.704f,1.648f,1.644f,1.323f,0.570f,-0.290f,-0.311f,0.086f,0.248f,-0.173f,-0.919f,-1.592f,-2.115f,-1.907f,-1.722f,-1.869f,-2.256f,-2.700f,-3.307f,-3.792f,-3.363f,-2.226f,-1.483f,-1.426f,-1.863f,-2.188f,-2.533f,-2.645f,-2.173f,-1.224f,-0.482f,-0.530f,-1.091f,-1.783f,-2.571f,-2.669f,-2.512f,-2.192f,-1.847f,-1.947f,-2.478f,-3.797f,-4.077f,-3.903f,-3.703f,-4.027f,-4.036f,-3.823f,-3.889f,-4.200f,-4.204f,-3.462f,-2.741f,-2.498f,-2.627f,-2.631f,-2.787f,-3.189f,-2.745f,-2.245f,-1.663f,-1.692f,-2.302f,-2.396f',RIGHT_TRACE));
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'turn.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
let run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
// Old-rule probe: absolute pitch with the RT23 (12-|pitch|)/6 taper must fail.
const old=path.join(out,'turn_rt23.exe');
r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,
 '-DNO_LEAN_REF','-DBALANCE_TURN_LEAN_SPAN=6.0f',file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'-lm','-o',old],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const oldRun=cp.spawnSync(old,[],{encoding:'utf8'});
process.stdout.write(oldRun.stdout||'');
assert.notEqual(oldRun.status,0,'the RT23 absolute-pitch lean taper should fail this replay');
console.log('RT23-style probe fails as expected (left pivot throttled by its own lean).');
console.log('PASS RT24 lean taper: both pivot directions hold the requested rate on the recorded lean trace.');
