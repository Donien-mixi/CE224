// Bounded recovery supervisor: no motors, no live hardware.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/recovery_host');fs.mkdirSync(out,{recursive:true});
const code=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_recovery.h"
static void prime(BalanceRecovery *s){for(int i=0;i<100;i++)assert(!BalanceRecovery_Update(s,true,0,0,0,0,.3f,1000+i*5));assert(s->primed&&!s->active);}
int main(void){
 BalanceRecovery s;BalanceRecovery_Clear(&s);
 // RT5: transient command tracking error must clear; sustained error or fast
 // physical rotation still triggers recovery, symmetrically in both directions.
 for(int sign=-1;sign<=1;sign+=2){
  prime(&s);
  for(int i=0;i<11;i++){assert(!BalanceRecovery_UpdateMode(&s,true,sign*13,20,sign*.36f,0,.3f,1000+5*i,true));assert(!s.active);}
  BalanceRecovery_UpdateMode(&s,true,0,0,0,0,.3f,1055,true);assert(!s.active&&s.quiet==0);
  for(int i=0;i<12;i++)BalanceRecovery_UpdateMode(&s,true,sign*13,20,sign*.36f,0,.3f,1100+5*i,true);
  assert(s.active&&s.started_ms==1155);
  BalanceRecovery_Clear(&s);prime(&s);
  // RT8: gyro spikes below150dps are suspension bumps (log27), not a disturbance.
  BalanceRecovery_UpdateMode(&s,true,0,sign*149,0,0,.3f,1195,true);assert(!s.active);
  BalanceRecovery_UpdateMode(&s,true,0,sign*150,0,0,.3f,1200,true);assert(s.active);
  BalanceRecovery_Clear(&s);
 }

 // Cannot widen startup or an unprepared first release.
 for(int i=0;i<500;i++){assert(!BalanceRecovery_Update(&s,false,60,200,1,.6f,1,5*i));assert(!s.active&&!s.primed);}
 for(int i=0;i<99;i++){assert(!BalanceRecovery_Update(&s,true,0,0,0,0,.3f,i*5));assert(!s.primed);}
 assert(!BalanceRecovery_Update(&s,true,10,90,.4f,.1f,.3f,500));assert(!s.active);
 prime(&s);
 for(int sign=-1;sign<=1;sign+=2){
  BalanceRecovery_Clear(&s);prime(&s);
  assert(!BalanceRecovery_Update(&s,true,sign*10,sign*100,sign*.5f,sign*.1f,.3f,2000));assert(s.active&&s.episodes==1);
  assert(BalanceRecovery_Angle(&s,25)==50&&BalanceRecovery_Speed(&s,.8f)==1.2f);
  assert(fabsf(BalanceRecovery_Distance(&s,.3f)-.9f)<1e-6f);
  // A second impact cannot refresh the four-second timer.
  assert(!BalanceRecovery_Update(&s,true,sign*10,sign*100,sign*.5f,0,.3f,5999));assert(s.started_ms==2000);
  assert(BalanceRecovery_Update(&s,true,sign*10,sign*100,sign*.5f,0,.3f,6000)==4096);
  // Exit requires 300ms quiet AND return into the original position budget.
  BalanceRecovery_Clear(&s);prime(&s);BalanceRecovery_Update(&s,true,10,100,.5f,0,.3f,2000);
  for(int i=0;i<100;i++){BalanceRecovery_Update(&s,true,0,0,0,.31f,.3f,2100+i*5);}assert(s.active);
  for(int i=0;i<59;i++){BalanceRecovery_Update(&s,true,0,0,0,.1f,.3f,2700+i*5);assert(s.active);}
  assert(!BalanceRecovery_Update(&s,true,0,0,0,.1f,.3f,2995)&&!s.active&&!s.primed);
  assert(BalanceRecovery_Angle(&s,25)==25&&BalanceRecovery_Speed(&s,.8f)==.8f&&BalanceRecovery_Distance(&s,.3f)==.3f);
 }
 BalanceRecovery_Clear(&s);prime(&s);BalanceRecovery_Update(&s,true,10,100,.5f,.7f,.3f,2000);assert(s.distance_limit==1);
 assert(!BalanceRecovery_Update(&s,false,0,0,0,0,.3f,2100)&&!s.active&&!s.primed&&s.episodes==0);
 assert(BalanceRecovery_Update(&s,true,NAN,0,0,0,.3f,2100)==512);
 // Unsigned time arithmetic must survive HAL tick wrap.
 BalanceRecovery_Clear(&s);prime(&s);uint32_t start=0xfffffff0U;
 BalanceRecovery_Update(&s,true,10,100,.5f,0,.3f,start);
 assert(BalanceRecovery_Update(&s,true,10,100,.5f,0,.3f,start+3999U)==0);
 assert(BalanceRecovery_Update(&s,true,10,100,.5f,0,.3f,start+4000U)==4096);
 puts("PASS bounded recovery: dwell, startup inhibit, symmetric trigger, exact ordinary limits, distance clamp, quiet/distance exit, no timeout refresh, nonfinite and tick wrap.");
}
`;
const src=path.join(out,'test.c'),exe=path.join(out,'test.exe');fs.writeFileSync(src,code);
const deployed=path.join(root,'Xe_Can_Bang/Core/Inc/balance_recovery.h');
const header=fs.existsSync(deployed)?path.dirname(deployed):path.join(root,'Simulation/experiments/r12_push_recovery_20261018');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+header,src,'-lm','-o',exe],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(exe,[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
