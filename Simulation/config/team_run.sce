// Edit freely for the team's experiments; simulation never writes firmware.
team_pid=[48 3.6 32 .5 .3]; // Restored R7: angle Kp,Kd; velocity Kp,Ki; position Kp
team_adaptive_imu=[]; // R7 uses fixed alpha=0.98.
team_capture_angle_gain=[]; // R7 has no R10 scheduled angle gain.
team_first_armed_zero=%t; // R7 brakes on ARM tick; control starts next tick.
team_duration=300; // longer than all three complete floor captures; not a hardware guarantee
team_pitch_deg=1;team_gyro_dps=0;team_wheel_speed=0;
team_seed=1;
// Zero means UNIDENTIFIED offset, not proof that the real CG lies on the axle.
// x positive toward battery WAGO, negative toward 5V WAGO.
team_cg_x_offset_mm=0;
team_pitch_reference_error_deg=0; // positive: sensor reports a more rearward angle
// Diagnostic only: +0.29deg reproduces the observed mean under assumed CG x=0.
// It is NOT an accepted IMU calibration; a CG/reference tradeoff can do the same.
team_compare_equivalent_reference_deg=[]; // .29 was one 14/10 hypothesis, not shared by the three 15/10 captures
team_gyro_adc_bias_dps=-.186220; // ADC phase from all three 15/10 captures; not residual gyro drift
// Six-channel held tuples recur every114..115 logged frames in all three runs.
// Near-nominal effective refresh relative to STM32 time, NOT a register change
// or an independent measurement of the absolute Bosch clock/observation delay.
team_imu_odr_hz=99.1244; // configuration register remains nominal100Hz on the real R7
// Effective stable-regime R7 fit14/10: verified on a separate time window.
// NOT independently measured ground-loaded torque/friction/Bosch delay.
// Previous comparison: force=.75,friction=.75,tau=.003,delay=4.
team_force_scale=.55;team_friction_scale=.4125;
team_torque_tau=.003;team_delay_ticks=3;
team_substeps=256;
team_plot=%t;
