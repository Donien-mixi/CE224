// RT11 replay, giu nguyen y do: 38 mau REC1 da ghi o moc 72cm/s KHONG duoc vao
// lai REC1. RT39 doi guard vuot toc do: no so voi TRAN cua interlock
// (max(envelope,|v_ref|+0.12)), khong so voi tham chieu tuc thoi, va can100ms.
//
// Test phai FAIL neu guard lai giam xuong duoi tran (probe SLACK=0.05, tuong
// duong luat cu "|v| > ordinary-0.08").
const fs=require('node:fs'),cp=require('node:child_process'),assert=require('node:assert/strict');
const rows=JSON.parse(fs.readFileSync('tools/fixtures/rt11_rec72.json','utf8'));
const vals=rows.map(r=>'{'+[r.pitch,r.gyro,r.speed,r.ref].map(x=>Number(x).toExponential(8)+'f').join(',')+'}').join(',\n');
const src=`#include <assert.h>
#include <stdio.h>
#include "balance_recovery.h"
static const float cases[][4]={
${vals}
};
int main(void){
 int hits=0;
 for(unsigned i=0;i<sizeof(cases)/sizeof(*cases);i++){
  const float*p=cases[i];
  BalanceRecovery rec={0};
  for(int k=0;k<40;k++){BalanceRecovery_OverSpeed(&rec,true,p[2],p[3],0,.3f,.8f,1000+5*k);if(rec.active)break;}
  if(rec.active){hits++;continue;}
  assert(!BalanceRecovery_UpdateMode(&rec,true,p[0],p[1],p[2]-p[3],0,.3f,1000,true));
  assert(!rec.active);
 }
 BalanceRecovery genuine={0};
 for(int k=0;k<40;k++)BalanceRecovery_OverSpeed(&genuine,true,.95f,.50f,0,.3f,.8f,1000+5*k);
 assert(genuine.active);
 const float slack=BALANCE_RECOVERY_OVERSPEED_SLACK;
 printf("%u recorded 72cm/s REC snapshots: %d mau vao REC1 (bien guard=%.2f)\\n",
        (unsigned)(sizeof(cases)/sizeof(*cases)),hits,(double)slack);
 /* Probe SLACK>0 tai hien luat cu: assert nay PHAI fail. */
 assert(hits==0);
 printf("genuine unstable recovery: active=%d\\n",(int)genuine.active);
 return 0;
}`;
fs.mkdirSync('.build/replay72',{recursive:true});
const file='.build/replay72/test.c';
fs.writeFileSync(file,src);
const inc='-IXe_Can_Bang/Core/Inc';
function build(extra,tag){
  const exe='.build/replay72/'+tag+'.exe';
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
let r=cp.spawnSync(build(null,'new'),[],{encoding:'utf8'});
assert.equal(r.status,0,(r.stdout||'')+(r.stderr||''));
process.stdout.write(r.stdout);
r=cp.spawnSync(build(['-DBALANCE_RECOVERY_OVERSPEED_SLACK=0.05f'],'probe'),[],{encoding:'utf8'});
assert.notEqual(r.status,0,'guard duoi tran phai lam test fail');
console.log('old-constant probe: guard duoi tran bi bat lai (SLACK=0.05, exit '+r.status+') — PASS RT39.');
