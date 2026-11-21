// RT22: the headroom governor must treat forward and reverse the same and must
// never freeze a held command. The body-motion trace below is a real forward
// ramp recorded in log RT21 (t143.9-147.0, preset80): pitch peaks at8.5deg and
// the governor stalled v_ref at0.20/0.49/0.58. Reverse is the same trace mirrored
// about the car's measured neutral posture (+1.5deg), so a symmetric policy must
// produce the same |v_ref| timeline.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt22_sym');
fs.mkdirSync(out,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"
/* 78 samples at40ms: real forward acceleration from rest at0.80m/s preset. */
static const float PITCH[78]={2.35f,1.55f,0.79f,0.04f,-0.62f,-0.83f,-0.47f,0.37f,1.51f,2.87f,4.18f,5.54f,6.72f,7.42f,7.90f,8.54f,8.30f,8.29f,8.08f,7.63f,6.96f,6.09f,5.16f,4.34f,3.63f,2.97f,2.48f,2.21f,2.27f,2.37f,2.73f,3.28f,3.95f,4.59f,5.21f,5.75f,6.18f,6.55f,6.83f,7.08f,7.27f,7.15f,6.86f,6.37f,5.92f,5.36f,4.85f,4.54f,4.17f,3.86f,3.61f,3.54f,3.62f,3.62f,3.65f,3.74f,3.96f,4.17f,4.41f,4.39f,4.30f,3.98f,3.57f,3.11f,2.72f,2.44f,2.18f,2.12f,2.30f,2.45f,2.44f,2.34f,2.25f,2.09f,1.92f,2.03f,2.43f,2.90f};
static const float GYRO[78]={-6.2f,-22.1f,-19.9f,-17.6f,-15.0f,5.6f,15.1f,25.7f,32.6f,32.3f,35.6f,31.5f,26.9f,8.0f,19.0f,-10.1f,3.6f,-1.9f,-7.5f,-16.8f,-14.3f,-22.2f,-23.5f,-19.6f,-16.6f,-15.4f,-8.5f,-4.9f,3.5f,6.3f,7.0f,14.8f,17.3f,16.2f,15.4f,13.5f,12.3f,5.7f,7.5f,8.9f,-0.1f,-5.0f,-7.0f,-13.4f,-16.7f,-9.9f,-6.3f,-9.6f,-8.2f,-5.0f,-9.9f,2.3f,5.2f,-0.5f,-0.5f,3.8f,6.1f,5.7f,4.9f,-3.3f,-3.4f,-11.0f,-10.4f,-10.4f,-5.2f,-13.5f,-4.2f,5.1f,5.2f,3.3f,-3.6f,-1.2f,-2.0f,-4.4f,-3.5f,5.2f,14.0f,5.0f};
static const float NEUTRAL=1.5f, PRESET=0.80f;
struct R { float t95, final, max_pressure_s, max_crawl_gap_s; int tripped; };
/* Governor-in-the-loop: the recorded body motion, speed modelled as a0.25s lag of
 * the reference. dir=-1 mirrors the posture about the measured neutral. */
static struct R ramp(int dir,float eq,int ready){
 BalanceGovernor g={0};g.equilibrium=eq;g.equilibrium_ready=ready!=0;
 float vref=0,spd=0,old=0,t95=-1,maxp=0,press_run=0,freeze=0,maxfreeze=0;int tripped=0;
 for(int i=0;i<1200;i++){          /* 6s at5ms */
  int k=i/8; if(k>77)k=77;
  float pitch=dir>0?PITCH[k]:(2*NEUTRAL-PITCH[k]);
  float gyro=dir>0?GYRO[k]:-GYRO[k];
  float rate=fabsf(vref)>.50f?.22f:.50f;
  float target=dir*PRESET;
  old=vref;
  float next=BalanceGovernor_Step(&g,old,target,rate,dir,pitch,gyro);
  assert(isfinite(next));
  vref=next;
  if(g.pressure){tripped=1;press_run+=.005f;if(press_run>maxp)maxp=press_run;}
  else press_run=0;
  /* A stall: pressure on but the reference did not progress at all. */
  if(g.pressure&&fabsf(vref-old)<1e-6f){freeze+=.005f;if(freeze>maxfreeze)maxfreeze=freeze;}
  else freeze=0;
  spd+=(vref-spd)*.005f/.25f;
  g.speed=spd;
  if(t95<0&&fabsf(vref)>=.95f*PRESET)t95=i*.005f;
 }
 struct R r={t95,vref,maxp,maxfreeze,tripped};return r;
}
int main(void){
 /* 1. Learned neutral: forward and reverse must ramp identically. */
 struct R f=ramp(+1,NEUTRAL,1), b=ramp(-1,NEUTRAL,1);
 printf("learned: fwd t95=%.2f final=%.3f maxP=%.2f | rev t95=%.2f final=%.3f maxP=%.2f\n",f.t95,f.final,f.max_pressure_s,b.t95,b.final,b.max_pressure_s);
 assert(f.t95>0&&b.t95>0);                       /* both reach95% of the preset */
 assert(fabsf(f.final)>.7999f&&fabsf(b.final)>.7999f); /* exact preset */
 assert(fabsf(f.t95-b.t95)<.40f);                /* equal acceleration */
 assert(f.max_pressure_s<1.0f&&b.max_pressure_s<1.0f);
 assert(f.max_crawl_gap_s<.50f&&b.max_crawl_gap_s<.50f); /* never frozen */
 /* 2. Neutral not learned yet (the RT21 log state): the raise to9deg alone must
  *    still keep forward and reverse within0.5s of each other. */
 struct R f0=ramp(+1,0,0), b0=ramp(-1,0,0);
 printf("unlearned: fwd t95=%.2f final=%.3f | rev t95=%.2f final=%.3f\n",f0.t95,f0.final,b0.t95,b0.final);
 assert(f0.t95>0&&b0.t95>0);
 assert(fabsf(f0.final)>.7999f&&fabsf(b0.final)>.7999f);
 assert(fabsf(f0.t95-b0.t95)<.50f);
 /* 3. Preset spread:50 and72 also reach their exact target in both directions. */
 float preset[2]={.50f,.72f};
 for(int p=0;p<2;p++){
  for(int d=-1;d<=1;d+=2){
   BalanceGovernor g={0};float vref=0,spd=0;
   for(int i=0;i<1200;i++){
    float next=BalanceGovernor_Step(&g,vref,d*preset[p],fabsf(vref)>.5f?.22f:.5f,d,NEUTRAL,0);
    vref=next;spd+=(vref-spd)*.005f/.25f;g.speed=spd;
   }
   printf("preset%.2f dir%+d final=%.4f\n",preset[p],d,vref);
   assert(fabsf(fabsf(vref)-preset[p])<.0002f);
  }
 }
 /* 4. A real overload (14deg lean, PWM saturated) must still be limited, but it
  *    crawls instead of freezing: <=0.06m/s^2 means <=0.06m/s in one second. */
 {
  BalanceGovernor g={0};g.duty=1.0f;float vref=0,mark=0;
  for(int i=0;i<200;i++){vref=BalanceGovernor_Step(&g,vref,.80f,.5f,1,14.0f,0);g.speed=vref;g.duty=1.0f;}
  assert(g.pressure&&g.limited);
  mark=vref;
  for(int i=0;i<200;i++){vref=BalanceGovernor_Step(&g,vref,.80f,.5f,1,14.0f,0);g.speed=vref;g.duty=1.0f;}
  printf("overload: vref %.3f -> %.3f in1s\n",mark,vref);
  assert(fabsf(vref-mark)<=.0701f&&vref<.80f);
 }
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'sym.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra){
  const exe=path.join(out,extra.length?'old.exe':'sym.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,...extra,file,'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,r.stderr||String(r.error));
  return exe;
}
// The same test must FAIL against the RT21 constants:5deg/8deg lean,3deg clear,
//15deg gyro and a0m/s^2 cap (a full freeze). This proves the test is sensitive
// to the reported bug, not just to the new numbers.
const oldExe=build(['-DBALANCE_GOV_LEAN_DEG=5.0f','-DBALANCE_GOV_LEAN_LAUNCH_DEG=8.0f',
  '-DBALANCE_GOV_CLEAR_DEG=3.0f','-DBALANCE_GOV_CLEAR_GYRO=15.0f','-DBALANCE_GOV_LAUNCH_GYRO=25.0f',
  '-DBALANCE_GOV_CRAWL=0.0f']);
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'the RT21 constants should fail this symmetry test');
console.log('old-constant probe fails as expected (forward stall / asymmetry).');
const exe=build([]);
const r=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(r.stdout||'');
assert.equal(r.status,0,(r.stdout||'')+(r.stderr||''));
console.log('PASS RT22 symmetry: forward/reverse equal ramp, exact preset50/72/80, no freeze, overload still limited.');
