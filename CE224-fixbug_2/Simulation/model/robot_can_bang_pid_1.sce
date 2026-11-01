// =====================================================================
// ROBOT HAI BANH CAN BANG - KHUNG THEO "thông số chi tiết linh kiện.txt"
// Cap nhat kich thuoc/toa do trong "thông số cần thiết 1.txt" (09/10/2026).
// Execute trong SciNotes. x: tien/lui 80mm; y: trai/phai 200mm; z: len.
// Goc tai truc banh; theta>0 nga ve +x. Mo hinh pitch phang, than cung,
// lan khong truot; mac dinh pitch, co nhanh yaw hai banh tuy chon; chua co roll.
// Khoi luong do 888g; +94g o hai motor theo user; PWM bu theo profile.
// Latest: 56 jobs10/10; per-wheel encoder, four no-load speed curves and noise.
// Jobs8..12 ruler references confirmed; local installed-angle error modeled.
// Repeated jobs2..7: provisional positive XYZ scale/bias; motion flags remain.
// Conditional calibrated profile is tested, not approved; loaded torque unverified.
// 12/10: optional separate-wheel pitch/yaw diagnostic in pair_coupled.sci.
// Requires explicit acknowledgement; force/friction/slip are NOT identified.
// A4950 tren breadboard tang2; cao do mica DO SUA 08/10 uu tien hon cot danh nghia55mm.
// Khong tu tang torque/giam tau de lam PID dat; khong in ket luan dat gia.
// =====================================================================
funcprot(0); // Giu console de doi chieu cac lan tuning.
if ~isdef("export_png") then export_png=%t; end
if ~isdef("animate") then animate=%f; end
if ~isdef("do_plot") then do_plot=%t; end
simulation_root=get_absolute_file_path("robot_can_bang_pid_1.sce")+"../";
if isdef("simulation_output_override") then
    out_dir=simulation_output_override; clear simulation_output_override;
else
    exec(simulation_root+"scripts/new_run.sci",-1);
    out_dir=simulation_new_run(simulation_root,"direct_model");
end
// chi_khai_bao cho phep kiem tra khai bao/ham, khong quet PID/ve do thi.
if ~isdef("chi_khai_bao") then chi_khai_bao=%f; end
// Dat lai khi Execute lan sau; khong de co kiem tra cu lam bo qua mo phong.
kiem_tra_khai_bao=chi_khai_bao; chi_khai_bao=%f;
deg=%pi/180; // rad tren mot do; rad -> do: chia deg
// Log closed-loop 20 Hz: filtered angles/speeds, NOT hardware identification.
// Team entry point uses a parameter snapshot, never requires private test logs.
team_mode=%f;if isdef("simulation_team_mode") then team_mode=simulation_team_mode;end
if team_mode then
    exec(simulation_root+"config/vehicle_baseline.sce",-1);
else
log_file=simulation_root+"data/processed/twip_20261007_numeric.csv";
co_log=isfile(log_file); log_gyro_std=0;
if co_log then
    DL=csvRead(log_file,",");
    log_t=(DL(:,1)-DL(1,1))/1000;
    log_dt=diff(log_t);
    log_x=[0;cumsum((DL(2:$,4)+DL(1:$-1,4)).*log_dt/2)];
    log_gyro_std=stdev(DL(:,3));
    log_flip=sum(DL(2:$,8).*DL(1:$-1,8)<0);
    mprintf("\n=== DOI CHIEU LOG 07/10: %d mau, %.3fs ===\n",size(DL,1),log_t($));
    mprintf("Pitch FILTERED: mean %.4fdo, std %.4fdo; gyro std %.4fdps\n",mean(DL(:,2)),stdev(DL(:,2)),log_gyro_std);
    mprintf("Tich phan v da loc: xpk %.3fcm, xcuoi %.3fcm; PWM doi dau %d lan\n",max(abs(log_x))*100,log_x($)*100,log_flip);
    mprintf("KHONG coi gyro std la nhieu tinh; KHONG fit torque/tau/dau IMU tu log closed-loop 20Hz.\n");
    mprintf("V lam tron 0.01m/s: sai so tich phan worst-case +/-%.3fm. Vi tri that CHUA XAC NHAN.\n",.005*log_t($));
end
// Prefer latest capture; keep the previous capture only as a fallback.
data_root=simulation_root+"data/processed/";
raw_root=simulation_root+"data/raw/";
hardware_file=data_root+"hardware_20261008_numeric.csv";
hardware_raw=raw_root+"twip_hardware_test_20261008.txt";
motor_file=data_root+"hardware_20261008_run4_motor.csv";
co_motor_bench=%f; log_day_du=%f; ten_log="20261008";
if isfile(data_root+"hardware_20261008_run4_numeric.csv") then
    hardware_file=data_root+"hardware_20261008_run4_numeric.csv";
    hardware_raw=raw_root+"twip_hardware_test_20261008_run4.txt";
    ten_log="20261008_run4";
end
if isfile(data_root+"hardware_20261008_run5_numeric.csv") then
    hardware_file=data_root+"hardware_20261008_run5_numeric.csv";
    hardware_raw=raw_root+"twip_hardware_test_20261008_run5.txt";
    motor_file=data_root+"hardware_20261008_run5_motor.csv";
    ten_log="20261008_run5";
end
if isfile(data_root+"hardware_20261008_run7_numeric.csv") then
    hardware_file=data_root+"hardware_20261008_run7_numeric.csv";
    hardware_raw=raw_root+"twip_hardware_test_20261008_run7.txt";
    motor_file=data_root+"hardware_20261008_run7_motor.csv";
    ten_log="20261008_run7";
end
co_log_tinh=isfile(hardware_file);
co_measurement_plan=isfile(data_root+"measurement_20261010/analysis.json");
calibration_da_xac_thuc=%f;
imu_noise_pitch=.05; imu_noise_gyro=.30; imu_bias_residual=0;
motor_da_nhan_dang=%f;
plant_da_xac_thuc=%f; dau_da_xac_thuc=%f;
// User confirmed PWM+ -> physical forward toward battery WAGO, 08/10.
motor_sign_thuc=1; motor_sign_da_xac_thuc=%t;
// User confirmed raw pitch decreases on forward tilt, 08/10.
// Run7 verifies gyroY = -d(raw accel pitch)/dt in slow body tilts.
// Loaded torque/tau still require independent identification.
if ~isdef("imu_acc_sign_thuc") then imu_acc_sign_thuc=-1; end
if co_log_tinh & ~co_measurement_plan then
    DH=csvRead(hardware_file,",");
    // Run4 has only post-motor static stage10; never relabel it as pre-test.
    ih=find(DH(:,4)==1 & DH(:,5)==1);
    if size(ih,"*")<100 then ih=find(DH(:,4)==10 & DH(:,5)==1); end
    if size(ih,"*")>=100 then
        imu_static_pitch=atan(DH(ih,13),DH(ih,15))/deg;
        imu_static_gyro=DH(ih,11)/16.4;
        imu_noise_pitch=stdev(imu_static_pitch);
        imu_noise_gyro=stdev(imu_static_gyro);
        // Read calibration bias from the exact raw capture, never assume it is universal.
        hw_header=mgetl(hardware_raw);
        hm=grep(hw_header,"$TINFO,");
        if size(hm,"*")==0 then error("Hardware log missing TINFO calibration"); end
        hfields=strsplit(hw_header(hm(1)),",");
        hw_bias_y=evstr(hfields(14));
        imu_bias_residual=mean(imu_static_gyro)-hw_bias_y;
        mprintf("\n=== BMI160 TINH DA DO: ID%d, %d LUOT DOC / ODR100Hz ===\n",DH(ih(1),4),size(ih,"*"));
        mprintf("Std accel-pitch %.6fdo; gyroY %.6fdps; residual bias %.6fdps\n",imu_noise_pitch,imu_noise_gyro,imu_bias_residual);
        mprintf("Mean accel-pitch %.6fdo KHONG dung lam trim/CoM; sensor co the bi lech gan.\n",mean(imu_static_pitch));
        mprintf("Mau lap100Hz KHONG la mau doc lap200Hz; std tinh la noise+rung tren gia ke.\n");
    else co_log_tinh=%f;
    end
    missing_internal=sum(max(diff(DH(:,1))-1,0));
    log_day_du=DH(1,1)==0 & missing_internal==0 & and(DH(:,5)==1) & max(DH(:,16))==0;
    for sid=0:11
        log_day_du=log_day_du & sum(DH(:,4)==sid)>0;
    end
    mprintf("Log %s: %.3f..%.3fs; thieu dau %d mau, thieu giua %d; PWM max %.0f.\n", ..
        ten_log,DH(1,2)/1000,DH($,2)/1000,DH(1,1),missing_internal,max(abs(DH(:,6:7))));
    if isfile(motor_file) then
        DM=csvRead(motor_file,","); co_motor_bench=%t;
        // Plateau tail slopes are independent of transient fit. R has multi-level data.
        irp=find(DM(:,2)==2 & DM(:,3)>0 & DM(:,1)==8);
        irn=find(DM(:,2)==2 & DM(:,3)<0 & DM(:,1)==8);
        motor_R_pos=[DM(irp,3),ones(size(irp,"*"),1)]\abs(DM(irp,8));
        motor_R_neg=[abs(DM(irn,3)),ones(size(irn,"*"),1)]\abs(DM(irn,8));
        ibp=find(DM(:,1)==9 & DM(:,3)>0); ibn=find(DM(:,1)==9 & DM(:,3)<0);
        // L has only +/-1000; scaling the R curve for L remains a MODEL ASSUMPTION.
        ratio_L_pos=abs(DM(ibp(1),8)/DM(ibp(2),8));
        ratio_L_neg=abs(DM(ibn(1),8)/DM(ibn(2),8));
        motor_L_pos=ratio_L_pos*motor_R_pos; motor_L_neg=ratio_L_neg*motor_R_neg;
        if sum(DH(:,4)==7)>0 then
            ilp=find(DM(:,2)==1 & DM(:,3)>0 & DM(:,1)==7);
            iln=find(DM(:,2)==1 & DM(:,3)<0 & DM(:,1)==7);
            motor_L_pos=[DM(ilp,3),ones(size(ilp,"*"),1)]\abs(DM(ilp,8));
            motor_L_neg=[abs(DM(iln,3)),ones(size(iln,"*"),1)]\abs(DM(iln,8));
            motor_da_nhan_dang=log_day_du & size(ilp,"*")==4 & size(iln,"*")==4 & ..
                size(irp,"*")==4 & size(irn,"*")==4;
            mprintf("L no-load: +PWM v=%.8f*pwm%+.6f; -PWM |v|=%.8f*|pwm|%+.6f m/s\n", ..
                motor_L_pos(1),motor_L_pos(2),motor_L_neg(1),motor_L_neg(2));
        end
        speed_slope=((1+ratio_L_pos)*motor_R_pos(1)+(1+ratio_L_neg)*motor_R_neg(1))/4;
        // Equal hypothetical stall forces => harmonic mean back-EMF speed.
        drive_slopes=[motor_L_pos(1) motor_L_neg(1);motor_R_pos(1) motor_R_neg(1)];
        speed_slope=4/sum(1 ./drive_slopes);
        mprintf("R no-load: +PWM v=%.8f*pwm%+.6f; -PWM |v|=%.8f*|pwm|%+.6f m/s\n", ..
            motor_R_pos(1),motor_R_pos(2),motor_R_neg(1),motor_R_neg(2));
        mprintf("L/R tai1000: + %.5f, - %.5f; slope hieu dung %.8f m/s/PWM.\n", ..
            ratio_L_pos,ratio_L_neg,speed_slope);
        good=find(DM(:,7)==1);
        mprintf("Tau SPEED banh tu do %.1f..%.1fms; KHONG gan vao tau TORQUE cua plant.\n", ..
            min(DM(good,10))*1000,max(DM(good,10))*1000);
    end
    tilt_file=data_root+"imu_tilt_run7_verification.csv";
    if ten_log=="20261008_run7" & isfile(tilt_file) then
        VT=csvRead(tilt_file,",");
        // Verified sign only: do not use hand-motion regression as a gain calibration.
        dau_da_xac_thuc=motor_sign_da_xac_thuc & imu_acc_sign_thuc==-1 & ..
            log_day_du & VT(1)==1 & VT(2)>=50 & VT(3)>.85 & VT(5)>=29 & VT(6)==0;
        mprintf("IMU nghieng run7: %d cua so, corr(gyroY,-dPitch/dt)=%.6f; DAU DAT=%d\n", ..
            VT(2),VT(3),bool2s(dau_da_xac_thuc));
        mprintf("Khong scale gyro theo gain fit %.6f: gia toc tay va phep chieu truc gay sai lech.\n",VT(4));
    end
    ipost=find(DH(:,4)==10 & DH(:,5)==1);
    if size(ipost,"*")>=100 then
        imu_post_pitch=atan(DH(ipost,13),DH(ipost,15))/deg;
        imu_post_gyro=DH(ipost,11)/16.4;
        mprintf("IMU sau motor: pitch std %.6fdo; gyroY std %.6fdps; residual %.6fdps\n", ..
            stdev(imu_post_pitch),stdev(imu_post_gyro),mean(imu_post_gyro)-hw_bias_y);
    end
end
if co_measurement_plan then exec(simulation_root+"config/load_measurement_20261010.sce",-1); end
end // private measurement loader / portable parameter snapshot
function I=I_hop(m,dx,dz)
    I=m*(dx^2+dz^2)/12;
endfunction

