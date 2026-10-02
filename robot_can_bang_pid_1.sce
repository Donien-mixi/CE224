// =====================================================================
//  ROBOT TỰ CÂN BẰNG (TWIP) — MÔ PHỎNG TUNING PID ĐÚNG THEO FIRMWARE
//
//  Mô phỏng bám sát thiết kế điều khiển trong đồ án (Core/Src/pid.c):
//
//    VÒNG NGOÀI (200Hz): PI VẬN TỐC
//        ev  = v_target - v_actual
//        I   = I + Ki2*ev*Ts ;   I = clamp(I, ±10)      (MAX_INTEGRAL_VELOCITY)
//        theta_target = clamp(Kp2*ev + I, ±8 do thuong / ±15 do dua)
//
//    VÒNG TRONG (200Hz): PD GÓC
//        e_theta = (theta_target + trim) - pitch          (do)
//        pwm     = Kp1*e_theta - Kd1*gyro_rate            (gyro: do/s)
//
//    CHẤP HÀNH:
//        out = PID_Apply_Deadband(pwm, 250, 2499)         (bu vung chet + kep bien)
//        PWM -> lực F = (out/2499)*Fmax ; dap ung dong co bac 1 (tau_m)
//
//  ***  Kết quả in ra  Kp1, Kd1, Kp2, Ki2 ĐÚNG ĐƠN VỊ FIRMWARE  ***
//      -> dán thẳng vào pid.h, KHÔNG đổi code PID trong đồ án.
//
//  Cách chạy: mở bằng SciNotes, bấm Execute (F5).
//  Chỉ sửa: BƯỚC 1 (linh kiện) và BƯỚC 1b (động cơ).
//
//  Quy ước: gốc tại TRỤC BÁNH, x hướng tới, z hướng lên; theta>0 ngả về +x.
//           pwm>0 của firmware tương ứng đẩy xe về -x (khớp dấu gain DƯƠNG
//           đang chạy đúng trên xe). Vì vậy trong mô hình dùng F = -kf*out.
//
//  CẤU HÌNH CƠ KHÍ THỰC TẾ (theo "thông số chi tiết linh kiện.txt"):
//    - 3 tấm mica 80x200x3 mm -> tầng 1, tầng 2, mái che; mỗi tầng cách 55 mm,
//      tầng 1 cách mặt đất 55 mm.
//    - Gầm tầng 1: 2 cụm động cơ GA25-370 + gá + bánh 65 mm + khớp lục giác.
//    - Tầng 1: Pin LiPo 3S 190 g (giữa) + Buck XL4015 15 g.
//    - Tầng 2: breadboard 77 g + STM32 + ESP32 + A4950 + BMI160 (2 g, giữa) + dây.
//    - Mặt dưới tầng 2: 2 cút WAGO. Trụ đồng M3 ở 4 góc mỗi tầng.
// =====================================================================
funcprot(0); clc;
if ~isdef("export_png") then export_png = %f; end   // %t: lưu đồ thị ra PNG
if ~isdef("animate")    then animate    = %t; end   // %f: bỏ hoạt hình
if ~isdef("out_dir")    then out_dir    = pwd(); end
if ~isdef("do_plot")    then do_plot    = %t; end   // %f: bỏ vẽ đồ thị (chạy kết quả nhanh)

deg = %pi/180;   // rad -> do

// ---------------------------------------------------------------------
//  Hàm tiện ích: quán tính riêng quanh trục ngang qua tâm
// ---------------------------------------------------------------------
function I = I_tam(m, dx, dz)          // tấm/hộp: dx dọc hướng đi, dz chiều dày
    I = m*(dx^2 + dz^2)/12;
endfunction
function I = I_cot(m, R, h)            // trụ đứng bán kính R, cao h
    I = m*(3*R^2 + h^2)/12;
endfunction

