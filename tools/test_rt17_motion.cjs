// Regressions identified in RT16 logs10/11. Exercise production headers directly.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt17/host';fs.mkdirSync(dir,{recursive:true});
const code=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_governor.h"
static BalanceGovernor gov;
#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&gov,o,t,r,w,p,g)
#include "balance_turn.h"
#include "balance_recovery.h"
static BalanceMotion s;static BalanceTurn t;static unsigned ms,seq;
static void reset(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);BalanceGovernor_Reset(&gov);ms=1000;}
static void tick(unsigned mode,int d,int turn,float pitch,float gyro,float speed){
 ms+=5;seq++;BalanceGovernor_Observe(&gov,400,400,speed);
 assert(BalanceMotion8_Update(&s,&t,mode,seq*4+(d>0?1:d<0?2:0),turn,s.x_ref,pitch,gyro,speed,0,t.w_ref,ms));
}
int main(void){
 // Same physical pitch/gyro and equal motor headroom: no direction-only latch.
 for(int sign=-1;sign<=1;sign+=2){
  gov=(BalanceGovernor){.speed=sign*.6f,.pressure=true};
#ifdef BALANCE_GOVERNOR_EQUILIBRIUM
  gov.equilibrium=2.0f;
#endif
  for(int i=0;i<60;i++)BalanceGovernor_Step(&gov,sign*.6f,sign*.95f,.4f,sign,4.2f,0);
  assert(!gov.pressure);
  gov=(BalanceGovernor){.speed=sign*.6f,.duty=.95f};
  BalanceGovernor_Step(&gov,sign*.6f,sign*.95f,.4f,sign,4.2f,10);assert(gov.pressure);
  gov=(BalanceGovernor){.speed=sign*.6f};
  BalanceGovernor_Step(&gov,sign*.6f,sign*.95f,.4f,sign,sign*11,0);assert(gov.pressure);
  // New reverse gesture immediately stops increasing the old reference speed.
  gov=(BalanceGovernor){.speed=sign*.4f,.acceleration=sign*.5f};
  float next=BalanceGovernor_Step(&gov,sign*.4f,-sign*.95f,1,-sign,0,0);
  assert(sign*next<.4f&&sign*next>=0);
 }
#ifdef BALANCE_GOVERNOR_EQUILIBRIUM
 BalanceGovernor_Reset(&gov);
 for(int i=0;i<159;i++)BalanceGovernor_ObserveRest(&gov,true,2,0,0,0); /* RT22:100 dwell+60 samples */
 assert(!gov.equilibrium_ready);
 BalanceGovernor_ObserveRest(&gov,true,2,0,0,0);assert(gov.equilibrium_ready&&gov.equilibrium==2);
 for(int i=0;i<1000;i++)BalanceGovernor_ObserveRest(&gov,false,-10,100,.7f,1);
 assert(gov.equilibrium==2); // moving/impact/steering cannot move the neutral reference
 for(int i=0;i<300;i++)BalanceGovernor_ObserveRest(&gov,true,.2f,0,0,0);
 assert(gov.equilibrium==0); // measured near-zero posture keeps the original predictor
#endif
 // The abort's own dwell is enough; no second500ms readiness hurdle.
 reset();s.used=true;BalanceMotion_Cancel(&s,0,0,128,ms);
 for(int i=0;i<39;i++){tick(1,0,0,1,0,0);assert(s.cancel_flags);}
 tick(1,0,0,1,0,0);assert(!s.cancel_flags&&s.ready&&s.release_required);
 tick(1,0,0,1,0,0);assert(s.flags&1U);
 tick(1,1,0,1,30,0);assert(s.v_ref>0); // harmless subsequent gyro spike cannot re-lock readiness
 reset();s.used=true;BalanceMotion_Cancel(&s,0,0,128,ms);
 for(int i=0;i<200;i++)tick(1,1,0,1,0,0);
 assert(s.release_required&&!(s.flags&1U)&&fabsf(s.v_ref)<1e-6f); // no stale held-command resume
 tick(1,0,0,1,0,0);tick(1,1,0,1,0,0);assert(s.v_ref>0);
#ifdef BALANCE_RECOVERY_LIVE_COMMANDS
 BalanceRecovery rec;BalanceRecovery_Clear(&rec);
 assert(BalanceRecovery_DriveMode(&rec,true,0,0,0,0)==1);
 // Observed~350dps/2g event now enters bounded REC1; >900dps still cuts.
 s.used=true;assert(!BalanceMotion_Impact(&s,true,350,4.5f));
 assert(BalanceMotion_Impact(&s,true,901,1)==8192);
 BalanceRecovery_Impact(&rec,true,true,350,4.5f,.05f,.3f,1000);assert(rec.active&&rec.started_ms==1000);
 BalanceRecovery_Impact(&rec,true,true,-350,4.5f,.05f,.3f,3000);assert(rec.started_ms==1000&&rec.episodes==1);
 assert(BalanceRecovery_DriveMode(&rec,true,5,20,.2f,0)==2);
 assert(BalanceRecovery_DriveMode(&rec,true,15,30,.4f,0)==3);
 assert(BalanceRecovery_DriveMode(&rec,true,5,20,.2f,.5f)==3);
 assert(!BalanceRecovery_DriveMode(&rec,false,0,0,0,0));
 assert(BalanceRecovery_UpdateMode(&rec,true,15,100,.4f,0,.3f,1000+BALANCE_RECOVERY_TIMEOUT_MS,true)==4096);
 for(int sign=-1;sign<=1;sign+=2){
  /* RT39: chỉ vượt TRẦN của interlock (max(0.8,|ref|+0.12)) và phải đủ 100 ms. */
  BalanceRecovery_Clear(&rec);
  for(int i=0;i<(int)BALANCE_RECOVERY_OVERSPEED_TICKS-1;i++)
   BalanceRecovery_OverSpeed(&rec,true,sign*.90f,sign*.50f,0,.3f,.8f,1000+5*i);
  assert(!rec.active);
  BalanceRecovery_OverSpeed(&rec,true,sign*.90f,sign*.50f,0,.3f,.8f,2000);assert(rec.active);
  /* Cua 0.78 tren tham chieu 0.749 KHONG phai vuot toc do. */
  BalanceRecovery_Clear(&rec);
  for(int i=0;i<60;i++)BalanceRecovery_OverSpeed(&rec,true,sign*.78f,sign*.749f,0,.3f,.8f,3000+5*i);
  assert(!rec.active);
  /* Xe dang lui trong khi lenh tien: khong phai runaway. */
  BalanceRecovery_Clear(&rec);
  for(int i=0;i<60;i++)BalanceRecovery_OverSpeed(&rec,true,sign*-.20f,sign*.26f,0,.3f,.8f,4000+5*i);
  assert(!rec.active);
 }
 for(int sign=-1;sign<=1;sign+=2){
  reset();s.ready=true;s.speed_preset=2;
  for(int i=0;i<1000;i++)tick(2,sign,sign,0,0,s.v_ref);
  assert(sign*s.v_ref>.249f&&fabsf(s.v_ref)<=.25001f&&fabsf(t.w_ref)<=.40001f&&(s.flags&32768U));
  float old=s.v_ref;
  for(int i=0;i<500;i++){tick(2,0,0,0,0,s.v_ref);assert(fabsf(s.v_ref)<=fabsf(old)+1e-6f);old=s.v_ref;}
  assert(fabsf(s.v_ref)<1e-5f&&fabsf(t.w_ref)<1e-5f);
 }
 // A held gesture no longer bypasses the braking anchor when REC1 starts.
 reset();s.used=s.ready=true;s.requested=1;s.v_ref=.7f;s.speed_preset=2;
 tick(3,1,0,15,80,.7f);assert(s.braking&&s.recovery_limited&&s.recovery_active);
 for(int i=0;i<400;i++)tick(3,1,0,15,80,s.v_ref);
 assert(fabsf(s.v_ref)<1e-5f);
 for(int i=0;i<280;i++)tick(1,1,0,0,0,s.v_ref);
 assert(s.v_ref<=.50001f&&s.recovery_limited); // no automatic sprint during the1.5s softening
 for(int i=0;i<900;i++)tick(1,1,0,0,0,s.v_ref);
 assert(!s.recovery_limited&&s.v_ref>.75f); // RT20: preset restored without a new gesture
 // A direction change still clears the softening immediately.
 reset();s.used=s.ready=true;s.requested=1;s.v_ref=.7f;s.speed_preset=2;
 tick(3,1,0,15,80,.7f);for(int i=0;i<400;i++)tick(3,1,0,15,80,s.v_ref);
 for(int i=0;i<50;i++)tick(1,1,0,0,0,s.v_ref);
 assert(s.recovery_limited);
 tick(1,0,0,0,0,s.v_ref);tick(1,-1,0,0,0,s.v_ref);
 assert(!s.recovery_limited);
#endif
 puts("PASS RT17: symmetric headroom, reversal response, prompt post-abort admission, no stale replay, bounded live REC1, hard guards retained.");
}
`;
fs.writeFileSync(dir+'/test.c',code);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','Xe_Can_Bang/Core/Src/balance_recovery.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
