// Operator measurement15/10/2026. VM follows LiPo3S, not the XL4015 5V rail.
motor_vm_reference=11.7; // measured at driver VM now; NOT a live ADC signal
motor_vm_minimum=11.2; // operator recharges here; not a firmware cutoff
motor_vm_full=12.6;
motor_bench_reference=11.1; // historical hypothesis, bench voltage was not recorded
motor_bridge_r_nominal=.6; // Allegro full bridge typ25C, source+sink ohms
motor_bridge_r_stress=1.5; // Allegro max150C, stress only, not measured board R
motor_loaded_identified=%f;
