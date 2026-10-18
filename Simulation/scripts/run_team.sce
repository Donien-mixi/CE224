// Self-contained baseline: no dependency on historical logs/reports or generated builds.
simulation_root=get_absolute_file_path("run_team.sce")+"../";
exec(simulation_root+"scripts/new_run.sci",-1);
exec(simulation_root+"config/run_settings.sce",-1);
exec(simulation_root+"config/team_run.sce",-1);
if isdef("team_cli") then if team_cli then team_plot=%f;end;end
out=simulation_new_run(simulation_root,"team_baseline");
simulation_output_override=out;diary(out+"/scilab_console.txt");
try
 simulation_team_mode=%t;chi_khai_bao=%t;do_plot=%f;export_png=%f;
 exec(simulation_root+"model/robot_can_bang_pid_1.sce",-1);
 exec(simulation_root+"model/capture_authority.sci",-1);
 exec(simulation_root+"model/geometry_uncertainty.sci",-1);
 dau_acc=-1;dau_gyro=1;dau_pid=-1;
 code=mgetl(simulation_root+"model/robot_can_bang_pid_1.sce");ia=grep(code,"Ts=0.005;");ib=grep(code,"S=nhieu_co_dinh(S,1);");
 execstr(code(ia(1):ib(1)));
 [Pc,Sc,eq_true,eq_observed]=geometry_uncertainty(P,S,team_cg_x_offset_mm,team_pitch_reference_error_deg);
 Pc.tau=team_torque_tau;Pc.Fstall=P.Fstall*team_force_scale;Pc.Fmax=P.Fmax*team_force_scale;
 Pc.Tstat=P.Tstat*team_friction_scale;Pc.Tkin=P.Tkin*team_friction_scale;
 Sc.nsub=team_substeps;Sc.tend=team_duration;Sc.sensor_delay_ticks=team_delay_ticks;
 Sc.th0=(team_pitch_deg+team_pitch_reference_error_deg)*deg;Sc.pitch_est0=team_pitch_deg*deg;Sc.initial_omega=team_gyro_dps*deg;
 Sc.gyro_adc_bias=team_gyro_adc_bias_dps;
 Sc.imu_odr=team_imu_odr_hz;
 Sc.initial_velocity=team_wheel_speed+P.r*Sc.initial_omega;Sc.initialize_sensor_rate=%t;
 Sc.trim=0;Sc.audit_control=%t;Sc.first_armed_zero=team_first_armed_zero;Sc.trial_interlocks=%t;
 Sc.integral_start=3;Sc.integral_limit=2;Sc.integral_gate_angle=5;Sc.integral_gate_speed=.15;
 if size(team_capture_angle_gain,"*")>0 then Sc.capture_angle_gain=team_capture_angle_gain;end
 if isdef("team_adaptive_imu") then
  if size(team_adaptive_imu,"*")>0 then Sc.adaptive_imu=team_adaptive_imu;end
 end
 Sc=nhieu_co_dinh(Sc,team_seed);
 // The model declaration prints nominal P. Record the actual per-run overrides.
 csvWrite([Pc.Fstall,Pc.Fmax,Pc.Tstat,Pc.Tkin,Pc.tau,Sc.sensor_delay_ticks,Sc.nsub,Sc.imu_odr],out+"/effective_dynamic_parameters_used.csv");
 mputl("Fstall_N,Fmax_N,Tstatic_Nm,Tkinetic_Nm,force_tau_s,extra_sensor_delay_ticks,substeps,effective_IMU_refresh_Hz",out+"/effective_dynamic_parameters_columns.txt");
 mprintf("ACTUAL per-run effective overrides: Fmax %.5fN, Tstat %.6fNm, tau %.2fms, extra delay %.1fms, nsub%d. NOT independent hardware measurements.\n",Pc.Fmax,Pc.Tstat,1000*Pc.tau,1000*Sc.sensor_delay_ticks*Sc.Ts,Sc.nsub);
 mprintf("Effective IMU refresh %.6fHz relative to200Hz controller; six-channel repeat cadence inferred from15/10 logs. Register nominal100Hz.\n",Sc.imu_odr);
 [T,Y,F,U,E]=mo_phong(Pc,team_pid,Sc,kf);
 have_equivalent=%f;
 if size(team_compare_equivalent_reference_deg,"*")>0 then
  // Same physical/controller/noise assumptions; reference error is unmeasured.
  // Do NOT interpret this as a newly calibrated BMI160 zero or measured CG.
  Se=Sc;de=team_compare_equivalent_reference_deg;
  Se.angle_model(2)=Se.angle_model(2)+de;
  Se.th0=Se.th0+de/Se.angle_model(1)*deg;
  [Te,Ye,Fe,Ue,Ee]=mo_phong(Pc,team_pid,Se,kf);have_equivalent=%t;
  csvWrite([Te' Ye' Ee' Ue'],out+"/trajectory_equivalent_reference.csv");
  csvWrite([de,Te($),max(abs(Ee(1,:)/deg)),max(abs(Ee(6,:)))],out+"/equivalent_reference_summary.csv");
 end
 csvWrite([T' Y' E' U'],out+"/trajectory.csv");
 csvWrite([P.xc*1000,Pc.xc*1000,team_pitch_reference_error_deg,eq_true,eq_observed],out+"/geometry_uncertainty_used.csv");
 mputl("nominal_cg_x_mm,effective_cg_x_mm,reference_error_deg,static_true_eq_deg,static_observed_eq_deg",out+"/geometry_uncertainty_columns.txt");
 mputl("time_s,x_axle_m,v_axle_mps,pitch_true_rad,omega_true_radps,force_state_N,pitch_est_rad,gyro_radps,acc_pitch_rad,pwm_base,v_encoder_mps,x_encoder_m,target_deg,pwm_left,pwm_right,integral_deg,drive_average",out+"/trajectory_columns.txt");
 csvWrite([T($),team_duration,max(abs(E(1,:)/deg)),max(abs(E(6,:))),max(abs(E(5,:)))],out+"/summary.csv");
 mputl("duration_s,requested_duration_s,peak_est_pitch_deg,peak_encoder_travel_m,peak_encoder_speed_mps",out+"/summary_columns.txt");
 mprintf("\nCONDITIONAL current trial: %.3f/%.1fs, peak pitch %.3fdeg, encoder travel %.2fcm.\n",T($),team_duration,max(abs(E(1,:)/deg)),100*max(abs(E(6,:))));
 mprintf("Sagittal mean-force model: differential motor imbalance/yaw NOT validated.\n");
 mprintf("VALIDATION NOT PASSED: three complete hands-off R7 floor runs15/10 fell after93.60,95.85,153.78s.\n");
 mprintf("Stable nominal simulation alone cannot authorize a new hardware PID. See reports/r7_falls_20261015.\n");
 mputl(["hardware_long_balance_validation=not_passed"; ..
        "reason=complete_R7_floor_falls_not_explained_by_nominal_model"; ..
        "CG_offset_identified=false";"firmware_candidate_validated=false"],out+"/validation_status.txt");
 mprintf("CG x %.4fmm; reference error %.4fdeg are separate hypotheses, NOT an accepted calibration.\n",Pc.xc*1000,team_pitch_reference_error_deg);
 if have_equivalent then
  mprintf("Additional equilibrium-reference HYPOTHESIS +%.4fdeg: %.3fs, encoder travel %.2fcm. Not a sensor calibration or identified CG.\n",de,Te($),100*max(abs(Ee(6,:))));
 end
 mprintf("No automatic PID search or firmware changes. Edit config/team_run.sce for experiments.\n");
 if team_plot then
  f=scf();clf(f);
  if have_equivalent then
   subplot(3,1,1);plot(T',E(1,:)'/deg,Te',Ee(1,:)'/deg);xgrid();ylabel("Estimated pitch deg");title("R7: nominal / equivalent reference hypothesis (NOT identified CG)");legend("nominal", "reference hypothesis +"+string(de)+"deg");
   subplot(3,1,2);plot(T',100*E(6,:)',Te',100*Ee(6,:)');xgrid();ylabel("Encoder travel cm");
   subplot(3,1,3);plot(T',E(4,:)',Te',Ee(4,:)');xgrid();ylabel("Base PWM before deadband");xlabel("Time s");
  else
   subplot(3,1,1);plot(T',E(1,:)'/deg,T',E(7,:)');xgrid();ylabel("Pitch / target deg");title("Conditional current trial: estimated angle / target");
   subplot(3,1,2);plot(T',100*E(6,:)');xgrid();ylabel("Encoder travel cm");
   subplot(3,1,3);plot(T',E(8,:)',T',E(9,:)');xgrid();ylabel("PWM left / right");xlabel("Time s");
  end
 end
 mputl("status=completed",out+"/run_status.txt");
 mprintf("RESULTS: %s\n",out);
catch
 msg=lasterror();mputl(["status=failed";msg],out+"/run_status.txt");diary(0);error(msg);
end
diary(0);clear simulation_team_mode;
