// RT38 â€” chuyen huong lien tuc: khong phanh khi doi nut, va khong cat toc do
// khi moc chay truoc 20cm.
//
// Log (2)(5) build RT36: ngay sau cu dao chieu -0,9 -> +0,9 m/s, 72 % mau bi
// phanh va 52 % vao REC1, toc do khong ve lai 0,5 trong 1,2 s. Nguyen nhan:
// (a) nut dieu khien gui "neutral" vai goi khi doi huong -> motion phanh ngay;
// (b) khi moc vuot 20 cm, target bi dat 0 -> v_ref tut roi ramp lai.
// RT38 giu lenh cu toi120ms va neo moc thay vi cat toc do.
//
// Test phai FAIL voi hanh vi cu (RELEASE_HOLD_TICKS=0 hoac LEAD_CUT=1).
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt38_continuous');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"

static BalanceMotion s;static uint32_t ms,seq;static float pos;
static void init(void){BalanceMotion_Clear(&s);s.ready=true;s.speed_preset=2;ms=1000;seq=0;pos=0;}
/* tick(dir, speed, position_step_scale) */
static void tick(int dir,float speed,float pos_scale){
 ms+=5;seq++;
 assert(BalanceMotion_Update(&s,true,seq*4U+(dir>0?1U:dir<0?2U:0U),pos,2.0f,0.0f,speed,ms));
 pos+=pos_scale*s.v_ref*.005f;
}
int main(void){
 setbuf(stdout,NULL);
 int brake_seen;

 /* A. +0,9 -> 3 goi neutral -> -0,9: khong duoc phanh, phai dao chieu lien tuc. */
 init();brake_seen=0;
 for(int i=0;i<500;i++)tick(1,s.v_ref,1.0f);
 assert(s.v_ref>0.85f);
 for(int i=0;i<3;i++){tick(0,s.v_ref,1.0f);if(s.flags&1024U)brake_seen=1;}
 printf("sau3 goi neutral: v_ref=%.3f brake=%d\n",s.v_ref,brake_seen);
 assert(!brake_seen);
 assert(fabsf(s.v_ref)>0.80f);
 for(int i=0;i<600;i++){tick(-1,s.v_ref,1.0f);if(s.flags&1024U)brake_seen=1;}
 printf("sau dao chieu: v_ref=%.3f brake=%d\n",s.v_ref,brake_seen);
 assert(!brake_seen);
 assert(s.v_ref<-0.80f);

 /* B. Nha han that (40 goi neutral): phai phanh va ve0. */
 init();brake_seen=0;
 for(int i=0;i<500;i++)tick(1,s.v_ref,1.0f);
 for(int i=0;i<40;i++){tick(0,s.v_ref,1.0f);if(s.flags&1024U)brake_seen=1;}
 for(int i=0;i<400;i++)tick(0,0.0f,1.0f);
 printf("nha han: brake=%d v_ref=%.3f\n",brake_seen,s.v_ref);
 assert(brake_seen);
 assert(fabsf(s.v_ref)<0.02f);

 /* C. Moc chay truoc: xe chi di nua toc do -> moc vuot20cm, toc do phai giu. */
 init();brake_seen=0;
 for(int i=0;i<800;i++)tick(1,s.v_ref,0.5f);
 float lead=fabsf(s.x_ref-pos);
 printf("moc vuot: lead=%.3f m v_ref=%.3f\n",lead,s.v_ref);
 assert(lead<=0.205f);
 assert(s.v_ref>0.85f);          /* khong bi cat ve0 */

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'continuous.c');
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

const oldBrake=build(['-DBALANCE_MOTION_RELEASE_HOLD_TICKS=1'],'old_brake');
const r1=cp.spawnSync(oldBrake,[],{encoding:'utf8'});
assert.notEqual(r1.status,0,'phanh ngay khi neutral thi test phai fail');
console.log('old-constant probe: phanh ngay bi bat (exit '+r1.status+')');

const oldLead=build(['-DBALANCE_MOTION_LEAD_CUT=1'],'old_lead');
const r2=cp.spawnSync(oldLead,[],{encoding:'utf8'});
assert.notEqual(r2.status,0,'cat toc do khi moc vuot thi test phai fail');
console.log('old-constant probe: cat toc do khi moc vuot bi bat (exit '+r2.status+')');
console.log('PASS RT38: doi huong 3 goi neutral khong phanh; moc vuot20cm van giu toc do.');

