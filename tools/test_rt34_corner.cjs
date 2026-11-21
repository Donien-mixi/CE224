// RT34 — trần tốc độ do đường (cornering budget).
//
// Log 38 (RT32, MAX 90, rẽ ở 0,90 m/s): bánh ngoài bị đẩy tới PWM 2499 (100 %),
// yaw vọt tới 37 % trên tham chiếu rồi mới hồi, và tốc độ xe tự tụt 0,90 → 0,66
// trong cú cua. Nguyên nhân vật lý: bánh ngoài cần v + |w|·0,131 = 1,05 m/s trong
// khi động cơ chỉ tới ~0,95 m/s ở pack 11,7 V.
//
// RT34 hạ *tham chiếu tốc độ* khi đang cua xuống mức bánh còn làm được
// (BUDGET − |w_ref|·0,131), đi qua ramp của BalanceMotion nên mượt, và không bao
// giờ nâng tham chiếu. Test này phải FAIL nếu ai tắt giới hạn (BUDGET lớn).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt34_corner');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_turn.h"
static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;
static void init(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);s.ready=true;s.speed_preset=2;ms=1000;seq=0;
 BalanceMotion_corner_budget(0.0f);}
/* 1 tick 5ms. dir/turn_cmd giu lenh; speed la van toc banh do duoc. */
static void tick(int dir,int turn_cmd,float speed){
 ms+=5;seq++;
 assert(BalanceMotion8_Update(&s,&t,1U,seq*4+(dir>0?1:dir<0?2:0),turn_cmd,s.x_ref,1.5f,0,speed,0,t.w_ref,ms));
}
int main(void){
 setbuf(stdout,NULL);

 /* 1. Chay thang MAX 90 (khong cua): tham chieu phai la0.90, budget=0. */
 init();
 for(int i=0;i<600;i++)tick(1,0,s.v_ref);
 printf("thang: v_ref=%.3f budget=%.2f\n",s.v_ref,BalanceMotion_corner_budget(-1.0f));
 assert(fabsf(s.v_ref-0.90f)<0.01f);
 assert(BalanceMotion_corner_budget(-1.0f)==0.0f);

 /* 2. Giu nguyen lenh tien + bat cua trai (1.15rad/s) o0.90:
  *    RT36: budget 0.90-1.15*0.131=0.749 -> v_ref ha ve ~0.75, khong vot.
  *    RT39: budget 0.87-0.151=0.719 (banh ngoai 0.87, con ~10% duty du tru). */
 float before=s.v_ref;
 for(int i=0;i<200;i++)tick(1,1,s.v_ref);
 float cb=BalanceMotion_corner_budget(-1.0f);
 printf("vao cua: v_ref %.3f -> %.3f (budget=%.3f) w_ref=%.2f\n",before,s.v_ref,cb,t.w_ref);
 assert(fabsf(cb-0.719f)<0.02f);
 assert(s.v_ref<0.75f&&s.v_ref>0.69f);
 /* ha phai muot: khong tick nao doi qua 1.5 lan ramp cho phep */
 init();
 for(int i=0;i<600;i++)tick(1,0,s.v_ref);
 for(int i=0;i<200;i++){float a=s.v_ref;tick(1,1,s.v_ref);if(fabsf(s.v_ref-a)>0.012f){
   printf("nhay: %.3f -> %.3f tai tick %d\n",a,s.v_ref,i); assert(0);} }

 /* 3. Nha cua: budget ve0 va tham chieu tro lai0.90 trong ~1s. */
 for(int i=0;i<300;i++)tick(1,0,s.v_ref);
 printf("nha cua: v_ref=%.3f budget=%.2f\n",s.v_ref,BalanceMotion_corner_budget(-1.0f));
 assert(BalanceMotion_corner_budget(-1.0f)==0.0f);
 assert(fabsf(s.v_ref-0.90f)<0.02f);

 /* 4. O0.50/0.80: RT36 ha ca preset80 xuong cung budget 0.719, nen cua o
  *    preset80 va preset90 co cung toc do ~0.72 (yeu cau 70-75 cm/s). */
 init();s.speed_preset=1;
 for(int i=0;i<900;i++)tick(1,0,s.v_ref);
 float p80=s.v_ref;
 for(int i=0;i<200;i++)tick(1,1,s.v_ref);
 printf("preset80 khi cua: %.3f -> %.3f\n",p80,s.v_ref);
 assert(fabsf(p80-0.80f)<0.01f);
 assert(s.v_ref>0.69f&&s.v_ref<0.75f);   /* cung budget voi MAX */

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'corner.c');
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

/* 5. Old-constant probe: budget cu 0.95 (cua ~0.80) thi test phai fail. */
const oldExe=build(['-DBALANCE_TURN_VNL_BUDGET=0.95f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'budget cu 0.95 thi test phai fail');
console.log('old-constant probe: budget 0.95 bi bat (exit '+oldRun.status+')');
console.log('PASS RT34/RT36: cua ha tham chieu ve ~0,75 muot; thang va preset80 = 0,80 khi thang.');
