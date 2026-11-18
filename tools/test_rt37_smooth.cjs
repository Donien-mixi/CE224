// RT37 — ngưỡng nghiêng của governor (bug: xuất phát/tăng tốc bị ngập ngừng).
//
// Log (2)(5) build RT36: trong 2 s đầu mỗi cú xuất phát xe ngả 7-12°, governor
// bật `limited` 25-44 % mẫu và ramp rơi xuống 0,06 m/s² từng đợt → cảm giác
// ngập ngừng. Ngưỡng cũ 9° (thường) / 11° (xuất phát) thấp hơn độ ngả bình
// thường của xe. RT37 nâng lên 10° / 13°; trần ngã 25°, taper cua 8-12° nên vẫn
// còn biên. Mô phỏng MAX90 giữ 11/12 (chỉ ca pin 11,2 V).
//
// Test phải FAIL với ngưỡng cũ 9/11.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt37_smooth');
fs.mkdirSync(out,{recursive:true});

const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_governor.h"

/* Chay 1 buoc khong co lag, duty thap: chi con nguong nghieng quyet dinh. */
static int pressure(float pitch,float old){
 BalanceGovernor g;BalanceGovernor_Reset(&g);
 g.equilibrium=2.0f;
 BalanceGovernor_Observe(&g,1200,1200,old);      /* duty~0.48, speed=old */
 g.acceleration=.2f;
 (void)BalanceGovernor_Step(&g,old,.90f,.70f,1,pitch,0.0f);
 return g.pressure?1:0;
}
int main(void){
 setbuf(stdout,NULL);

 /* 1. Xuat phat, nghieng13,2deg -> lech11,2deg: KHONG con ap luc. */
 int a=pressure(13.2f,0.20f);
 printf("xuat phat 13.2deg (lech11.2): pressure=%d (phai0)\n",a);
 assert(a==0);

 /* 2. Xuat phat nghieng15,5deg (lech13,5): van phai ghim. */
 int b=pressure(15.5f,0.20f);
 printf("xuat phat 15.5deg (lech13.5): pressure=%d (phai1)\n",b);
 assert(b==1);

 /* 3. Dang chay0,60m/s, nghieng12,0deg (lech10,0): KHONG ap luc. */
 int c=pressure(12.0f,0.60f);
 printf("dang chay 12.0deg (lech10.0): pressure=%d (phai0)\n",c);
 assert(c==0);

 /* 4. Dang chay nghieng12,5deg (lech10,5): phai ghim. */
 int d=pressure(12.5f,0.60f);
 printf("dang chay 12.5deg (lech10.5): pressure=%d (phai1)\n",d);
 assert(d==1);

 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'smooth.c');
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

const oldExe=build(['-DBALANCE_GOV_LEAN_DEG=9.0f','-DBALANCE_GOV_LEAN_LAUNCH_DEG=11.0f'],'old');
const oldRun=cp.spawnSync(oldExe,[],{encoding:'utf8'});
assert.notEqual(oldRun.status,0,'nguong cu 9/11 thi test phai fail');
console.log('old-constant probe: nguong 9/11 bi bat (exit '+oldRun.status+')');
console.log('PASS RT37: nguong nghieng 10/13 do - khong cat gia toc o do nga binh thuong, van ghim khi qua 13,5.');
