// Build a copy of Core/Inc with the pre-RT8 supervisory constants, so tests can
// run the same scenario against "before" and "after" and compare.
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

function ensureLegacyHeaders(root) {
  const inc = path.join(root, 'Xe_Can_Bang/Core/Inc');
  const legacy = path.join(root, '.build/legacy_inc');
  fs.rmSync(legacy, { recursive: true, force: true });
  fs.cpSync(inc, legacy, { recursive: true });
  const patch = (file, pairs) => {
    const p = path.join(legacy, file);
    let t = fs.readFileSync(p, 'utf8');
    for (const [a, b] of pairs) {
      assert(t.includes(a), `legacy patch anchor missing in ${file}: ${a}`);
      t = t.split(a).join(b);
    }
    fs.writeFileSync(p, t);
  };
  patch('balance_recovery.h', [['fabsf(gyro)>=150.0f', 'fabsf(gyro)>=60.0f']]);
  patch('balance_motion.h', [
    ['dir*error>.30f', 'dir*error>.18f'],
    ['wanted*(s->x_ref-position)>.20f', 'wanted*(s->x_ref-position)>.10f']
  ]);
  patch('balance_turn.h', [
    ['fmaxf(.40f,fminf(1,(.22f-fabsf', 'fmaxf(.20f,fminf(1,(.16f-fabsf']
  ]);
  return legacy;
}
module.exports = { ensureLegacyHeaders };
