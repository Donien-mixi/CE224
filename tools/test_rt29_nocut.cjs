// RT29: the disturbance brake (REC1 entry, hold_anchor) must not have a 4 s
// completion deadline. Log RT28 (12)(3) auto-cut the motor three times, all
// fault 4096, exactly 4 s after a REC1 entry, on a vehicle that was upright.
const fs=require('node:fs'),path=require('node:path'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),out=path.join(root,'.build/rt29_nocut');
fs.mkdirSync(out,{recursive:true});
const src=String.raw`
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include "balance_motion.h"
int main(void){
 setbuf(stdout,NULL);
 BalanceMotion s;BalanceMotion_Clear(&s);s.used=true;s.ready=true;
 /* 1. Disturbance brake: REC1 entry, anchor held, vehicle still rolling slowly
  * because the operator keeps pushing. No fault, ever. */
 s.braking=true;s.hold_anchor=true;s.cancel_origin=0.0f;s.cancel_started_ms=1000;
 for(uint32_t t=1000;t<=12000;t+=5)assert(BalanceMotion_Fault(&s,0.20f,t)==0);
 printf("disturbance brake12s at0.20m: no fault\n");
 /* 2. A commanded release brake keeps its4s envelope (RT19 behaviour). */
 BalanceMotion_Clear(&s);s.used=true;s.ready=true;
 s.braking=true;s.cancel_origin=0.0f;s.cancel_started_ms=1000;
 assert(BalanceMotion_Fault(&s,0.20f,4995)==0);
 assert(BalanceMotion_Fault(&s,0.20f,5000)==4096);
 printf("commanded brake still times out at4s: fault4096\n");
 /* 3. The distance bound still applies to the disturbance brake (runaway bound). */
 BalanceMotion_Clear(&s);s.used=true;s.ready=true;
 s.braking=true;s.hold_anchor=true;s.cancel_origin=0.0f;s.cancel_started_ms=1000;
 assert(BalanceMotion_Fault(&s,1.20f,1500)==0);
 assert(BalanceMotion_Fault(&s,1.30f,1500)==8);
 printf("disturbance brake still bounds1.25m: fault8\n");
 /* 4. REC1's own4s timeout is untouched (balance_recovery.h). */
 puts("PASS");
 return 0;
}
`;
const file=path.join(out,'nocut.c');
fs.writeFileSync(file,src);
const inc='-I'+path.join(root,'Xe_Can_Bang/Core/Inc');
const exe=path.join(out,'nocut.exe');
let r=cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe',['-std=c11','-Wall','-Wextra','-Werror',inc,file,path.join(root,'Xe_Can_Bang/Core/Src/balance_recovery.c'),path.join(root,'Xe_Can_Bang/Core/Src/balance_motion.c'),'-lm','-o',exe],{encoding:'utf8'});
assert.equal(r.status,0,r.stderr||String(r.error));
const run=cp.spawnSync(exe,[],{encoding:'utf8'});
process.stdout.write(run.stdout||'');
assert.equal(run.status,0,(run.stdout||'')+(run.stderr||''));
console.log('PASS RT29: no 4s cut on a disturbance brake; commanded brake and the 1.25m bound unchanged.');
