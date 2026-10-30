// A/B proof for RT8: the SAME scenarios compiled against the legacy constants
// (gyro60dps, 10cm budget, 18cm blockage, 16cm turn knee / 20% floor) and the
// RT8 constants. If legacy reproduces the reported bugs and RT8 does not, the
// fix is demonstrated rather than asserted.
const fs = require('node:fs'), path = require('node:path'), cp = require('node:child_process'), assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const inc = path.join(root, 'Xe_Can_Bang/Core/Inc');
const legacy = require('./lib/legacy_headers.cjs').ensureLegacyHeaders(root);
const out = path.join(root, '.build/rt8_ab');
fs.mkdirSync(out, { recursive: true });

const src = String.raw`
#include <stdio.h>
#include "balance_motion.h"
#include "balance_turn.h"
#include "balance_recovery.h"
static void prime(BalanceRecovery*s,uint32_t*now){
 BalanceRecovery_Clear(s);
 for(int i=0;i<130;i++){*now+=5;BalanceRecovery_UpdateMode(s,true,0,0,0,0,.30f,*now,false);}
}
int main(void){
 uint32_t now=1000;
 /* 1) gyro spike while primed, idle (no maneuver), pitch2deg */
 int gl[]={60,100,149,150,200};
 for(int i=0;i<5;i++){
  BalanceRecovery r;prime(&r,&now);
  BalanceRecovery_UpdateMode(&r,true,2.0f,(float)gl[i],0,0,.30f,now+5,false);
  printf("gyro,%d,%d\n",gl[i],(int)r.active);
 }
 /* 2) pitch path must stay: idle trigger 8deg */
 float pl[]={7.9f,8.1f,11.9f,12.1f};
 for(int i=0;i<4;i++){
  BalanceRecovery r;prime(&r,&now);
  BalanceRecovery_UpdateMode(&r,true,pl[i],0,0,0,.30f,now+5,false);
  printf("pitch,%.1f,%d\n",pl[i],(int)r.active);
 }
 /* 3) reverse with the body ahead by offset (gesture already held) */
 double off[]={.05,.12,.17,.19,.21};
 for(int i=0;i<5;i++){
  BalanceMotion m;BalanceMotion_Clear(&m);
  uint32_t t0=now;m.ready=true;m.used=true;m.v_ref=-.5f;m.x_ref=-(float)off[i];
  m.requested=-1;
  m.last_word=42;m.last_ms=t0;
  float pos=0;
  for(int k=0;k<40;k++){t0+=5;pos-=0.48f*0.005f;BalanceMotion_Update(&m,true,42,pos,0,0,-.48f,t0);}
  printf("reverse,%.2f,%.3f,%u\n",off[i],m.v_ref,m.flags&(8|64|128));
  now=t0+5;
 }
 /* 4) pure turn then centre drift */
 double dr[]={.05,.11,.15,.19,.23};
 for(int i=0;i<5;i++){
  BalanceMotion m;BalanceTurn t;BalanceMotion_Clear(&m);BalanceTurn_Clear(&t);
  uint32_t t0=now;m.ready=true;m.used=true;m.x_ref=0;m.last_word=0;m.last_ms=t0;
  float yaw=0;
  for(int k=0;k<100;k++){t0+=5;uint32_t w=((uint32_t)(k+1)<<2);
   BalanceMotion8_Update(&m,&t,true,w,1,0,0,0,0,yaw,t.w_ref,t0);yaw+=t.w_ref*0.005f;}
  for(int k=0;k<300;k++){t0+=5;uint32_t w=((uint32_t)(101+k)<<2);
   BalanceMotion8_Update(&m,&t,true,w,1,(float)dr[i],0,0,0,yaw,t.w_ref,t0);yaw+=t.w_ref*0.005f;}
  printf("turn,%.2f,%.3f,%u\n",dr[i],t.w_ref,m.flags&(64|128));
  now=t0+5;
 }
 /* 5) wall: body never moves, reverse still held -> must still cancel */
 {
  BalanceMotion m;BalanceMotion_Clear(&m);
  uint32_t t0=now;m.ready=true;m.used=true;m.requested=-1;m.last_word=0;m.last_ms=t0;
  int ticks=-1;
  for(int k=0;k<400;k++){
   t0+=5;uint32_t w=((uint32_t)(k+1)<<2)|2U;
   BalanceMotion_Update(&m,true,w,0,0,0,0,t0);
   if(m.flags&64){ticks=k;break;}
  }
  printf("wall,0,%d\n",ticks);
 }
 return 0;
}
`;
const file = path.join(out, 'ab.c');
fs.writeFileSync(file, src);
const exeOld = path.join(out, 'ab_legacy.exe'), exeNew = path.join(out, 'ab_rt8.exe');
function build(exe, legacyFirst) {
  const args = ['-std=c11', '-O2', '-Wall', '-Wextra', file, '-lm', '-o', exe];
  args.unshift(...(legacyFirst ? ['-I' + legacy, '-I' + inc] : ['-I' + inc]));
  const r = cp.spawnSync('C:/msys64/ucrt64/bin/gcc.exe', args, { encoding: 'utf8' });
  assert.equal(r.status, 0, r.stderr);
}
build(exeOld, true); build(exeNew, false);
const run = exe => cp.spawnSync(exe, [], { encoding: 'utf8' });
const oldRun = run(exeOld), newRun = run(exeNew);
assert.equal(oldRun.status, 0, oldRun.stderr);
assert.equal(newRun.status, 0, newRun.stderr);

