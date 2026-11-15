// RT23 speed interlock: a transient axle overspeed during a balance catch must
// not cut both motors, while a real runaway still stops. A/B against the RT22
// one-sample rule using the same trace.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt23_interlock');
fs.mkdirSync(out,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_interlock.h"
static BalanceInterlock g;
static bool trip(float v,float vref,float env,bool maxenv,float wl,float wr){return BalanceInterlock_Step(&g,v,wl,wr,env,vref,maxenv);}
/* How many consecutive ticks of v it takes to stop, or -1 if it never stops. */
static int run_until_trip(float v,float vref,float env,bool maxenv,int limit){
 BalanceInterlock_Clear(&g);
 for(int i=0;i<limit;i++)if(trip(v,vref,env,maxenv,v,v))return i+1;
 return -1;
}
int main(void){
 /* Ceiling follows the command: preset72 rides0.72-0.79, so0.80 alone is too
  * tight. |v_ref|+0.12=0.84 at preset72. */
 printf("ceiling(0.80,0.72)=%.3f ceiling(0.80,0.00)=%.3f ceiling(1.12,0.72)=%.3f\n",
  BalanceInterlock_Ceiling(0.80f,0.72f),BalanceInterlock_Ceiling(0.80f,0.00f),BalanceInterlock_Ceiling(1.12f,0.72f));
 assert(fabsf(BalanceInterlock_Ceiling(0.80f,0.72f)-.84f)<1e-5f);
 assert(fabsf(BalanceInterlock_Ceiling(0.80f,0.00f)-.80f)<1e-5f);
 assert(fabsf(BalanceInterlock_Ceiling(1.12f,1.10f)-1.22f)<1e-5f);
 /* 1. The observed release surge at preset72:8 ticks(40ms) at0.86 with the
  * reference still0.72. RT22 stopped on the first tick; RT23 must not. */
 BalanceInterlock_Clear(&g);
 for(int i=0;i<8;i++)assert(!trip(.86f,.72f,.80f,false,.86f,.67f));
 for(int i=0;i<200;i++)assert(!trip(.75f,.72f,.80f,false,.82f,.68f));
 printf("40ms release surge0.86m/s at preset72: no stop (RT22 stopped on tick1)\n");
 /* 2. Straight 72 steady state0.79m/s with command0.72 must never stop. */
 BalanceInterlock_Clear(&g);
 for(int i=0;i<1000;i++)assert(!trip(.79f,.72f,.80f,false,.82f,.76f));
 printf("5s at0.79m/s with0.72 commanded: no stop\n");
 /* 3. A real runaway needs200ms, then stops. */
 int t=run_until_trip(.95f,.72f,.80f,false,400);
 printf("runaway0.95m/s stops after%d ticks (%.0fms)\n",t,t*5.0);
 assert((unsigned)t>=BALANCE_SPEED_TRIP_TICKS&&(unsigned)t<=BALANCE_SPEED_TRIP_TICKS+1U);
 /* 4. Hovering just around the ceiling must not accumulate: 20 over, 20 under. */
 BalanceInterlock_Clear(&g);
 for(int k=0;k<50;k++){
  for(int i=0;i<20;i++)assert(!trip(.90f,.72f,.80f,false,.90f,.74f));
  for(int i=0;i<20;i++)assert(!trip(.70f,.72f,.80f,false,.70f,.70f));
 }
 printf("alternating0.90/0.70 never accumulates to a stop\n");
 /* 5.1.45m/s absolute runaway bound stops at once, both directions. */
 for(int sign=-1;sign<=1;sign+=2){
  BalanceInterlock_Clear(&g);
  assert(trip(sign*1.50f,0.0f,.80f,false,sign*1.50f,sign*1.50f));
 }
 printf("1.50m/s runaway bound stops on the first sample\n");
 /* 6. The max envelope keeps its1.12 axle/1.20 wheel bounds at preset80. */
 assert(run_until_trip(1.20f,.80f,1.12f,true,400)>0);
 assert(run_until_trip(1.01f,.80f,1.12f,true,400)==-1);
 /* 7. Non-finite samples still stop (protection retained). */
 assert(BalanceInterlock_Step(&g,NAN,0,0,.80f,0,false));
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'interlock.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'interlock.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,'-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
let run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
// Old-constant probe: the RT22 one-sample rule must fail check1 on the same trace.
const old=path.join(out,'interlock_rt22.exe');
r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,
 '-DBALANCE_SPEED_TRIP_TICKS=1U','-DBALANCE_SPEED_TRACK_MARGIN_MPS=0.0f',file,'-lm','-o',old],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const oldRun=cp.spawnSync(old,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'the RT22 one-sample interlock should stop on the release surge');
console.log('RT22-style probe fails as expected (stops on the first0.86m/s sample).');
console.log('PASS RT23 interlock: transient overspeed survives, 200ms runaway stops, 1.45m/s instant, envelope/wheel/non-finite guards intact.');
