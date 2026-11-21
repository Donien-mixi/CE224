// RT20 regression: a press made while REC1 is still active must not leave the
// vehicle capped at 0.50m/s for the rest of the run.
// Log L2 (2)(2), 80cm/s: REC1 at219.17s, release219.33, re-press220.47 inside
// REC1, REC1 ends220.71 -> v_ref stayed0.500 until227.75 (7s) despite preset80.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt20/host';fs.mkdirSync(dir,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;static float pos;
static void reset(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);ms=1000;seq=0;pos=0;
 s.ready=true;s.used=true;s.speed_preset=2;}
/* permission1 normal,2 recovering+quiet,3 recovering+upset */
static void step(unsigned permission,int dir){
 seq++;ms+=5;pos+=s.v_ref*.005f;
 assert(BalanceMotion8_Update(&s,&t,permission,seq*4+(dir>0?1:dir<0?2:0),0,pos,0,0,s.v_ref,0,0,ms));}
int main(void){
 reset();
 for(int i=0;i<700;i++)step(1,1);
 assert(fabsf(s.v_ref-.90f)<.01f);            // preset2 reaches0.90

 for(int i=0;i<200;i++)step(2,1);             // REC1 active: cap0.25
 assert(s.recovery_limited&&s.recovery_active);
 assert(fabsf(s.v_ref-.25f)<.02f);
 for(int i=0;i<20;i++)step(2,0);              // release INSIDE REC1
 for(int i=0;i<20;i++)step(2,1);              // re-press INSIDE REC1
 assert(s.recovery_limited);

 int clear_tick=-1;
 for(int i=1;i<=500;i++){
  step(1,1);                                 // REC1 has ended, user still holds
  if(!s.recovery_limited&&clear_tick<0)clear_tick=i;
  if(i<300) assert(fabsf(s.v_ref)<=.505f);   // softening must hold for1.5s
 }
 assert(clear_tick>=295&&clear_tick<=310);   // bounded, not "until next gesture"
 assert(fabsf(s.v_ref-.90f)<.02f);           // preset restored without a new gesture

 reset();
 for(int i=0;i<700;i++)step(1,1);
 for(int i=0;i<200;i++)step(2,1);
 for(int i=0;i<20;i++)step(2,1);              // no release at all during REC1
 int c2=-1;
 for(int i=1;i<=500;i++){step(1,1);if(!s.recovery_limited&&c2<0)c2=i;}
 assert(c2>=295&&c2<=310);                    // same bound for a continuously held press
 assert(fabsf(s.v_ref-.90f)<.02f);

 reset();
 for(int i=0;i<700;i++)step(1,1);
 for(int i=0;i<200;i++)step(2,1);
 for(int i=0;i<10;i++)step(1,0);              // normal release after REC1
 step(1,-1);
 assert(!s.recovery_limited);                 // direction change still clears immediately
 printf("clear_tick=%d continuous=%d\n",clear_tick,c2);
 puts("PASS RT20: post-REC1 cap is bounded to1.5s, clears for a press made during recovery, still restores0.90, and a direction change clears immediately.");
 return 0;}
`;
fs.writeFileSync(dir+'/test.c',src);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','Xe_Can_Bang/Core/Src/balance_motion.c','Xe_Can_Bang/Core/Src/balance_turn.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});
if(r.status!==0){process.stdout.write(r.stdout);process.stderr.write(r.stderr);}
assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