// =====================================================================
//  BƯỚC 1. KHAI BÁO LINH KIỆN THÂN ROBOT
//  Mỗi dòng: [khối lượng (kg), x tâm (m), z tâm (m), quán tính riêng (kg.m^2)]
//  Thêm pin, mạch, động cơ... bằng cách thêm dòng.
// =====================================================================
// ----- KÍCH THƯỚC KHUNG & VỊ TRÍ (gốc tại TRỤC BÁNH, z hướng lên) -----
// Tấm mica 80 mm (rộng, trái-phải) x 200 mm (dài, hướng đi) x 3 mm.
//   - Tầng 1 (sàn dưới): cách mặt đất 55 mm  -> z = 55 - r = 22.5 mm
//   - Tầng 2           : cách tầng 1  55 mm  -> z = 77.5 mm
//   - Mái che          : cách tầng 2  55 mm  -> z = 132.5 mm
// ĐÃ XÁC NHẬN: trục bánh chạy NGANG theo chiều rộng 80 mm (2 cụm động cơ ở 2 đầu
//   cạnh 80 mm); trục ở GIỮA chiều dài 200 mm (xe cân bằng kinh điển).
//   Mô hình phẳng (x-z) chỉ dùng x,z của từng linh kiện, KHÔNG dùng bề rộng y.
// CAO ĐỘ (đúng file thông số): tầng 1 cách mặt đất 55 mm; mỗi tầng cách 55 mm.
//   Đổi về gốc TRỤC BÁNH (bánh R = 32.5 mm): tầng1 = 22.5 mm, tầng2 = 77.5 mm,
//   mái che = 132.5 mm. Linh kiện đặt ngay mặt tầng (bề dày riêng rất nhỏ, bỏ qua).
// GIẢ ĐỊNH: linh kiện đặt cân đối (xc = 0).
ten = ["Mica sàn 1"; "Mica sàn 2"; "Mica mái che"; "Pin LiPo 3S"; "Buck XL4015"; ..
       "Breadboard"; "Linh kiện trên board (STM32/ESP32/A4950/BMI160)"; "Còi (ước lượng)"; ..
       "WAGO (cút điện)"; "Dây điện"; "2 Động cơ GA25"; "2 gá chữ động cơ"; ..
       "Cọc đồng T1 (+x)"; "Cọc đồng T1 (-x)"; "Cọc đồng T2 (+x)"; "Cọc đồng T2 (-x)"];
than = [ ..
  // [khối lượng (kg), x tâm (m), z tâm (m), quán tính riêng (kg.m^2)]
  0.0317,  0.000,  0.0225,  I_tam(0.0317, 0.20, 0.003);   // mica tầng 1 (95g/3 = 31.7g)
  0.0317,  0.000,  0.0775,  I_tam(0.0317, 0.20, 0.003);   // mica tầng 2
  0.0317,  0.000,  0.1325,  I_tam(0.0317, 0.20, 0.003);   // mica mái che
  0.1900,  0.000,  0.0225,  I_tam(0.1900, 0.08, 0.022);   // Pin LiPo 3S 190g, giữa tầng 1
  0.0150,  0.000,  0.0225,  I_tam(0.0150, 0.045, 0.012);  // Buck XL4015 15g, tầng 1
  0.0770,  0.000,  0.0775,  I_tam(0.0770, 0.16, 0.010);   // breadboard 77g, trên tầng 2
  0.0220,  0.000,  0.0775,  I_tam(0.0220, 0.06, 0.015);   // STM32 7 + ESP32 10 + A4950 3 + BMI160 2 = 22g
  0.0020,  0.000,  0.0775,  I_tam(0.0020, 0.02, 0.010);   // Còi — không có trong bảng cân, ước lượng 2g
  0.0120,  0.000,  0.0700,  I_tam(0.0120, 0.05, 0.015);   // 2 cút WAGO 12g, ngay dưới tầng 2
  0.0150,  0.000,  0.0600,  I_tam(0.0150, 0.10, 0.020);   // dây điện 15g (rải rác)
  0.2000,  0.000,  0.0000,  I_tam(0.2000, 0.05, 0.025);   // 2 động cơ GA25 200g, ngang mức trục bánh
  0.0200,  0.000,  0.0100,  I_tam(0.0200, 0.04, 0.030);   // 2 gá chữ 20g, sát trục bánh
  0.0120,  0.090,  0.0500,  I_cot(0.0120, 0.003, 0.055);  // 2 cọc đồng T1 (mỗi cọc 6g) nối lên tầng 2
  0.0120, -0.090,  0.0500,  I_cot(0.0120, 0.003, 0.055);
  0.0120,  0.090,  0.1050,  I_cot(0.0120, 0.003, 0.055);  // 2 cọc đồng T2 nối lên mái che
  0.0120, -0.090,  0.1050,  I_cot(0.0120, 0.003, 0.055)
];

