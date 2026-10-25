// Only run controls. Physical model, PID search and acceptance gates stay in model/.
export_png=%t;
do_plot=%t;
animate=%f;
chi_khai_bao=%f;
// Refine implicit friction integration; full run verifies128 vs256 subdivisions.
sim_nsub=128;
// If retry2..7 exists, simulate proposed XYZ+installed-reference correction.
// Motion/fixture warnings remain; this switch never approves hardware deployment.
use_provisional_accel_calibration=%t;
// Current human goal: self-balance with limited travel, not precise position hold.
// These are proposed engineering limits, not amended formal Roadmap acceptance.
balance_objective=struct("name","bounded_balance","angle_band",2.0, ..
 "xmax",0.10,"xendmax",0.10,"vendmax",0.05,"rungmax",5.0, ..
 "drift_rate_max",0.002,"drift_window",10);
// Preserve the model defaults for all tuning options not set here.
