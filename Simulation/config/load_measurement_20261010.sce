// Reproducible inputs from analyze_measurement_plan.py. No firmware is changed.
// IMU offsets/six-face alignment are NOT accepted: nominal is a conditional
// post-calibration model; residual offsets are explicitly stressed below.
measurement_root=simulation_root+"data/processed/measurement_20261010/";
ten_log="20261010_plan56";
DH=csvRead(measurement_root+"static_initial.csv",",");
ih=(1:size(DH,1))';
imu_static_pitch=atan(DH(:,13),DH(:,15))/deg;
imu_static_gyro=DH(:,11)/16.4;
hw_bias_y=mean(imu_static_gyro);
imu_bias_residual=0; // ASSUMES applying the measured static bias in firmware later.
imu_noise_pitch=stdev(imu_static_pitch); imu_noise_gyro=stdev(imu_static_gyro);
DW=csvRead(measurement_root+"static_warm.csv",",");
imu_post_pitch=atan(DW(:,13),DW(:,15))/deg; imu_post_gyro=DW(:,11)/16.4;
MC=csvRead(measurement_root+"motor_curves.csv",",");
motor_L_pos=MC(1,3:4)'; motor_L_neg=MC(2,3:4)';
motor_R_pos=MC(3,3:4)'; motor_R_neg=MC(4,3:4)';
drive_slopes=[motor_L_pos(1) motor_L_neg(1);motor_R_pos(1) motor_R_neg(1)];
speed_slope=4/sum(1 ./drive_slopes);
ratio_L_pos=(motor_L_pos(1)*1000+motor_L_pos(2))/(motor_R_pos(1)*1000+motor_R_pos(2));
ratio_L_neg=(motor_L_neg(1)*1000+motor_L_neg(2))/(motor_R_neg(1)*1000+motor_R_neg(2));
DM=csvRead(measurement_root+"motor_fits.csv",",");
measurement_ppr=csvRead(measurement_root+"encoder_ppr.csv",",");
measurement_db=csvRead(measurement_root+"deadband.csv",",");
measurement_angle=csvRead(measurement_root+"imu_angle_model.csv",",");
faces_retry_root=simulation_root+"data/processed/measurement_20261010_faces_retry/";
co_faces_retry=isfile(faces_retry_root+"analysis.json");
if co_faces_retry then
 measurement_accel_candidate=csvRead(faces_retry_root+"calibration_candidate.csv",",");
 measurement_angle_candidate=csvRead(faces_retry_root+"angle_model_candidate.csv",",");
 ten_log="20261010_plan56_faces_retry";
end
measurement_running_pwm=mean(-MC(:,4)./MC(:,3));
co_log_tinh=%t;co_motor_bench=%t;
// All56 completed; two one-frame transport gaps. Initial-static and all motor
// records are complete. Exact six-face calibration is still rejected.
log_day_du=%f;motor_da_nhan_dang=%t;plant_da_xac_thuc=%f;
// Old operator direction confirmation + new integrated gyro/acc correlation .99838.
dau_da_xac_thuc=motor_sign_da_xac_thuc & imu_acc_sign_thuc==-1;
calibration_da_xac_thuc=%f;
mprintf("\n=== NEW 56-JOB DATASET 10/10: 271184 rows, 2 isolated transport gaps ===\n");
mprintf("Static biasY=%.6fdps; static pitch noise=%.6fdeg gyroY=%.6fdps\n",hw_bias_y,imu_noise_pitch,imu_noise_gyro);
mprintf("Warm bias change=%.6fdps; integrated gyro vs -raw pitch corr=.998378\n",mean(imu_post_gyro)-hw_bias_y);
mprintf("Operator confirmed TEN turns: PPR L=%.3f R=%.3f; old937.2 is tested separately.\n",measurement_ppr);
if co_faces_retry then
 mprintf("Repeated jobs2..7: all30006 rows received; notStill warnings on3..6.\n");
 mprintf("Module +Y marking -> negative raw Y per human; register sign preserved, POSITIVE calibration scale.\n");
 mprintf("Provisional accel bias[g] %.9f %.9f %.9f; sensitivity %.9f %.9f %.9f\n",measurement_accel_candidate);
 mprintf("Full calibration acceptance FALSE; off-axis residual up to .053724g.\n");
 copyfile(faces_retry_root+"analysis.json",out_dir+"/inputs/faces_retry_analysis.json");
 copyfile(faces_retry_root+"calibration_candidate.csv",out_dir+"/inputs/calibration_candidate.csv");
 copyfile(faces_retry_root+"angle_model_candidate.csv",out_dir+"/inputs/angle_model_candidate.csv");
else
 mprintf("Six-face +/-Y labels reversed; cross-axis residual up to .057g; calibration NOT accepted.\n");
end
mprintf("Jobs8..12: independent ruler angles, fixed BEFORE capture (latest operator correction).\n");
mprintf("Local RAW angle = %.9f*reference %+.6fdeg; max fit residual %.6fdeg.\n",measurement_angle(1:3));
if co_faces_retry then
 mprintf("Raw-angle profile retained for comparison; new XYZ candidate is CONDITIONAL only.\n");
else
 mprintf("Provisional installed-angle error model only; XYZ calibration pending new jobs2..7.\n");
end
mprintf("Reference ruler accuracy not recorded; step56 had NO added load.\n");
mprintf("PWM DB medians L(+/-)=%g/%g R(+/-)=%g/%g; no-load speed tau %.1f..%.1fms NOT torque tau.\n", ..
 measurement_db(1,1),measurement_db(1,2),measurement_db(2,1),measurement_db(2,2),min(DM(:,6))*1000,max(DM(:,6))*1000);
for mi=1:4
 mprintf("wheel%g sign%+g: v=%.9f*absPWM%+.6f m/s (conditional PPR)\n",MC(mi,:));
end
copyfile(measurement_root+"analysis.json",out_dir+"/inputs/measurement_analysis.json");
copyfile(measurement_root+"inventory.csv",out_dir+"/inputs/measurement_inventory.csv");
copyfile(measurement_root+"operator_confirmations.json",out_dir+"/inputs/operator_confirmations.json");
copyfile(measurement_root+"imu_angle_model.csv",out_dir+"/inputs/imu_angle_model.csv");
copyfile(simulation_root+"config/load_measurement_20261010.sce",out_dir+"/inputs/measurement_loader.sce");
