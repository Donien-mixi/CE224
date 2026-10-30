// RT8 supervisory-limits unit test on a synthetic plant.
// A) reverse must keep the requested speed with a 12cm run-up offset (old10cm
//    budget throttled it; the real log27 sat at 9.6cm for every reverse leg).
// B) a pure turn with a 12cm centre drift must keep full rate (old16cm knee
//    halved the rate; log27 left turns drifted ~11cm).
const fs = require('node:fs'), path = require('node:path'), cp = require('node:child_process'), assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..'), out = path.join(root, '.build/rt8_tracking');
fs.mkdirSync(out, { recursive: true });
const src = String.raw`
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"
#include "balance_turn.h"
int main(void){
 /* A) reverse with the body12cm ahead of the reference */
 BalanceMotion m;BalanceMotion_Clear(&m);
 uint32_t now=10000;
 m.ready=true;m.used=true;m.v_ref=-.5f;m.x_ref=-.12f;m.last_word=42;m.last_ms=now;
 float pos=0;
 for(int i=0;i<20;i++){
  now+=5;pos-=0.48f*0.005f;
  if(!BalanceMotion_Update(&m,true,42,pos,0,0,-.48f,now))return 2;
 }
 printf("reverse v_ref=%.3f xerr=%.3f flags=%u\n",m.v_ref,m.x_ref-pos,m.flags&(8|64|128));
 if(m.v_ref>-.45f)return 10;           /* throttled: old budget */
 if(m.flags&(8|64|128))return 11;      /* limited / cancelled */

 /* B) pure turn, then the body drifts12cm off the captured centre while held */
 BalanceMotion m2;BalanceTurn t;BalanceMotion_Clear(&m2);BalanceTurn_Clear(&t);
 now=20000;m2.ready=true;m2.used=true;m2.x_ref=0;m2.last_word=0;m2.last_ms=now;
 float yaw=0;                                     /* the axle really turns: do not
                                                     feed yaw_rate=0, that is the
                                                     blocked-turn detector */
 for(int i=0;i<100;i++){                         /* gesture captures the centre */
  now+=5;
  uint32_t word=((uint32_t)(i+1)<<2);
  if(!BalanceMotion8_Update(&m2,&t,true,word,1,0,0,0,0,yaw,t.w_ref,now))return 3;
  yaw+=t.w_ref*0.005f;
 }
 for(int i=0;i<300;i++){                         /* centre drifts and stays */
  now+=5;
  uint32_t word=((uint32_t)(101+i)<<2);
  if(!BalanceMotion8_Update(&m2,&t,true,word,1,0.12f,0,0,0,yaw,t.w_ref,now))return 3;
  yaw+=t.w_ref*0.005f;
 }
 printf("turn w_ref=%.3f xerr=%.3f flags=%u\n",t.w_ref,m2.x_ref-0.12f,m2.flags&(64|128|512));
 if(t.w_ref<1.15f)return 12;                  /* governor still halving */
 if(m2.flags&(64|128))return 13;              /* must not look blocked */

 puts("PASS rt8 tracking: reverse keeps speed at12cm run-up; turn keeps full rate at12cm drift");
 return 0;
}
`;
const file = path.join(out, 'track.c'), exe = path.join(out, 'track.exe');
fs.writeFileSync(file, src);
const inc = path.join(root, 'Xe_Can_Bang/Core/Inc');
let r = cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe', ['-std=c11', '-O2', '-Wall', '-Wextra', file, '-I' + inc, '-lm', '-o', exe], { encoding: 'utf8' });
assert.equal(r.status, 0, r.stderr);
r = cp.spawnSync(exe, [], { encoding: 'utf8' });
process.stdout.write(r.stdout);
assert.equal(r.status, 0, r.stdout + r.stderr || ('exit ' + r.status));
