// RT43 — trần MAX theo duty còn dư ở đường thẳng.
//
// Log RT42 tại nhà: chạy toàn bộ ở MAX 90, duty p95 0,90 (0-33 % mẫu >=0,96),
// vượt tham chiếu 0,95-0,96, ngã 2 lần khi bánh ghim 2499 và lệch trái/phải.
// Khi bánh >=96 % duty, tham chiếu MAX hạ theo hệ số (sàn 0,86 -> 0,774 m/s).
// Pack tươi (duty <=90 %) thì hệ số về1 và MAX vẫn đạt 0,90.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt43_max');
fs.mkdirSync(out,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"
static BalanceMotion s;static uint32_t now,seq;
static void reset(void){BalanceMotion_Clear(&s);s.ready=true;s.speed_preset=2;now=1000;seq=0;BalanceMotion_maxhead(1.0f);}
static void tick(float duty,bool active){now+=5;seq++;
 BalanceMotion_maxhead(BalanceMotion_maxhead_next(duty,active));
 assert(BalanceMotion_Update(&s,true,4U*seq+1,s.x_ref,0,0,s.v_ref,now));}
int main(void){
 /* 1. Banh ghim duty: tham chieu MAX bi ha, khong vuot 0,78 m/s. */
 reset();
 for(int i=0;i<600;i++)tick(0.99f,true);
 float held=s.v_ref;
 printf("duty0.99: he so%.2f  v_ref %.3f m/s\n",(double)BalanceMotion_maxhead(-1.0f),(double)held);
 assert(held>0.70f&&held<0.79f);
 assert(BalanceMotion_maxhead(-1.0f)>=BALANCE_MOTION_MAXHEAD_FLOOR-0.005f);
 /* 2. Pack khoe (duty thap): he so ve1, MAX dat dung0,90. */
 reset();
 for(int i=0;i<900;i++)tick(0.85f,true);
 printf("duty0.85: he so%.2f  v_ref %.3f m/s\n",(double)BalanceMotion_maxhead(-1.0f),(double)s.v_ref);
 assert(fabsf(BalanceMotion_maxhead(-1.0f)-1.0f)<1e-6f);
 assert(fabsf(s.v_ref-.90f)<.005f);
 /* 3. Dang cua (active=false) khong anh huong duong thang. */
 reset();
 for(int i=0;i<300;i++)tick(0.99f,false);
 assert(fabsf(BalanceMotion_maxhead(-1.0f)-1.0f)<1e-6f);
 /* 4. Het ghim thi hoi lai. */
 reset();
 for(int i=0;i<300;i++)tick(0.99f,true);
 for(int i=0;i<400;i++)tick(0.80f,true);
 assert(BalanceMotion_maxhead(-1.0f)>0.99f);
 puts("PASS RT43: MAX tu ha khi banh ghim duty, van dat0,90 voi pack khoe, cua khong bi anh huong.");
 return 0;
}`;
const file=path.join(out,'rt43.c');
fs.writeFileSync(file,src);
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror','-I'+path.join(root,'Xe_Can_Bang/Core/Inc'),
     ...(extra||[]),file,'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
let r=cp.spawnSync(build(null,'new'),[],{encoding:'utf8'});
assert.equal(r.status,0,(r.stdout||'')+(r.stderr||''));process.stdout.write(r.stdout);
const oldExe=build(['-DBALANCE_MOTION_MAXHEAD_FLOOR=1.0f'],'old');
const rr=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(rr.status,0,'khong ha tran thi test phai fail');
console.log('old-constant probe: tran MAX khong ha (FLOOR=1.0) bi bat (exit '+rr.status+')');
console.log('PASS RT43: MAX 90 khong con bao hoa banh o pack yeu.');
