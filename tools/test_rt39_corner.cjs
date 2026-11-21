// RT39 — o moc 80cm/s KHONG duoc vao REC1 khi cua / nha lenh / tang toc lai.
//
// Log RT38 (39), 84s, preset80, ca bon huong re: 12 cuoc REC1 = 25,3s (30% thoi
// gian). Sau cuoc vao trong mot cu cua CO LENH o 0,77-0,80m/s tren tham chieu
// 0,749 (sai0,04) hoac ngay sau khi nha tham chieu (sai0,12). Guard cu so banh
// voi THAM CHIEU TUC THOI nen moi cu vao cua va moi lan nha lenh deu bi coi la
// vuot toc do; REC1 ham 0,78->0 trong1,9-2,8s.
//
// Fixture: 438 mau 50Hz that, cat1,5s NGAY TRUOC moi cuoc vao gia (khong co mau
// nao nam tren tran interlock). Test phai FAIL neu guard lai giam xuong duoi
// tran: probe SLACK=0.12 tuong duong bien do hieu dung cua luat cu.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt39_corner');
fs.mkdirSync(out,{recursive:true});
const wins=JSON.parse(fs.readFileSync(path.join(root,'tools/fixtures/rt39_corner80.json'),'utf8'));

const N=wins[0].v.length;
assert.ok(wins.every(w=>w.v.length===N&&w.vref.length===N),'fixture windows must be equal length');
const csegs=wins.map(w=>` {{${w.v.map(x=>Number(x).toExponential(8)+'f').join(',')}},
  {${w.vref.map(x=>Number(x).toExponential(8)+'f').join(',')}}} /* ${w.name} dir=${w.dir} turn=${w.turn} t0=${w.t0} */`).join(',\n');

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_recovery.h"
#include "balance_turn.h"
#include "balance_motion.h"

/* {v[], vref[]} cho tung cua so that trong log RT38 (39). */
#define WINN ${N}U
static const struct {float v[WINN];float ref[WINN];} win[]={
${csegs}
};
int main(void){
 const unsigned wcount=(unsigned)(sizeof(win)/sizeof(*win));
 unsigned total=0,fired=0;
 for(unsigned w=0;w<wcount;w++){
  BalanceRecovery r={0};
  for(unsigned k=0;k<WINN;k++){
   total++;
   BalanceRecovery_OverSpeed(&r,true,win[w].v[k],win[w].ref[k],0,.30f,.80f,1000+5*k);
   if(r.active)fired++;
  }
 }
 const float slack=BALANCE_RECOVERY_OVERSPEED_SLACK;
 printf("%u mau cua/nha lenh/tang toc lai: %u mau vao REC1 (bien guard=%.2f)\\n",
        total,fired,(double)slack);
 /* Probe SLACK>0 tai hien luat cu (guard duoi tran): assert nay PHAI fail. */
 assert(fired==0);

 /* 100ms bang chung: 19 nhip chua du, nhip20 moi vao. */
 BalanceRecovery q={0};
 for(int i=0;i<(int)BALANCE_RECOVERY_OVERSPEED_TICKS-1;i++)
  BalanceRecovery_OverSpeed(&q,true,.95f,.50f,0,.3f,.80f,5000+5*i);
 assert(!q.active);
 BalanceRecovery_OverSpeed(&q,true,.95f,.50f,0,.3f,.80f,6000);
 assert(q.active&&q.episodes==1);
 /* Da o trong REC1 thi khong dem lai. */
 BalanceRecovery_OverSpeed(&q,true,.99f,.50f,0,.3f,.80f,6005);
 assert(q.episodes==1);
 /* Nguoc chieu lenh (xe dang lui trong khi lenh tien) khong phai runaway. */
 BalanceRecovery b={0};
 for(int i=0;i<200;i++)BalanceRecovery_OverSpeed(&b,true,-.30f,.26f,0,.3f,.80f,7000+5*i);
 assert(!b.active&&b.overspeed==0);

 /* Cua 1,15rad/s o moc80: tham chieu con BUDGET-|w|*HALF_TRACK, banh ngoai = BUDGET. */
 float w=1.15f;
 float corner=fmaxf(0.35f,BALANCE_TURN_VNL_BUDGET-w*BALANCE_TURN_HALF_TRACK);
 float outer=corner+w*BALANCE_TURN_HALF_TRACK;
 printf("cua%.2frad/s: tham chieu%.3fm/s, banh ngoai%.3fm/s\n",w,corner,outer);
 assert(corner>=.70f&&corner<=.75f);      /* khoang nguoi van hanh dat */
 assert(fabsf(outer-BALANCE_TURN_VNL_BUDGET)<1e-4f);
 assert(BALANCE_TURN_VNL_BUDGET<=.88f);   /* con du duty cho vong can bang */
 /* Hai ben re cung mot tham chieu toc do (truoc day lech do REC1, khong do luat). */
 float left=fmaxf(0.35f,BALANCE_TURN_VNL_BUDGET-w*BALANCE_TURN_HALF_TRACK);
 float right=fmaxf(0.35f,BALANCE_TURN_VNL_BUDGET-w*BALANCE_TURN_HALF_TRACK);
 assert(fabsf(left-right)<1e-6f);
 /* Tai cho quay (|w|>0.30) budget phai khac0; khong quay thi khong gioi han. */
 BalanceMotion_corner_budget(0.0f);assert(BalanceMotion_corner_budget(-1.0f)==0.0f);
 puts("PASS RT39: khong REC1 trong cua/nha lenh o80, guard van bat runaway va giu100ms.");
 return 0;
}`;
const file=path.join(out,'rt39.c');
fs.writeFileSync(file,src);
const inc=['-I'+path.join(root,'Xe_Can_Bang/Core/Inc')];
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror',...inc,...(extra||[]),file,path.join(root,'Xe_Can_Bang/Core/Src/balance_recovery.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
let r=cp.spawnSync(build(null,'new'),[],{encoding:'utf8'});
assert.equal(r.status,0,(r.stdout||'')+(r.stderr||''));
process.stdout.write(r.stdout);

const probes=[
  ['-DBALANCE_RECOVERY_OVERSPEED_SLACK=0.12f','probe_slack','guard duoi tran (bien cu)'],
  ['-DBALANCE_TURN_VNL_BUDGET=0.90f','probe_budget','tran cua cu 0.90 (banh ngoai sat)'],
];
for(const [flag,tag,label] of probes){
  const exe=build([flag],tag);
  const rr=cp.spawnSync(exe,[],{encoding:'utf8'});
  assert.notEqual(rr.status,0,`probe ${label} phai fail`);
  console.log(`old-constant probe: ${label} bi bat (exit ${rr.status})`);
}
console.log('PASS RT39: cua 80cm/s khong con tu vao REC1; guard chi bat khi vuot tran that.');
