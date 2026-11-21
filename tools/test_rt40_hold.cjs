// RT40 — ba loi tu 8 log cua RT39.
//
// 1. Nghieng do phanh (log RT40): 15 cuoc REC1 vao khi dang CO LENH o pitch12-15deg
//    voi gyro1-61dps, tuc la do nghieng cua mot cu phanh binh thuong, va sau do
//    chan lenh moi1,4-2,2s (tong8,4s "co lenh ma khong chay" trong8 log).
//    Nguong nghieng khi co lenh 12 -> 16deg. Probe TILT=12 phai fail.
// 2. Cat motor khi bi day (02_chong_chiu): fault4096 dung4s sau khi vao REC1 trong
//    khi xe van dung (pitch7,6deg, gyro15dps, |v|0,20). Han REC1 4 ->8s.
//    Probe TIMEOUT=4000 phai fail.
// 3. Banh ghim100% khi cua (08, preset90, 3 cu nga): khi dang cua ma duty>=0,96 thi
//    ha tham chieu tuyen tinh theo he so du duty. Probe FLOOR=1,0 phai fail.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt40_hold');
fs.mkdirSync(out,{recursive:true});
const wins=JSON.parse(fs.readFileSync(path.join(root,'tools/fixtures/rt40_brake_lean.json'),'utf8'));
const N=wins[0].pitch.length;
assert.ok(wins.every(w=>w.pitch.length===N),'windows must be equal length');
const seg=wins.map(w=>` {{${w.pitch.map(x=>Number(x).toExponential(7)+'f').join(',')}},
  {${w.gyro.map(x=>Number(x).toExponential(7)+'f').join(',')}},
  {${w.err.map(x=>Number(x).toExponential(7)+'f').join(',')}}} /* ${w.file} t0=${w.t0} */`).join(',\n');

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_recovery.h"
#include "balance_turn.h"
#include "balance_motion.h"

/* Cua so that 240ms ngay truoc moi cuoc REC1 do nghieng-phanh trong log RT40. */
#define WINN ${N}U
static const struct {float pitch[WINN];float gyro[WINN];float err[WINN];} win[]={
${seg}
};
int main(void){
 /* 1. Do nghieng cua cu phanh co lenh KHONG duoc vao REC1. */
 unsigned total=0,fired=0;
 for(unsigned w=0;w<sizeof(win)/sizeof(*win);w++){
  BalanceRecovery r={0};
  for(int i=0;i<130;i++)BalanceRecovery_UpdateMode(&r,true,0,0,0,0,.30f,1000+5*i,false);
  assert(r.primed);
  /* 50Hz samples, 200Hz loop: feed each sample four ticks. */
  for(unsigned k=0;k<WINN;k++)for(int q=0;q<4;q++){
   total++;
   BalanceRecovery_UpdateMode(&r,true,win[w].pitch[k],win[w].gyro[k],win[w].err[k],0,.30f,2000+5*(int)(k*4+q),true);
   if(r.active)fired++;
  }
 }
 printf("%u mau nghieng-phanh trong%u cua so: %u mau vao REC1\n",
        total,(unsigned)(sizeof(win)/sizeof(*win)),fired);
 assert(fired==0);

 /* 2. Bi giu 6s trong khi van dung: khong cat motor. */
 BalanceRecovery p={0};
 for(int i=0;i<130;i++)BalanceRecovery_UpdateMode(&p,true,0,0,0,0,.30f,2000+5*i,false);
 assert(p.primed);
 assert(!BalanceRecovery_UpdateMode(&p,true,10.0f,20.0f,0,0,.30f,2700,false));
 assert(p.active);
 uint16_t f=0;
 for(int i=0;i<1200;i++){
  f=BalanceRecovery_UpdateMode(&p,true,7.0f,15.0f,.20f,0,.30f,3005+5*i,false);
  if(f)break;
 }
 printf("bi giu 6s: active=%d fault=%u (phai active=1 fault=0)\n",(int)p.active,(unsigned)f);
 assert(p.active&&f==0);

 /* 3. Du duty khi cua: he so giam, tham chieu ha; tha ra thi hoi lai. */
 BalanceTurn_headroom(1.0f);
 for(int i=0;i<200;i++)BalanceTurn_headroom(BalanceTurn_headroom_next(0.99f,true));
 float low=BalanceTurn_headroom(-1.0f);
 float budget=low*fmaxf(0.35f,BALANCE_TURN_VNL_BUDGET-1.15f*BALANCE_TURN_HALF_TRACK);
 printf("duty0.99: he so%.2f -> tham chieu cua%.3f m/s\n",(double)low,(double)budget);
 assert(low<0.85f&&low>=BALANCE_TURN_HEADROOM_FLOOR-0.005f&&budget<0.66f);
 for(int i=0;i<300;i++)BalanceTurn_headroom(BalanceTurn_headroom_next(0.80f,true));
 assert(BalanceTurn_headroom(-1.0f)>0.99f);
 /* Chay thang (khong cua) khong bi anh huong: he so ve1 ngay. */
 BalanceTurn_headroom(0.70f);
 assert(BalanceTurn_headroom(BalanceTurn_headroom_next(0.99f,false))==1.0f);
 puts("PASS RT40: nghieng-phanh khong phai nhiu, giu day khong cat motor, cua tu ha khi banh ghim duty.");
 return 0;
}`;
const file=path.join(out,'rt40.c');
fs.writeFileSync(file,src);
function build(extra,tag){
  const exe=path.join(out,tag+'.exe');
  const r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',
    ['-std=c11','-Wall','-Wextra','-Werror','-I'+path.join(root,'Xe_Can_Bang/Core/Inc'),
     ...(extra||[]),file,path.join(root,'Xe_Can_Bang/Core/Src/balance_recovery.c'),'-lm','-o',exe],{encoding:'utf8'});
  assert.equal(r.status,0,(r.stderr||'')+String(r.error||''));
  return exe;
}
let r=cp.spawnSync(build(null,'new'),[],{encoding:'utf8'});
assert.equal(r.status,0,(r.stdout||'')+(r.stderr||''));
process.stdout.write(r.stdout);

const probes=[
  ['-DBALANCE_RECOVERY_TILT_MANEUVER=12.0f','probe_tilt','nguong nghieng cu 12do'],
  ['-DBALANCE_RECOVERY_TIMEOUT_MS=4000U','probe_timeout','han REC1 cu 4s'],
  ['-DBALANCE_TURN_HEADROOM_FLOOR=1.0f','probe_headroom','khong ha tham chieu khi banh ghim'],
];
for(const [flag,tag,label] of probes){
  const exe=build([flag],tag);
  const rr=cp.spawnSync(exe,[],{encoding:'utf8'});
  assert.notEqual(rr.status,0,`probe ${label} phai fail`);
  console.log(`old-constant probe: ${label} bi bat (exit ${rr.status})`);
}
console.log('PASS RT40: het ngap ngung do nghieng-phanh, khong cat motor khi bi giu, cua tu ha khi banh ghim duty.');
