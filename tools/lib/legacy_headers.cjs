// Build a copy of Core/Inc + the two moved modules with pre-RT8 constants, so the
// A/B suites can run the same scenario against "before" and "after".
// Tunables now live in Core/Inc/tuning.h; code-level constants that moved into
// Core/Src (balance_motion.c) are patched in the Src copy instead.
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');

function ensureLegacyHeaders(root) {
  const inc = path.join(root, 'Xe_Can_Bang/Core/Inc');
  const legacy = path.join(root, '.build/legacy_inc');
  const src = path.join(root, '.build/legacy_src');
  fs.rmSync(legacy, { recursive: true, force: true });
  fs.cpSync(inc, legacy, { recursive: true });
  fs.rmSync(src, { recursive: true, force: true });
  fs.mkdirSync(src, { recursive: true });
  for (const f of ['balance_motion.c', 'balance_turn.c']) {
    fs.copyFileSync(path.join(root, 'Xe_Can_Bang/Core/Src', f), path.join(src, f));
  }
  const patch = (dir, file, pairs) => {
    const p = path.join(dir, file);
    let t = fs.readFileSync(p, 'utf8');
    for (const [a, b] of pairs) {
      assert(t.includes(a), `legacy patch anchor missing in ${file}: ${a}`);
      t = t.split(a).join(b);
    }
    fs.writeFileSync(p, t);
  };
  patch(legacy, 'tuning.h', [
    /* REC1 ngưỡng gyro thời RT7 là 60 °/s. */
    ['#define BALANCE_RECOVERY_GYRO_DPS 150.0f', '#define BALANCE_RECOVERY_GYRO_DPS 60.0f'],
    /* RT23 moved the centre-drift knee22cm->45cm/floor40%->55%; the legacy
     * baseline is the RT7 16cm/10cm/20% rule these two suites compare against. */
    ['#define BALANCE_TURN_CENTER_KNEE 0.45f', '#define BALANCE_TURN_CENTER_KNEE 0.16f'],
    ['#define BALANCE_TURN_CENTER_SPAN 0.20f', '#define BALANCE_TURN_CENTER_SPAN 0.10f'],
    ['#define BALANCE_TURN_CENTER_FLOOR 0.55f', '#define BALANCE_TURN_CENTER_FLOOR 0.20f']
  ]);
  patch(src, 'balance_motion.c', [
    ['dir*error>.30f', 'dir*error>.18f'],
    ['wanted*(s->x_ref-position)>.20f', 'wanted*(s->x_ref-position)>.10f']
  ]);
  return { inc: legacy, src };
}
module.exports = { ensureLegacyHeaders };
