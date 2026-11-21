// RT28: a disturbance recovery must keep the position anchor. Logs RT27 (10)(2)
// and (11)(2) show REC1 entry re-capturing x_ref at the pushed position (+0.219 ->
// +0.110 and -0.967 -> -0.943 in one tick), so the vehicle returned there instead
// of its own place. This drives the real motion layer through an idle push.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt28_anchor');
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
static BalanceMotion s;static BalanceTurn t;static uint32_t ms;static int seq;
static void init(void){BalanceGovernor_Reset(&gov);BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);
 s.ready=true;s.speed_preset=2;s.used=true;s.x_ref=0.0f;s.v_ref=0.0f;s.stable=100;ms=1000;seq=0;}
/* One5ms tick: neutral word (no drive command), body displaced by pos, pushed. */
static void tick(unsigned perm,float pos,float pitch,float gyro,float speed,int dir){
 ms+=5;seq++;
 assert(BalanceMotion8_Update(&s,&t,perm,(uint32_t)seq<<2|(dir>0?1U:dir<0?2U:0U),0,pos,pitch,gyro,speed,0,0,ms));
}
int main(void){
 setbuf(stdout,NULL);
 /* 1. Vehicle has been driven before and stands at its anchor (x_ref=0). A push
  * moves it back10cm and REC1 starts (permission3). The anchor must not move. */
 init();
 float worst=0;
 for(int i=0;i<60;i++){                       /* pushed -0.10 m, still upright */
  tick(3,-0.10f,-2.0f,-130.0f,-0.45f,0);
  if(fabsf(s.x_ref)>worst)worst=fabsf(s.x_ref);
 }
 printf("anchor during REC1 entry + push: %.4f m (was0.000)\n",s.x_ref);
 assert(fabsf(s.x_ref)<1e-6f);
 assert(worst<1e-6f);
 /* 2. While REC1 continues and the vehicle is pushed further, the anchor stays. */
 for(int i=0;i<200;i++){tick(3,-0.28f,-6.0f,-60.0f,-0.60f,0);assert(fabsf(s.x_ref)<1e-6f);}
 /* 3. Recovery ends: brake completes, the anchor is still the original one, so the
  * position loop can pull the vehicle home. */
 for(int i=0;i<120;i++)tick(1,-0.02f,1.0f,5.0f,0.02f,0);
 printf("after settle: x_ref=%.4f braking=%d hold=%d\n",s.x_ref,(int)s.braking,(int)s.hold_anchor);
 assert(fabsf(s.x_ref)<1e-6f);
 /* 4. A new gesture still captures its own start (unchanged behaviour). */
 init();
 tick(1,0.50f,2.0f,3.0f,0.05f,1);              /* fresh forward press at x=+0.50 */
 printf("new gesture: x_ref=%.3f used=%d\n",s.x_ref,(int)s.used);
 assert(fabsf(s.x_ref-0.50f)<0.02f);
 /* 4b. Sensitivity: the RT27 call (BeginBrake at the measured position) DOES
  * move the anchor, which is exactly what this test forbids. */
 {BalanceMotion c=s;float before=c.x_ref;BalanceMotion_BeginBrake(&c,-0.28f,ms);
  printf("old call would move x_ref %.3f -> %.3f\n",before,c.x_ref);
  assert(fabsf(c.x_ref-before)>0.05f);}
 /* 5. A commanded release still re-anchors at the stop point (RT19 behaviour). */
 init();
 for(int i=0;i<40;i++)tick(1,0.30f,2.0f,3.0f,0.40f,1);   /* drive */
 for(int i=0;i<200;i++)tick(1,0.30f,1.0f,2.0f,0.01f,0);   /* release, stop at+0.30 */
 printf("commanded stop: x_ref=%.3f braking=%d\n",s.x_ref,(int)s.braking);
 assert(fabsf(s.x_ref-0.30f)<0.05f);
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'anchor.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'anchor.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
console.log('PASS RT28 anchor: REC1 entry keeps x_ref, a new gesture and a commanded stop still capture their own points.');
