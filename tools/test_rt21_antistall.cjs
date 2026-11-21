// RT21-A regression: the governor must not deadlock the reference when the
// vehicle cannot break static friction immediately (log RT20: v_ref frozen at
// 0.09-0.14m/s for up to 1.64s while dir=+1 and v~0). A real wall must still
// be detected by the motion blockage logic.
const fs=require('fs'),cp=require('child_process'),assert=require('assert/strict');
const dir='.build/rt21/host';fs.mkdirSync(dir,{recursive:true});
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
static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;static float pos,peak;
static void reset(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);BalanceGovernor_Reset(&gov);
 ms=1000;seq=0;pos=0;peak=0;s.ready=true;s.used=true;s.speed_preset=2;}
static void step(int dir,float speed){
 seq++;ms+=5;BalanceGovernor_Observe(&gov,300,300,speed);
 assert(BalanceMotion8_Update(&s,&t,1,seq*4+(dir>0?1:dir<0?2:0),0,pos,2.0f,0,speed,0,0,ms));
 if(fabsf(s.v_ref)>peak)peak=fabsf(s.v_ref);}
int main(void){
 /* A: truly stuck wheel. The shaper must keep trying, and the motion blockage
  * detector must still cancel — the anti-deadlock must not hide a wall. */
 reset();
 for(int i=0;i<400&&!s.cancel_flags;i++)step(1,0.0f);
 assert(peak>.25f);
 assert(s.cancel_flags&128U);
 printf("A: peak=%.3f cancel_at=%.2fs\n",peak,(ms-1000)/1000.0);

 /* B: static friction releases once the reference reaches 0.18m/s. Before the
  * fix the reference froze at ~0.14 and the vehicle never moved. */
 reset();float v=0;
 for(int i=0;i<1000;i++){
  step(1,v);
  float vref=fabsf(s.v_ref);
  if(vref>0.14f)v=vref;              // wheel starts turning and follows
  pos+=v*.005f;
 }
 assert(!s.cancel_flags);
 assert(fabsf(s.v_ref-.90f)<.03f);
 printf("B: v_ref=%.3f v=%.3f pos=%.3f no_cancel\n",s.v_ref,v,pos);
 puts("PASS RT21-A: no low-speed deadlock, preset reached once friction breaks, real wall still cancels.");
 return 0;}
`;
fs.writeFileSync(dir+'/test.c',src);
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror','-I'+(process.env.TWIP_HEADER_DIR||'Xe_Can_Bang/Core/Inc'),dir+'/test.c','Xe_Can_Bang/Core/Src/balance_motion.c','Xe_Can_Bang/Core/Src/balance_turn.c','Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',dir+'/test.exe'],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr);
r=cp.spawnSync(dir+'/test.exe',[],{encoding:'utf8'});
if(r.status!==0){process.stdout.write(r.stdout);process.stderr.write(r.stderr);}
assert.equal(r.status,0,r.stderr);process.stdout.write(r.stdout);
