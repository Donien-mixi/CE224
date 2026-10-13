// Edit freely for the team's experiments; simulation never writes firmware.
team_pid=[48 3.6 32 .5 .3]; // angle Kp,Kd; velocity Kp,Ki; position Kp
team_duration=20;
team_pitch_deg=1;team_gyro_dps=0;team_wheel_speed=0;
team_seed=1;
// Conditional reference values, NOT measured ground-loaded motor properties.
team_force_scale=.75;team_friction_scale=.75;
team_torque_tau=.003;team_delay_ticks=4;
team_substeps=128;
team_plot=%t;
