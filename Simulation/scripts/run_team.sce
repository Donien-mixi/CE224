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
 dau_acc=-1;dau_gyro=1;dau_pid=-1;
 code=mgetl(simulation_root+"model/robot_can_bang_pid_1.sce");ia=grep(code,"Ts=0.005;");ib=grep(code,"S=nhieu_co_dinh(S,1);");
 execstr(code(ia(1):ib(1)));
 Pc=P;Pc.tau=team_torque_tau;Pc.Fstall=P.Fstall*team_force_scale;Pc.Fmax=P.Fmax*team_force_scale;
 Pc.Tstat=P.Tstat*team_friction_scale;Pc.Tkin=P.Tkin*team_friction_scale;
 Sc=S;Sc.nsub=team_substeps;Sc.tend=team_duration;Sc.sensor_delay_ticks=team_delay_ticks;
 Sc.th0=team_pitch_deg*deg;Sc.pitch_est0=Sc.th0;Sc.initial_omega=team_gyro_dps*deg;
 Sc.initial_velocity=team_wheel_speed+P.r*Sc.initial_omega;Sc.initialize_sensor_rate=%t;
 Sc.trim=0;Sc.audit_control=%t;Sc.first_armed_zero=%f;Sc.trial_interlocks=%t;
 Sc.integral_start=3;Sc.integral_limit=2;Sc.integral_gate_angle=5;Sc.integral_gate_speed=.15;
 Sc.capture_angle_gain=[3 6 1.75];Sc=nhieu_co_dinh(Sc,team_seed);
 [T,Y,F,U,E]=mo_phong(Pc,team_pid,Sc,kf);
 csvWrite([T' Y' E' U'],out+"/trajectory.csv");
 mputl("time_s,x_axle_m,v_axle_mps,pitch_true_rad,omega_true_radps,force_state_N,pitch_est_rad,gyro_radps,acc_pitch_rad,pwm_base,v_encoder_mps,x_encoder_m,target_deg,pwm_left,pwm_right,integral_deg,drive_average",out+"/trajectory_columns.txt");
 csvWrite([T($),team_duration,max(abs(E(1,:)/deg)),max(abs(E(6,:))),max(abs(E(5,:)))],out+"/summary.csv");
 mputl("duration_s,requested_duration_s,peak_est_pitch_deg,peak_encoder_travel_m,peak_encoder_speed_mps",out+"/summary_columns.txt");
 mprintf("\nCONDITIONAL R10 baseline: %.3f/%.1fs, peak pitch %.3fdeg, encoder travel %.2fcm.\n",T($),team_duration,max(abs(E(1,:)/deg)),100*max(abs(E(6,:))));
 mprintf("Sagittal mean-force model: differential motor imbalance/yaw NOT validated.\n");
 mprintf("No automatic PID search or firmware changes. Edit config/team_run.sce for experiments.\n");
 if team_plot then
  f=scf();clf(f);
  subplot(3,1,1);plot(T',E(1,:)'/deg,T',E(7,:)');xgrid();ylabel("Pitch / target deg");title("Conditional R10 baseline: estimated angle / target");
  subplot(3,1,2);plot(T',100*E(6,:)');xgrid();ylabel("Encoder travel cm");
  subplot(3,1,3);plot(T',E(8,:)',T',E(9,:)');xgrid();ylabel("PWM left / right");xlabel("Time s");
 end
 mputl("status=completed",out+"/run_status.txt");
 mprintf("RESULTS: %s\n",out);
catch
 msg=lasterror();mputl(["status=failed";msg],out+"/run_status.txt");diary(0);error(msg);
end
diary(0);clear simulation_team_mode;
