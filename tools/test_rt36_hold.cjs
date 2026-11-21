// RT36 — idle hold settle (bug: xe tự đứng nhưng dao động quanh vị trí ban đầu).
//
// Log idle 80-140 s: x_ref = 0, xe đứng ở -0,07 m, PWM +300 giữ liên tục và
// trượt qua lại ±3-5 cm (stick-slip) vì mốc cách 7 cm > lực ma sát nghỉ.
// RT36: sau 3 s đứng yên thật, mốc tự "chảy" về vị trí đo được với 2 cm/s,
// trong phạm vi 15 cm; không chảy khi đang chạy/đang phanh/đang phục hồi.
//
// Test phải FAIL nếu tắt cơ chế (BALANCE_MOTION_HOLD_RELAX=0).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt36_hold');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"

static BalanceMotion s;
static uint32_t ms;
static void reset(void){BalanceMotion_Clear(&s);s.used=true;s.ready=true;s.x_ref=0.0f;ms=1000;}
static int step(float pos,float v){
 ms+=5;
 return BalanceMotion_Update(&s,true,0U,pos,2.0f,0.0f,v,ms);
}
int main(void){
 setbuf(stdout,NULL);

 /* 1. Dung yen 6 s cach moc7 cm -> moc phai chay ve sat vi tri. */
 reset();
 for(int i=0;i<1800;i++)assert(step(-0.07f,0.0f));
 printf("dung yen: x_ref=%.4f (vi tri -0.0700)\n",s.x_ref);
 assert(s.x_ref<-0.060f);
 assert(s.x_ref>-0.080f);
 /* sai so con lai <=2 cm + 1 buoc chay */
 assert(fabsf(s.x_ref+0.07f)<=0.0205f);

 /* 2. Dang chay 0.30 m/s: moc KHONG duoc chay theo. */
 reset();
 float pos=0.0f;
 for(int i=0;i<600;i++){pos-=0.30f*0.005f;assert(step(pos,0.30f));}
 printf("dang chay: x_ref=%.4f vi tri=%.4f\n",s.x_ref,pos);
 assert(s.x_ref>-0.02f);
 assert(fabsf(s.x_ref-pos)>0.10f);

 /* 3. Trong REC1 (recovery_active): khong chay moc. */
 reset();s.recovery_active=true;
 for(int i=0;i<1200;i++)assert(step(-0.07f,0.0f));
 printf("recovery: x_ref=%.4f\n",s.x_ref);
 assert(fabsf(s.x_ref)<1e-4f);

 /* 4. Da huy quy dao (cancel_flags): khong chay moc. */
 reset();s.cancel_flags=64U;
 for(int i=0;i<1200;i++)assert(step(-0.07f,0.0f));
 assert(fabsf(s.x_ref)<1e-4f);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'hold.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
const exe=build(null,'new');
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));

const oldExe=build(['-DBALANCE_MOTION_HOLD_RELAX=0.0f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'tat chay moc thi test phai fail');
console.log('old-constant probe: khong chay moc bi bat (exit '+oldRun.status+')');
console.log('PASS RT36 hold: dung yen thi moc chay ve vi tri, dang chay/phanh/REC1 thi khong.');
