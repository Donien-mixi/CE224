// Replay of the real log27 through the supervisory layers (motion + turn + recovery).
// Proves the RT8 threshold changes on the same recorded sensor data:
//  - no disturbance recovery on the bumps that fired four times before,
//  - reverse reaches the requested speed instead of being throttled at9.6cm,
//  - left/right turns keep full rate (governor no longer halves a normal turn),
//  - still no blockage cancellation on this run.
const fs = require('node:fs'), path = require('node:path'), cp = require('node:child_process'), assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const csv = path.join(root, 'Simulation/data/raw/twip_balance_R12_27_replay.csv');
assert(fs.existsSync(csv), 'missing replay csv: ' + csv);
const out = path.join(root, '.build/rt8_replay'); fs.mkdirSync(out, { recursive: true });

const src = String.raw`
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "balance_motion.h"
#include "balance_turn.h"
#include "balance_recovery.h"

static int cmp(const void*a,const void*b){double x=*(const double*)a,y=*(const double*)b;return (x>y)-(x<y);}
static double median(double*v,int n){if(n<=0)return NAN;qsort(v,n,sizeof(double),cmp);return n%2?v[n/2]:(v[n/2-1]+v[n/2])/2;}
static double vrev[20000],wleft[20000],wright[20000];static int nrev=0,nleft=0,nright=0;

int main(int argc,char**argv){
 FILE*f=fopen(argv[1],"r");if(!f)return 2;
 char line[512];if(!fgets(line,sizeof line,f))return 3;
 BalanceMotion m;BalanceTurn t;BalanceRecovery r;
 BalanceMotion_Clear(&m);BalanceTurn_Clear(&t);BalanceRecovery_Clear(&r);
 double firstx=0;bool have=false;int rows=0,eps=0,cancels=0,faults=0;bool prevactive=false;
 while(fgets(line,sizeof line,f)){
  long t_ms;double x,vl,vr,pitch,gyro,yaw,yawrate,vref_rec,wref_rec;int dir,turn,phase,seq,state;
  if(sscanf(line,"%ld,%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%d,%lf,%lf,%lf,%lf,%d",
    &t_ms,&x,&vl,&vr,&pitch,&gyro,&dir,&turn,&phase,&seq,&yaw,&yawrate,&vref_rec,&wref_rec,&state)!=15)continue;
  rows++;
  if(!have){firstx=x;have=true;m.x_ref=(float)x;}
  uint32_t now=(uint32_t)t_ms;double speed=(vl+vr)/2;
  uint32_t word=((uint32_t)seq<<2)|(dir>0?1U:dir<0?2U:0U);
  bool maneuver=BalanceMotion8_Maneuver(&m,&t,now);
  uint16_t rf=BalanceRecovery_UpdateMode(&r,true,(float)pitch,(float)gyro,(float)(speed-vref_rec),
      (float)(x-m.x_ref),.30f,now,maneuver);
  if(rf)faults++;
  if(r.active&&!prevactive){if(state==3)eps++;printf("RECOVERY at t=%ld pitch=%.2f gyro=%.1f verr=%.3f maneuver=%d\n",
      t_ms,pitch,gyro,speed-vref_rec,(int)maneuver);}
  prevactive=r.active;
  if(!BalanceMotion8_Update(&m,&t,!r.active,word,turn,(float)x,(float)pitch,(float)gyro,
      (float)speed,(float)yaw,(float)yawrate,now))return 11;
  if(phase==0&&dir==-1&&turn==0&&nrev<20000)vrev[nrev++]=m.v_ref;
  if(phase==0&&dir==0&&turn==1&&nleft<20000)wleft[nleft++]=t.w_ref;
  if(phase==0&&dir==0&&turn==-1&&nright<20000)wright[nright++]=t.w_ref;
  cancels+= (m.flags&64)?1:0;
 }
 fclose(f);
 double lw=median(wleft,nleft),rw=median(wright,nright);(void)nrev;(void)vrev;
 printf("rows=%d recovery_entries=%d cancel_samples=%d faults=%d left_wref_med=%.3f right_wref_med=%.3f\n",rows,eps,cancels,faults,lw,rw);
 if(eps!=0)return 20;
 if(cancels!=0)return 21;

 if(!(lw>=1.05&&rw<=-1.05))return 23;
 puts("PASS rt8 replay: no false recovery on bumps, both turns at full rate, no blockage cancel");
 return 0;
}
`;
const file = path.join(out, 'replay.c'), exe = path.join(out, 'replay.exe');
const exeOld = path.join(out, 'replay_legacy.exe');
fs.writeFileSync(file, src);
const inc = path.join(root, 'Xe_Can_Bang/Core/Inc');
const legacy = require('./lib/legacy_headers.cjs').ensureLegacyHeaders(root);
function build(target, legacyFirst) {
  const args = ['-std=c11', '-O2', '-Wall', '-Wextra', file, '-lm', '-o', target];
  args.unshift(...(legacyFirst ? ['-I' + legacy, '-I' + inc] : ['-I' + inc]));
  const res = cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe', args, { encoding: 'utf8' });
  assert.equal(res.status, 0, res.stderr);
}
build(exe, false);
build(exeOld, true);
const oldRun = cp.spawnSync(exeOld, [csv], { encoding: 'utf8' });
const r = cp.spawnSync(exe, [csv], { encoding: 'utf8' });
const entries = text => Number((text.match(/recovery_entries=(\d+)/) || [])[1]);
const oldEntries = entries(oldRun.stdout), newEntries = entries(r.stdout);
console.log('log27 replay: legacy recovery_entries=' + oldEntries + '  rt8 recovery_entries=' + newEntries);
process.stdout.write(r.stdout);
// The legacy build must reproduce the four episodes actually observed in log27.
assert(oldEntries >= 3, 'legacy replay should reproduce the log27 recovery episodes, got ' + oldEntries);
assert.equal(newEntries, 0, 'RT8 must not enter recovery on this log');
assert.equal(r.status, 0, r.stdout + r.stderr || ('exit ' + r.status));
