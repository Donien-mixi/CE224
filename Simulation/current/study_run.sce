simulation_root=get_absolute_file_path("study_run.sce")+"../";
if ~isdef("motor_stage") then motor_stage="screen";end
if ~isdef("motor_cli") then motor_cli=%f;end
exec(simulation_root+"scripts/new_run.sci",-1);exec(simulation_root+"config/run_settings.sce",-1);
out=simulation_new_run(simulation_root,"motor_catalog_"+motor_stage);simulation_output_override=out;diary(out+"/scilab_console.txt");
report=simulation_root+"reports/motor_catalog_20261015/";if ~isdir(report) then mkdir(report);end
try
 simulation_team_mode=%t;chi_khai_bao=%t;do_plot=%f;export_png=%f;
 exec(simulation_root+"model/robot_can_bang_pid_1.sce",-1);exec(simulation_root+"model/geometry_uncertainty.sci",-1);
 exec(simulation_root+"model/pair_coupled.sci",-1);exec(simulation_root+"model/guarded_heading.sci",-1);exec(simulation_root+"model/capture_authority.sci",-1);
 exec(simulation_root+"config/motor_pair_r8_20261011.sce",-1);exec(simulation_root+"current/control.sci",-1);
 exec(simulation_root+"current/motor.sci",-1);exec(simulation_root+"current/observer.sci",-1);
 if ~c_link("twip_pair_catalog_v2") then link(simulation_root+"current/pair_catalog_v2.dll","twip_pair_catalog_v2","c");end
 bugfix_variant="blend";exec(simulation_root+"current/profiles.sce",-1);
 dau_acc=-1;dau_gyro=1;dau_pid=-1;
 code=mgetl(simulation_root+"model/robot_can_bang_pid_1.sce");ia=grep(code,"Ts=0.005;");ib=grep(code,"S=nhieu_co_dinh(S,1);");execstr(code(ia(1):ib(1)));
 track=2*y_banh;Iz=0;
 for k=1:size(lk,1)
  own=lk(k,1)*(lk(k,5)^2+lk(k,6)^2)/12;
  if lk(k,8)==3 then own=lk(k,1)*(3*(lk(k,5)/2)^2+lk(k,6)^2)/12;end
  if lk(k,8)==4 then own=lk(k,1)*(lk(k,5)/2)^2/2;end
  Iz=Iz+own+lk(k,1)*(lk(k,2)^2+lk(k,3)^2);
 end
 Iz=Iz+P.Meq*track^2/4;
 [nominal,catalog]=catalog_motor(P,pair_no_load_curve,motor_vm_reference,motor_bridge_r_nominal);
 mprintf("ACTUAL CATALOG override after legacy declaration: VM %.2fV, bridge %.2fohm, Fmax %.6fN, Tstat pair %.6fNm, Tkin pair %.6fNm. Torque/tau are not measured.\n",motor_vm_reference,motor_bridge_r_nominal,nominal.Fmax,nominal.Tstat,nominal.Tkin);
 csvWrite(catalog,out+"/catalog_parameters.csv");csvWrite([track Iz nominal.Fstall nominal.Fmax nominal.Tstat nominal.Tkin],out+"/nominal.csv");
 offsets=-pair_no_load_curve(:,2)./pair_no_load_curve(:,1);fric=[offsets(1) offsets(3) offsets(2) offsets(4)]/mean(offsets);
 // qx,qv,qtheta,qomega,R,design_tau,design_delay,Qforce,Racc_deg; type0=old candidate,type1=observer.
 settings=[200 1 150 .1 5 .006 3 .01 .5 1;200 1 150 .1 5 .006 4 .01 .5 1; ..
           200 1 150 .1 5 .006 4 .001 1.5 1;200 1 150 .1 5 .006 4 .01 .5 0];
 if isdef("motor_settings") then settings=motor_settings;end
 cases=[bugfix_cases,ones(12,1)*[11.7 1 1 .6 0 1 0]];
 if isdef("motor_cases") then cases=motor_cases;end
 ids=[2 7 11 12];if isdef("motor_case_ids") then ids=motor_case_ids;end
 duration=30;if isdef("motor_duration") then duration=motor_duration;end
 substeps=128;if isdef("motor_substeps") then substeps=motor_substeps;end
 csvWrite(cases,out+"/cases.csv");csvWrite(settings,out+"/settings.csv");csvWrite(fric,out+"/directional_friction.csv");
 mputl("t_s,x_true_m,v_true_mps,theta_true_rad,omega_true_radps,force_pair_N,pitch_CF_rad,gyro_radps,acc_angle_rad,pwm_mean,v_encoder_mps,x_encoder_filtered_m,legacy_theta_target,pwm_L,pwm_R,yaw_integral_Nm,yaw_true_rad,yaw_rate_true_radps,encoder_RminusL_m,pwm_half_difference,legacy_heading_limit,force_L_N,force_R_N,obs_x_m,obs_v_mps,obs_theta_rad,obs_omega_radps,obs_force_N,drive_mean",out+"/trajectory_columns.csv");
 mprintf("CATALOG CAMPAIGN limits: transient travel<=15cm, late pitch RMS<=2deg, speed RMS<=0.05m/s, yaw end<=5deg, recovery<=5s.\n");
 files=["run.sce";"observer.sci";"motor.sci";"launch.sce";"pair_catalog.c";"pair_catalog_v2.c";"control_native.c";"phase1_catalog_control.h";"phase1_catalog_params.h"];
 for f=files';if isfile(simulation_root+"current/"+f) then copyfile(simulation_root+"current/"+f,out+"/inputs/"+f);end;end
 copyfile(simulation_root+"current/control.sci",out+"/inputs/control.sci");
 copyfile(simulation_root+"config/motor_pair_r8_20261011.sce",out+"/inputs/motor_pair.sce");mputl(out,report+motor_stage+"_path.txt");summary=[];
 copyfile(simulation_root+"model/robot_can_bang_pid_1.sce",out+"/inputs/model.sce");copyfile(simulation_root+"config/vehicle_baseline.sce",out+"/inputs/vehicle_baseline.sce");
 copyfile(simulation_root+"config/motor_supply.sce",out+"/inputs/motor_supply.sce");
 for current_file=["motion_native.c" "study_model.sci" "contact.sci" "pair_contact.c"]
  copyfile(simulation_root+"current/"+current_file,out+"/inputs/SPEED2_"+current_file);
 end
 exec(simulation_root+"current/contact.sci",-1);
 exec(simulation_root+"current/study_model.sci",-1);
 exec(simulation_root+"current/observer_motion.sci",-1);
 for ip=1:size(settings,1)
  z=settings(ip,:);
  if z(10)==1 then cfg=observer_design(nominal,track,Iz,z(1:4),z(5),z(6),z(7),z(8),z(9));
   if size(z,"*")>=11 then cfg.blend=z(11);end
   csvWrite(cfg.K5,out+msprintf("/gain%d.csv",ip));csvWrite(cfg.Aobs,out+msprintf("/observer_A%d.csv",ip));csvWrite(cfg.Bobs,out+msprintf("/observer_B%d.csv",ip));csvWrite(cfg.Hobs,out+msprintf("/observer_H%d.csv",ip));csvWrite(cfg.Lobs,out+msprintf("/observer_L%d.csv",ip));
  else
   cfg=literature_design(nominal,track,Iz,z(1:4),z(5),.998,.03,1,1);
   cfg.yawKp=Iz*64;cfg.yawKd=Iz*12.8;cfg.yawKi=Iz*10;cfg.yawCap=.08;cfg.yawIcap=.04;
  end
  if size(z,"*")>=12 then
   cfg.frictionLR=cfg.friction*[fric(1) fric(3);fric(2) fric(4)];
   cfg.startFrictionLR=cfg.frictionLR+z(12)*(nominal.Tstat/(2*nominal.r)*[160 200;200 140]/175-cfg.frictionLR);
   if size(z,"*")>=13 then cfg.frictionLR=cfg.frictionLR*z(13);end
   if size(z,"*")>=14 then if z(14)>0 then cfg.feedback5=%t;end;end
  end
  if isdef("motor_native") then
   if motor_native then
    if norm(z-[500 1 150 .1 5 .006 3 .01 .5 1 .25 1],1)>1e-9 then error("Native coefficients require exact selected profile");end
    if ~c_link("twip_catalog_control") then link(simulation_root+"current/control_native.dll","twip_catalog_control","c");end
    cfg.native=%t;
    if isdef("motor_verify_native") then cfg.verify_native=%t;end
   end
  end
  csvWrite(cfg.K,out+msprintf("/control_K%d.csv",ip));
  if isfield(cfg,"observer") then
   csvWrite([cfg.track cfg.r cfg.fs cfg.voltage cfg.forceCap cfg.yawKp cfg.yawKd cfg.yawKi cfg.yawIcap cfg.yawCap cfg.angle_gain cfg.blend],out+msprintf("/allocation%d.csv",ip));
   csvWrite(cfg.vnl,out+msprintf("/vnl%d.csv",ip));csvWrite(cfg.frictionLR,out+msprintf("/friction%d.csv",ip));csvWrite(cfg.startFrictionLR,out+msprintf("/start_friction%d.csv",ip));
  end
  for ic=ids
   cc=cases(ic,:);[pp,cat]=catalog_motor(P,pair_no_load_curve,cc(14),cc(17));
   [Pc,Sc,eq,obs]=geometry_uncertainty(pp,S,cc(20)*1000,cc(13));Pc.Fstall=Pc.Fstall*cc(15);Pc.Fmax=Pc.Fmax*cc(15);Pc.Tkin=Pc.Tkin*cc(16);Pc.Tstat=Pc.Tstat*cc(16);Pc.zpush=.1325;
   Sc.tend=duration;Sc.nsub=substeps;Sc.sensor_delay_ticks=cc(7);Sc.imu_odr=99.1244;Sc.gyro_adc_bias=-.186220;Sc.gyro_bias=cc(18);Sc.trim=0;
   Sc.th0=(cc(19)+cc(13))/Sc.angle_model(1)*deg;Sc.pitch_est0=cc(19)*deg;Sc.first_armed_zero=%t;
   Sc.integral_start=3;Sc.integral_limit=2;Sc.integral_gate_angle=5;Sc.integral_gate_speed=.15;
   Sc.trial_interlocks=%t;Sc.audit_control=%t;Sc.audit_motor=%t;Sc.audit_observer=%t;Sc.imu_alpha=.998;
   Sc.pulses=[];if cc(11)<>0 then Sc.pulses=8;end;Sc.Fp=cc(11);Sc.wp=cc(12);
   Sc.coupled_pair=[track Iz .0001 cc(1:6) cc(8:10)];Sc.allow_unvalidated_pair_diagnostic=%t;Sc.catalog_friction=fric;Sc.catalog_start_friction=[160 200 200 140]/175;
   Sc.heading_policy=0;if z(10)>=0 then Sc.research_control=cfg;else Sc.imu_alpha=.98;end
   if z(10)==0 then Sc.imu_alpha=cfg.alpha;end
   if isdef("motor_override") then [Pc,Sc]=motor_override(Pc,Sc,ic);end
   if size(Sc.pulses,"*")>0 then csvWrite(Sc.pulses,out+msprintf("/pulses_case%02d.csv",ic));end
   seed=1100+ic;if isdef("motor_seed_offset") then seed=seed+motor_seed_offset;end
   Sc=nhieu_co_dinh(Sc,seed);[T,Y,F,U,E]=mo_phong(Pc,[48 3.6 32 .5 .3],Sc,kf);
   summary=[summary;ic ip T($) duration max(abs(Y(3,:)/deg)) max(abs(E(6,:))) max(abs(E(5,:))) E(11,$)/deg max(abs(E(11,:)/deg))];
   csvWrite([T' Y' E' U'],out+msprintf("/case%02d_policy%d.csv",ic,ip));csvWrite(summary,out+"/summary.csv");
   mprintf("Catalog %s case%d profile%d %.3f/%.0fs pitch %.2f yaw %.2f x %.2fcm\n",motor_stage,ic,ip,T($),duration,max(abs(Y(3,:)/deg)),E(11,$)/deg,100*max(abs(E(6,:))));
  end
 end
 mputl("status=completed",out+"/run_status.txt");
catch
 msg=lasterror();mputl(["status=failed";msg],out+"/run_status.txt");mprintf("FAILED %s\n",msg);diary(0);if motor_cli then quit;else error(msg);end
end
diary(0);if motor_cli then quit;end