// ----- PHẦN QUAY (bánh + khớp lục giác): nằm ở Meq, KHÔNG tính vào thân -----
r      = 0.0325;   // bán kính bánh 65 mm
m_banh = 0.050;    // 1 bên: 35g bánh + 15g khớp lục giác = 50g
m_truc = 0.0;      // không có trục xuyên suốt (mỗi bánh 1 trục motor riêng)
H_ve   = 0.135;    // chiều cao thân từ trục bánh đến mái che (chỉ để vẽ)

// ----- Môi trường -----
g    = 9.81;       // m/s^2
bx   = 0.01;       // ma sát lăn (N.s/m)
d_th = 0.001;      // cản khớp xoay (N.m.s/rad)

// =====================================================================
//  BƯỚC 1b. ĐỘNG CƠ GA25-370 12V 280RPM (encoder Hall AB) — hshop.vn
//  Thông số: 12V; hộp số 21.3:1; 280 RPM (không tải) / 215 RPM (có tải);
//            dòng 0.07A (không tải) / 1.8A (tối đa); mo-men định mức 0.4 kg.cm;
//            mo-men tối đa 2 kg.cm; dài hộp số 19 mm; encoder 11 xung/kênh/vòng.
//  => ENCODER_PPR firmware = 11 * 21.3 * 4 ~ 937 xung/vòng bánh (KHÔNG phải 1320).
// =====================================================================
tau_dc = 0.1961;   // mo-men TỐI ĐA mỗi động cơ sau hộp số: 2 kg.cm = 0.1961 N.m
so_dc  = 2;
tau_m  = 0.020;    // thời hằng đáp ứng lực (ước lượng ~ J.w0/tau_stall)
Fmax   = so_dc*tau_dc/r;
kf     = Fmax/2499;   // N trên 1 đơn vị PWM (đúng thang firmware 0..2499)

// ---------------------------------------------------------------------
//  Tính thông số tổng hợp
// ---------------------------------------------------------------------
mi = than(:,1);
m  = sum(mi);
l  = sum(mi.*than(:,3))/m;                 // chiều cao trọng tâm
xc = sum(mi.*than(:,2))/m;                 // lệch trọng tâm theo x
I_truc = sum(than(:,4) + mi.*(than(:,2).^2 + than(:,3).^2));
I  = I_truc - m*(l^2 + xc^2);              // quán tính quanh trọng tâm
Iw = 0.5*m_banh*r^2;                       // 1 bánh (đĩa đặc)
M  = 2*m_banh + m_truc;
Meq = M + 2*Iw/r^2;                        // khối lượng tương đương của xe

P = struct("m",m, "l",l, "I",I, "Meq",Meq, "g",g, "bx",bx, "dth",d_th, ..
           "Fmax",Fmax, "tau",tau_m);

mprintf("\n=====================================================\n");
mprintf(" BƯỚC 1. THÔNG SỐ TÍNH TỪ LINH KIỆN\n");
mprintf("=====================================================\n");
mprintf(" %-8s %8s %8s %8s\n", "Linh kiện", "m (kg)", "x (m)", "z (m)");
for k = 1:size(than,1)
    mprintf(" %-8s %8.4f %8.3f %8.3f\n", ten(k), than(k,1), than(k,2), than(k,3));
end
mprintf(" -----------------------------------------------\n");
mprintf(" Khối lượng thân      m   = %.3f kg\n", m);
mprintf(" Chiều cao trọng tâm  l   = %.4f m\n", l);
mprintf(" Quán tính quanh t.tâm I  = %.5f kg.m^2\n", I);
mprintf(" Khối lượng xe        M   = %.3f kg  (tương đương %.3f kg)\n", M, Meq);
mprintf(" Lực động cơ tối đa  Fmax = %.1f N   (kf = %.5f N/PWM)\n", Fmax, kf);
if abs(xc) > 1e-6 then
    mprintf(" !! Trọng tâm lệch %.1f mm theo x: robot sẽ đứng nghiêng %.2f độ\n", ..
            xc*1000, -atan(xc/l)*180/%pi);
end

// =====================================================================
//  BƯỚC 2. MÔ HÌNH TUYẾN TÍNH HÓA
// =====================================================================
function [A, B, al, be] = mo_hinh(P)
    Mm = [P.Meq+P.m, P.m*P.l; P.m*P.l, P.I+P.m*P.l^2];
    Mi = inv(Mm);
    A = zeros(4,4); B = zeros(4,1);
    A(1,2) = 1; A(3,4) = 1;
    A([2 4],[1 3]) = Mi*[0 0; 0 P.m*P.g*P.l];
    A([2 4],[2 4]) = Mi*[-P.bx 0; 0 -P.dth];
    B([2 4]) = Mi*[1; 0];
    al = A(4,3);  be = -B(4);
