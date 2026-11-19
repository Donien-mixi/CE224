const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');fs.mkdirSync('.build/governor_test',{recursive:true});const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_governor.h"
static BalanceGovernor gov;
#define BALANCE_MOTION_FILTER(o,t,r,w,p,g) BalanceGovernor_Step(&gov,o,t,r,w,p,g)
#define BALANCE_MOTION_ANCHOR(r,p,d) BalanceGovernor_Anchor(&gov,&r,p,d)
#include "balance_turn.h"
static BalanceMotion s;
static BalanceTurn t;
static uint32_t ms,seq;
static void step(int dir,int pwm){ms+=5;seq++;BalanceGovernor_Observe(&gov,pwm,pwm,s.v_ref);assert(BalanceMotion8_Update(&s,&t,true,seq*4+(dir>0?1:dir<0?2:0),0,s.x_ref,0,0,s.v_ref,0,0,ms));}
static void init(void){BalanceGovernor_Reset(&gov);BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);s.ready=true;s.speed_preset=2;}
 int main(void){
 init();float old=0,last_a=0;
 for(int i=0;i<1400;i++){step(1,800);assert(fabsf(s.v_ref-old)<.00351f);assert(isfinite(gov.acceleration));if(i<300)assert(fabsf(gov.acceleration-last_a)<.0151f);old=s.v_ref;last_a=gov.acceleration;}
 assert(fabsf(s.v_ref-.90f)<.0001f&&s.ready&&!s.cancel_flags);
 // RT19: preset2 is0.90m/s; the governor must still reach it with reserve.
 for(int i=0;i<10000;i++){step(1,800);assert(fabsf(s.v_ref-.90f)<.0001f);}
 float high=s.v_ref;for(int i=0;i<200;i++){step(1,2499);}assert(gov.limited&&s.v_ref<high-.02f&&s.v_ref>high-.15f&&s.requested==1&&s.ready);
 for(int i=0;i<1600;i++){step(-1,800);}assert(fabsf(s.v_ref+.90f)<.0001f);
 init();for(int i=0;i<350;i++){step(1,800);}float before=s.v_ref;
 for(int i=0;i<250;i++){step(1,2380);}/* RT22: pressure crawls at0.06m/s^2 instead of freezing: <=0.075m/s over1.25s */
 assert(gov.limited&&s.v_ref>before-.01f&&s.v_ref<before+.10f&&s.v_ref<.90f&&s.requested==1&&s.ready);
 for(int i=0;i<1000;i++){step(1,800);}assert(!gov.pressure&&s.v_ref>before+.1f);
 for(int i=0;i<800;i++){step(0,2499);}assert(fabsf(s.v_ref)<.0001f); // pressure cannot prevent operator release
 gov.pressure=true;float anchor=1.0f;BalanceGovernor_Anchor(&gov,&anchor,0,1);assert(fabsf(anchor-.10f)<.0001f);
 init();s.used=true;s.requested=1;s.v_ref=.3f;s.x_ref=.09f;s.last_word=5;s.last_ms=ms;gov.pressure=true;gov.duty=.95f;
 for(int i=0;i<80;i++){ms+=5;seq++;BalanceGovernor_Observe(&gov,2380,2380,0);assert(BalanceMotion8_Update(&s,&t,true,seq*4+1,0,0,0,0,0,0,0,ms));}assert(s.cancel_flags&128U); // reference debt bound must not mask a wall
 BalanceGovernor_Reset(&gov);assert(BalanceGovernor_Step(&gov,0,.95f,.6f,1,0,0)<.0001f); // no launch acceleration step
 // A recovered low-speed launch can resume after100ms, but not sooner.
 gov=(BalanceGovernor){.pressure=true,.speed=.20f};
 for(int i=0;i<19;i++){BalanceGovernor_Step(&gov,.20f,.95f,.6f,1,3.2f,0);assert(gov.pressure);}
 BalanceGovernor_Step(&gov,.20f,.95f,.6f,1,3.2f,0);assert(!gov.pressure);
 // This does not reduce high-speed dwell or clear pressure without PWM reserve.
 gov=(BalanceGovernor){.pressure=true,.speed=.8f};
 for(int i=0;i<59;i++){BalanceGovernor_Step(&gov,.8f,.95f,.3f,1,0,0);assert(gov.pressure);}
 BalanceGovernor_Step(&gov,.8f,.95f,.3f,1,0,0);assert(!gov.pressure);
 gov=(BalanceGovernor){.speed=.2f};
 BalanceGovernor_Step(&gov,.2f,.95f,.6f,1,4,15);assert(!gov.pressure);
 gov=(BalanceGovernor){.speed=.6f};
 BalanceGovernor_Step(&gov,.6f,.95f,.3f,1,9,15);assert(gov.pressure); //RT37:10deg predicted entry at speed; RT17 fixes the exit latch
 gov=(BalanceGovernor){.pressure=true,.speed=.2f};
 for(int i=0;i<19;i++){BalanceGovernor_Step(&gov,.2f,.5f,.6f,1,0,0);assert(gov.pressure);}
 BalanceGovernor_Step(&gov,.2f,.5f,.6f,1,0,0);assert(!gov.pressure); //RT16 all low-speed launches use100ms
 gov=(BalanceGovernor){.pressure=true,.speed=.2f,.duty=.91f};
 for(int i=0;i<100;i++){BalanceGovernor_Step(&gov,.2f,.95f,.6f,1,0,0);assert(gov.pressure);}
 gov=(BalanceGovernor){.speed=.2f};
 for(int i=0;i<100;i++)BalanceGovernor_Step(&gov,.2f,.95f,.6f,1,0,0);
 assert(fabsf(gov.acceleration-.50f)<.0001f);
 assert(isnan(BalanceGovernor_Step(&gov,NAN,.95f,.6f,1,0,0)));
 for(int sign=-1;sign<=1;sign+=2){
  gov=(BalanceGovernor){.acceleration=.6f*sign,.speed=.4f*sign};float v=.4f*sign;
  for(int i=0;i<400;i++){float next=BalanceGovernor_Step(&gov,v,0,.9f,0,0,0);assert(fabsf(next)<=fabsf(v)+.000001f);assert(next*sign>=-.000001f);v=next;}
  assert(fabsf(v)<.00001f);
 }
 puts("PASS governor: jerk/ramp, preset2(0.90) reach/long hold/reverse, pressure continuous hold/resume, neutral monotonicity both directions and nonfinite.");}
`;fs.writeFileSync('.build/governor_test/test.c',src);let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-IXe_Can_Bang/Core/Inc','.build/governor_test/test.c','-lm','-o','.build/governor_test/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);r=cp.spawnSync('.build/governor_test/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