// 1. KHUNG / TOA DO TAM / KHOI LUONG
r=0.065/2; khung_x=0.080; khung_y=0.200; day_mica=0.0016;
m_tong_do=0.888;
// User attributes the 94g difference to heavier GA25 motor assemblies below floor1.
// Split equally for the symmetric nominal model; individual motor weights unmeasured.
m_motor_pair=0.200+(m_tong_do-0.794); m_motor_each=m_motor_pair/2;
// User confirmed wheel diameter 65mm: keep rolling radius 32.5mm.
// Axle floor height is an independent measured reference for frame elevations.
cao_truc_dat=0.0325;
// Latest corrected measurements (08/10): top surfaces22.5/77.5/132.5mm above axle.
// Both top-to-top spacings are55mm, consistent with the installed pillars.
z_tren=[0.0225 0.0775 0.1325]; z_san=z_tren-day_mica/2; z_duoi=z_tren-day_mica;
z_imu_do=0.0895; // Measured CHIP CENTER above axle; not height above the ground.
cao_san1_dat=cao_truc_dat+z_tren(1); cach_tang=diff(z_tren);
// GIA DINH kich thuoc [dx,dy,dz] (m), can thay bang so do.
// Pin GIUA san 1; buck KE BEN theo y, khong tu tao lech trong tam x.
// Battery 110x40x25mm: long side along the 200mm transverse frame dimension,
// retaining its centered placement on floor1, as in the accepted layout.
// Inferred 09/10: rotate XL4015 90deg in plane to fit mica at measured y=85mm.
kt_pin=[0.040 0.110 0.025]; kt_buck=[0.052 0.025 0.012]; y_buck=0.085;
kt_board=[0.055 0.165 0.010];
kt_stm=[0.020 0.050 0.010]; kt_esp=[0.025 0.060 0.010];
kt_driver=[0.020 0.025 0.010]; kt_imu=[0.015 0.015 0.004];
kt_wago=[0.030 0.050 0.012]; kt_day=[0.055 0.165 0.006];
kt_motor=[0.025 0.050 0.025]; // GA25 tru nam theo y, duong kinh 25mm
R_cot=0.003; x_cot=0.035; y_cot=0.090;
// Inferred, not remeasured: 23.3mm is mounting bottom (0.8mm above mica).
z_day_pin_buck=0.0233;
z_pin=z_day_pin_buck+kt_pin(3)/2; z_buck=z_day_pin_buck+kt_buck(3)/2;
// Breadboard rests on mica2; boards rest on its top. Keep measured IMU chip z.
z_board=z_tren(2)+kt_board(3)/2; z_board_top=z_board+kt_board(3)/2;
z_mach=z_board_top+kt_stm(3)/2;
y_esp=0.049; // Inferred +4mm shift: 1.5mm clearance to BMI160, still on breadboard.
z_cot2=mean(z_san(2:3)); // 104.2mm, consistent nominal frame reference.
x_wago=0.030; z_wago=0.0684;
cao_ga=z_duoi(1)-kt_motor(3)/2;
kt_ga=[0.030 0.012 cao_ga]; z_ga=(z_duoi(1)+kt_motor(3)/2)/2;
y_motor=(khung_y-kt_motor(2))/2; y_ga=0.0885;
day_banh=0.026; R_khop=0.006; dai_khop=0.018;
y_banh=khung_y/2+day_banh/2+dai_khop;
// lk = [m(kg), x,y,z(m), dx,dy,dz(m), loai].
// 1 mica; 2 hop/mach; 3 motor tru nam y; 4 cot tru dung.
// 48g gom 8 cot trong tai lieu; KHONG tu them khoi luong cot bat ga.
// Cot ve theo khoang cach mica DO MOI, tam giua hai mat tham chieu mica.
// Neu dung mat giua mica, ghep cot co sai so 1.6mm, khong phai so do CAD.
ten=["Mica 1";"Mica 2";"Mica 3";"Pin LiPo 3S";"Buck XL4015"; ..
     "Breadboard";"STM32";"ESP32";"Dual A4950";"BMI160"; ..
     "WAGO truoc";"WAGO sau";"Day tren breadboard"; ..
     "GA25 trai";"GA25 phai";"Ga trai";"Ga phai"; ..
     "Cot T1 truoc trai";"Cot T1 truoc phai";"Cot T1 sau trai";"Cot T1 sau phai"; ..
     "Cot T2 truoc trai";"Cot T2 truoc phai";"Cot T2 sau trai";"Cot T2 sau phai"];
lk=[ ..
  0.095/3,0,0,z_san(1),khung_x,khung_y,day_mica,1;
  0.095/3,0,0,z_san(2),khung_x,khung_y,day_mica,1;
  0.095/3,0,0,z_san(3),khung_x,khung_y,day_mica,1;
  0.190,0,0,z_pin,kt_pin,2;
  0.015,0,y_buck,z_buck,kt_buck,2;
  0.077,0,0,z_board,kt_board,2;
  0.007,0,-0.055,z_mach,kt_stm,2;
  0.010,0,y_esp,z_mach,kt_esp,2;
  0.003,0,-0.010,z_mach,kt_driver,2;
  0.002,0,0.010,z_imu_do,kt_imu,2;
  0.006,x_wago,0,z_wago,kt_wago,2;
  0.006,-x_wago,0,z_wago,kt_wago,2;
  0.015,0,0,0.0905,kt_day,2;
  m_motor_each,0,-y_motor,0,kt_motor,3;
  m_motor_each,0,y_motor,0,kt_motor,3;
  0.010,0,-y_ga,z_ga,kt_ga,2;
  0.010,0,y_ga,z_ga,kt_ga,2;
  0.006,x_cot,-y_cot,mean(z_san(1:2)),2*R_cot,2*R_cot,cach_tang(1),4;
  0.006,x_cot,y_cot,mean(z_san(1:2)),2*R_cot,2*R_cot,cach_tang(1),4;
  0.006,-x_cot,-y_cot,mean(z_san(1:2)),2*R_cot,2*R_cot,cach_tang(1),4;
  0.006,-x_cot,y_cot,mean(z_san(1:2)),2*R_cot,2*R_cot,cach_tang(1),4;
  0.006,x_cot,-y_cot,z_cot2,2*R_cot,2*R_cot,cach_tang(2),4;
  0.006,x_cot,y_cot,z_cot2,2*R_cot,2*R_cot,cach_tang(2),4;
  0.006,-x_cot,-y_cot,z_cot2,2*R_cot,2*R_cot,cach_tang(2),4;
  0.006,-x_cot,y_cot,z_cot2,2*R_cot,2*R_cot,cach_tang(2),4
];
than=zeros(size(lk,1),4);
for k=1:size(lk,1)
    mk=lk(k,1); dx=lk(k,5); dz=lk(k,7); Ik=I_hop(mk,dx,dz);
    if lk(k,8)==3 then Ik=mk*(dx/2)^2/2; end
    if lk(k,8)==4 then Ik=mk*(3*(dx/2)^2+dz^2)/12; end
    than(k,:)=[mk,lk(k,2),lk(k,4),Ik];
end
mi=than(:,1); m=sum(mi); l=sum(mi.*than(:,3))/m;
xc=sum(mi.*than(:,2))/m; yc=sum(mi.*lk(:,3))/m;
I_truc=sum(than(:,4)+mi.*(than(:,2).^2+than(:,3).^2));
I=I_truc-m*(l^2+xc^2);
m_banh=0.070/2; m_khop=0.030/2;
// Banh dia dac, khop tru dong chat: GIA DINH phan bo khoi luong, tach rieng.
Iw_banh=0.5*m_banh*r^2; Iw_khop=0.5*m_khop*R_khop^2;
Iw=Iw_banh+Iw_khop; M=2*(m_banh+m_khop); Meq=M+2*Iw/r^2;
if abs(m+M-m_tong_do)>1e-9 then error("Tong khoi luong khong khop so can thuc te."); end
if I<=0 | l<=0 then error("Trong tam/quan tinh khong hop le."); end
if kt_pin(3)>z_duoi(2)-z_tren(1) then error("Pin khong vua tang 1."); end
if min([z_pin-kt_pin(3)/2,z_buck-kt_buck(3)/2])<z_tren(1)-1e-9 then error("Pin/buck xuyen mica1.");end
if max([z_pin+kt_pin(3)/2,z_buck+kt_buck(3)/2])>z_duoi(2) then error("Pin/buck cham mica2.");end
if min(lk(7:10,4)-lk(7:10,7)/2)<z_board_top-1e-9 then error("Mach xuyen breadboard.");end
if abs(y_buck)+kt_buck(2)/2>khung_y/2 then
    mprintf("BO TRI DO MOI: XL4015 nho ra bien mica %.1fmm theo y; giu dung toa do tai lieu.\n",1000*(abs(y_buck)+kt_buck(2)/2-khung_y/2));
end
if kt_board(1)>khung_x | kt_board(2)>khung_y then error("Breadboard vuot khung."); end
if max(lk(7:10,4)+lk(7:10,7)/2)>z_duoi(3) then error("Mach vuot tang 2."); end
for k=7:10
    if abs(lk(k,2))+lk(k,5)/2>kt_board(1)/2 | abs(lk(k,3))+lk(k,6)/2>kt_board(2)/2 then
        error("Mach khong nam tron tren breadboard.");
    end
end
for i=7:9
    for j=i+1:10
        if abs(lk(i,2)-lk(j,2))<(lk(i,5)+lk(j,5))/2-1e-9 & ..
           abs(lk(i,3)-lk(j,3))<(lk(i,6)+lk(j,6))/2-1e-9 then
            error("Hai mach chong len nhau tren breadboard; can doi toa do.");
        end
    end
end

// 1b. MOTOR / MA SAT: speed slopes log day du do duoc; force/friction/torque tau GIA DINH.
// PWM bu240 cua firmware KHONG phai nguong quay120/180/120/120 trong metadata.
// GA25-370 12V/280RPM/21.3:1 theo Docs/Phan_Cung_Va_Ket_Noi.md.
// Torque toi da 2kgf.cm chua phai stall do thuc; duong torque-speed GAN DUNG.
// Chua co quan tinh rotor/hop so, dong dien/sut ap tai va truot lop.
g=9.81; bx=0.01; tau_dc=2*9.80665/100; so_dc=2; tau_m=0.020;
// Supplier sheet: rated0.4kgf.cm, maximum2kgf.cm; maximum is NOT verified stall.
tau_dinh_muc=0.4*9.80665/100; rpm_co_tai=215; I_khong_tai=.07; I_tai_max=1.8;
encoder_ppr=11*21.3*4; // TI12 quadrature x4; do not multiply the 22AB count by4 again.
if co_measurement_plan then encoder_ppr=mean(measurement_ppr); end
// Wiring confirmed: VM follows LiPo3S directly; XL4015 supplies the 5V logic rail.
// 15/10 operator reports VM=11.7V now, recharges near11.2V; full3S=12.6V.
// Historical bench VM was not measured: retain its old11.1V reference separately.
exec(simulation_root+"config/motor_supply.sce",-1);
V_nom=12.0; V_pin=motor_vm_reference; bench_voltage_reference=motor_bench_reference;
Fstall_nom=so_dc*tau_dc/r; voltage_scale=V_pin/V_nom;
Fmax=Fstall_nom*voltage_scale; kf=Fmax/2499;
v_noload=(280/60)*2*%pi*r; // tai 12V; model tu ap voltage_scale
if co_motor_bench then
    // Fit only the speed/back-EMF slope at the unknown voltage of this capture.
    // The historical capture voltage is assumed; vnl is an effective coefficient,
    // not a measured 12V RPM. Absolute torque/friction/current remain unidentified.
    v_noload=speed_slope*2499/(bench_voltage_reference/V_nom);
end
PWM_bua_L=240; PWM_bua_R=240; PWM_bua=(PWM_bua_L+PWM_bua_R)/2;
if co_measurement_plan then
    // Proposed direction-specific controller compensation; not flashed firmware.
    PWM_bua_L=mean(measurement_db(1,:));PWM_bua_R=mean(measurement_db(2,:));
    PWM_bua=mean(measurement_db);
end
// Ma sat HOP SO theo banh tuong doi THAN, khong dong nhat voi lop/dat.
T_stat=r*kf*PWM_bua; T_kin=0.85*T_stat; // suy ra, chua phai torque do
if co_measurement_plan then T_kin=r*kf*measurement_running_pwm; end
b_hopso=0.00001; w_eps=0.001; // N.m.s/rad va rad/s, GIA DINH
P=struct("m",m,"l",l,"xc",xc,"I",I,"Meq",Meq,"g",g,"r",r, ..
         "bx",bx,"bgear",b_hopso,"Tstat",T_stat,"Tkin",T_kin,"weps",w_eps, ..
         "Fstall",Fstall_nom,"Fmax",Fmax,"voltage",voltage_scale, ..
         "vnl",v_noload,"tau",tau_m,"zpush",z_san(3),"xpush",0, ..
         "ximu",lk(10,2),"zimu",lk(10,4));
P.directional=co_motor_bench; P.emf_sign=motor_sign_thuc; P.speed_scale=1;
P.solver=1; // Dissipative endpoint friction; h/2 convergence remains mandatory.
 if co_motor_bench then P.drive_vnl=drive_slopes*2499/(bench_voltage_reference/V_nom); end
exec(simulation_root+"config/load_native_kernel.sce",-1);
mprintf("Native interval accelerator=%d; equations/parameters unchanged.\n",bool2s(native_available));
theta_eq=-atan(xc/l);
mprintf("\n=== KHUNG 80x200mm / 3 MICA / 2 TANG / BANH 65mm ===\n");
mprintf("Cao truc do %.1fmm; r lan dang dung %.1fmm (khong dong nhat hai so).\n",cao_truc_dat*1000,r*1000);
mprintf("Cao do MAT TREN mica tren truc: %.1f / %.1f / %.1f mm\n",z_tren*1000);
mprintf("Cao do MAT TREN mica so voi dat: %.1f / %.1f / %.1f mm\n",(z_tren+cao_truc_dat)*1000);
mprintf("Kich thuoc/tam linh kien chua do: GIA DINH trong Buoc 1.\n");
mprintf("%-24s %7s %8s %8s %8s\n","Linh kien","m(g)","x(mm)","y(mm)","z(mm)");
for k=1:size(lk,1)
    mprintf("%-24s %7.2f %8.2f %8.2f %8.2f\n",ten(k),lk(k,1:4)*1000);
end
mprintf("Than %.6fkg; banh+khop %.6fkg; TONG %.6fkg\n",m,M,m+M);
mprintf("Motor pair %.3fg, nominal each %.3fg: phan tang khoi luong o cum motor.\n",1000*m_motor_pair,1000*m_motor_each);
mprintf("Trong tam than x=%.3f y=%.3f z=%.3fmm tren truc\n",xc*1000,yc*1000,l*1000);
mprintf("Trong tam TOAN XE (gom banh/khop tai truc): z=%.3fmm tren truc.\n",m*l/(m+M)*1000);
mprintf("I_tam=%.9f; I_truc=%.9fkg.m2; Meq=%.6fkg\n",I,I_truc,Meq);
mprintf("Motor gia dinh: pin %.1fV Fmax %.3fN vnl tai pin %.3fm/s\n",V_pin,Fmax,v_noload*voltage_scale);
if co_motor_bench then mprintf("vnl da rang buoc boi slope no-load; torque va tau_m van GIA DINH.\n"); end
mprintf("PWM BU mo phong: L=%d R=%d; Tstat GIA DINH %.5fN.m (khong phai ma sat da do)\n",PWM_bua_L,PWM_bua_R,T_stat);
mprintf("Lech y chi de doi chieu bo tri, khong mo phong roll/yaw.\n");