endfunction

[A, B, al, be] = mo_hinh(P);
p_ho = spec(A);
mprintf("\n=====================================================\n");
mprintf(" BƯỚC 2. MÔ HÌNH   theta'' = %.1f*theta - %.3f*F\n", al, be);
mprintf("=====================================================\n");
mprintf(" Cực hệ hở (1 cực dương -> robot ngã, thời hằng %.0f ms)\n", 1000/max(real(p_ho)));
mprintf(" Điều kiện tối thiểu: Kp1 > %.0f  (PWM/do)\n", al*deg/(be*kf));

// =====================================================================
//  HÀM ĐIỀU KHIỂN ĐÚNG THEO FIRMWARE
// =====================================================================

// Nội suy Kp1, Kd1 cho vòng trong PD theo (wn, zeta) đặt cực:
//   char:  s^2 + 2*zeta*wn*s + wn^2   (đơn vị rad)
function [Kp1, Kd1] = pid_goc(wn, zeta, al, be, kf)
    Kp1 = (wn^2 + al)*deg/(be*kf);
    Kd1 = 2*zeta*wn*deg/(be*kf);
endfunction

// PID_Apply_Deadband(pwm, 250, 2499)  — y hệt pid.c
function out = pid_deadband(pwm, deadband, max_pwm)
    if pwm > 1 then
        out = min(pwm + deadband, max_pwm);
    elseif pwm < -1 then
        out = max(pwm - deadband, -max_pwm);
    else
        out = 0;
    end
endfunction

function yd = f_robot(y, Fc, Fd, P)
    // y = [x; x'; theta; theta'; lực thực Fa]  (Fc: lực đang ra, Fd: nhiễu)
    xd = y(2); th = y(3); thd = y(4); Fa = y(5);
    c = cos(th); s = sin(th);
    a11 = P.Meq + P.m;  a12 = P.m*P.l*c;  a22 = P.I + P.m*P.l^2;
    r1 = Fa + Fd - P.bx*xd + P.m*P.l*thd^2*s;
    r2 = P.m*P.g*P.l*s - P.dth*thd + Fd*P.l*c;
    dt_ = a11*a22 - a12^2;
    yd = [xd; (a22*r1 - a12*r2)/dt_; thd; (a11*r2 - a12*r1)/dt_; (Fc - Fa)/P.tau];
endfunction

// Mô phỏng bám sát vi điều khiển thật:
//  - Cascade: vòng ngoài PI vận tốc -> theta_target (độ)
//            vòng trong PD góc -> PWM ; deadband 250 ; kẹp ±2499
//  - 200 Hz, ZOH, trễ 1 chu kỳ, nhiễu IMU/encoder, bão hoà lực, chống windup
//  K = [Kp1 Kd1 Kp2 Ki2]  (đơn vị firmware)
function [T, Y, F] = mo_phong(P, K, S, kf)
    N = round(S.tend/S.Ts);  h = S.Ts/S.nsub;
    y = [0; 0; S.th0; 0; 0];  Ii = 0;  u_ra = 0;
    T = zeros(1, N+1);  Y = zeros(5, N+1);  F = zeros(1, N+1);
    Y(:,1) = y;  n = N + 1;
    for k = 1:N
        t = (k-1)*S.Ts;
        // --- Đo lường (có nhiễu) ---
        th_m   = y(3) + S.n_th(k);      // rad
        thd_m  = y(4) + S.n_g(k);       // rad/s
        v_m    = y(2) + S.n_v(k);       // m/s (encoder)

        // --- VÒNG NGOÀI: PI vận tốc ---
        ev = S.vt - v_m;
        Ii = Ii + K(4)*ev*S.Ts;
        Ii = max(min(Ii, 10), -10);              // MAX_INTEGRAL_VELOCITY
        theta_t = K(3)*ev + Ii;                  // do
        theta_t = max(min(theta_t, S.tilt_max), -S.tilt_max);

        // --- VÒNG TRONG: PD góc ---
        eth = (theta_t + S.trim) - th_m/deg;     // do
        pwm = K(1)*eth - K(2)*(thd_m/deg);       // PWM

        // --- Bù vùng chết + kẹp biên + quy đổi ra lực ---
        out = pid_deadband(pwm, 250, 2499);
        Fc  = -kf*out;

        // --- Trễ 1 chu kỳ ---
        uc = u_ra;  u_ra = Fc;

        // --- Tích phân cơ hệ bằng RK4 ---
        for j = 1:S.nsub
            tj = t + (j-1)*h;
            Fd = 0; if tj >= S.tp & tj < S.tp + S.wp then Fd = S.Fp; end
            k1 = f_robot(y, uc, Fd, P);
            k2 = f_robot(y + h/2*k1, uc, Fd, P);
            k3 = f_robot(y + h/2*k2, uc, Fd, P);
            k4 = f_robot(y + h*k3, uc, Fd, P);
            y = y + h/6*(k1 + 2*k2 + 2*k3 + k4);
        end
        T(k+1) = k*S.Ts;  Y(:,k+1) = y;  F(k+1) = uc;
        if abs(y(3)) > %pi/2 then n = k + 1; break; end   // đã ngã
    end
    T = T(1:n);  Y = Y(:,1:n);  F = F(1:n);
