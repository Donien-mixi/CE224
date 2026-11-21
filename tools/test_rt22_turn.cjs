// RT22/RT23: sharper steering must still respect the wheel-speed, lateral, lean
// and blockage limits, and the RT23 scaling fixes must hold. Measured on the real
// BalanceMotion8_Update / BalanceTurn code.
const fs=require('node:fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt22_turn');
fs.mkdirSync(out,{recursive:true});
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
static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;static float drift;
static void init(void){BalanceGovernor_Reset(&gov);BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);s.ready=true;s.speed_preset=2;ms=1000;seq=0;drift=0;}
/* One 5ms tick. turn_cmd!=0 holds the steering; yaw_rate follows w_ref unless
 * overridden, so the blockage detector is not fooled by a synthetic standstill.
 * drift is the pivot's linear migration away from the captured centre. */
static void tick(int dir,int turn_cmd,float pitch,float speed,float yaw_rate_scale){
 ms+=5;seq++;
 float yaw_rate=yaw_rate_scale*t.w_ref;
 assert(BalanceMotion8_Update(&s,&t,1U,seq*4+(dir>0?1:dir<0?2:0),turn_cmd,s.x_ref-drift,pitch,0,speed,0,yaw_rate,ms));
}
int main(void){
 /* 1. Pure turn at the fast preset:2.4rad/s within0.6s (was2.2 in0.8s). */
 init();
 for(int i=0;i<120;i++)tick(0,1,2.0f,0.0f,1.0f);
 printf("pure turn0.6s: w_ref=%.2f\n",t.w_ref);
 assert(t.w_ref>=2.30f&&t.w_ref<=2.41f);
 /* 2. Diagonal at0.8m/s: previously capped at0.5rad/s by the lateral ceiling. */
 init();s.v_ref=.80f;
 for(int i=0;i<120;i++)tick(1,1,2.0f,0.8f,1.0f);
 printf("diagonal0.8m/s: w_ref=%.2f flags=0x%x\n",t.w_ref,s.flags);
 assert(t.w_ref>=0.90f&&t.w_ref<=1.16f);
 /* 3. Lateral ceiling van chan: w_ref*guard <=0.90, voi guard la toc do da
  *    lenh (RT36) chu khong phai |v| do: guard=max(|v_ref|,|v|-0.10). */
 assert(t.w_ref*fmaxf(fabsf(s.v_ref),fabsf(0.8f)-0.10f)<=BALANCE_TURN_LATERAL_ACCEL+.01f);
 /* 4. RT23 wheel cap follows the COMMANDED speed plus a10cm tracking allowance
  * and a per-wheel envelope. At preset72 a measured1.12m/s (envelope+0.14) still
  * pins the cap to0.10rad/s. */
 init();s.speed_preset=1;s.v_ref=1.04f;
 for(int i=0;i<120;i++)tick(1,1,2.0f,1.12f,1.0f);
 printf("preset72 runaway measured1.12: w_ref=%.2f\n",t.w_ref);
 assert(t.w_ref<=.11f);
 /* 5. RT23 regression: the observed fast-preset diagonal (RT32: commanded0.80, measured
  *0.79) must hold1.15rad/s; RT22 gave0.10-0.46 and jittered. */
 init();s.speed_preset=1;s.v_ref=.80f;
 for(int i=0;i<120;i++)tick(1,1,2.0f,0.79f,1.0f);
 printf("preset80 diag v_ref0.80 measured0.79: w_ref=%.2f flags=0x%x\n",t.w_ref,s.flags);
 assert(t.w_ref>=1.10f&&t.w_ref<=1.20f);
 /* 6. RT23 regression: 15cm of pivot creep is normal, so the requested rate must
  * not fade (RT22 centre taper cut this to0.70*rate =>1.68rad/s). */
 init();
 for(int i=0;i<40;i++)tick(0,1,2.0f,0.0f,1.0f);
 drift=.15f;
 for(int i=0;i<120;i++)tick(0,1,2.0f,0.0f,1.0f);
 printf("pure turn with15cm pivot creep: w_ref=%.2f\n",t.w_ref);
 assert(t.w_ref>=2.30f);
 /* 7. A genuine slide is still throttled: 60cm of migration cuts the rate. */
 init();
 for(int i=0;i<40;i++)tick(0,1,2.0f,0.0f,1.0f);
 drift=.60f;
 for(int i=0;i<120;i++)tick(0,1,2.0f,0.0f,1.0f);
 printf("pure turn with60cm slide: w_ref=%.2f\n",t.w_ref);
 assert(t.w_ref<=2.40f*BALANCE_TURN_CENTER_FLOOR+.02f);
 /* 8. RT24: the lean taper measures the lean BEYOND the learned posture, so a10deg
  * absolute lean with a0 reference still cuts (12-10)/4=0.5 ->1.20rad/s, while the
  * same10deg lean of a normal left pivot (reference+1.07) is only8.9deg of excess
  * and keeps full authority. Both behaviours are checked in test_rt24_sym. */
 init();
 for(int i=0;i<120;i++)tick(0,1,10.0f,0.0f,1.0f);
 printf("lean10deg: w_ref=%.2f\n",t.w_ref);
 assert(t.w_ref>=1.10f&&t.w_ref<=1.30f);
 /* 9. A real blockage (no yaw at all) still cancels the gesture. */
 init();
 int cancelled=0;
 for(int i=0;i<400;i++){tick(0,1,2.0f,0.0f,0.0f);if(s.cancel_flags){cancelled=1;break;}}
 assert(cancelled);
 /* 10. Neutral stops the requested rate immediately. */
 init();
 for(int i=0;i<100;i++)tick(0,1,2.0f,0.0f,1.0f);
 assert(t.w_ref>2.0f);
 tick(0,0,2.0f,0.0f,1.0f);
 assert(t.w_ref==0.0f);
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'turn.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'turn.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
// Sensitivity: the RT21 turn constants must fail the sharper-steering checks.
const old=path.join(out,'turn_old.exe');
r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,
 '-DBALANCE_TURN_PURE_RATE=(s->speed_preset?2.2f:1.2f)','-DBALANCE_TURN_DIAGONAL_RATE=(s->speed_preset?.55f:.80f)',
 '-DBALANCE_TURN_ACCEL=3.0f','-DBALANCE_TURN_LATERAL_ACCEL=0.4f',file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',old],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const oldRun=cp.spawnSync(old,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'the RT21 turn constants should fail the sharper-steering checks');
console.log('old-constant probe fails as expected (0.50rad/s diagonal, 2.2rad/s pure).');
// Sensitivity: the RT22 wheel cap / centre taper must fail the RT23 regressions.
const prior=path.join(out,'turn_rt22.exe');
r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,
 '-DBALANCE_TURN_WHEEL_ENV_72=0.80f','-DBALANCE_TURN_CENTER_KNEE=0.22f','-DBALANCE_TURN_CENTER_SPAN=0.10f',
 '-DBALANCE_TURN_CENTER_FLOOR=0.40f',file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',prior],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const priorRun=cp.spawnSync(prior,[],{encoding:'utf8'});
assert.notEqual(priorRun.status,0,'the RT22 turn scaling should fail the RT23 regression checks');
console.log('RT22-scaling probe fails as expected (0.46rad/s preset72 diagonal, 15cm creep drops the rate).');
console.log('PASS RT22/RT23 steering:2.4rad/s pure,1.12 diagonal at0.8m/s, preset72 diagonal keeps1.15, 15cm creep keeps the rate, wheel/lateral/lean/blockage limits intact.');
