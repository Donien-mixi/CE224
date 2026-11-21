// RT36 â€” release brake cap (bug: ngáº¯t lá»‡nh mÃ  xe cÃ²n trÃ´i ~1 m rá»“i má»›i dá»«ng).
//
// Log spd90 290.99 s: nháº£ lá»‡nh lÃºc -0,92 m/s, v_ref máº¥t 1,56 s má»›i vá»0 vÃ  xe
// Ä‘i thÃªm ~1 m; rev_turn 976.35 s: -0,76 m/s, 1,8 s. NguyÃªn nhÃ¢n: governor káº¹p
// ramp phanh xuá»‘ng0,65 m/s^2 (motion cho phÃ©p0,90). RT36 nÃ¢ng cap lÃªn0,85:
// v_ref vá»0 trong ~1,1 s, quÃ£ng phanh tá»«0,90 cÃ²n ~0,48 m, váº«n dÆ°á»›i decel0,90
// vÃ  jerk2,5 cá»§a motion.
//
// Test pháº£i FAIL vá»›i cap cÅ©0,65.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt36_brake');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"

int main(void){
 setbuf(stdout,NULL);
 BalanceGovernor g;BalanceGovernor_Reset(&g);
 g.equilibrium=2.0f;
 float old=0.90f;int ticks=0;
 /* Nha lenh: target0, rate0.90 (BALANCE_MOTION_DECEL), khong con lenh. */
 while(fabsf(old)>0.01f&&ticks<500){
  old=BalanceGovernor_Step(&g,old,0.0f,0.90f,0,2.0f,0.0f);
  assert(isfinite(old));
  ticks++;
 }
 printf("tu0.90 ve0 trong %d tick (%.2f s), v_ref=%.4f\n",ticks,ticks*0.005,old);
 assert(ticks>=180);        /* khong phanh kieu cat dien */
 assert(ticks<=280);        /* ~1.40 s */
 assert(fabsf(old)<0.02f);

 /* Quang duong tuong doi: tich phan v_ref trong luc phanh. */
 BalanceGovernor_Reset(&g);g.equilibrium=2.0f;
 old=0.90f;float x=0;
 for(int i=0;i<400&&fabsf(old)>0.005f;i++){x+=old*0.005f;old=BalanceGovernor_Step(&g,old,0.0f,0.90f,0,2.0f,0.0f);}
 printf("quang phanh = %.3f m\n",x);
 assert(x<0.66f);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'brake.c');
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

const oldExe=build(['-DBALANCE_GOV_BRAKE_CAP=0.65f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'cap phanh cu0,65 thi test phai fail');
console.log('old-constant probe: cap0,65 bi bat (exit '+oldRun.status+')');
console.log('PASS RT36 brake: nha lenh tu0,90 dung trong ~1,3 s / <0,66 m (cap0,65 cu: ~1,55 s / 0,70 m).');