endfunction

function [J, Fpk, thpk, ts, nga, rung, Jv] = danh_gia(T, Y, F, S)
    th = Y(3, :);
    // Làm mượt lực (trung bình trượt ~0.1s) để đo "rung" TẦN SỐ THẤP.
    // Việc này bỏ qua chattering tần số cao do bù deadband 250 gây ra
    // (đó là đặc tính của firmware, không phải mất ổn định).
    Ws = 20;  Fs = F;  acc = 0;
    for k = 1:length(F)
        acc = acc + F(k);
        if k > Ws then acc = acc - F(k-Ws); end
        Fs(k) = acc/min(k, Ws);
    end
    iw = find(T >= S.tp - 0.5 & T < S.tp);
    if iw == [] then rung = %inf; else rung = stdev(Fs(iw)); end
    J  = inttrap(T, T.*abs(th));                       // ITAE góc
    Jv = inttrap(T, T.*abs(S.vt - Y(2,:)));            // ITAE vận tốc
    Fpk = max(abs(F));  thpk = max(abs(th));
    idx = find(T < S.tp & abs(th) > 0.5*%pi/180);
    if idx == [] then ts = 0; else ts = T(max(idx)); end
    nga = thpk > %pi/4 | T($) < S.tend - 1e-6;
endfunction

// Các trường hợp sai lệch mà bộ PID PHẢI chịu được: [hệ số m, hệ số l, hệ số tau_m]
function Pk = sai_lech(P, h)
    Pk = P;  Pk.m = P.m*h(1);  Pk.l = P.l*h(2);
    Pk.I = P.I*h(1)*h(2)^2;    Pk.tau = P.tau*h(3);
endfunction

// =====================================================================
//  BƯỚC 3. QUÉT BỘ SỐ  (vòng trong theo wn,zeta ; vòng ngoài theo Kp2,Ki2)
// =====================================================================
Ts     = 0.005;                 // 200 Hz
sig_th = 0.05*%pi/180;          // nhiễu góc tĩnh sau lọc Mahony/Madgwick (rad)
sig_g  = 0.1*%pi/180;           // nhiễu gyro BMI160 bao gồm rung cơ học (rad/s)
sig_v  = 0.01;                  // nhiễu vận tốc encoder (m/s)

// Kịch bản: nghiêng sẵn 5 độ, t = 1.5 s bị huých 10 N trong 0.01 s.
// GĐ1 (giữ vị trí): vt = 0.  GĐ2 (bám tốc độ): đặt vt = 0.5 (bước).
S = struct("th0", 5*%pi/180, "tp", 1.5, "wp", 0.01, "Fp", 10, "tend", 3, ..
           "Ts", Ts, "nsub", 4, "vt", 0.0, "tilt_max", 8.0, "trim", 0.0);
rand("seed", 1);
Nn = round(S.tend/S.Ts);
S.n_th = sig_th*rand(1, Nn, "normal");
S.n_g  = sig_g*rand(1, Nn, "normal");
S.n_v  = sig_v*rand(1, Nn, "normal");

wn_list  = 8:4:32;                 // dải băng thông vòng góc: 8,12,...,32 rad/s
z_list   = [0.6 0.8 1.0];          // hệ số tắt vòng góc
Kp2_list = [1 2 3 4];              // tỉ lệ vòng vận tốc (deg/(m/s))
Ki2_list = [0.05 0.1 0.2];         // tích phân vòng vận tốc
Ntop     = 30;                     // số ứng viên nominal tốt nhất đem kiểm bền

