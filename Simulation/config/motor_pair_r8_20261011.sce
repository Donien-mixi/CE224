// Fitted to R7 W01-W09 NO-LOAD encoder slopes; W10-W12 (300 PWM) held out.
// Row order L+,L-,R+,R-. v_abs = gain*abs(PWM)+intercept, fit200..600.
// Running intercept is NOT the static startup threshold or a torque measurement.
pair_no_load_curve=[.0004171349529655699,-.03302857486293187; ..
 .00040363461813956855,-.03413098477623709; ..
 .0004066591092192742,-.028490306044233363; ..
 .00042171545397395264,-.02711270473115662];
pair_no_load_fit_PWM=[200 600];
pair_loaded_torque_identified=%f;
pair_feedforward_positive=[23.220000077847235,-.01271658079024256];
pair_feedforward_negative=[17.772434415517104,-.021906868909677874];
// Conservative HALF of fitted no-load compensation. Floor gains remain uncertain.
pair_feedforward_weight=.5;
// kp,kd,beta,totalCap,weight,A+,B+,A-,B-,slewPWM/5ms
pair_controller=[500 40 .95 24 pair_feedforward_weight pair_feedforward_positive pair_feedforward_negative 2];