// 2. DONG LUC HOC: Q_motor=[Fa;-r*Fa] CO PHAN LUC LEN THAN.
function [a11,a12,a22,detM]=co_he(th,P)
    a11=P.Meq+P.m; a12=P.m*(P.l*cos(th)-P.xc*sin(th));
    a22=P.I+P.m*(P.l^2+P.xc^2); detM=a11*a22-a12^2;
endfunction
function [A,B,al,be]=mo_hinh(P)
    ell=sqrt(P.l^2+P.xc^2);
    Mm=[P.Meq+P.m,P.m*ell;P.m*ell,P.I+P.m*ell^2];
    q=[1/P.r;-1]; D=[P.bx,0;0,0]+P.bgear*(q*q');
    A=zeros(4,4); B=zeros(4,1); A(1,2)=1; A(3,4)=1;
    A([2 4],3)=Mm\[0;P.m*P.g*ell];
    A([2 4],[2 4])=-(Mm\D); B([2 4])=Mm\[1;-P.r];
    al=A(4,3); be=-B(4);
endfunction
[A,B,al,be]=mo_hinh(P);
mprintf("\nHe tuyen tinh quanh %.3fdo: al=%.4f be=%.4f (co phan luc)\n",theta_eq/deg,al,be);
function [kp,kd]=pid_goc(wn,zeta,al,be,kf)
    kp=(wn^2+al)*(%pi/180)/(be*kf); kd=2*zeta*wn*(%pi/180)/(be*kf);
endfunction
function out=pid_deadband(pwm,deadband,max_pwm)
    if pwm>1 then
        out=fix(min(pwm+deadband,max_pwm));
    elseif pwm< -1 then
        out=fix(max(pwm-deadband,-max_pwm));
    else
        out=0;
    end
endfunction
function vnl=motor_vnl(P,u,relative_v)
    vnl=P.vnl;
    if P.directional then
        electrical=u*P.emf_sign;
        if abs(u)<1e-10 then electrical=relative_v*P.emf_sign; end
        c=1; if electrical<0 then c=2; end
        // Separate measured curves for L/R, retained as a pitch-only reduction.
        vnl=P.speed_scale*2/sum(1 ./P.drive_vnl(:,c));
    end
endfunction
// Phan tron truoc ma sat kho: y=[x,v,theta,omega,Fa].
// u: duty VAT LY (-1..1) da qua tre. Fa truoc ma sat hop so.
function yd=f_robot(y,u,Fd,P)
    v=y(2); th=y(3); w=y(4); Fa=y(5); c=cos(th); s=sin(th);
    sc=P.xc*c+P.l*s; [a11,a12,a22,detM]=co_he(th,P);
    Tvis=-P.bgear*(v/P.r-w); Tshaft=P.r*Fa+Tvis;
    r1=Tshaft/P.r+Fd+P.m*sc*w^2-P.bx*v;
    r2=-Tshaft+P.m*P.g*sc+Fd*(P.zpush*c-P.xpush*s);
    // Back-EMF theo toc do BANH TUONG DOI THAN; u=0 co phanh dien.
    vnl=motor_vnl(P,u,v-P.r*w);
    Ft=P.Fstall*(P.voltage*u-(v-P.r*w)/vnl);
    Ft=max(min(Ft,P.Fmax),-P.Fmax); // torque/dong gioi han GAN DUNG
    yd=[v;(a22*r1-a12*r2)/detM;w;(a11*r2-a12*r1)/detM;(Ft-Fa)/P.tau];
endfunction
// Tich phan MIDPOINT AN: motor/back-EMF/ma sat giai DONG THOI.
// Khong tich phan motor tren toc do khong ma sat roi moi sua v/omega.
// Complementarity: torque ma sat du de wr=0, hoac truot tai Tkin.
function [yn,Fnet]=buoc_midpoint(y,u,Fd,h,P)
    v0=y(2); w0=y(4); Fa0=y(5); vn=v0; wn=w0; Fmid=Fa0;
    wr0=v0/P.r-w0;
    if abs(wr0)<=P.weps then limit=P.Tstat; else limit=P.Tkin; end
    vnl=motor_vnl(P,u,v0-P.r*w0);
    ce=P.Fstall/vnl; rho=h/(2*P.tau); Tfr=0;
    // 3 corrector cho hinh hoc / centrifugal / nhanh gioi han dong.
    for it=1:3
        thm=y(3)+h*(w0+wn)/4; wm=(w0+wn)/2;
        [m11,m12,m22,detM]=co_he(thm,P);
        sc=P.xc*cos(thm)+P.l*sin(thm);
        d11=P.bx+P.bgear/P.r^2; d12=-P.bgear/P.r; d22=P.bgear;
        raw=P.Fstall*P.voltage*u-ce*((v0+vn)/2-P.r*(w0+wn)/2);
        if abs(raw)>P.Fmax then
            af=(Fa0+rho*max(min(raw,P.Fmax),-P.Fmax))/(1+rho); bf=0;
        else
            af=(Fa0+rho*P.Fstall*P.voltage*u-rho*ce*(v0-P.r*w0)/2)/(1+rho);
            bf=rho*ce/(2*(1+rho));
        end
        // [M/h + D/2 + bE*[1;-r]*[1,-r]] * [v1;w1].
        H11=m11/h+d11/2+bf; H12=m12/h+d12/2-bf*P.r;
        H22=m22/h+d22/2+bf*P.r^2; dh=H11*H22-H12^2;
        b1=(m11/h-d11/2)*v0+(m12/h-d12/2)*w0+af+Fd+P.m*sc*wm^2;
        b2=(m12/h-d12/2)*v0+(m22/h-d22/2)*w0-P.r*af+P.m*P.g*sc+ ..
            Fd*(P.zpush*cos(thm)-P.xpush*sin(thm));
        vb=(H22*b1-H12*b2)/dh; wb=(H11*b2-H12*b1)/dh;
        iv=(H22/P.r+H12)/dh; iw=(-H12/P.r-H11)/dh;
        gamma=iv/P.r-iw;
        Tfr=max(min(-(vb/P.r-wb)/gamma,limit),-limit);
        vn=vb+iv*Tfr; wn=wb+iw*Tfr;
        Fmid=af-bf*(vn-P.r*wn);
    end
    yn=[y(1)+h*(v0+vn)/2;vn;y(3)+h*(w0+wn)/2;wn;2*Fmid-Fa0];
    Tvis=-P.bgear*((v0+vn)/(2*P.r)-(w0+wn)/2);
    Fnet=Fmid+(Tvis+Tfr)/P.r;
endfunction
// Backward Euler: friction and electrical damping use endpoint velocities.
// The scalar friction projection opposes endpoint shaft motion. It avoids using
// an endpoint sticking impulse together with midpoint velocities for dissipation.
// First-order method: acceptance still requires explicit h/2 convergence.
function [yn,Fnet]=buoc_backward(y,u,Fd,h,P)
    v0=y(2); w0=y(4); Fa0=y(5); vn=v0; wn=w0;
    wr0=v0/P.r-w0; limit=P.Tkin;
    if abs(wr0)<=P.weps then limit=P.Tstat; end
    vnl=motor_vnl(P,u,v0-P.r*w0); ce=P.Fstall/vnl; rho=h/P.tau;
    for it=1:3
        th1=y(3)+h*wn; [m11,m12,m22,detM]=co_he(th1,P);
        sc=P.xc*cos(th1)+P.l*sin(th1);
        d11=P.bx+P.bgear/P.r^2; d12=-P.bgear/P.r; d22=P.bgear;
        raw=P.Fstall*P.voltage*u-ce*(vn-P.r*wn);
        if abs(raw)>P.Fmax then
            af=(Fa0+rho*max(min(raw,P.Fmax),-P.Fmax))/(1+rho); bf=0;
        else
            af=(Fa0+rho*P.Fstall*P.voltage*u)/(1+rho); bf=rho*ce/(1+rho);
        end
        H11=m11/h+d11+bf; H12=m12/h+d12-bf*P.r;
        H22=m22/h+d22+bf*P.r^2; dh=H11*H22-H12^2;
        b1=m11/h*v0+m12/h*w0+af+Fd+P.m*sc*wn^2;
        b2=m12/h*v0+m22/h*w0-P.r*af+P.m*P.g*sc+ ..
            Fd*(P.zpush*cos(th1)-P.xpush*sin(th1));
        vb=(H22*b1-H12*b2)/dh; wb=(H11*b2-H12*b1)/dh;
        iv=(H22/P.r+H12)/dh; iw=(-H12/P.r-H11)/dh; gamma=iv/P.r-iw;
        Tfr=max(min(-(vb/P.r-wb)/gamma,limit),-limit);
        vn=vb+iv*Tfr; wn=wb+iw*Tfr; Fend=af-bf*(vn-P.r*wn);
    end
    yn=[y(1)+h*vn;vn;y(3)+h*wn;wn;Fend];
    Fnet=Fend+(-P.bgear*(vn/P.r-wn)+Tfr)/P.r;
endfunction
function [yn,Fnet]=buoc_co_he(y,u,Fd,h,P)
    if P.solver==1 then [yn,Fnet]=buoc_backward(y,u,Fd,h,P);
    else [yn,Fnet]=buoc_midpoint(y,u,Fd,h,P); end
endfunction
// Encoder tuong doi than, x4; LPF 0.75; tich phan vi tri nhu firmware.
// IMU: gia toc tai BMI160 tren board; complementary 0.98; gyro do/s.
function [T,Y,F,U,E]=mo_phong(P,K,S,kf)
    // R8 floor logs invalidate the claim that preserved PWM sum validates balance.
    // Explicit acknowledgement is required for any pitch-only differential diagnostic.
    if isfield(S,"wheel_pair_r8") then
        if ~isfield(S,"allow_unvalidated_pair_diagnostic") then
            error("R8 differential model rejected by floor logs: no yaw/load state. Use explicit diagnostic profile; not hardware validation.");
        end
        if ~S.allow_unvalidated_pair_diagnostic then error("Unvalidated pair diagnostic not enabled");end
    end
    P.emf_sign=S.motor_sign;
    if isfield(S,"solver") then P.solver=S.solver; end
    use_native=native_available;
    if isfield(S,"native") then use_native=S.native & native_available;end
    if use_native then Ppacked=native_parameters(P);end
    // Diagnostic only. Normal tuning always keeps real sensor quantization.
    quant_sensor=%t;
    if isfield(S,"quant_sensor") then quant_sensor=S.quant_sensor; end
    rad=%pi/180; N=round(S.tend/S.Ts); h=S.Ts/S.nsub;
    initial_omega=0;if isfield(S,"initial_omega") then initial_omega=S.initial_omega;end
    initial_velocity=0;if isfield(S,"initial_velocity") then initial_velocity=S.initial_velocity;end
    y=[0;initial_velocity;S.th0;initial_omega;0]; Ii=0; vf=0; xhat=0;
    // Optional restart diagnostics: initial conditions only, never future log
    // states. Force and absolute attitude remain unknown without extra sensors.
    if isfield(S,"initial_force_N") then y(5)=S.initial_force_N;end
    if isfield(S,"initial_integral_deg") then Ii=S.initial_integral_deg;end
    if isfield(S,"initial_encoder_speed") then vf=S.initial_encoder_speed;end
    if isfield(S,"initial_encoder_position") then xhat=S.initial_encoder_position;end
    pair_state=[0;0]; // Optional historical R8 command.
    coupled=%f;if isfield(S,"coupled_pair") then coupled=%t;end
    pair=[0;0;0;0];lz=0;old_drive=[0;0];vfLR=[vf;vf];heading_state=zeros(7,1);heading_limit=8;
    if coupled then
        if ~isfield(S,"allow_unvalidated_pair_diagnostic") then error("Coupled diagnostic requires explicit acknowledgement: loaded model is NOT identified.");end
        if ~S.allow_unvalidated_pair_diagnostic then error("Coupled diagnostic not enabled");end
        Ppacked=native_parameters(P);
    end
    thhat=S.acc_mult*S.raw_acc_sign*S.th0;
    if isfield(S,"pitch_est0") then thhat=S.pitch_est0;end
    // Isolated literature experiment. Inputs are sensor/encoder estimates only.
    // Absent by default: the restored R7 path remains numerically unchanged.
    research=%f;research_state=[0;thhat;0;0];research_irow=1;
    if isfield(S,"research_control") then research=%t;end
    if research & ~coupled then error("Literature control requires the explicitly unvalidated two-wheel plant, not a mean-PWM proxy");end
    if research then
        if isfield(S.research_control,"observer") then
            research_state=observer_initial(thhat,S.research_control);research_irow=S.research_control.nobs+1;
        end
    end
    phase=[0.25;-0.25]; // GIA DINH pha encoder hai banh
    actual_ppr=[S.ppr;S.ppr];
    if isfield(S,"actual_ppr") then actual_ppr=matrix(S.actual_ppr,-1,1); end
    // Canonical two-wheel column. A row silently broadcasts against travel,
    // creating four counts/velocities and corrupting the native input packet.
    if size(actual_ppr,"*")<>2 | or(actual_ppr<=0) then error("Expected two positive per-wheel PPR values");end
    cnt_prev=round(-y(3)*actual_ppr/(2*%pi)+phase);
    if ~quant_sensor then cnt_prev=-y(3)*actual_ppr/(2*%pi)+phase; end
    cnt_zero=cnt_prev;
    u_prev=0; n=N+1;
    if isfield(S,"initial_drive") then u_prev=S.initial_drive;end
    T=zeros(1,n); Y=zeros(5,n); F=zeros(1,n); U=zeros(1,n); Y(:,1)=y;
    // Raw accel pitch = raw_acc_sign*theta; raw gyroY = -raw_acc_sign*omega.
    // Run7 slow tilts verify raw_acc_sign=-1, raw gyroY positive on forward tilt.
    // BMI160 0x28: ODR100Hz; BWP normal: acc40.5Hz/gyro39.9Hz.
    // LPF bac 1 khop cutoff la XAP XI, khong phai ham truyen Bosch da do.
    la=[S.raw_acc_sign*P.g*sin(y(3));P.g*cos(y(3))]; lg=0;
    acc_hold=atan(la(1),la(2)); gyro_hold=0;
    if isfield(S,"initialize_sensor_rate") then
        if S.initialize_sensor_rate then lg=-S.raw_acc_sign*initial_omega;gyro_hold=lg;end
    end
    // Explicit observation delay envelope. This is NOT a measured Bosch delay.
    // Floor logs reject the previous zero-extra-delay nominal model; fit/stress
    // scripts vary this parameter without changing geometry or sensor signs.
    sensor_delay_ticks=0;
    if isfield(S,"sensor_delay_ticks") then sensor_delay_ticks=S.sensor_delay_ticks;end
    acc_queue=ones(1,sensor_delay_ticks+1)*acc_hold;
    gyro_queue=ones(1,sensor_delay_ticks+1)*gyro_hold;
    // Datasheet scales and ODR tolerance; controller still assumes nominal scales.
    acc_gain=[1;1]; gyro_gain=1; imu_odr=100; acc_bias_g=[0;0];
    if isfield(S,"acc_gain") then acc_gain=S.acc_gain;end
    if isfield(S,"gyro_gain") then gyro_gain=S.gyro_gain;end
    if isfield(S,"imu_odr") then imu_odr=S.imu_odr;end
    if isfield(S,"acc_bias_g") then acc_bias_g=S.acc_bias_g;end
    // Sensor quantization occurs BEFORE subtracting the calibrated gyro bias.
    // Keep its ADC phase, rather than re-quantizing a zero-mean log at zero bias.
    gyro_adc_bias=0;
    if isfield(S,"gyro_adc_bias") then gyro_adc_bias=S.gyro_adc_bias;end
    imu_period=1/imu_odr;
    imu_next=modulo(-S.imu_phase,2)*S.Ts;
    E=zeros(4,n); E(:,1)=[thhat;0;S.acc_mult*acc_hold;0];
    audit_control=%f;
    if isfield(S,"audit_control") then audit_control=S.audit_control;end
    if audit_control then
        E=[E;zeros(6,n)];E(5:6,1)=[vf;xhat];E(10,1)=Ii;
    end
    if coupled then
        if ~audit_control then error("Coupled diagnostic requires audit_control=true");end
        E=[E;zeros(5,n)];
    end
    recovering=%f;recovery_quiet=0;recovery_vmean=0;recovery_enter_time=0;audit_recovery=%f;
    if isfield(S,"audit_recovery") then audit_recovery=S.audit_recovery;end
    if audit_recovery then E=[E;zeros(1,n)];end
    audit_motor=%f;if isfield(S,"audit_motor") then audit_motor=S.audit_motor;end
    if audit_motor then
        if ~coupled | audit_recovery then error("Motor force audit requires coupled plant without recovery-mode audit");end
        E=[E;zeros(2,n)];
    end
    audit_observer=%f;if isfield(S,"audit_observer") then audit_observer=S.audit_observer;end
    if audit_observer then
        if ~audit_motor then error("Observer audit requires motor audit");end
        E=[E;zeros(5,n)];
    end
    external_stopped=%f; // Latched gateway/operator STOP, independent of PID stability.
    for k=1:N
        t=(k-1)*S.Ts;
        if t+1e-10>=imu_next then
            // Polling at200Hz: read the latest sample; hold it until next update.
            while imu_next<=t+1e-10
                imu_next=imu_next+imu_period;
            end
            // Nhieu chi tao mot lan tren mau MOI, mau cu giu nguyen.
            ca=cos(S.n_th(k)); sa=sin(S.n_th(k));
            a_sensor=acc_gain.*[ca*la(1)+sa*la(2);-sa*la(1)+ca*la(2)]+acc_bias_g*P.g;
            raw_g=a_sensor/P.g;
            if isfield(S,"accel_calibration") then
                ac=S.accel_calibration;
                raw_g=ac(2,:)'.*raw_g+ac(1,:)';
            end
            aq=raw_g*4096;
            aq=max(min(round(aq),32767),-32768);
            decoded_g=aq/4096;
            if isfield(S,"accel_calibration") then
                decoded_g=(decoded_g-ac(1,:)')./ac(2,:)';
            end
            acc_hold=atan(decoded_g(1),decoded_g(2));
            gq=round((gyro_gain*lg+S.n_g(k)+(S.gyro_bias+gyro_adc_bias)*rad)/rad*16.4);
            gyro_hold=(max(min(gq,32767),-32768)/16.4-gyro_adc_bias)*rad;
            if ~quant_sensor then
                acc_hold=atan(a_sensor(1),a_sensor(2));
                gyro_hold=gyro_gain*lg+S.n_g(k)+S.gyro_bias*rad;
            end
            // Empirical installed-angle error, not an inverse firmware correction.
            // Restrict gain error to measured +/-10deg; hold error outside the range.
            // This local observation model is superseded once XYZ calibration is valid.
            if isfield(S,"angle_model") then
                am=S.angle_model;
                raw_local=max(min(acc_hold/rad,am(5)),am(4));
                acc_hold=acc_hold+((am(1)-1)*raw_local+am(2))*rad;
            end
            // Optional slow effective zero/load-bias uncertainty. This is not
            // a new BMI160 calibration inferred from a closed-loop capture.
            if isfield(S,"angle_bias_drift_deg") then
                drift_fraction=max(0,min(1,(t-S.angle_bias_drift_start)/S.angle_bias_drift_ramp));
                acc_hold=acc_hold+S.angle_bias_drift_deg*drift_fraction*rad;
            end
        end
        acc_queue=[acc_queue(2:$),acc_hold];gyro_queue=[gyro_queue(2:$),gyro_hold];
        th_acc=S.acc_mult*acc_queue(1); gyro=S.gyro_sign*gyro_queue(1);
        alpha=S.imu_alpha;
        if recovering & isfield(S,"recovery_alpha") then alpha=S.recovery_alpha;end
        // Diagnostic adaptive weighting: reduce accel correction during rotation
        // or large innovation. No signal is replaced by recorded future angles.
        if isfield(S,"adaptive_imu") then
            cfg=S.adaptive_imu;pred=thhat+gyro*S.Ts;
            enable_adaptive=%t;
            // Optional catch-only gate preserves fixed R7 filtering near upright.
            if size(cfg,"*")>=4 then
                risk=abs(thhat/rad);
                if size(cfg,"*")>=5 & thhat*gyro>0 then risk=max(risk,abs(thhat/rad+cfg(5)*gyro/rad));end
                enable_adaptive=risk>cfg(4);
            end
            if enable_adaptive & (abs(gyro/rad)>cfg(1) | abs((th_acc-pred)/rad)>cfg(2)) then alpha=cfg(3);end
        end
        thhat=alpha*(thhat+gyro*S.Ts)+(1-alpha)*th_acc;
        // Cat theo GOC UOC LUONG nhu FSM; 45do that la gioi han mo hinh.
        if abs(thhat)>=45*rad then n=k; break; end
        travel=[y(1);y(1)];
        if coupled then travel=travel+[-1;1]*S.coupled_pair(1)*pair(1)/2;end
        count=round((travel/P.r-y(3)).*actual_ppr/(2*%pi)+phase);
        if ~quant_sensor then count=(travel/P.r-y(3)).*actual_ppr/(2*%pi)+phase; end
        vrLR=(count-cnt_prev)*(2*%pi*P.r/S.ppr)/S.Ts;
        vfLR=(1-S.enc_beta)*vrLR+S.enc_beta*vfLR;
        delta=((count(2)-cnt_zero(2))-(count(1)-cnt_zero(1)))*(2*%pi*P.r/S.ppr);
        vraw=mean(vrLR);cnt_prev=count;
        vf=(1-S.enc_beta)*vraw+S.enc_beta*vf; xhat=xhat+vf*S.Ts;
        if isfield(S,"recovery_controller") then
            rc=S.recovery_controller;risk=abs(thhat/rad);
            recovery_vmean=.95*recovery_vmean+.05*vf;
            if thhat*gyro>0 then risk=max(risk,abs(thhat/rad+rc(4)*gyro/rad));end
            if ~recovering & (risk>rc(1) | abs(vf)>rc(2) | abs(gyro/rad)>rc(3)) then recovering=%t;recovery_quiet=0;recovery_enter_time=t;end
            if recovering then
                quiet=abs(thhat/rad)<rc(7) & abs(recovery_vmean)<rc(8) & abs(gyro/rad)<rc(9);
                if quiet then recovery_quiet=recovery_quiet+S.Ts;else recovery_quiet=0;end
                if recovery_quiet>=rc(10) then recovering=%f;recovery_quiet=0;end
            end
        end
        if isfield(S,"external_stop_at") then
            if t+1e-10>=S.external_stop_at then external_stopped=%t;end
        end
        if isfield(S,"trial_interlocks") & ~external_stopped then
            if S.trial_interlocks & (abs(thhat/rad)>25 | abs(xhat)>.30 | abs(vf)>.80) then n=k;break;end
        end
        if S.mode==1 then v_cmd=max(min(K(5)*(S.xt-xhat),1),-1); else v_cmd=S.vt; end
        ev=v_cmd-vf;
        outer_limit=S.tilt_max;
        if isfield(S,"capture_authority") then outer_limit=capture_tilt_limit(thhat/rad,S.capture_authority);end
        // Optional floor revision: slow integral only after capture, with bounded
        // angle authority and conditional integration at theta saturation.
        integral_limit=10;integrate=%t;
        if isfield(S,"integral_limit") then integral_limit=S.integral_limit;end
        if isfield(S,"integral_start") then integrate=t>=S.integral_start;end
        if isfield(S,"integral_gate_angle") then integrate=integrate & abs(thhat/rad)<S.integral_gate_angle;end
        if isfield(S,"integral_gate_speed") then integrate=integrate & abs(vf)<S.integral_gate_speed;end
        if integrate then
            candidate_I=max(min(Ii+K(4)*ev*S.Ts,integral_limit),-integral_limit);
            pending_theta=K(3)*ev+candidate_I;
            if ~isfield(S,"integral_start") | abs(pending_theta)<=outer_limit | pending_theta*ev<0 then Ii=candidate_I;end
        end
        theta_t=max(min(K(3)*ev+Ii,outer_limit),-outer_limit);
        pwm=S.pid_sign*(K(1)*(theta_t+S.trim-thhat/rad)-K(2)*gyro/rad);
        if recovering then
            blend=1;
            if isfield(S,"recovery_blend") then
                // Return toward the R7 gains once mean drift disappears, rather
                // than requiring a high-gain oscillation to satisfy quiet gates.
                blend=min(1,abs(recovery_vmean)/.08);
                if t-recovery_enter_time<.15 then blend=1;end
            end
            pwm=S.pid_sign*(K(1)*(1+blend*(rc(5)-1))*(theta_t+S.trim-thhat/rad)-K(2)*(1+blend*(rc(6)-1))*gyro/rad);
        end
        if isfield(S,"capture_angle_gain") then
            catch_angle=thhat/rad;
            if isfield(S,"capture_angle_prediction_s") & thhat*gyro>0 then
                catch_angle=catch_angle+S.capture_angle_prediction_s*gyro/rad;
            end
            pwm=pwm*capture_angle_gain(catch_angle,S.capture_angle_gain);
        end
        if isfield(S,"first_armed_zero") then
            if S.first_armed_zero & k==1 then pwm=0;Ii=0;theta_t=0;end
        end
        dbL=S.DB_L;dbR=S.DB_R;
        if isfield(S,"DB_direction") then
            dc=1;if pwm<0 then dc=2;end
            dbL=S.DB_direction(1,dc);dbR=S.DB_direction(2,dc);
        end
        outL=pid_deadband(pwm,dbL,2499); outR=pid_deadband(pwm,dbR,2499);
        if isfield(S,"wheel_pair_r8") & ~external_stopped then
            [outL,outR,pair_state,pair_ff]=wheel_pair_r8(outL,outR,0,0,pair_state,S.wheel_pair_r8);
        end
        if external_stopped then
            // Mirror firmware Motor_Stop: both PWM=0 selects electrical brake.
            // No modeled hand catch; post-STOP falling cannot identify the PID.
            outL=0;outR=0;pwm=0;Ii=0;theta_t=0;v_cmd=0;
            if research then research_state(research_irow)=0;heading_state(2)=0;end
        end
        if research & ~external_stopped then
            if isfield(S.research_control,"observer") then
                xcounter=mean(count-cnt_zero)*(2*%pi*P.r/S.ppr);
                [outL,outR,research_state]=observer_control(thhat,gyro,th_acc,xcounter,vfLR,delta,research_state,S.research_control,S.Ts);
            else
                [outL,outR,research_state]=literature_control(thhat,gyro,xhat,vfLR,delta,research_state,S.research_control,S.Ts);
            end
            pwm=(outL+outR)/2;theta_t=0;Ii=research_state(research_irow);
            heading_state(2)=(outL-outR)/2;heading_limit=2499;
            if isfield(S,"first_armed_zero") then
                if S.first_armed_zero & k==1 then
                    outL=0;outR=0;pwm=0;research_state(research_irow)=0;
                    if isfield(S.research_control,"observer") then research_state($)=0;end
                end
            end
        end
        if coupled & ~research then
            if S.heading_policy==4 then
                hc=[];if isfield(S,"guarded_heading_cfg") then hc=S.guarded_heading_cfg;end
                [outL,outR,heading_state,heading_limit]=guarded_heading(outL,outR,delta,vfLR(2)-vfLR(1),thhat/rad,gyro/rad,theta_t,vf,t,heading_state,hc);
            else
                [outL,outR,heading_state,heading_limit]=pair_heading(outL,outR,delta,vfLR(2)-vfLR(1),thhat/rad,gyro/rad,theta_t,vf,t,heading_state,S.heading_policy);
            end
        end
        command=S.motor_sign*(outL+outR)/(2*2499);
        // Optional per-wheel loaded-effort sensitivity, NOT a fitted torque model.
        // Unlike a pure PWM average, unequal gains/losses expose pitch/yaw coupling.
        // Additional losses are residual uncertainties; existing P.Tstat/Tkin remain.
        if isfield(S,"pair_effort_probe") then
            command=S.motor_sign*pair_effort_probe(outL,outR,S.pair_effort_probe)/2499;
        end
        // Probe coupling from differential assist with unequal wheel force gains.
        // Common equivalent PWM is a scenario input, not an identified force bound.
        if isfield(S,"mean_pwm_bias") & ~external_stopped then
            if outL<>0 | outR<>0 then command=command+S.motor_sign*S.mean_pwm_bias/2499;end
        end
        fsum=0;
        if coupled then
            drive=S.motor_sign*[outL;outR]/2499;
            if isfield(S,"catalog_friction") then
                [y,pair,la,lg,lz,fsum]=catalog_interval(y,pair,la,lg,lz,Ppacked,S,t,drive,old_drive);
            else
                [y,pair,la,lg,lz,fsum]=pair_interval(y,pair,la,lg,lz,Ppacked,S,t,drive,old_drive);
            end
            old_drive=drive;
        elseif use_native then
            [y,la,lg,fsum]=native_interval(y,la,lg,Ppacked,S,t,command,u_prev);
        else
        for j=1:S.nsub
            // Tach tai thoi diem PWM cap nhat: khong lam tron 0.35ms thanh h.
            ta=(j-1)*h; tb=j*h; cuts=[ta tb];
            if ta<S.latency & S.latency<tb then cuts=[ta S.latency tb]; end
            for ic=1:length(cuts)-1
                dt=cuts(ic+1)-cuts(ic); tj=t+cuts(ic); Fd=0;
                for ip=1:size(S.pulses,"*")
                    if tj+1e-10>=S.pulses(ip) & tj+1e-10<S.pulses(ip)+S.wp then Fd=Fd+S.Fp; end
                end
                u=command; if cuts(ic)+1e-10<S.latency then u=u_prev; end
                y0=y; [y,Fn]=buoc_co_he(y,u,Fd,dt,P); fsum=fsum+Fn*dt;
                ym=(y0+y)/2; c=cos(ym(3)); s=sin(ym(3));
                aa=(y(2)-y0(2))/dt; aw=(y(4)-y0(4))/dt;
                xi=P.ximu*c+P.zimu*s; zi=P.zimu*c-P.ximu*s;
                aix=aa+zi*aw-xi*ym(4)^2; aiz=-xi*aw-zi*ym(4)^2;
                raw_a=[-S.raw_acc_sign*(c*aix-s*(aiz+P.g));s*aix+c*(aiz+P.g)];
                ba=exp(-2*%pi*S.acc_fc*dt); bg=exp(-2*%pi*S.gyr_fc*dt);
                la=ba*la+(1-ba)*raw_a; lg=bg*lg-S.raw_acc_sign*(1-bg)*ym(4);
            end
        end
        end
        U(k+1)=(S.latency*u_prev+(S.Ts-S.latency)*command)/S.Ts; u_prev=command;
        T(k+1)=k*S.Ts; Y(:,k+1)=y; F(k+1)=fsum/S.Ts;
        E(1:4,k+1)=[thhat;gyro;th_acc;pwm];
        if audit_control then E(5:10,k+1)=[vf;xhat;theta_t;outL;outR;Ii];end
        if coupled then E(11:15,k+1)=[pair(1);pair(2);delta;heading_state(2);heading_limit];end
        if audit_recovery then E($,k+1)=bool2s(recovering);end
        if audit_motor then E(16:17,k+1)=pair(3:4);end
        if audit_observer then
            if research then
                if isfield(S.research_control,"observer") then E(18:22,k+1)=research_state(1:5);end
            end
        end
        if abs(y(3))>=45*rad | or(isnan(y)) | or(isinf(y)) then n=k+1; break; end
    end
    T=T(1:n); Y=Y(:,1:n); F=F(1:n); U=U(1:n); E=E(:,1:n);
endfunction
function [J,Fpk,thpk,ts,nga,rung,Jv,xpk,xend,vend]=danh_gia(T,Y,F,S)
    rad=%pi/180; errth=Y(3,:)-S.trim*rad;
    J=inttrap(T,T.*abs(errth)); Jv=inttrap(T,T.*abs(S.vt-Y(2,:)));
    Fpk=max(abs(Y(5,:))); thpk=max(abs(Y(3,:)));
    nga=thpk>=45*rad | T($)<S.tend-1e-6 | or(isnan(Y)) | or(isinf(Y));
    xpk=max(abs(Y(1,:)-S.xt)); xend=abs(Y(1,$)-S.xt);
    tail=find(T>=S.tend-0.5);
    if size(tail,"*")<2 then rung=%inf; vend=%inf;
    else rung=stdev(Y(4,tail)/rad); vend=max(abs(Y(2,tail))); end
    idx=find(T<S.tp & abs(errth)>0.5*rad); ts=0;
    if size(idx,"*")>0 then ts=T(idx($)); end
endfunction
function dat=dat_vi_tri(T,Y,F,S)
    [J,Fpk,thpk,ts,nga,rung,Jv,xpk,xend,vend]=danh_gia(T,Y,F,S);
    start_ss=S.tend-1; if S.tend>=65 then start_ss=S.tend-60; end
    if isfield(S,"objective_profile") then
        if S.objective_profile=="bounded_balance" then start_ss=min(5,S.tend-1);end
    end
    tail=find(T>=start_ss); ess=%inf;
    if ~nga & size(tail,"*")>=2 then ess=max(abs(Y(3,tail)/(%pi/180)-S.trim)); end
    band=.3;if isfield(S,"angle_band") then band=S.angle_band;end
    drift_ok=%t;
    if isfield(S,"drift_rate_max") then
        id=find(T>=max(0,S.tend-S.drift_window));
        drift_ok=%f;
        if length(id)>=2 then
            rate=abs((Y(1,id($))-Y(1,id(1)))/(T(id($))-T(id(1))));
            drift_ok=rate<=S.drift_rate_max;
        end
    end
    dat=~nga & xpk<S.xmax & xend<=S.xendmax & vend<=S.vendmax & rung<=S.rungmax & ess<=band & drift_ok;
endfunction
function [ess,settle]=chi_so_goc(T,Y,S)
    e=abs(Y(3,:)/(%pi/180)-S.trim); ess=%inf; settle=[];
    band=.3;if isfield(S,"angle_band") then band=S.angle_band;end
    start_ss=S.tend-1; if S.tend>=65 then start_ss=S.tend-60; end
    if isfield(S,"objective_profile") then
        if S.objective_profile=="bounded_balance" then start_ss=min(5,S.tend-1);end
    end
    if T($)>=S.tend-1e-6 then ess=max(e(find(T>=start_ss))); end
    for j=1:size(S.pulses,"*")
        stop=S.tend; if j<size(S.pulses,"*") then stop=S.pulses(j+1); end
        ts=%inf;
        if T($)>=stop-S.Ts then
            ix=find(T>=S.pulses(j)+S.wp & T<stop);
            bad=ix(find(e(ix)>band)); ts=0;
            if size(bad,"*")>0 then ts=T(bad($))+S.Ts-S.pulses(j)-S.wp; end
            // Phai con it nhat 0.5s trong band de goi la da xac lap.
            if stop-S.pulses(j)-S.wp-ts<0.5 then ts=%inf; end
        end
        settle=[settle ts];
    end
endfunction
function [p,C]=cuc_day_du(P,K,S,kf)
    // Chi la phep sang loc lien tuc, BO QUA IMU/tre/ma sat kho/bao hoa.
    [A,B,al,be]=mo_hinh(P); rad=%pi/180;
    C=zeros(7,7); C(1:3,1:3)=A(2:4,2:4); C(1:3,5)=B(2:4);
    C(4,6)=-K(4); C(4,7)=-K(4)*K(5);
    sp=S.motor_sign*S.pid_sign;
    ap=S.acc_mult*S.raw_acc_sign; gp=-S.raw_acc_sign*S.gyro_sign;
    C(5,:)=[-P.Fstall/P.vnl,-sp*ap*kf*K(1)/rad,-sp*gp*kf*K(2)/rad+P.Fstall*P.r/P.vnl, ..
        sp*kf*K(1),-1,-sp*kf*K(1)*K(3),-sp*kf*K(1)*K(3)*K(5)]/P.tau;
    te=-S.Ts/log(S.enc_beta); C(6,1)=1/te; C(6,3)=-P.r/te; C(6,6)=-1/te; C(7,6)=1;
    p=spec(C);
endfunction
function S=nhieu_co_dinh(S,seed)
    rand("seed",seed); rand("normal"); n=round(S.tend/S.Ts);
    // Empirical static trace replay preserves correlation and repeated IMU samples.
    // Only the zero-mean variation is used; mean pitch is NOT a trim measurement.
    if co_log_tinh then
        np=imu_static_pitch; ng=imu_static_gyro;
        if isfield(S,"post_noise") then
            if S.post_noise & isdef("imu_post_pitch") then np=imu_post_pitch; ng=imu_post_gyro; end
        end
        id=modulo(0:n-1,size(np,"*"))+1;
        S.n_th=matrix(np(id)-mean(np),1,n)*(%pi/180);
        S.n_g=matrix(ng(id)-mean(ng),1,n)*(%pi/180);
    else
        pitch_sigma=.05;gyro_sigma=.30;
        if team_mode then pitch_sigma=imu_noise_pitch;gyro_sigma=imu_noise_gyro;end
        S.n_th=pitch_sigma*(%pi/180)*rand(1,n); S.n_g=gyro_sigma*(%pi/180)*rand(1,n);
    end
endfunction
function Pk=sai_lech(P,h)
    // Bat dinh tong hop; khong gia vo la di chuyen tung linh kien.
    Pk=P; Pk.m=P.m*h(1); Pk.l=P.l*h(2); Pk.xc=P.xc*h(2);
    Pk.I=P.I*h(1)*h(2)^2; Pk.tau=P.tau*h(3);
endfunction

// HINH KHUNG: 2 PHEP CHIEU TU CUNG BANG lk; hien 3 mica va cac cum.
function [handles,labels]=ve_khung(lk,ten,th,xpos,view,r,ybanh,daybanh,Rkhop,daikhop)
    handles=[]; labels=[]; c=cos(th); s=sin(th);
    palette=["cyan","orange","green","magenta","blue","red","black"];
    for i=1:size(lk,1)
        X=lk(i,2)+lk(i,5)/2*[-1 1 1 -1 -1]; Z=lk(i,4)+lk(i,7)/2*[-1 -1 1 1 -1];
        xx=xpos+c*X+s*Z; zz=c*Z-s*X;
        if view==2 then xx=lk(i,3)+lk(i,6)/2*[-1 1 1 -1 -1]; end
        // Day ve net tren board; khoi luong/day cao van dung bang lk.
        if i==13 then zz=mean(zz)+0.001*[-1 1 -1 1 -1]; end
        xpoly(xx,zz); e=gce(); e.thickness=2; ci=3;
        if i<=3 then ci=1; elseif i==4 then ci=2; elseif i==5 then ci=3;
        elseif i==6 then ci=5; elseif i<=10 then ci=4; elseif i<=12 then ci=6;
        elseif i==13 then ci=7; elseif i<=15 then ci=5; elseif i<=17 then ci=7; else ci=2; end
        e.foreground=color(palette(ci));
        if or(i==[1 4 5 6 7 8 9 10 11 13 14 16 18]) then handles=[handles;e]; labels=[labels;ten(i)]; end
    end
    phi=linspace(0,2*%pi,65);
    if view==1 then
        xpoly(xpos+r*cos(phi),r*sin(phi)); e=gce(); e.thickness=3;
        handles=[handles;e]; labels=[labels;"Banh 65mm (2 banh chong hinh)"];
    else
        for side=[-1 1]
            xpoly(side*ybanh+daybanh/2*[-1 1 1 -1 -1],r*[-1 -1 1 1 -1]);
            e=gce(); e.thickness=3; e.foreground=color("black");
            yh=side*(ybanh-daybanh/2-daikhop/2);
            xpoly(yh+daikhop/2*[-1 1 1 -1 -1],Rkhop*[-1 -1 1 1 -1]); e=gce(); e.foreground=color("magenta");
        end
    end
    xgrid(); a=gca(); a.isoview="on";
    if view==1 then a.data_bounds=[xpos-0.09,-0.04;xpos+0.09,0.16];
    else a.data_bounds=[-0.16,-0.04;0.16,0.16]; end
endfunction
function luu_hinh(f,filename,export_png,out_dir)
    if export_png then xs2png(f,out_dir+"/"+filename); end
endfunction
function f=ve_ben_vung(KQ_ben,ca_ten,ten_ho_so,export_png,out_dir)
    f=scf(4);clf(f);f.figure_size=[1100 900];
    mau=["blue","red","magenta","green","black","cyan","orange","purple","brown","grey"];
    for panel=1:2
        subplot(2,1,panel);hh=[];
        for i=1:length(KQ_ben)
            yy=KQ_ben(i)(2)(3,:)/(%pi/180);
            if panel==2 then yy=100*KQ_ben(i)(2)(1,:); end
            plot(KQ_ben(i)(1),yy);e=gce();e=e.children(1);e.foreground=color(mau(modulo(i-1,size(mau,"*"))+1));hh=[hh;e];
        end
        xgrid();
        if panel==1 then
            plot([0 8],[.3 .3],"r--");plot([0 8],[-.3 -.3],"r--");
            ylabel("theta that(do)");title(ten_ho_so+": bat dinh (goc va vi tri)");
        else
            ylabel("x that(cm)");xlabel("t(s)");legend(hh,ca_ten,1);
        end
    end
    luu_hinh(f,"hinh4_ben_vung.png",export_png,out_dir);
endfunction

// Phan sau chi chay khi Execute BINH THUONG; kiem tra ham khong quet PID.
if ~kiem_tra_khai_bao then
// Save the physical inputs beside this run; column names are listed in order.
csvWrite(lk,out_dir+"/bo_tri_linh_kien.csv");
mputl(ten,out_dir+"/ten_linh_kien.txt");
csvWrite([m+M,m,M,r,cao_truc_dat,m_motor_pair,m_motor_each,xc,yc,l,I,I_truc, ..
 Meq,lk(10,4),z_tren,kt_pin],out_dir+"/thong_so_co_hoc.csv");
mputl(["m_total_kg";"m_body_kg";"m_wheels_couplings_kg";"rolling_radius_m"; ..
 "measured_axle_floor_height_m";"estimated_motor_pair_kg";"estimated_motor_each_kg"; ..
 "body_com_x_m";"body_com_y_m";"body_com_z_m";"body_pitch_I_com_kgm2"; ..
 "body_pitch_I_axle_kgm2";"wheel_equivalent_mass_kg";"imu_height_m"; ..
 "mica1_top_height_m";"mica2_top_height_m";"mica3_top_height_m"; ..
 "battery_dx_m";"battery_dy_m";"battery_dz_m"],out_dir+"/thong_so_co_hoc_ten_cot.txt");
if co_motor_bench then
    bench_dir=out_dir+"/bench_"+ten_log;
    if ~isdir(bench_dir) then mkdir(bench_dir); end
    csvWrite(DM,bench_dir+"/motor_fit.csv");
end
if do_plot then
    if co_motor_bench & ~co_measurement_plan then
        fb=scf(12); clf(fb); fb.figure_size=[1150 850];
        bt=(DH(:,2)-DH(1,2))/1000;
        subplot(3,1,1); plot(bt,DH(:,6:7)); xgrid(); ylabel("PWM L/R");
        title("LOG "+ten_log+": PWM và encoder của các giai đoạn đã ghi");
        subplot(3,1,2); plot(bt,DH(:,8:9)); xgrid(); ylabel("encoder raw ticks");
        subplot(3,1,3);
        bw=4; bv=(DH(1+bw:$,8:9)-DH(1:$-bw,8:9))*(2*%pi*r/937.2)/(.005*bw);
        plot(bt(1+bw:$),bv); xgrid(); ylabel("speed raw sign, m/s"); xlabel("time from first saved sample(s)");
        luu_hinh(fb,"bench_pwm_encoder.png",export_png,bench_dir);
        ff=scf(13); clf(ff); ff.figure_size=[1150 800];
        picks=[find(DM(:,2)==1 & DM(:,3)==1800 & DM(:,1)==7); ..
               find(DM(:,2)==1 & DM(:,3)==-1800 & DM(:,1)==7); ..
               find(DM(:,2)==2 & DM(:,3)==1800 & DM(:,1)==8); ..
               find(DM(:,2)==2 & DM(:,3)==-1800 & DM(:,1)==8)];
        for bj=1:size(picks,"*")
            bm=DM(picks(bj),:); bc=7+bm(2);
            bi=find(DH(:,4)==bm(1) & DH(:,2)>=bm(4)*1000-.1 & DH(:,2)<=(bm(4)+bm(5))*1000+.1);
            bprev=bi(1)-1; btime=(DH(bi,2)-DH(bprev,2))/1000;
            bdist=(DH(bi,bc)-DH(bprev,bc))*(2*%pi*r/937.2);
            bq=1-exp(-btime/bm(10)); bcoef=[btime,bq]\bdist;
            bmodel=[btime,bq]*bcoef;
            subplot(2,2,bj); plot(btime,[bdist,bmodel]*1000); xgrid(); xlabel("s"); ylabel("wheel travel on stand, mm");
            title(msprintf("wheel%d PWM%d, tau SPEED %.0fms; RMSE %.3fmm",bm(2),bm(3),bm(10)*1000, ..
                sqrt(mean((bdist-bmodel).^2))*1000));
        end
        luu_hinh(ff,"bench_speed_fit.png",export_png,bench_dir);
        fr=scf(14); clf(fr); fr.figure_size=[1150 800];
        for bj=1:size(picks,"*")
            bm=DM(picks(bj),:); bc=7+bm(2);
            bi=find(DH(:,4)==bm(1) & DH(:,2)>=bm(4)*1000-.1 & DH(:,2)<=(bm(4)+bm(5))*1000+.1);
            bprev=bi(1)-1; btime=(DH(bi,2)-DH(bprev,2))/1000;
            bdist=(DH(bi,bc)-DH(bprev,bc))*(2*%pi*r/937.2);
            bq=1-exp(-btime/bm(10)); bcoef=[btime,bq]\bdist;
            berr=bdist-[btime,bq]*bcoef;
            subplot(2,2,bj); plot(btime,berr*1000); xgrid(); xlabel("s"); ylabel("measured-fit residual, mm");
            title(msprintf("wheel%d PWM%d; 1 encoder tick = %.3fmm",bm(2),bm(3),2*%pi*r/937.2*1000));
        end
        luu_hinh(fr,"bench_fit_residual.png",export_png,bench_dir);
    end
    if co_log_tinh then
        fi=scf(11); clf(fi); fi.figure_size=[1100 800];
        hi_t=(DH(ih,2)-DH(ih(1),2))/1000;
        subplot(3,1,1); plot(hi_t,imu_static_pitch); xgrid(); ylabel("atan2(ax,az), deg");
        title(msprintf("BMI160 tĩnh ID%d: log %s; góc trung bình không phải trim",DH(ih(1),4),ten_log));
        subplot(3,1,2); plot(hi_t,imu_static_gyro-hw_bias_y); xgrid(); ylabel("gyroY-bias, dps");
        subplot(3,1,3); plot(hi_t,DH(ih,13:15)/4096); xgrid(); ylabel("accel XYZ, g"); xlabel("time(s)");
        luu_hinh(fi,"imu_tinh_hardware_"+ten_log+".png",export_png,out_dir);
    end
    if co_log then
        fl=scf(10); clf(fl); fl.figure_size=[1200 850];
        subplot(4,1,1); plot(log_t,DL(:,2)); xgrid(); ylabel("pitch filtered (deg)");
        title("LOG THUC 20Hz: khong dong nhat pitch filtered voi goc that");
        subplot(4,1,2); plot(log_t,DL(:,3)); xgrid(); ylabel("gyro (dps)");
        subplot(4,1,3); plot(log_t,[DL(:,8),DL(:,9)]); xgrid(); ylabel("PWM L/R");
        subplot(4,1,4); plot(log_t,log_x*100); xgrid(); ylabel("integral v (cm)"); xlabel("time(s)");
        title("Vi tri SUY RA tu v lam tron/da loc, khong phai tick encoder hay vi tri do doc lap");
        luu_hinh(fl,"log_thuc_20261007.png",export_png,out_dir);
    end
    fg=scf(6); clf(fg); fg.figure_size=[1200 680];
    subplot(1,2,1); [hh,nh]=ve_khung(lk,ten,0,0,1,r,y_banh,day_banh,R_khop,dai_khop);
    plot([-0.08 0.08],[-r -r],"k"); xlabel("x: tien/lui (m)"); ylabel("z: tren truc banh (m)");
    title("NHIN NGANG: chieu chay 80mm, 3 mica / 2 tang");
    subplot(1,2,2); [hh,nh]=ve_khung(lk,ten,0,0,2,r,y_banh,day_banh,R_khop,dai_khop);
    plot([-0.17 0.17],[-r -r],"k"); xlabel("y: trai/phai (m)"); ylabel("z: tren truc banh (m)");
    title("NHIN TRUOC: khung 200mm, motor va banh hai ben");
    luu_hinh(fg,"hinh6_bo_tri_linh_kien.png",export_png,out_dir);
end

// 3. TIM PID CA HE; khong suy ra ca 5 he so tu wn cua rieng goc.
thu_muc_goc=out_dir;
// Run the current normalized control convention by default.
// To compare the previous incorrect signs, set ho_so_kiem_tra=[1 2] before Execute.
if ~isdef("ho_so_kiem_tra") then ho_so_kiem_tra=2; end
for ho_so=ho_so_kiem_tra
 if ho_so==1 then
  ten_ho_so="firmware_dau_cu"; dau_gyro=1; dau_acc=1; dau_pid=1;
 else
  ten_ho_so="phuong_an_chuan_hoa";
  dau_gyro=-imu_acc_sign_thuc; dau_acc=imu_acc_sign_thuc; dau_pid=-1;
 end
 out_dir=thu_muc_goc+"/tuning_hardware_"+ten_log+"/"+ten_ho_so;
 if ~isdir(out_dir) then mkdir(out_dir); end
 mprintf("\n=== HO SO %s ===\n",ten_ho_so);
 mprintf("PWM+ TIEN va raw pitch am khi nghieng truoc da xac nhan; raw_acc=%d; acc_mult=%d gyro_mult=%d PID_mult=%d\n", ..
  imu_acc_sign_thuc,dau_acc,dau_gyro,dau_pid);
 if ho_so==2 then mprintf("QUY UOC: pitch tien duong, gyroY giu dau, PID output doi dau; dau xac thuc=%d.\n",bool2s(dau_da_xac_thuc)); end
Ts=0.005;
if ~isdef("sim_nsub") then sim_nsub=32; end
S=struct("th0",theta_eq+5*deg,"trim",theta_eq/deg,"mode",1,"xt",0,"vt",0, ..
 "Ts",Ts,"nsub",sim_nsub,"ndelay",0,"tend",8,"tp",2,"pulses",[], ..
 "wp",0.01,"Fp",10,"tilt_max",8,"DB_L",PWM_bua_L,"DB_R",PWM_bua_R, ..
 "ppr",encoder_ppr,"enc_beta",0.75,"imu_alpha",0.98,"xmax",0.10, ..
 "xendmax",0.02,"vendmax",0.03,"rungmax",5.0, ..
 "gyro_sign",dau_gyro,"motor_sign",motor_sign_thuc,"imu_phase",0,"gyro_bias",imu_bias_residual, ..
 "raw_acc_sign",imu_acc_sign_thuc,"acc_mult",dau_acc,"pid_sign",dau_pid, ..
 "acc_fc",40.5,"gyr_fc",39.9,"latency",0.00035,"quant_sensor",%t, ..
 "imu_odr",100,"acc_gain",[1;1],"gyro_gain",1,"acc_bias_g",[0;0]);
S.angle_band=.3;S.objective_profile="strict_stage1";
if isdef("balance_objective") then
 S.objective_profile=balance_objective.name;S.angle_band=balance_objective.angle_band;
 S.xmax=balance_objective.xmax;S.xendmax=balance_objective.xendmax;
 S.vendmax=balance_objective.vendmax;S.rungmax=balance_objective.rungmax;
 S.drift_rate_max=balance_objective.drift_rate_max;S.drift_window=balance_objective.drift_window;
 mprintf("CURRENT OBJECTIVE %s: travel<=%.1fcm, settled pitch<=%.1fdeg, final-window net drift<=%.1fmm/s.\n", ..
  S.objective_profile,100*S.xmax,S.angle_band,1000*S.drift_rate_max);
end
mputl(S.objective_profile,out_dir+"/objective_profile.txt");
csvWrite([S.angle_band,S.xmax,S.xendmax,S.vendmax,S.rungmax],out_dir+"/objective_limits.csv");
if co_measurement_plan then
 S.actual_ppr=matrix(measurement_ppr,-1,1);S.DB_direction=measurement_db;
 S.gyro_adc_bias=hw_bias_y;
 if measurement_angle(6)==1 then S.angle_model=measurement_angle;end
 // Diagnostic conditional profile: applying XYZ correction and ruler-derived
 // installed zero in future firmware. It is NOT a claim of completed calibration.
 if ~isdef("use_provisional_accel_calibration") then use_provisional_accel_calibration=%t;end
 if co_faces_retry & use_provisional_accel_calibration then
  ac=measurement_accel_candidate;
  S.accel_calibration=[ac([1 3]);ac([4 6])];
  S.angle_model=measurement_angle_candidate;
  S.angle_zero_removed_raw_deg=S.angle_model(2);S.angle_model(2)=0;
  mprintf("CONDITIONAL calibrated profile: XYZ + raw installed-zero %.6fdeg removed, gain %.9f, residual +/-%.6fdeg.\n", ..
   S.angle_zero_removed_raw_deg,S.angle_model(1),S.angle_model(3));
  mprintf("This calibration loader does not change firmware; notStill/fixture/reference uncertainty remains.\n");
 end
end
if isfield(S,"angle_model") then csvWrite(S.angle_model,out_dir+"/sensor_angle_model_used.csv");end
if isfield(S,"accel_calibration") then csvWrite(S.accel_calibration,out_dir+"/sensor_xyz_calibration_used.csv");end
csvWrite([100,40.5,39.9,4096,16.4,encoder_ppr,tau_dinh_muc,tau_dc, ..
 280,rpm_co_tai,I_khong_tai,I_tai_max],out_dir+"/datasheet_constraints.csv");
mputl(["imu_odr_Hz";"acc_fc_Hz";"gyro_fc_Hz";"acc_LSB_per_g"; ..
 "gyro_LSB_per_dps";"encoder_ticks_per_wheel_rev";"rated_torque_Nm_per_motor"; ..
 "max_torque_unverified_stall_Nm_per_motor";"no_load_rpm_12V"; ..
 "loaded_rpm_unspecified_test";"no_load_current_A";"max_load_current_A_not_verified_stall"], ..
 out_dir+"/datasheet_constraints_columns.txt");
S=nhieu_co_dinh(S,1);
csvWrite([S.Ts,S.nsub,S.ppr,S.enc_beta,S.imu_alpha,S.DB_L,S.DB_R,P.solver, ..
 P.Fmax,P.tau,P.Tstat,P.Tkin],out_dir+"/cau_hinh_mo_phong.csv");
// Don vi: Kp1 PWM/do, Kd1 PWM/(do/s), Kp2 do/(m/s), Ki2 do/m, Kpos 1/s.
// Targeted Scilab search for the corrected measurements, plus two legacy baselines.
// These are candidates; no gain is treated as physically validated.
Ks=[54.1 4.02 4 .8 .6;126.6 9.66 2.5 .8 .6; ..
336.44934 17.61384 31.88188 1.18956 .6; ..
266.28137 12.54312 27.48864 .11362 .6; ..
309.37485 15.42723 26.77523 .96581 .6; ..
274.81223 13.02288 26.81177 .29842 .6];
for kp=[280 315 350]
 for kd=[14 18 22]
  for kv=[24 30]
   for ki=[0.15 0.6]
    Ks=[Ks;kp kd kv ki .6];
   end
  end
 end
end
// $PID hien chi truyen4 gains; firmware DEFAULT_KP_POSITION=0.6.
if co_measurement_plan & isfile(simulation_root+"config/pid_candidates_20261010.csv") then
 Ks=csvRead(simulation_root+"config/pid_candidates_20261010.csv",",");
end
long_priority=%f;
if co_measurement_plan & isfile(simulation_root+"config/pid_candidates_65s_priority.csv") then
 Ks=csvRead(simulation_root+"config/pid_candidates_65s_priority.csv",",");long_priority=%t;
end
// Mac dinh khoa Kpos de khong tu can them thay doi firmware vong vi tri.
Ks(:,5)=.6;
// Tuy chon kiem lai mot/danh sach bo: gan K_retest=[kp kd kv ki kpos]
// truoc Execute. Tu xoa override sau khi doc; lan Execute sau lai quet du.
chay_day_du=%t;
if isdef("K_retest") then
 if size(K_retest,"*")>0 then Ks=K_retest; chay_day_du=%f; end
 clear K_retest;
end
// R: 5 gains,cost,t_end,xpeak,xend,v_tail,ess_tail,rung,pass8,maxRealPole.
R=[]; mprintf("\n=== QUET %d BO PID: DUNG YEN 8s TU GOC 5do ===\n",size(Ks,1));
for i=1:size(Ks,1)
 K=Ks(i,:); [pp,CC]=cuc_day_du(P,K,S,kf);
 [T,Y,F]=mo_phong(P,K,S,kf);
 [J,Fpk,thpk,ts,nga,rung,Jv,xpk,xend,vend]=danh_gia(T,Y,F,S);
 [ess,settle]=chi_so_goc(T,Y,S); pass=dat_vi_tri(T,Y,F,S);
 cost=(xpk/S.xmax)^2+(xend/S.xendmax)^2+(vend/S.vendmax)^2+(ess/S.angle_band)^2+(rung/S.rungmax)^2;
 // Neu nga, xep theo thoi gian song; khong cho moi bo cung cost=1e12.
 if nga | isinf(cost) | isnan(cost) then cost=1e6+1e4*(S.tend-T($)); end
 R=[R;K,cost,T($),xpk,xend,vend,ess,rung,bool2s(pass),max(real(pp))];
 if modulo(i,10)==0 then mprintf("Da quet %d/%d\n",i,size(Ks,1)); end
end
if chay_day_du then csvWrite(R,out_dir+"/tuning_gd1_quyet_dinh.csv");
else csvWrite(R,out_dir+"/tuning_gd1_kiem_lai.csv"); end
if co_log then
    ib=find(abs(R(:,1)-54.1)<1e-6 & abs(R(:,2)-4.02)<1e-6);
    if size(ib,"*")>0 then
        if R(ib(1),7)<8 then
            mprintf("CANH BAO DOI CHIEU: PID header log khong song du8s trong mo hinh; khong du co so goi model da khop xe.\n");
        end
    end
end
pool=find(R(:,13)==1); if size(pool,"*")==0 then pool=(1:size(R,1))'; end
[vv,ord]=gsort(-R(pool,6)); pool=pool(ord);
// Long-run search has stronger evidence than an8s final-second score.
// Preserve its priority so a candidate passing65s is not silently skipped.
if long_priority & chay_day_du then
 pool=(1:size(R,1))';mprintf("TOP6 priority from full65s refinement; short8s scores remain recorded.\n");
end
mprintf("Qua bai dung yen 8s: %d/%d\n",sum(R(:,13)),size(R,1));

// 4. TOP 6: 65s KHONG DAY + 20s/3 DAY. Chon uu tien dung yen that.
S65=S; S65.tend=65; S65.pulses=[]; S65=nhieu_co_dinh(S65,65);
Sp=S; Sp.tend=20; Sp.pulses=[5 10 15]; Sp=nhieu_co_dinh(Sp,20);
L=[]; best=pool(1); bestcost=%inf; dat65=%f;
for it=1:min(6,size(pool,"*"))
 i=pool(it); K=R(i,1:5);
 [Ta,Ya,Fa,Ua,Ea]=mo_phong(P,K,S65,kf); pa=dat_vi_tri(Ta,Ya,Fa,S65);
 [Ja,Fpa,tha,tsa,nga,ra,Jva,xa,xea,va]=danh_gia(Ta,Ya,Fa,S65);
 [ea,sa]=chi_so_goc(Ta,Ya,S65);
 Sref=S65;Sref.nsub=2*S65.nsub;
 [Tar,Yar,Far]=mo_phong(P,K,Sref,kf);par=dat_vi_tri(Tar,Yar,Far,Sref);
 [ear,tmp]=chi_so_goc(Tar,Yar,Sref);
 nr=min(length(Ta),length(Tar));dxr=max(abs(Ya(1,1:nr)-Yar(1,1:nr)));
 dthr=max(abs(Ya(3,1:nr)-Yar(3,1:nr)))/deg;
 phr=Ta($)>=65 & Tar($)>=65 & dxr<=.005 & dthr<=.2;
 [Tb,Yb,Fb,Ub]=mo_phong(P,K,Sp,kf); [eb,sb]=chi_so_goc(Tb,Yb,Sp);
 [Jb,Fpb,thb,tsb,ngb,rb,Jvb,xb,xeb,vb]=danh_gia(Tb,Yb,Fb,Sp);
 pb=~ngb & and(sb<0.8); cp=(xb/.1)^2+(xeb/.02)^2+(eb/.3)^2+(rb/5)^2+sum((min(sb,20)/.8).^2);
 if S.objective_profile=="bounded_balance" then
  cp=(xa/S.xmax)^2+(xea/S.xendmax)^2+(va/S.vendmax)^2+(max(ea,ear)/S.angle_band)^2+(ra/S.rungmax)^2;
 end
 if isinf(cp) | isnan(cp) then cp=1e6+(Sp.tend-Tb($))*1e4; end
 // Dau tien chon bo dat dung yen65; neu cung dat, uu tien chong day.
 // If standing is unverified, rank by worst-angle and convergence error first.
 if ~pa | ~par | ~phr then
  cp=1e8+(max(ea,ear)/S.angle_band)^2+(dxr/.005)^2+(dthr/.2)^2;
  if isinf(cp) | isnan(cp) then cp=1e10; end
 end
 if cp<bestcost then
  best=i; bestcost=cp;
  Tbest65=Ta;Ybest65=Ya;Fbest65=Fa;Ubest65=Ua;Ebest65=Ea;
  Tbest65fine=Tar;Ybest65fine=Yar;Fbest65fine=Far;
  Tbestp=Tb;Ybestp=Yb;Fbestp=Fb;Ubestp=Ub;
 end
 L=[L;i,bool2s(pa),Ta($),xa,xea,ea,va,ra,bool2s(pb),xb,sb,cp,bool2s(par),ear,dxr,dthr,bool2s(phr)];
 mprintf("TOP %d: 65s=%d/%d H=%d xpk=%.2fcm ess=%.3f/%.3fdo; day ts=%s s\n", ..
  it,bool2s(pa),bool2s(par),bool2s(phr),xa*100,ea,ear,strcat(string(sb)," / "));
end
if chay_day_du then csvWrite(L,out_dir+"/tuning_gd1_top6.csv");
else csvWrite(L,out_dir+"/tuning_gd1_top_kiem_lai.csv"); end
K=R(best,1:5); Kp1=K(1); Kd1=K(2); Kp2=K(3); Ki2=K(4); Kp_pos=K(5);
[T,Y,F,U]=mo_phong(P,K,S,kf);
T65=Tbest65;Y65=Ybest65;F65=Fbest65;U65=Ubest65;E65=Ebest65;
dat65=dat_vi_tri(T65,Y65,F65,S65);
Tp=Tbestp;Yp=Ybestp;Fp=Fbestp;Up=Ubestp;[essp,tsp]=chi_so_goc(Tp,Yp,Sp);
[J,Fpk,thpk,ts,ngp,rp,Jv,xpp,xep,vp]=danh_gia(Tp,Yp,Fp,Sp);
dat_day=~ngp & and(tsp<0.8) & xpp<Sp.xmax & xep<=Sp.xendmax & vp<=Sp.vendmax & rp<=Sp.rungmax;
[J,Fpk,thpk,ts,nga,rung,Jv,xpk,xend,vend]=danh_gia(T65,Y65,F65,S65);
[ess65,tmp]=chi_so_goc(T65,Y65,S65);
mprintf("\n=== UNG VIEN MO HINH: Kp1=%.5f Kd1=%.5f Kp2=%.5f Ki2=%.5f Kpos=%.5f ===\n",K);
if abs(K(5)-.6)>1e-9 then mprintf("Kpos KHAC firmware0.6: can doi DEFAULT_KP_POSITION; $PID khong truyen Kpos.\n"); end
mprintf("65s: thoi gian THAT %.3fs, xpk %.3fcm, xcuoi %.3fcm, vtail %.4fm/s, ess %.4fdo, rung %.3fdo/s; DAT=%d\n", ..
 T65($),100*xpk,100*xend,vend,ess65,rung,bool2s(dat65));
mprintf("ess65 la MAX |theta-trim| tren TOAN BO 60s cuoi (5..65s), khong chi 1s cuoi.\n");
mprintf("3 day 10N/10ms: ts=%s s (band +/-%.2fdo, <0.8s); DAT=%d; xpk %.3fcm\n", ..
 strcat(string(tsp)," / "),Sp.angle_band,bool2s(dat_day),xpp*100);

// Kiem tra bat dinh: khong tu thay doi thong so danh nghia de duoc DAT.
ca_ten=["Danh nghia";"Than nang +20%";"l/I cao +20%";"l/I thap -20%";"Motor cham x2"; ..
 "Torque con 50%";"Pin12.6V";"Pha IMU+5ms";"Gyro bias0.5dps";"Tre PWM1ms";"Stress gyro log20Hz"; ..
 "Speed slope -10%";"Speed slope +10%";"IMU sau motor";"Friction con50%";"Friction150%"; ..
 "Gyro gain min DS";"Gyro gain max DS";"Accel Xmin Zmax DS";"Accel Xmax Zmin DS"; ..
 "ODR99Hz";"ODR101Hz";"Cap torque rated";"Accel offset +40mg";"Accel offset -40mg"; ..
 "Old controller PPR937";"Current DB240";"Accel residual +10mg";"Accel residual -10mg";"Torque tau5ms"; ..
 "Ruler fit residual +";"Ruler fit residual -"];
ca_hs=ones(size(ca_ten,"*"),3);
ca_hs(2,:)=[1.2 1 1];ca_hs(3,:)=[1 1.2 1];ca_hs(4,:)=[1 .8 1];ca_hs(5,:)=[1 1 2];
KQ_ben=list(); dat_ben=%t; R_ben=[];
for c=1:size(ca_hs,1)
 Pc=sai_lech(P,ca_hs(c,:)); Sc=S;
 if c==6 then Pc.Fstall=P.Fstall*.5; Pc.Fmax=P.Fmax*.5; Pc.Tstat=P.Tstat*.5; Pc.Tkin=P.Tkin*.5; end
 if c==7 then Pc.voltage=12.6/12; Pc.Fmax=Pc.Fstall*Pc.voltage; end
 if c==8 then Sc.imu_phase=1; end
 if c==9 then Sc.gyro_bias=.5; end
 if c==10 then Sc.latency=.001; end
 if c==12 then Pc.vnl=.9*P.vnl; Pc.speed_scale=.9; end
 if c==13 then Pc.vnl=1.1*P.vnl; Pc.speed_scale=1.1; end
 if c==14 then
  Sc.post_noise=%t; Sc=nhieu_co_dinh(Sc,1);
  if isdef("imu_post_gyro") then Sc.gyro_bias=mean(imu_post_gyro)-hw_bias_y; end
 end
 if c==15 then Pc.Tstat=.5*P.Tstat; Pc.Tkin=.5*P.Tkin; end
 if c==16 then Pc.Tstat=1.5*P.Tstat; Pc.Tkin=1.5*P.Tkin; end
 if c==17 then Sc.gyro_gain=15.9/16.4;end
 if c==18 then Sc.gyro_gain=16.9/16.4;end
 if c==19 then Sc.acc_gain=[3932;4260]/4096;end
 if c==20 then Sc.acc_gain=[4260;3932]/4096;end
 if c==21 then Sc.imu_odr=99;end
 if c==22 then Sc.imu_odr=101;end
 // Conservative capability case, NOT a measured current limiter or thermal model.
 if c==23 then Pc.Fmax=so_dc*tau_dinh_muc/P.r;end
 // Board-level offset magnitude listed as typical in Table2, not a guaranteed bound.
 if c==24 then Sc.acc_bias_g=[.040;0];end
 if c==25 then Sc.acc_bias_g=[-.040;0];end
 if c==26 then Sc.ppr=937.2;end
 if c==27 then Sc.DB_L=240;Sc.DB_R=240;if isfield(Sc,"DB_direction") then Sc.DB_direction=240*ones(2,2);end;end
 if c==28 then Sc.acc_bias_g=[.010;0];end
 if c==29 then Sc.acc_bias_g=[-.010;0];end
 if c==30 then Pc.tau=.005;end
 if c==31 & isfield(Sc,"angle_model") then Sc.angle_model(2)=Sc.angle_model(2)+Sc.angle_model(3);end
 if c==32 & isfield(Sc,"angle_model") then Sc.angle_model(2)=Sc.angle_model(2)-Sc.angle_model(3);end
 // Deliberately conservative disturbance replay, NOT fitted sensor noise:
 // the measured trace contains real angular motion and aliasing at 20 Hz.
 if c==11 & co_log then
    for kk=1:length(Sc.n_g)
       tt=(kk-1)*Sc.Ts;
       jj=find(log_t<=modulo(tt,log_t($)));
       Sc.n_g(kk)=(DL(jj($),3)-mean(DL(:,3)))*deg;
    end
 end
 [Tk,Yk,Fk]=mo_phong(Pc,K,Sc,kf); KQ_ben($+1)=list(Tk,Yk);
 pc=dat_vi_tri(Tk,Yk,Fk,S); dat_ben=dat_ben & pc;
 [J,Fpk,thpk,ts,nga,rung,Jv,xk,xek,vk]=danh_gia(Tk,Yk,Fk,S); [ek,tmp]=chi_so_goc(Tk,Yk,S);
 mprintf("%-20s: t=%.2fs xpk=%.2fcm ess=%.3fdo; DAT=%d\n",ca_ten(c),Tk($),100*xk,ek,bool2s(pc));
 R_ben=[R_ben;c,Tk($),xk,xek,vk,ek,bool2s(pc)];
end
csvWrite(R_ben,out_dir+"/bat_dinh.csv");
mputl(ca_ten,out_dir+"/bat_dinh_ten_ca.txt");
// Verify the actual standstill objective too, not only the pushed trajectory.
// Same controller/noise; only halve the integration step. A coarse pass may fail here.
S65fine=S65; S65fine.nsub=2*S65.nsub;
T65fine=Tbest65fine;Y65fine=Ybest65fine;F65fine=Fbest65fine;
ncompare=min(length(T65),length(T65fine));
dx65=max(abs(Y65(1,1:ncompare)-Y65fine(1,1:ncompare)));
dth65=max(abs(Y65(3,1:ncompare)-Y65fine(3,1:ncompare)))/deg;
same_end65=abs(T65($)-T65fine($))<1e-6 & T65fine($)>=S65.tend-1e-6;
dat_h65=dx65<=.005 & dth65<=.2 & same_end65;
dat65_fine=dat_vi_tri(T65fine,Y65fine,F65fine,S65fine);
[ess_fine65,tmp]=chi_so_goc(T65fine,Y65fine,S65fine);
csvWrite([T65fine' Y65fine' F65fine'],out_dir+"/dung_yen_65s_h2.csv");
csvWrite([K,S65.nsub,S65fine.nsub,dx65,dth65,bool2s(dat_h65), ..
 bool2s(dat65_fine),T65fine($),ess_fine65],out_dir+"/hoi_tu_dung_yen_ket_luan.csv");
mprintf("Dung yen h/2: dx=%.6fmm dtheta=%.6fdo; HOI TU=%d; DAT65_FINE=%d; ess=%.6fdo\n", ..
 dx65*1000,dth65,bool2s(dat_h65),bool2s(dat65_fine),ess_fine65);
// Hoi tu tren bai co day20s, cung chuoi nhieu, cung xung chinh xac.
Sf=Sp; Sf.nsub=2*Sp.nsub; [Tf,Yf,Ff]=mo_phong(P,K,Sf,kf);
nf=min(length(Tp),length(Tf)); dx_h=max(abs(Yp(1,1:nf)-Yf(1,1:nf)));
dth_h=max(abs(Yp(3,1:nf)-Yf(3,1:nf)))/deg;
dat_h=dx_h<=.005 & dth_h<=.2 & abs(Tp($)-Tf($))<1e-6 & Tp($)>=Sp.tend-1e-6;
mprintf("h/2 (%d -> %d sub): dx=%.3fmm dtheta=%.3fdo; DAT=%d\n",Sp.nsub,Sf.nsub,dx_h*1000,dth_h,bool2s(dat_h));
csvWrite([Tp' Yp'],out_dir+"/hoi_tu_h.csv");
csvWrite([Tf' Yf'],out_dir+"/hoi_tu_h2.csv");
[p_kin,Acl]=cuc_day_du(P,K,S,kf);
mprintf("Max real cuc lien tuc GAN DUNG = %.6f (khong thay bai phi tuyen)\n",max(real(p_kin)));
dat_tong=dat65 & dat65_fine & dat_h65 & dat_day & dat_ben & dat_h;
if S.objective_profile=="bounded_balance" then
 // Unmeasured10N push and its settling benchmark remain diagnostics.
 // Retain standstill numerical convergence, uncertainty and physical validation.
 dat_tong=dat65 & dat65_fine & dat_h65 & dat_ben;
 mprintf("Bounded-balance objective:10N impulse/0.8s benchmark is diagnostic, not primary acceptance.\n");
end
// Equal-wheel pitch reduction: average torque only; cannot certify either motor temperature.
torque65=abs(Y65(5,:))*P.r/so_dc;
torque65fine=abs(Y65fine(5,:))*P.r/so_dc;
csvWrite([max(torque65),sqrt(mean(torque65.^2)),mean(bool2s(torque65>tau_dinh_muc)), ..
 max(torque65fine),sqrt(mean(torque65fine.^2)),tau_dinh_muc,tau_dc], ..
 out_dir+"/motor_torque_proxy.csv");
mputl(["peak_avg_torque_Nm";"rms_avg_torque_Nm";"fraction_above_rated"; ..
 "fine_peak_avg_torque_Nm";"fine_rms_avg_torque_Nm";"rated_Nm";"unverified_max_Nm"], ..
 out_dir+"/motor_torque_proxy_columns.txt");
mprintf("MOTOR avg-per-wheel proxy: peak %.5f RMS %.5f Nm; rated %.5f Nm. Not a thermal/current certification.\n", ..
 max(torque65),sqrt(mean(torque65.^2)),tau_dinh_muc);
// A numerical pass is not permission to deploy an unidentified physical plant.
dat_ap_dung=dat_tong & motor_da_nhan_dang & plant_da_xac_thuc & dau_da_xac_thuc & calibration_da_xac_thuc;
csvWrite([bool2s(co_measurement_plan),bool2s(calibration_da_xac_thuc),encoder_ppr, ..
 bool2s(plant_da_xac_thuc)],out_dir+"/calibration_acceptance.csv");
csvWrite([bool2s(dat65),bool2s(dat_day),bool2s(dat_ben),bool2s(dat_h), ..
          bool2s(motor_da_nhan_dang),bool2s(dat_ap_dung)],out_dir+"/gate_ap_dung_thuc.csv");
mprintf("Gate ap dung xe: motor da nhan dang=%d; du co so ap dung=%d\n",bool2s(motor_da_nhan_dang),bool2s(dat_ap_dung));
csvWrite([bool2s(log_day_du),bool2s(motor_da_nhan_dang),bool2s(plant_da_xac_thuc), ..
 bool2s(dau_da_xac_thuc),bool2s(dat_tong),bool2s(dat_ap_dung)],out_dir+"/gate_nhan_dang.csv");
csvWrite([bool2s(dat65),bool2s(dat65_fine),bool2s(dat_h65),bool2s(dat_day), ..
 bool2s(dat_ben),bool2s(dat_h),bool2s(dat_ap_dung)],out_dir+"/gate_day_du.csv");
if dat65 then mprintf("DAT MUC TIEU %s TRONG65s DANH NGHIA.\n",S.objective_profile);
else mprintf("CHUA DAT MUC TIEU %s TRONG65s.\n",S.objective_profile); end
if ~dat65_fine | ~dat_h65 then mprintf("CHUA XAC NHAN DUNG YEN: bai65s buoc tinh nho/hoi tu chua dat.\n"); end
if dat_tong then mprintf("DAT CAC BAI THU DONG LUC HOC DA MO PHONG.\n");
else mprintf("CHUA DAT TOAN BO BAI DAY/BAT DINH/HOI TU; xem tung dong tren.\n"); end
mprintf("Gate cat PWM <5ms va UART1s CAN THU FIRMWARE: script khong mo phong FSM/UART.\n");
mprintf("XAC THUC DAU=%d; PLANT DUOI TAI=%d. Torque/tau, pin tai, ma sat va truot chua do.\n", ..
    bool2s(dau_da_xac_thuc),bool2s(plant_da_xac_thuc));
csvWrite([T65' Y65' F65' U65'],out_dir+"/tuning_gd1_dung_yen_65s.csv");
csvWrite([T65' E65'],out_dir+"/imu_va_pwm_65s.csv");
csvWrite([Tp' Yp' Fp' Up'],out_dir+"/tuning_gd1_ba_lan_day.csv");
csvWrite([K, bool2s(dat65),bool2s(dat_day),bool2s(dat_ben),bool2s(dat_h), ..
 xpk,xend,vend,ess65,tsp,dx_h,dth_h],out_dir+"/tuning_gd1_ket_luan.csv");

// 5. DO THI DU THOI GIAN THAT; theta la goc THAT, khong phai theta IMU.
if do_plot then
 f1=scf(1); clf(f1); f1.figure_size=[1000 600];
 plot(1:size(R,1),log10(1+R(:,6)),"b."); plot(best,log10(1+R(best,6)),"rp");
 xgrid();xlabel("Thu tu ung vien");ylabel("log10(1+cost)");title("Quet gains (Kpos mac dinh0.6): bo nga bi phat theo thoi gian song");
 luu_hinh(f1,"hinh1_quet_pid.png",export_png,out_dir);
 f2=scf(2);clf(f2);f2.figure_size=[1100 850];
 subplot(3,1,1);plot(T65,Y65(3,:)/deg);plot([0 65],S.angle_band*[1 1],"r--");plot([0 65],-S.angle_band*[1 1],"r--");
 xgrid();ylabel("theta that(do)");title(ten_ho_so+": "+msprintf("Kp1 %.2f Kd1 %.2f Kp2 %.2f Ki2 %.3f Kpos %.3f",K));
 subplot(3,1,2);plot(T65,100*Y65(1,:),"b");plot([0 65],[10 10],"r--");plot([0 65],[-10 -10],"r--");
 xgrid();ylabel("x that(cm)");
 subplot(3,1,3);plot(T65,100*Y65(2,:),"b");xgrid();ylabel("v that(cm/s)");xlabel("t(s)");
 luu_hinh(f2,"hinh2_dap_ung.png",export_png,out_dir);
 f3=scf(3);clf(f3);f3.figure_size=[1000 800];
 subplot(3,1,1);plot(Tp,Yp(3,:)/deg);plot([0 20],S.angle_band*[1 1],"r--");plot([0 20],-S.angle_band*[1 1],"r--");
 xgrid();ylabel("theta(do)");title(ten_ho_so+": 3 day10N/10ms tai mica3 (gia dinh)");
 subplot(3,1,2);plot(Tp,100*Yp(1,:));xgrid();ylabel("x that(cm)");
 subplot(3,1,3);plot(Tp,Yp(5,:),"b");plot(Tp,Fp,"k");xgrid();ylabel("Luc(N)");xlabel("t(s)");
 legend(["Dien tu";"Truc sau ma sat"],1);luu_hinh(f3,"hinh3_so_sanh.png",export_png,out_dir);
 f4=ve_ben_vung(KQ_ben,ca_ten,ten_ho_so,export_png,out_dir);
 f7=scf(7);clf(f7);f7.figure_size=[1000 700];
 subplot(2,1,1);plot(Tp,Yp(3,:)/deg,"b");plot(Tf,Yf(3,:)/deg,"r");xgrid();ylabel("theta(do)");
 legend([msprintf("h=%.6fms",Sp.Ts/Sp.nsub*1000);msprintf("h/2=%.6fms",Sf.Ts/Sf.nsub*1000)],1);
 title(ten_ho_so+": do nhay buoc tinh, cung nhieu/xung day");
 subplot(2,1,2);plot(Tp,100*Yp(1,:),"b");plot(Tf,100*Yf(1,:),"r");xgrid();ylabel("x(cm)");xlabel("t(s)");
 luu_hinh(f7,"hinh7_hoi_tu.png",export_png,out_dir);
 f15=scf(15);clf(f15);f15.figure_size=[1000 700];
 ic65=find(T65>=5);if size(ic65,"*")==0 then ic65=1:length(T65);end
 if65=find(T65fine>=5);if size(if65,"*")==0 then if65=1:length(T65fine);end
 subplot(2,1,1);plot(T65(ic65),Y65(3,ic65)/deg,"b");plot(T65fine(if65),Y65fine(3,if65)/deg,"r");
 plot([5 65],S.angle_band*[1 1],"k--");plot([5 65],-S.angle_band*[1 1],"k--");xgrid();ylabel("theta that(do)");
 title("Dung yen 5..65s: cung PID/nhieu, giam mot nua buoc tinh");
 legend(msprintf("%d sub",S65.nsub),msprintf("%d sub",S65fine.nsub),1);
 subplot(2,1,2);plot(T65(ic65),100*Y65(1,ic65),"b");plot(T65fine(if65),100*Y65fine(1,if65),"r");
 xgrid();ylabel("x that(cm)");xlabel("t(s)");
 luu_hinh(f15,"hinh15_hoi_tu_dung_yen.png",export_png,out_dir);
 f8=scf(8);clf(f8);f8.figure_size=[1000 700];tail=find(T65>=5);
 if size(tail,"*")==0 then tail=1:length(T65); end
 subplot(2,1,1);plot(T65(tail),Y65(3,tail)/deg,"b");plot([5 65],S.angle_band*[1 1],"r--");plot([5 65],-S.angle_band*[1 1],"r--");
 xgrid();ylabel("theta that(do)");title(ten_ho_so+": dung yen60s sau khoi dong (neu hoan thanh)");
 subplot(2,1,2);plot(T65(tail),100*Y65(1,tail),"b");xgrid();ylabel("x that(cm)");xlabel("t(s)");
 luu_hinh(f8,"hinh8_dung_yen_phong_to.png",export_png,out_dir);
 f9=scf(9);clf(f9);f9.figure_size=[1000 700];
 subplot(2,1,1);plot(T65,Y65(3,:)/deg,"b");plot(T65,E65(1,:)/deg,"r");plot(T65,E65(3,:)/deg,"g");
 xgrid();ylabel("goc(do)");legend(["Goc that";"Complementary";"Accel giu100Hz"],1);title(ten_ho_so);
 subplot(2,1,2);plot(T65,Y65(4,:)/deg,"b");plot(T65,E65(2,:)/deg,"r");xgrid();ylabel("toc do goc(do/s)");xlabel("t(s)");
 legend(["omega that";"gyro dung trong PID"],1);luu_hinh(f9,"hinh9_imu.png",export_png,out_dir);
 f5=scf(5);clf(f5);f5.figure_size=[850 650];
 [hh,nh]=ve_khung(lk,ten,Y65(3,$),Y65(1,$),1,r,y_banh,day_banh,R_khop,dai_khop);
 plot([Y65(1,$)-.09 Y65(1,$)+.09],[-r -r],"k");xlabel("x(m)");ylabel("z tren truc(m)");
 title(msprintf("Khung tai t=%.2fs: theta=%.3fdo x=%.2fcm",T65($),Y65(3,$)/deg,Y65(1,$)*100));
 luu_hinh(f5,"hinh5_khung_cuoi.png",export_png,out_dir);
 if animate then
  for k=1:20:min(600,length(T))
   clf(f5);ve_khung(lk,ten,Y(3,k),Y(1,k),1,r,y_banh,day_banh,R_khop,dai_khop);
   title(msprintf("t=%.2fs theta=%.3fdo",T(k),Y(3,k)/deg));sleep(20);
  end
 end
end
end // cac ho so duoc chon: dau cu va/hoac dau chuan hoa
// Keep the caller's output root for the next Execute; profile folders must not nest.
out_dir=thu_muc_goc;
if isdef("K_retest") then K_retest=[]; end
end // ~kiem_tra_khai_bao