F_du_tru = 0.8;                    // chỉ dùng tối đa 80% lực động cơ
rung_max = 0.30*Fmax;              // rung TẦN SỐ THẤP tối đa 30% Fmax (đã làm mượt lực)

ca_ten = ["Danh nghĩa"; "Thân nặng +20%"; "Trọng tâm cao +20%"; ..
          "Trọng tâm thấp -20%"; "Động cơ chậm x2"];
ca_hs  = [1 1 1; 1.2 1 1; 1 1.2 1; 1 0.8 1; 1 1 2];

mprintf("\n=====================================================\n");
mprintf(" BƯỚC 3. QUÉT %d BỘ (nominal) + KIỂM BỀN %d ỨNG VIÊN\n", ..
        size(wn_list,"*")*size(z_list,"*")*size(Kp2_list,"*")*size(Ki2_list,"*"), Ntop);
mprintf("=====================================================\n");
mprintf(" Cascade PI vận tốc + PD góc, deadband 250, 200Hz, trễ 1 chu kỳ, nhiễu IMU\n");
mprintf(" Đạt khi: MỌI trường hợp không ngã, |F| <= %.0f%% Fmax, rung <= %.2f N\n", 100*F_du_tru, rung_max);
mprintf(" Chọn: bộ đạt bền vững có ITAE góc (danh nghĩa) nhỏ nhất\n");

// --- PHA A: quét nominal, xếp hạng theo ITAE ---
R = [];   // [wn zeta Kp2 Ki2 Kp1 Kd1 J Fpk thpk ts rung nga]
for z = z_list
    for wn = wn_list
        [Kp1, Kd1] = pid_goc(wn, z, al, be, kf);
        for Kp2 = Kp2_list
            for Ki2 = Ki2_list
                K = [Kp1 Kd1 Kp2 Ki2];
                [T, Y, F] = mo_phong(P, K, S, kf);
                [Jc, Fc, thc, tsc, ngac, rc, Jvc] = danh_gia(T, Y, F, S);
                R = [R; wn, z, Kp2, Ki2, Kp1, Kd1, Jc, Fc, thc, tsc, rc, bool2s(ngac)];
            end
        end
    end
end
cand = find(R(:,12) == 0 & R(:,8) <= F_du_tru*Fmax & R(:,11) <= rung_max);
if cand == [] then
    error("Không bộ nào đạt ngay ở nominal. Hãy tăng lực động cơ (tau_dc) hoặc giảm tau_m.");
end
[tmp_, ordc] = gsort(-R(cand,7));  cand = cand(ordc);   // J tăng dần
mprintf("\n Ứng viên nominal đạt: %d / %d bộ. Năm bộ tốt nhất:\n", size(cand,"*"), size(R,1));
mprintf("\n %4s %5s %6s %6s %8s %7s %8s %7s %7s\n", ..
        "wn", "zeta", "Kp2", "Ki2", "Kp1", "Kd1", "ITAE", "F max", "rung");
for k = cand(1:min(5, size(cand,"*")))'
    mprintf(" %4.0f %5.1f %6.1f %6.2f %8.0f %7.2f %8.4f %6.1fN %6.2fN\n", R(k,[1:6 7 8 11]));
end

// --- PHA B: kiểm bền các ứng viên tốt nhất, chọn bộ đầu tiên ĐẠT ---
best = -1;
for t = 1:min(Ntop, size(cand,"*"))
    k = cand(t);
    K = [R(k,5) R(k,6) R(k,3) R(k,4)];      // [Kp1 Kd1 Kp2 Ki2]
    dat = %t;
    for c = 1:size(ca_hs,1)
        [T, Y, F] = mo_phong(sai_lech(P, ca_hs(c,:)), K, S, kf);
        [Jc, Fc, thc, tsc, ngac, rc, Jvc] = danh_gia(T, Y, F, S);
        if ngac | Fc > F_du_tru*Fmax | rc > rung_max then dat = %f; break; end
    end
    if dat then best = k; break; end
end
if best < 0 then
    error("Không ứng viên nào vượt 5 trường hợp sai lệch. Tăng tau_dc, giảm tau_m, hoặc tăng Ntop.");
end
wn_b = R(best,1); z_b = R(best,2);
Kp2 = R(best,3); Ki2 = R(best,4);
Kp1 = R(best,5); Kd1 = R(best,6);
K = [Kp1 Kd1 Kp2 Ki2];
mprintf("\n Bộ ĐẠT bền vững qua %d trường hợp: wn=%g, zeta=%g, Kp2=%g, Ki2=%g\n", ..
        size(ca_hs,1), wn_b, z_b, Kp2, Ki2);

