// RT19 regression: heading anchor must stop chasing after a turn release, and
// the hold anchor must be re-captured after the post-brake roll-back.
// Reproduces log(13)(2): +59deg over4.9s on a straight command, and a9cm
// roll-back that left the force loop pushing ~+240PWM without motion.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt19/host';fs.mkdirSync(dir,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"
static BalanceGovernor gov;
#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&gov,o,t,r,w,p,g)
#define BALANCE_MOTION_ANCHOR(r,p,d) BalanceGovernor_Anchor(&gov,&r,p,d)
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;static float pos,yaw;
static void reset(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);BalanceGovernor_Reset(&gov);
 ms=1000;seq=0;pos=0;yaw=0;s.ready=true;}
static void step(int dir,int turn,float pitch,float gy,float speed,float yawrate){
 seq++;ms+=5;yaw+=yawrate*.005f;pos+=speed*.005f;BalanceGovernor_Observe(&gov,600,600,speed);
 assert(BalanceMotion8_Update(&s,&t,1,seq*4+(dir>0?1:dir<0?2:0),turn,pos,pitch,gy,speed,yaw,yawrate,ms));}
int main(void){
 reset();s.used=true;t.used=true;
 for(int i=0;i<300;i++)step(0,-1,2,0,0,-1.6f);
 assert(!t.braking);
 step(0,0,2,0,0,-0.25f);assert(t.braking);
 int ticks=1;while(t.braking&&ticks<400){step(0,0,2,0,0,-0.25f);ticks++;}
 assert(!t.braking&&ticks<=121);
 float frozen=t.psi_ref;
 for(int i=0;i<50;i++){step(0,0,2,0,0,-0.25f);assert(fabsf(t.psi_ref-frozen)<1e-6f);}
 assert(fabsf(yaw-t.psi_ref)>.05f);

 reset();s.used=true;
 for(int i=0;i<400;i++)step(1,0,3,0,.55f,0);
 assert(s.v_ref>.3f);
 step(0,0,3,0,.55f,0);assert(s.braking);
 float x_at_release=pos;
 for(int i=0;i<40;i++)step(0,0,3,0,.10f,0);
 for(int i=0;i<120;i++)step(0,0,3,0,.04f,0);
 int n=0;while(s.stop_ticks<40&&n<500){step(0,0,0,0,0,0);n++;}
 assert(fabsf(s.x_ref-pos)<1e-4f);
 float held=s.x_ref;
 for(int i=0;i<200;i++)step(0,0,0,0,.01f,0);
 assert(fabsf(s.x_ref-held)<1e-6f);
 printf("brake_end_ticks=%d release_x=%.3f final_x=%.3f err=%.5f\n",ticks,x_at_release,pos,pos-s.x_ref);
 puts("PASS RT19: heading anchor freezes within0.6s of a turn release and never chases a residual yaw; the hold anchor is recaptured after the post-brake roll-back and is not rebased by a later slow drift.");
 return 0;}
`;
fs.writeFileSync(dir+'/test.c',src);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
