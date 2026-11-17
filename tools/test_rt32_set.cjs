// RT32 (chốt 17/11/2026): bộ tốc độ cuối cùng 50 / 80 / 90 cm/s + bù lệch bánh trái.
//
// Quyết định của người vận hành sau khi test RT30 (0,86) và RT31 (0,90):
//   preset 0 = 0,50 m/s, preset 1 = 0,80 m/s, preset 2 (MAX) = 0,90 m/s.
// Bù lệch bánh trái: đo trên log RT30/RT31, cùng tốc độ bánh trái cần thêm
// ~98 PWM (tiến), ~160 (lùi), ~166 (quay tại chỗ) => 110 PWM hằng số, đặt sau
// bộ phân bổ R12 và kẹp ±MOTOR_MAX_PWM.
//
// Test này phải FAIL với hằng số của bản RT29 (0,72/0,80, không bù) — đó là
// bằng chứng nó thật sự kiểm tra chính sách mới.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt32_set');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"
#include "balance_profile.h"
#include "balance_trim.h"
#include "balance_interlock.h"

/* Đi thẳng giữ nút ở preset p, trả về vận tốc tham chiếu ổn định. */
static float ramp(int preset){
 BalanceMotion s;BalanceMotion_Clear(&s);s.speed_preset=(uint8_t)preset;
 float pos=0, v=0; uint32_t now=0;
 for(int i=0;i<120;i++){ now+=5; (void)BalanceMotion_Update(&s,true,0U,pos,1.0f,0.0f,0.0f,now); }
 for(int i=0;i<900;i++){
  now+=5;
  (void)BalanceMotion_Update(&s,true,(uint32_t)((i+1)<<2)|1U,pos,1.0f,0.0f,v,now);
  v=s.v_ref; pos+=v*0.005f;
 }
 return s.v_ref;
}

int main(void){
 setbuf(stdout,NULL);

 /* 1. Ba mốc tốc độ đúng chính sách 50/80/90. */
 float v0=ramp(0), v1=ramp(1), v2=ramp(2);
 printf("preset0=%.4f  preset1=%.4f  preset2=%.4f m/s\n",v0,v1,v2);
 assert(fabsf(v0-0.50f)<0.004f);
 assert(fabsf(v1-0.80f)<0.004f);
 assert(fabsf(v2-0.90f)<0.006f);

 /* 2. Trần thường (interlock/startup) = mức cao nhất 0,90. */
 assert(fabsf(BALANCE_STOP_SPEED-0.90f)<1e-6f);
 printf("BALANCE_STOP_SPEED=%.3f m/s\n",(double)BALANCE_STOP_SPEED);
 BalanceInterlock g;BalanceInterlock_Clear(&g);
 for(int i=0;i<400;i++)
  assert(!BalanceInterlock_Step(&g,0.90f,0.90f,0.90f,BALANCE_STOP_SPEED,0.90f,false));
 printf("0,90 m/s giu deu8s: interlock khong cat\n");

 /* 3. Bù lệch bánh trái: +110 theo dấu, kẹp biên, 0 thì không đổi. */
 const float MAXP=2499.0f;
 assert(fabsf(BALANCE_TRIM_LEFT_PWM-110.0f)<1e-3f);
 assert(fabsf(BalanceTrim_Left(1000.0f,110.0f,MAXP)-1110.0f)<1e-3f);
 assert(fabsf(BalanceTrim_Left(-1000.0f,110.0f,MAXP)+1110.0f)<1e-3f);
 assert(BalanceTrim_Left(0.0f,110.0f,MAXP)==0.0f);
 assert(BalanceTrim_Left(0.2f,110.0f,MAXP)==0.2f);            /* chết vùng nhỏ */
 assert(fabsf(BalanceTrim_Left(2450.0f,110.0f,MAXP)-MAXP)<1e-3f);   /* kẹp trên */
 assert(fabsf(BalanceTrim_Left(-2450.0f,110.0f,MAXP)+MAXP)<1e-3f);  /* kẹp dưới */
 assert(isfinite(BalanceTrim_Left(NAN,110.0f,MAXP))==0);
 printf("trim: +110 theo dau, kep +-%.0f, NaN giu nguyen\n",(double)MAXP);

 /* 4. Bất biến R12 không đổi. */
 assert(fabsf(CAT_K[0]-9.150813420f)<1e-6f);
 assert(fabsf(CAT_K[1]-6.128855436f)<1e-6f);
 assert(fabsf(CAT_K[2]-14.32794546f)<1e-6f);
 assert(fabsf(CAT_K[3]-1.160483551f)<1e-6f);

 puts("PASS");
 return 0;
}
`.replace('#include "balance_interlock.h"','#include "balance_interlock.h"\n#include "phase1_catalog_params.h"');

const file=path.join(out,'set.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,'-lm','-o',exe],
    {encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
const exe=build(null,'new');
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));

/* 5. Old-constant probe: chính sách RT29 (0,72/0,80, không bù) BẮT BUỘC fail. */
const oldExe=build([
  '-DBALANCE_MOTION_SPEED=(s->speed_preset==2? .80f:s->speed_preset? .72f:.50f)',
  '-DBALANCE_STOP_SPEED=0.80f',
  '-DBALANCE_TRIM_LEFT_PWM=0.0f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'old RT29 constants must fail this test');
console.log('old-constant probe: 0,72/0,80 khong bu bi bat (exit '+oldRun.status+')');
console.log('PASS RT32: preset 50/80/90, tran 0,90, bu banh trai 110 PWM co kep.');
