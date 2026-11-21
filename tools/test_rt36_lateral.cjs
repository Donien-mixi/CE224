// RT36 — lateral cap theo tốc độ LỆNH (bug: rẽ phải khi lùi ngập ngừng).
//
// Log rev_turn 857-866 s (lùi + rẽ phải, MAX): v_ref -0,80, |v| dao động
// 0,76-0,82, w_ref nhảy 1,09-1,15 và cờ 512 bật 87 % mẫu. Nguyên nhân: trần
// ngang tính bằng |v| ĐO (0,9/0,82=1,097) nên cứ |v| vượt 0,78 là trần cắt
// xuống dưới 1,15 rồi lại nhả. RT36 tính bằng tốc độ đã lệnh (v_ref, đã có
// trần cua), giống cách trần bánh đang làm.
//
// Test phải FAIL với công thức cũ (BALANCE_TURN_LATERAL_FROM_MEASURED=1).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt36_lateral');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_turn.h"

static BalanceMotion s;static BalanceTurn t;static uint32_t ms,seq;
static void init(void){BalanceMotion_Clear(&s);BalanceTurn_Clear(&t);s.ready=true;s.speed_preset=2;ms=1000;seq=0;
 BalanceMotion_corner_budget(0.0f);}
/* 1 tick. Do "nhieu do": |v| dao dong 0.76-0.82 trong khi v_ref ~0.749. */
static int tick(float speed){
 ms+=5;seq++;
 return BalanceMotion8_Update(&s,&t,1U,seq*4+1U,1,s.x_ref,1.5f,0.0f,speed,0,t.w_ref,ms);
}
int main(void){
 setbuf(stdout,NULL);

 /* 1. Vao cua 1 s, giu v_ref 0.749, do dao dong 0.76..0.82. */
 init();
 for(int i=0;i<600;i++)assert(tick(0.0f));          /* chay thang truoc */
 float lo=9,hi=-9;int lim=0;
 for(int i=0;i<400;i++){
  float sp=0.76f+0.06f*((i/10)%2);                  /* 0.76 / 0.82 moi 50ms */
  assert(tick(sp));
  if(i>200){
   if(t.w_ref<lo)lo=t.w_ref;
   if(t.w_ref>hi)hi=t.w_ref;
   if(s.flags&512U)lim++;
  }
 }
 printf("w_ref %.3f..%.3f (yeu cau 1.150) 512=%d/200\n",lo,hi,lim);
 assert(lo>1.13f);
 assert(hi<1.17f);
 assert(lim==0);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'lateral.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',inc,...(extra||[]),file,path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_turn.c'),'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
const exe=build(null,'new');
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));

const oldExe=build(['-DBALANCE_TURN_LATERAL_FROM_MEASURED=1'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'cong thuc do |v| do thi test phai fail');
console.log('old-constant probe: tran ngang theo |v| do bi bat (exit '+oldRun.status+')');
console.log('PASS RT36 lateral: w_ref giu 1,15 khi |v| do dao dong, khong bat 512.');
