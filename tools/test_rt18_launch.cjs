// RT17 log12: inverse launch, false blockage and post-capture admission.
// Production C headers, both signs; no fake encoder signs or force law.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt18/host';fs.mkdirSync(dir,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_governor.h"
static BalanceGovernor gov;
#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&gov,o,t,r,w,p,g)
#define BALANCE_MOTION_ANCHOR(r,p,d) BalanceGovernor_Anchor(&gov,&r,p,d)
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static unsigned ms,seq;
static void init(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);BalanceGovernor_Reset(&gov);ms=1000;seq=0;}
static void step(int d,float x,float p,float gy,float v,int pwm){
 ms+=5;seq++;BalanceGovernor_Observe(&gov,pwm,pwm,v);
 assert(BalanceMotion8_Update(&s,&t,1,seq*4+(d>0?1:d<0?2:0),0,x,p,gy,v,0,0,ms));
}
int main(void){
 for(int sign=-1;sign<=1;sign+=2){
  init();for(int i=0;i<99;i++){step(0,sign*.11f,sign*2.8f,0,0,300);assert(!s.ready);}
  step(0,sign*.11f,sign*2.8f,0,0,300);
  assert(s.ready&&s.x_ref==0&&!s.used); // no force/anchor jump at idle
  init();for(int i=0;i<200;i++)step(sign,sign*.11f,sign*2.8f,0,0,300);
  assert(!s.ready&&!s.used&&s.v_ref==0); // no queued press admitted before readiness
  for(int unsafe=0;unsafe<3;unsafe++){
   init();for(int i=0;i<200;i++)step(0,sign*.11f,sign*(unsafe==0?6:2.8f),unsafe==1?30:0,unsafe==2?.2f:0,300);
   assert(!s.ready); // relaxing the old ARM-position test cannot bypass motion/angle quietness
  }
  // Log148.187: modest PWM/angle, ref positive while measured still negative.
  gov=(BalanceGovernor){.speed=-sign*.102f,.duty=.2f,.acceleration=sign*.3f};
  BalanceGovernor_Step(&gov,sign*.021f,sign*.5f,.6f,sign,sign*1.74f,sign*8.7f);
  // RT21: a launch with the wheels still moving backwards is expected, not a
  // headroom claim. Duty, lean and a very large lag remain hard evidence.
  assert(!gov.pressure&&!gov.limited);
  for(int reason=0;reason<3;reason++){
   gov=(BalanceGovernor){.speed=-sign*.102f,.duty=reason==0?.95f:.2f};
   BalanceGovernor_Step(&gov,sign*(reason==2?.3f:.021f),sign*.5f,.6f,sign,sign*(reason==1?12:1.74f),0);
   assert(gov.pressure);
  }
  gov=(BalanceGovernor){.speed=-sign*.25f,.duty=.2f};
  BalanceGovernor_Step(&gov,sign*.021f,sign*.5f,.6f,sign,sign*1.74f,0);
  assert(!gov.pressure); // 0.27mps while accelerating is still a launch transient
  // Fresh press into a real stationary obstacle still cancels in bounded time.
  init();s.ready=true;unsigned cancelled=0;
  for(int i=0;i<600;i++){step(sign,0,0,0,0,400);if(s.cancel_flags){cancelled=ms-1000;break;}}
  assert(cancelled>0&&cancelled<=2500&&s.release_required&&!s.ready);
  for(int i=0;i<200;i++)step(sign,0,0,0,0,400);
  assert(s.release_required&&fabsf(s.v_ref)<1e-5f); // held packets cannot restart wall
  step(0,0,0,0,0,400);step(-sign,0,0,0,0,400);assert(s.ready&&-sign*s.v_ref>0);
  // Small demand/low-speed crossover is no longer the300ms wall detector.
  init();s.used=s.ready=true;s.requested=sign;s.x_ref=sign*.08f;s.v_ref=sign*.072f;
  gov.speed=sign*.026f;gov.pressure=true;gov.duty=.95f;
  for(int i=0;i<80;i++)step(sign,0,sign*4.17f,sign*6,sign*.026f,2380);
  assert(!s.cancel_flags); // the prior real-log logic probe cancelled here
  // A governor-held tiny reference must not disable real blockage forever.
  init();s.used=s.ready=true;s.requested=sign;s.x_ref=sign*.08f;s.v_ref=sign*.04f;
  gov.pressure=true;gov.duty=.95f;unsigned low_cancel=0;
  for(int i=0;i<320;i++){step(sign,0,sign*4,0,0,2380);if(s.cancel_flags){low_cancel=ms-1000;break;}}
  // RT21: the anti-stall lifts a frozen sub0.15 reference to0.15, which moves
 // the blockage evidence from300ms to200ms; cancellation stays bounded.
 assert(low_cancel>=950&&low_cancel<=1100);
  // Strong position evidence and high-reference blockage keep original bounds.
  init();s.used=s.ready=true;s.requested=sign;s.x_ref=sign*.31f;s.v_ref=sign*.05f;
  for(int i=0;i<20;i++)step(sign,0,0,0,0,400);
  assert(s.cancel_flags&128U);
  init();s.used=s.ready=true;s.requested=sign;s.x_ref=sign*.09f;s.v_ref=sign*.4f;
  for(int i=0;i<60;i++)step(sign,0,0,0,0,400);
  assert(s.cancel_flags&128U);
 }
 puts("PASS RT18: quiet offset admission/no idle rebase/no queued press, headroom preserved during inverse launch, true wall/strong lead cancellation, no stale restart, both signs.");
}
`;
fs.writeFileSync(dir+'/test.c',src);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