// =====================================================================
//  BƯỚC 4. KIỂM CHỨNG: cực vòng kín (mô hình liên tục) + từng sai lệch
// =====================================================================
// Trạng thái vòng kín: [x; x'; theta; theta'; Ii (tích phân vận tốc); Fa]
// (bỏ qua bão hoà/deadband/trễ để xem cực tuyến tính)
Acl = zeros(6,6);
Acl(1,2) = 1;
Acl(2,1:4) = A(2,:);  Acl(2,6) = B(2);
Acl(3,4) = 1;
Acl(4,1:4) = A(4,:);  Acl(4,6) = B(4);
Acl(5,2) = -Ki2;
Acl(6,2) =  kf*Kp1*Kp2/tau_m;
Acl(6,3) =  kf*Kp1/deg/tau_m;
Acl(6,4) =  kf*Kd1/deg/tau_m;
Acl(6,5) = -kf*Kp1/tau_m;
Acl(6,6) = -1/tau_m;
p_kin = spec(Acl);
mprintf("\n=====================================================\n");
mprintf(" BƯỚC 4. KIỂM CHỨNG BỘ SỐ ĐƯỢC CHỌN\n");
mprintf("=====================================================\n");
mprintf(" Cực vòng kín (phần thực): %s\n", strcat(msprintf("%.2f\n", real(p_kin)), "  "));
mprintf(" (1 cực ~0 thuộc vị trí x: vòng vận tốc không giữ vị trí nên xe trôi.)\n");
KQ_ben = list();
for c = 1:size(ca_hs,1)
    [Tk, Yk, Fk] = mo_phong(sai_lech(P, ca_hs(c,:)), K, S, kf);
    [Jk, Fpk_k, thk, tsk, ngak, rk, Jvk] = danh_gia(Tk, Yk, Fk, S);
    KQ_ben($+1) = list(Tk, Yk);
    kq = "đứng vững"; if ngak then kq = "NGÃ"; end
    mprintf(" %-22s góc max %5.2f độ, lực max %5.1f N, rung %4.2f N -> %s\n", ..
            ca_ten(c), thk*180/%pi, Fpk_k, rk, kq);
end

mprintf("\n=====================================================\n");
mprintf(" KẾT QUẢ: BỘ PID FIRMWARE ĐỀ XUẤT\n");
mprintf("=====================================================\n");
mprintf(" Kp1 = %.1f;  Kd1 = %.2f;   // vòng góc PD (pid.h)\n", Kp1, Kd1);
mprintf(" Kp2 = %.2f;  Ki2 = %.3f;  // vòng vận tốc PI (pid.h)\n", Kp2, Ki2);
mprintf(" (Dán trực tiếp vào DEFAULT_KP_ANGLE / KD_ANGLE / KP_VELOCITY / KI_VELOCITY)\n\n");

