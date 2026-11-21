// RT36 — governor anchor gate (bug: tiến chậm hơn lùi, cua trái chậm hơn cua phải).
//
// Log 18/11 (spd90 337-341 s): tiến giữ 0,90 nhưng xe chỉ 0,79-0,84, duty 0,83,
// lean dự đoán 0,6°, lag 0,108; x_ref bị kẹp đúng +0,100 m. Cùng lúc lùi (lag
// 0,015) đạt 0,90. Nguyên nhân: BalanceGovernor_Anchor kẹp x_ref mỗi khi
// `pressure` bật, kể cả khi pressure chỉ do lag và bánh còn dư (duty 0,83).
// RT36 chỉ cho kẹp khi duty thực sự bão hoà (>0,95).
//
// Test phải FAIL với hằng số cũ (ngưỡng duty = 0).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt36_anchor');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"

int main(void){
 setbuf(stdout,NULL);

 /* 1. Pressure do lag, duty 0,83 (con du dia): KHONG duoc kep moc. */
 BalanceGovernor g={0};
 g.pressure=true;g.duty=.83f;
 float ref=.30f;
 BalanceGovernor_Anchor(&g,&ref,0.0f,1);
 printf("duty0.83 pressure: ref=%.3f (phai giu 0.30)\n",ref);
 assert(fabsf(ref-.30f)<1e-6f);

 /* 2. Banh bao hoa: duty 0,96 -> kep con +0,10 m. */
 g.duty=.96f;ref=.30f;
 BalanceGovernor_Anchor(&g,&ref,0.0f,1);
 printf("duty0.96 pressure: ref=%.3f (phai 0.10)\n",ref);
 assert(fabsf(ref-.10f)<1e-5f);

 /* 3. Chieu lui: kep -0,10 m. */
 g.duty=.96f;ref=-.30f;
 BalanceGovernor_Anchor(&g,&ref,0.0f,-1);
 printf("duty0.96 lui: ref=%.3f (phai -0.10)\n",ref);
 assert(fabsf(ref+.10f)<1e-5f);

 /* 4. Khong pressure: khong kep du duty cao. */
 g.pressure=false;g.duty=.99f;ref=.30f;
 BalanceGovernor_Anchor(&g,&ref,0.0f,1);
 assert(fabsf(ref-.30f)<1e-6f);

 /* 5. Da trong 10 cm: khong kep (giu >6 cm cho bo phat hien ket). */
 g.pressure=true;g.duty=.99f;ref=.10f;
 BalanceGovernor_Anchor(&g,&ref,0.0f,1);
 assert(fabsf(ref-.10f)<1e-6f);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'anchor.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,'Xe_Can_Bang/Core/Src/balance_governor.c','-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
const exe=build(null,'new');
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));

const oldExe=build(['-DBALANCE_GOV_ANCHOR_DUTY=0.0f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'anchor cu (kep moi khi pressure) thi test phai fail');
console.log('old-constant probe: anchor cu bi bat (exit '+oldRun.status+')');
console.log('PASS RT36 anchor: chi kep moc khi duty > 0,95, khong kep vi lag.');