function parse(text) {
  const m = new Map();
  for (const line of text.trim().split('\n')) {
    const [k, p, v] = line.split(',');
    m.set(k + ',' + p, Number(v));
  }
  return m;
}
const O = parse(oldRun.stdout), N = parse(newRun.stdout);

// assertions: legacy shows the bugs, RT8 does not
assert.equal(O.get('gyro,60'), 1, 'legacy should trip at 60 dps');
assert.equal(O.get('gyro,100'), 1);
assert.equal(N.get('gyro,60'), 0, 'RT8 must ignore a 60 dps bump');
assert.equal(N.get('gyro,100'), 0);
assert.equal(N.get('gyro,149'), 0);
assert.equal(N.get('gyro,150'), 1, 'RT8 must still trip at 150 dps');
assert.equal(O.get('pitch,8.1'), 1); assert.equal(N.get('pitch,8.1'), 1, 'pitch protection kept');
assert.equal(O.get('pitch,12.1'), 1); assert.equal(N.get('pitch,12.1'), 1);
assert(O.get('reverse,0.12') > -0.45, 'legacy reverse should be throttled at 12 cm');
assert(N.get('reverse,0.12') <= -0.45, 'RT8 reverse must hold speed at 12 cm');
assert(O.get('reverse,0.17') > -0.45 && N.get('reverse,0.17') <= -0.45, 'RT8 must hold speed at 17 cm');
assert(N.get('reverse,0.21') > -0.45, 'RT8 still throttles past 20 cm');
assert(O.get('turn,0.11') < 0.9, 'legacy turn loses rate at 11 cm drift');
assert(N.get('turn,0.11') >= 1.15, 'RT8 keeps full turn rate at 11 cm drift');
assert(N.get('turn,0.19') > O.get('turn,0.19'), 'RT8 must be better than legacy at 19 cm');
assert(O.get('wall,0') >= 0 && N.get('wall,0') >= 0, 'both must still detect a blocked wheel');
assert(N.get('wall,0') <= 200, 'RT8 must cancel a wall within 1 s');

const rows = [];
const cmp = (kind, key, label, fmt) => rows.push([kind, label, fmt(O.get(kind + ',' + key)), fmt(N.get(kind + ',' + key))]);
for (const g of [60, 100, 149, 150, 200]) cmp('gyro', String(g), g + ' dps', v => v ? 'ph\u1ee5c h\u1ed3i' : 'b\u1ecf qua');
for (const p of ['7.9', '8.1', '12.1']) cmp('pitch', p, p + '\u00b0', v => v ? 'ph\u1ee5c h\u1ed3i' : 'b\u1ecf qua');
for (const o of ['0.05', '0.12', '0.17', '0.19', '0.21']) cmp('reverse', o, 'l\u1ec7ch ' + (o * 100).toFixed(0) + ' cm', v => v.toFixed(3) + ' m/s');
for (const d of ['0.05', '0.11', '0.15', '0.19', '0.23']) cmp('turn', d, 'tr\u00f4i ' + (d * 100).toFixed(0) + ' cm', v => v.toFixed(3) + ' rad/s');
const wallOld = O.get('wall,0'), wallNew = N.get('wall,0');
rows.push(['wall', 'k\u1eb9t t\u01b0\u1eddng', wallOld >= 0 ? ((wallOld * 5) + ' ms') : 'kh\u00f4ng hu\u1ef7', wallNew >= 0 ? ((wallNew * 5) + ' ms') : 'kh\u00f4ng hu\u1ef7']);

console.log('| T\u00ecnh hu\u1ed1ng | C\u0169 (RT7) | M\u1edbi (RT8) |');
console.log('|---|---|---|');
for (const [k, l, o, n] of rows) console.log('| ' + k + ' ' + l + ' | ' + o + ' | ' + n + ' |');
console.log('PASS rt8 A/B: legacy reproduces all three bugs, RT8 fixes them, pitch and wall protection unchanged');
