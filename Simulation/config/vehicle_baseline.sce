// Parameter snapshot for sharing source without the private measurement campaign.
// Derived from the project's 2026-10-10 measurements. Not a new calibration.
// Loaded force/friction/torque response and yaw remain unvalidated.
co_log=%f;co_log_tinh=%f;log_gyro_std=0;log_day_du=%f;
co_measurement_plan=%t;co_faces_retry=%t;co_motor_bench=%t;
ten_log="shared_baseline";calibration_da_xac_thuc=%f;plant_da_xac_thuc=%f;
motor_da_nhan_dang=%f;dau_da_xac_thuc=%t;
motor_sign_thuc=1;motor_sign_da_xac_thuc=%t;imu_acc_sign_thuc=-1;
imu_noise_pitch=.060690;imu_noise_gyro=.052528;imu_bias_residual=0;
hw_bias_y=-.179655;
measurement_ppr=[898.450;898.000];
measurement_db=[160 200;200 140];
motor_L_pos=[.000406707172819;-.0336634276769];
motor_L_neg=[.000398557111854;-.029823473919];
motor_R_pos=[.000398520938237;-.0261010806635];
motor_R_neg=[.000427798698252;-.032767136849];
drive_slopes=[motor_L_pos(1),motor_L_neg(1);motor_R_pos(1),motor_R_neg(1)];
speed_slope=4/sum(1 ./drive_slopes);
ratio_L_pos=(motor_L_pos(1)*1000+motor_L_pos(2))/(motor_R_pos(1)*1000+motor_R_pos(2));
ratio_L_neg=(motor_L_neg(1)*1000+motor_L_neg(2))/(motor_R_neg(1)*1000+motor_R_neg(2));
measurement_running_pwm=mean([-motor_L_pos(2)/motor_L_pos(1);-motor_L_neg(2)/motor_L_neg(1); ..
 -motor_R_pos(2)/motor_R_pos(1);-motor_R_neg(2)/motor_R_neg(1)]);
measurement_accel_candidate=[-.0416836098593,-.105884356548,.004738195334, ..
 .997272837552,1.00758977736,1.00929388014];
measurement_angle_candidate=[1.03912069574,2.16694264915,.529811535941,-10,10,1];
measurement_angle=measurement_angle_candidate;
mprintf("Shared parameter snapshot loaded. Gaussian noise approximates static variation; no raw replay.\n");
mprintf("Signs confirmed by operator. Loaded plant and complete calibration NOT independently validated.\n");
