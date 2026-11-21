const f=require('fs'),cp=require('child_process'),assert=require('assert/strict');f.mkdirSync('.build/rt11_response',{recursive:true});const source=String.raw`
#include <assert.h>
#include <stdio.h>
#include "balance_turn.h"
int main(void){
 BalanceMotion s={.ready=true,.speed_preset=2};uint32_t now=1000;int ticks=0;
 for(;ticks<700;ticks++){now+=5;assert(BalanceMotion_Update(&s,true,4U*(ticks+1)+1,s.x_ref,0,0,s.v_ref,now));}
 assert(fabsf(s.v_ref-.90f)<.0001f); //RT19 preset2=0.90m/s, ramped without a step
 int zero=-1;for(int i=0;i<680;i++){now+=5;assert(BalanceMotion_Update(&s,true,4U*(i+701)+2,s.x_ref,0,0,s.v_ref,now));if(zero<0&&s.v_ref<=0)zero=i;}
 assert(zero>=0&&zero<=200);assert(fabsf(s.v_ref+.90f)<.0001f);
 BalanceMotion_Clear(&s);s.ready=true;s.speed_preset=2;BalanceTurn t={0};float yaw=0;
 for(int i=0;i<150;i++){now+=5;yaw+=t.w_ref*.005f;assert(BalanceMotion8_Update(&s,&t,true,4U*(i+1),1,0,0,0,0,yaw,t.w_ref,now));}
 assert(fabsf(t.w_ref-2.4f)<.0001f); //RT22 pure-turn rate
 for(int i=0;i<220;i++){now+=5;yaw+=t.w_ref*.005f;assert(BalanceMotion8_Update(&s,&t,true,4U*(i+151),-1,0,0,0,0,yaw,t.w_ref,now));}
 assert(fabsf(t.w_ref+2.4f)<.0001f);
 // Lost movement refresh still requires release; PING cannot refresh this word.
 uint32_t word=s.last_word;now+=BALANCE_MOTION_STALE_MS+1U;assert(BalanceMotion8_Update(&s,&t,true,word,-1,0,0,0,0,yaw,0,now));assert(t.release_required&&t.w_ref==0);
 puts("PASS RT11: preset2(0.90) target/ramp, reverse zero<=1s, yaw ramp<=0.75s/reversal<=1.1s, movement lease retained.");}
`;f.writeFileSync('.build/rt11_response/test.c',source);let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-IXe_Can_Bang/Core/Inc','.build/rt11_response/test.c','Xe_Can_Bang/Core/Src/balance_motion.c','-lm','-o','.build/rt11_response/test.exe'],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);r=cp.spawnSync('.build/rt11_response/test.exe',[],{encoding:'utf8'});assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
