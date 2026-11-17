// Row order L+,L-,R+,R-. v_abs = gain*abs(PWM)+intercept.
// RT35 (17/11/2026): refitted from the on-car log twip_balance_R12 (1)(5).txt
// (8 652 samples, build RT34-CORNER/NET35, pack 11.7 V, chi doan chay thang
// giu lenh, |v|>0.35, PWM>150). Banh trai da tru bu 110 PWM cua RT32 de ra
// dung PWM cua LUAT R12. So voi duong cu (R7 W01-W09), chenh lech 1-2 %:
//   row   gain(log)  icpt(log)   v@2499   v@2499 (cu)
//   L+    .000417    -.0332      1.010    1.010
//   L-    .000396    -.0279      0.961    0.975
//   R+    .000394    -.0187      0.965    0.988
//   R-    .000432    -.0593      1.019    1.027
// Ghi chu: intercept la mat do/tai, khong phai nguong khoi dong tinh.
pair_no_load_curve=[.000417,-.0332; ..
 .000396,-.0279; ..
 .000394,-.0187; ..
 .000432,-.0593];
pair_no_load_fit_PWM=[200 600];
pair_loaded_torque_identified=%f;
pair_feedforward_positive=[23.220000077847235,-.01271658079024256];
pair_feedforward_negative=[17.772434415517104,-.021906868909677874];
// Conservative HALF of fitted no-load compensation. Floor gains remain uncertain.
pair_feedforward_weight=.5;
// kp,kd,beta,totalCap,weight,A+,B+,A-,B-,slewPWM/5ms
pair_controller=[500 40 .95 24 pair_feedforward_weight pair_feedforward_positive pair_feedforward_negative 2];
