// Build-ID sync guard. The page and the STM32 agree on a build ID: the dashboard
// only unlocks (matched()) and /operate only accepts ARM when $BINFO reports the
// exact string this gateway was built with. RT27 shipped with the ESP32 literals
// still reading BAL-20261114-... while the STM32 emitted BAL-20261115-..., so the
// page showed "Chờ thông tin phiên bản STM32 BAL-20261114-..." and refused to arm.
// This test fails if any literal drifts from balance_profile.h again.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const profile=read('Xe_Can_Bang/Core/Inc/balance_profile.h');
const stm32=/^#define BALANCE_BUILD "([^"]+)"/m.exec(profile);
assert.ok(stm32,'BALANCE_BUILD not found in balance_profile.h');
const BUILD=stm32[1];
console.log('STM32 build:',BUILD);
const web=/^#define TWIP_WEB_BUILD "([^"]+)"/m.exec(read('ESP32_S3_Gateway/firmware_build.h'));
assert.ok(web,'TWIP_WEB_BUILD not found');
const date=/BAL-(\d{8})-/.exec(BUILD)[1];
const milestone=BUILD.replace(/^BAL-\d{8}-/,'BAL-');
assert.ok(web[1].includes(milestone),`TWIP_WEB_BUILD must embed the milestone ${milestone}: ${web[1]}`);
assert.ok(web[1].includes('-'+date+'-'),`TWIP_WEB_BUILD must carry the same date ${date}: ${web[1]}`);
console.log('Gateway build:',web[1]);
const files=[];
for(const d of ['ESP32_S3_Gateway']){
  for(const f of fs.readdirSync(path.join(root,d))){
    if(/\.(h|ino|cpp)$/.test(f))files.push(`${d}/${f}`);
  }
}
for(const f of fs.readdirSync(path.join(root,'tools'))){
  if(/\.cjs$/.test(f)&&!/^test_drive8_/.test(f))files.push(`tools/${f}`);
}
const pat=/BAL-\d{8}-R12-8DIR-[A-Z0-9-]+/g;
const scan=(text)=>[...text.matchAll(pat)].map(m=>m[0]);
let checked=0;
for(const f of files){
  for(const lit of scan(read(f))){
    checked++;
    assert.equal(lit,BUILD,`${f} carries ${lit} but the STM32 build is ${BUILD}`);
  }
}
console.log(`checked ${checked} literals across ${files.length} files`);
assert.ok(checked>=8,`expected at least 8 build literals to check, saw ${checked}`);
// Sensitivity: the guard must fail on a one-character drift (this is the exact
// RT27 bug: 20261114 vs 20261115).
const drift=BUILD.replace(/-(\d{8})-/, '-20261114-');
assert.notEqual(drift,BUILD);
assert.notEqual(scan('$BINFO,'+drift+',1')[0],BUILD);
console.log('drift probe: a different date is rejected as expected');
console.log('PASS build-id sync: gateway literals match the STM32 build everywhere.');
