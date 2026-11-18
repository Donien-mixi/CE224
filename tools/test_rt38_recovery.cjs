// RT38 — REC1 khong duoc kich hoat boi cu dao chieu co lenh.
//
// Log (2)(5): ngay sau dao chieu -0,9 -> +0,9 m/s co 52 % mau o phase2 (REC1)
// trong 1,2 s; toc do khong ve lai0,5. Gyro cua cu dao chieu that su vuot150dps.
// RT38: khi dang giu lenh (maneuver) gyro phai duy tri60ms moi vao REC1; khi
// khong co lenh (bi day) van vao ngay.
//
// Test phai FAIL voi hanh vi cu (IMMEDIATE_MANEUVER=1).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt38_recovery');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_recovery.h"

static void prime(BalanceRecovery*s,uint32_t*now){
 BalanceRecovery_Clear(s);
 for(int i=0;i<130;i++){*now+=5;BalanceRecovery_UpdateMode(s,true,0,0,0,0,.30f,*now,false);}
 assert(s->primed);
}
int main(void){
 setbuf(stdout,NULL);
 uint32_t now=1000;

 /* 1. Khong co lenh: gyro200 -> vao REC1 ngay (bi day that). */
 BalanceRecovery r;prime(&r,&now);
 BalanceRecovery_UpdateMode(&r,true,2.0f,200.0f,0,0,.30f,now+5,false);
 printf("khong lenh gyro200: active=%d (phai1)\n",(int)r.active);
 assert(r.active);

 /* 2. Dang giu lenh: 1 mau gyro200 KHONG duoc vao REC1. */
 prime(&r,&now);
 BalanceRecovery_UpdateMode(&r,true,2.0f,200.0f,0,0,.30f,now+5,true);
 printf("co lenh 1 mau: active=%d (phai0)\n",(int)r.active);
 assert(!r.active);

 /* 3. Giu lenh + gyro200 du60ms (12 mau) -> vao REC1. */
 prime(&r,&now);
 for(int i=0;i<11;i++){now+=5;BalanceRecovery_UpdateMode(&r,true,2.0f,200.0f,0,0,.30f,now,true);}
 printf("co lenh 11 mau: active=%d (phai0)\n",(int)r.active);
 assert(!r.active);
 now+=5;BalanceRecovery_UpdateMode(&r,true,2.0f,200.0f,0,0,.30f,now,true);
 printf("co lenh 12 mau: active=%d (phai1)\n",(int)r.active);
 assert(r.active);

 /* 4. Giu lenh, gyro thap: khong vao. */
 prime(&r,&now);
 for(int i=0;i<40;i++){now+=5;BalanceRecovery_UpdateMode(&r,true,2.0f,90.0f,0,0,.30f,now,true);}
 assert(!r.active);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'recovery.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
const exe=build(null,'new');
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));

const oldExe=build(['-DBALANCE_RECOVERY_IMMEDIATE_MANEUVER=1'],'old');
const r2=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(r2.status,0,'kich hoat ngay khi co lenh thi test phai fail');
console.log('old-constant probe: REC1 ngay khi co lenh bi bat (exit '+r2.status+')');
console.log('PASS RT38: REC1 do gyro can60ms khi co lenh, van ngay khi khong lenh.');