// =====================================================================
//  BƯỚC 5. ĐỒ THỊ + HOẠT HÌNH
// =====================================================================
if do_plot & getscilabmode() <> "NWNI" then
    function luu(f, ten_file)
        if export_png then xs2png(f, out_dir + "/" + ten_file); end
    endfunction

    // --- Hình 1: kết quả quét (ITAE góc theo wn) ---
    f1 = scf(1); clf(f1); f1.figure_size = [900 520];
    mau = ["blue", "green", "magenta", "orange", "black"];
    hh = []; nh = [];
    for i = 1:size(z_list,"*")
        k = find(R(:,2) == z_list(i));
        plot(R(k,1), R(k,7));
        e = gce(); e = e.children(1); e.foreground = color(mau(i));
        e.thickness = 2; e.mark_style = 9; e.mark_foreground = color(mau(i));
        hh = [hh; e]; nh = [nh; "zeta = " + string(z_list(i))];
    end
    isok = zeros(size(R,1),1); isok(cand) = 1;   // 1 = đạt nominal
    kx = find(isok == 0);
    if kx <> [] then
        plot(R(kx,1), R(kx,7), "rx"); e = gce(); e = e.children(1);
        e.mark_size = 8; hh = [hh; e]; nh = [nh; "Không đạt"];
    end
    plot(wn_b, R(best,7), "rp"); e = gce(); e = e.children(1); e.mark_size = 16;
    hh = [hh; e]; nh = [nh; "Được chọn"];
    a1 = gca(); a1.log_flags = "nln";
    legend(hh, nh, 2);
    xgrid(); xlabel("wn vòng góc (rad/s)"); ylabel("ITAE góc (nhỏ = tốt, log)");
    title(msprintf("Bước 3: chọn wn=%g, zeta=%g, Kp2=%g, Ki2=%g", wn_b, z_b, Kp2, Ki2));
    luu(f1, "hinh1_quet_pid.png");

    // --- Hình 2: đáp ứng của bộ chọn ---
    [T, Y, F] = mo_phong(P, K, S, kf);
    f2 = scf(2); clf(f2); f2.figure_size = [900 750];
    subplot(3,1,1); plot(T, Y(3,:)*180/%pi, "b"); xgrid();
    ylabel("theta (độ)"); title(msprintf("Kp1=%.0f Kd1=%.2f Kp2=%.2f Ki2=%.2f", Kp1, Kd1, Kp2, Ki2));
    subplot(3,1,2); plot(T, Y(1,:), "r"); xgrid(); ylabel("x (m)");
    subplot(3,1,3); plot(T, F, "k"); plot([0 S.tend], [Fmax Fmax], "r--");
    plot([0 S.tend], -[Fmax Fmax], "r--"); xgrid(); ylabel("F (N)"); xlabel("t (s)");
    luu(f2, "hinh2_dap_ung.png");

    // --- Hình 3: so sánh wn chậm / chuẩn / gắt (cùng zeta) ---
    f3 = scf(3); clf(f3); f3.figure_size = [900 520];
    so_wn = [8, wn_b, min(wn_b + 12, max(wn_list))];
    so_mau = ["green", "blue", "red"];  hh = [];
    for i = 1:3
        [Kp1i, Kd1i] = pid_goc(so_wn(i), z_b, al, be, kf);
        Ki_ = [Kp1i Kd1i Kp2 Ki2];
        [Ti, Yi, Fi] = mo_phong(P, Ki_, S, kf);
        plot(Ti, Yi(3,:)*180/%pi);
        e = gce(); e = e.children(1); e.foreground = color(so_mau(i)); e.thickness = 2;
        hh = [hh; e];
    end
    legend(hh, ["Chậm wn=" + string(so_wn(1)); "Chọn wn=" + string(wn_b); "Gắt wn=" + string(so_wn(3))], 1);
    xgrid(); xlabel("t (s)"); ylabel("theta (độ)");
    title("So sánh 3 mức wn vòng góc (cùng zeta)");
    luu(f3, "hinh3_so_sanh.png");

    // --- Hình 4: độ bền ---
    f4 = scf(4); clf(f4); f4.figure_size = [900 520];
    m4 = ["blue", "red", "magenta", "green", "black"];  hh = [];
    for k = 1:length(KQ_ben)
        plot(KQ_ben(k)(1), KQ_ben(k)(2)(3,:)*180/%pi);
        e = gce(); e = e.children(1); e.foreground = color(m4(k)); e.thickness = 2;
        hh = [hh; e];
    end
    legend(hh, ca_ten, 1); xgrid(); xlabel("t (s)"); ylabel("theta (độ)");
    title("Cùng bộ PID khi thông số robot sai lệch");
    luu(f4, "hinh4_ben_vung.png");

    // --- Hình 5: hoạt hình ---
    if animate then
        x = Y(1,:); th = Y(3,:);
        f5 = scf(5); clf(f5);
        a = gca(); a.isoview = "on";
        plot([min(x)-1, max(x)+1], [-r, -r], "k");
        xpoly([x(1), x(1)+H_ve*sin(th(1))], [0, H_ve*cos(th(1))]);
        than_ve = gce(); than_ve.thickness = 5; than_ve.foreground = color("blue");
        xarc(x(1)-r, r, 2*r, 2*r, 0, 360*64);
        banh = gce(); banh.thickness = 2;
        for k = 1:10:size(T, "*")
            than_ve.data = [x(k), 0; x(k)+H_ve*sin(th(k)), H_ve*cos(th(k))];
            banh.data = [x(k)-r, r, 2*r, 2*r, 0, 360*64];
            a.data_bounds = [x(k)-0.4, -0.05; x(k)+0.4, 0.35];
            a.title.text = msprintf("t = %.2f s   theta = %.1f độ", T(k), th(k)*180/%pi);
            sleep(20);
        end
    end
end
