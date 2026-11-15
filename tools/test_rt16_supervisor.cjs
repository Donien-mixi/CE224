// Production reference supervisor against the failure patterns in seven RT15 logs.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt16/supervisor';fs.mkdirSync(dir,{recursive:true});
const code=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_governor.h"
static BalanceGovernor gov;
#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&gov,o,t,r,w,p,g)
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static unsigned now,seq;
static void step(int dir,float p,float v,float y){
 now+=5;seq++;BalanceGovernor_Observe(&gov,200,200,v);
 assert(BalanceMotion8_Update(&s,&t,true,seq*4+(dir>0?1:dir<0?2:0),0,0,p,0,v,0,y,now));
}
static void reset(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);BalanceGovernor_Reset(&gov);now=1000;}
int main(void){
 // Log3/7: neutral arrives during abort. It must not leave the4s timer armed
 // once the body actually settles. A 9cm old-anchor error is not a new wall.
 reset();s.used=true;s.x_ref=.09f;BalanceMotion_Cancel(&s,0,0,128,now);
 for(int i=0;i<101;i++)step(0,4,.02f,.2f);
 assert(!s.cancel_flags&&!BalanceMotion_Fault(&s,0,now+4000));
 // Completion cannot restart an old press, even with continuing new packets.
 reset();s.used=true;BalanceMotion_Cancel(&s,0,0,128,now);
 for(int i=0;i<240;i++)step(1,4,0,0);
 assert(!s.cancel_flags&&s.release_required&&!(s.flags&1U)&&fabsf(s.v_ref)<1e-5f);
 for(int i=0;i<105;i++)step(0,0,0,0);
 step(1,0,0,0);assert(s.ready&&s.v_ref>0);
 // Log8: ordinary braking must complete at the actual R12 quiet angle, not
 // require an unrelated absolute3deg zero. No position/count reset is involved.
 reset();s.ready=s.used=true;s.requested=1;BalanceMotion_BeginBrake(&s,0,now);
 for(int i=0;i<25;i++)step(0,4,.02f,0);
 assert(!s.braking&&!BalanceMotion_Fault(&s,0,now+4000));
 reset();BalanceMotion_BeginBrake(&s,0,now);
 assert(!BalanceMotion_Fault(&s,1.1f,now+2000));
 assert(BalanceMotion_Fault(&s,1.26f,now+2000)==8);
 // Real runaway and unsettled cancellation still trip, both directions.
 for(int sign=-1;sign<=1;sign+=2){
  reset();BalanceMotion_Cancel(&s,0,sign*.3f,128,now);
  for(int i=0;i<100;i++)step(0,15,sign*.3f,1);
  assert(s.cancel_flags&&BalanceMotion_Fault(&s,sign*1.01f,now)==8);
  assert(BalanceMotion_Fault(&s,0,5000)==4096);
 }
 // Real measured launch angles6deg must not latch only forward at any preset.
 // High duty and large lean still inhibit acceleration rather than remove guards.
 for(int preset=0;preset<3;preset++)for(int sign=-1;sign<=1;sign+=2){
  reset();s.ready=true;s.speed_preset=preset;
  for(int i=0;i<100;i++)step(sign,sign*6,sign*.05f,0);
  assert(s.v_ref*sign>.10f&&!s.cancel_flags);
 }
 gov=(BalanceGovernor){.speed=.2f};BalanceGovernor_Step(&gov,.2f,.95f,.7f,1,12,0);assert(gov.pressure); //RT22:11deg launch threshold
 puts("PASS RT16: neutral abort completion, no stale resume, brake settle, real runaway, symmetric biased launch.");
}
`;
fs.writeFileSync(dir+'/test.c',code);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
