// =====================================================================
//  ROBOT TỰ CÂN BẰNG: TỪ THÔNG SỐ CƠ KHÍ -> MÔ HÌNH -> TÌM PID -> MÔ PHỎNG
//
//  Cách chạy: mở file trong SciNotes, bấm Execute (F5).
//  Muốn đổi robot: chỉ sửa BƯỚC 1 (linh kiện) và BƯỚC 1b (động cơ).
//
//  Quy ước:  gốc tọa độ tại TRỤC BÁNH XE, x hướng đi tới, z hướng lên
//            theta > 0 : thân nghiêng về phía +x
//            F     > 0 : lực đẩy xe về phía +x
//            e = 0 - theta  ->  các hệ số PID mang dấu ÂM
// =====================================================================
funcprot(0); clc;
if ~isdef("export_png") then export_png = %f; end   // %t: lưu đồ thị ra PNG
if ~isdef("animate")    then animate    = %t; end   // %f: bỏ hoạt hình
if ~isdef("out_dir")    then out_dir    = pwd(); end

// ---------------------------------------------------------------------
//  Hàm tiện ích: quán tính riêng quanh trục ngang (trục bánh) qua tâm
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
ten = ["Sàn 1"; "Sàn 2"; "Sàn 3"; "Cột 1"; "Cột 2"; "Cột 3"; "Cột 4"];
than = [ ..
  0.25,    0.00, 0.000, I_tam(0.25, 0.08, 0.003);   // tấm 18x8x0.3 cm
  0.25,    0.00, 0.125, I_tam(0.25, 0.08, 0.003);
  0.25,    0.00, 0.250, I_tam(0.25, 0.08, 0.003);
  0.0625,  0.03, 0.125, I_cot(0.0625, 0.005, 0.25);  // cột R=5 mm, cao 25 cm
  0.0625,  0.03, 0.125, I_cot(0.0625, 0.005, 0.25);
  0.0625, -0.03, 0.125, I_cot(0.0625, 0.005, 0.25);
  0.0625, -0.03, 0.125, I_cot(0.0625, 0.005, 0.25)];
//  Ví dụ thêm pin 200 g đặt ở sàn 2:
//  ten = [ten; "Pin"]; than = [than; 0.2, 0, 0.14, I_tam(0.2, 0.07, 0.02)];

// Phần xe (bánh + trục)
r      = 0.0325;   // bán kính bánh (m)
m_banh = 0.05;     // khối lượng 1 bánh (kg)
m_truc = 0.1;      // trục nối 2 bánh (kg)
H_ve   = 0.25;     // chiều cao thân, chỉ dùng để vẽ (m)

// Môi trường
g    = 9.81;       // m/s^2
bx   = 0.01;       // ma sát lăn (N.s/m)
d_th = 0.001;      // cản khớp xoay (N.m.s/rad)

// =====================================================================
//  BƯỚC 1b. ĐỘNG CƠ  (thay bằng số trong datasheet của bạn)
// =====================================================================
tau_dc = 0.3;      // mô-men cực đại MỖI động cơ sau hộp số (N.m) - VÍ DỤ
so_dc  = 2;
tau_m  = 0.02;     // thời hằng đáp ứng lực của động cơ (s) - VÍ DỤ
Fmax   = so_dc*tau_dc/r;

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
mprintf(" Quán tính quanh trục     = %.5f kg.m^2\n", I_truc);
mprintf(" Quán tính quanh t.tâm I  = %.5f kg.m^2\n", I);
mprintf(" Khối lượng xe        M   = %.3f kg  (tương đương %.3f kg khi tính quán tính bánh)\n", M, Meq);
mprintf(" Lực động cơ tối đa  Fmax = %.1f N\n", Fmax);
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
mprintf(" BƯỚC 2. MÔ HÌNH  theta'''' ≈ %.1f*theta - %.3f*F\n", al, be);
mprintf("=====================================================\n");
mprintf(" Cực hệ hở: %s\n", strcat(msprintf("%.2f\n", real(p_ho)), "  "));
mprintf(" Cực dương %.2f -> không điều khiển, robot ngã với thời hằng %.0f ms\n", ..
        max(real(p_ho)), 1000/max(real(p_ho)));
mprintf(" Điều kiện tối thiểu để đứng được: |Kp| > %.1f N/rad\n", al/be);

// =====================================================================
//  BƯỚC 3. TÌM PID BẰNG ĐẶT CỰC + QUÉT + MÔ PHỎNG PHI TUYẾN
//  Cực mong muốn: (s^2 + 2*zeta*wn*s + wn^2)(s + p),  p = pr*wn
// =====================================================================
function K = pid_dat_cuc(wn, zeta, pr, al, be)
    p  = pr*wn;
    Kp = -(wn^2 + 2*zeta*wn*p + al)/be;
    Kd = -(2*zeta*wn + p)/be;
    Ki = -(wn^2*p)/be;
    K  = [Kp, Ki, Kd];
endfunction

function yd = f_robot(y, Fc, Fd, P)
    // y = [x; x'; theta; theta'; lực thực Fa]  (Fc: lệnh lực đang giữ)
    xd = y(2); th = y(3); thd = y(4); Fa = y(5);
    c = cos(th); s = sin(th);
    a11 = P.Meq + P.m;  a12 = P.m*P.l*c;  a22 = P.I + P.m*P.l^2;
    r1 = Fa + Fd - P.bx*xd + P.m*P.l*thd^2*s;
    r2 = P.m*P.g*P.l*s - P.dth*thd + Fd*P.l*c;
    dt_ = a11*a22 - a12^2;
    yd = [xd; (a22*r1 - a12*r2)/dt_; thd; (a11*r2 - a12*r1)/dt_; (Fc - Fa)/P.tau];
endfunction

// Mô phỏng như trên vi điều khiển thật:
//  - PID chạy rời rạc mỗi S.Ts giây, lệnh giữ nguyên giữa 2 lần tính (ZOH)
//  - lệnh tính xong ở chu kỳ k được xuất ra ở chu kỳ k+1 (trễ tính toán)
//  - góc đo có nhiễu S.n_th, gyro đo theta' có nhiễu S.n_g
//  - khâu I ngừng tích lũy khi lực bão hòa (chống windup)
function [T, Y, F] = mo_phong(P, K, S)
    N = round(S.tend/S.Ts);  h = S.Ts/S.nsub;
    y = [0; 0; S.th0; 0; 0];  z = 0;  u_ra = 0;
    T = zeros(1, N+1);  Y = zeros(5, N+1);  F = zeros(1, N+1);
    Y(:,1) = y;  n = N + 1;
    for k = 1:N
        t = (k-1)*S.Ts;
        th_m = y(3) + S.n_th(k);  thd_m = y(4) + S.n_g(k);
        e = -th_m;
        u = K(1)*e + K(2)*(z + e*S.Ts) - K(3)*thd_m;
        if abs(u) < P.Fmax then z = z + e*S.Ts; end
        u = max(min(u, P.Fmax), -P.Fmax);
        uc = u_ra;  u_ra = u;                       // trễ 1 chu kỳ
        for j = 1:S.nsub                            // RK4 bước h
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

function [J, Fpk, thpk, ts, nga, rung] = danh_gia(T, Y, F, S)
    th = Y(3, :);
    iw = find(T >= S.tp - 0.5 & T < S.tp);
    if iw == [] then rung = %inf; else rung = stdev(F(iw)); end
    J = inttrap(T, T.*abs(th));                    // ITAE của góc
    Fpk = max(abs(F));  thpk = max(abs(th));
    idx = find(T < S.tp & abs(th) > 0.5*%pi/180);
    if idx == [] then ts = 0; else ts = T(max(idx)); end
    nga = thpk > %pi/4 | T($) < S.tend - 1e-6;
endfunction

// Vi điều khiển và cảm biến (VÍ DỤ, sửa theo phần cứng của bạn)
Ts     = 0.005;                 // chu kỳ vòng điều khiển (s) = 200 Hz
sig_th = 0.1*%pi/180;           // nhiễu góc sau bộ lọc IMU (rad, độ lệch chuẩn)
sig_g  = 0.5*%pi/180;           // nhiễu gyro (rad/s, độ lệch chuẩn)

// Kịch bản thử: nghiêng sẵn 5 độ, đến t = 1.5 s bị huých 10 N trong 0.01 s
S = struct("th0", 5*%pi/180, "tp", 1.5, "wp", 0.01, "Fp", 10, "tend", 3, ..
           "Ts", Ts, "nsub", 5);
rand("seed", 1);                // cố định nhiễu để so sánh công bằng
Nn = round(S.tend/S.Ts);
S.n_th = sig_th*rand(1, Nn, "normal");
S.n_g  = sig_g*rand(1, Nn, "normal");
pr      = 0.2;                    // cực khâu I = 0.2*wn (chậm hơn cặp cực chính)
wn_list = 4:2:40;
z_list  = [0.5 0.6 0.7 0.8 1.0];
F_du_tru = 0.8;                   // chỉ dùng tối đa 80% lực động cơ
rung_max = 0.05*Fmax;             // lực "rung" do nhiễu tối đa 5% Fmax (động cơ không bị giật)

// Các trường hợp sai lệch mà bộ PID PHẢI chịu được: [hệ số m, hệ số l, hệ số tau_m]
ca_ten = ["Danh nghĩa"; "Thân nặng +20%"; "Trọng tâm cao +20%"; ..
          "Trọng tâm thấp -20%"; "Động cơ chậm x2"];
ca_hs  = [1 1 1; 1.2 1 1; 1 1.2 1; 1 0.8 1; 1 1 2];

function Pk = sai_lech(P, h)
    Pk = P;  Pk.m = P.m*h(1);  Pk.l = P.l*h(2);
    Pk.I = P.I*h(1)*h(2)^2;    Pk.tau = P.tau*h(3);
endfunction

mprintf("\n=====================================================\n");
mprintf(" BƯỚC 3. QUÉT %d BỘ PID x %d TRƯỜNG HỢP SAI LỆCH\n", ..
        size(wn_list,"*")*size(z_list,"*"), size(ca_hs,1));
mprintf("=====================================================\n");
mprintf(" Mỗi bộ PID: đặt cực -> mô phỏng phi tuyến (bão hòa, trễ động cơ,\n   PID rời rạc %.0f Hz, trễ 1 chu kỳ, nhiễu IMU)\n", 1/Ts);
mprintf(" Đạt khi: MỌI trường hợp không ngã, |F| <= %.0f%% Fmax, rung lực <= %.2f N\n", 100*F_du_tru, rung_max);
mprintf(" Chọn: bộ đạt có ITAE (trường hợp danh nghĩa) nhỏ nhất\n");
R = [];   // [wn zeta Kp Ki Kd J Fpk thpk ts dat rung]
for z = z_list
    for wn = wn_list
        K = pid_dat_cuc(wn, z, pr, al, be);
        dat = %t;
        for c = 1:size(ca_hs,1)
            [T, Y, F] = mo_phong(sai_lech(P, ca_hs(c,:)), K, S);
            [Jc, Fc, thc, tsc, ngac, rc] = danh_gia(T, Y, F, S);
            if c == 1 then J = Jc; Fpk = Fc; thpk = thc; ts = tsc; rung = rc; end
            if ngac | Fc > F_du_tru*Fmax | rc > rung_max then dat = %f; break; end
        end
        R = [R; wn, z, K, J, Fpk, thpk, ts, bool2s(dat), rung];
    end
end
ok = find(R(:,10) == 1);
if ok == [] then
    error("Không bộ PID nào đạt. Hãy tăng lực động cơ hoặc giảm tau_m.");
end
[tmp_, ord] = gsort(-R(ok,6));  ord = ok(ord);       // J tăng dần
mprintf("\n Đạt yêu cầu: %d / %d bộ. Năm bộ tốt nhất:\n", size(ok,"*"), size(R,1));
mprintf("\n %4s %5s %8s %8s %7s %8s %7s %7s %7s\n", "wn", "zeta", "Kp", "Ki", "Kd", "ITAE", "F max", "t_xl", "rung");
for k = ord(1:min(5, size(ord,"*")))'
    mprintf(" %4.0f %5.1f %8.1f %8.1f %7.2f %8.5f %6.1fN %6.2fs %6.2fN\n", R(k,[1:7 9 11]));
end
best = ord(1);
wn_b = R(best,1);  z_b = R(best,2);  K = R(best,3:5);
Kp = K(1); Ki = K(2); Kd = K(3);
bad = find(R(:,10) == 0 & R(:,2) == z_b);
if bad <> [] then
    mprintf("\n Với zeta = %g, các wn KHÔNG đạt: %s\n", z_b, strcat(string(R(bad,1)'), " "));
end

// =====================================================================
//  BƯỚC 4. KIỂM CHỨNG: cực vòng kín + từng trường hợp sai lệch
// =====================================================================
Acl = [A, zeros(4,1), B;
       0 0 -1 0 0 0;
       [0 0 -Kp -Kd Ki -1]/tau_m];
p_kin = spec(Acl);
mprintf("\n=====================================================\n");
mprintf(" BƯỚC 4. KIỂM CHỨNG BỘ PID ĐƯỢC CHỌN\n");
mprintf("=====================================================\n");
mprintf(" Cực vòng kín liên tục, chưa tính lấy mẫu (phần thực): %s\n", strcat(msprintf("%.2f\n", real(p_kin)), "  "));
mprintf(" (Hai cực ~0 thuộc vị trí và vận tốc x: PID chỉ giữ góc nên xe sẽ trôi.)\n");
KQ_ben = list();
for c = 1:size(ca_hs,1)
    [Tk, Yk, Fk] = mo_phong(sai_lech(P, ca_hs(c,:)), K, S);
    [Jk, Fpk_k, thk, tsk, ngak, rk] = danh_gia(Tk, Yk, Fk, S);
    KQ_ben($+1) = list(Tk, Yk);
    kq = "đứng vững"; if ngak then kq = "NGÃ"; end
    mprintf(" %-22s góc max %5.2f độ, lực max %5.1f N, rung %4.2f N -> %s\n", ca_ten(c), ..
            thk*180/%pi, Fpk_k, rk, kq);
end

mprintf("\n=====================================================\n");
mprintf(" KẾT QUẢ: BỘ PID CHUẨN (wn = %g rad/s, zeta = %g)\n", wn_b, z_b);
mprintf("=====================================================\n");
mprintf(" Kp = %.2f;  Ki = %.2f;  Kd = %.3f;\n", Kp, Ki, Kd);
mprintf(" (Dòng trên có thể dán thẳng vào Context của Xcos.)\n\n");

// =====================================================================
//  BƯỚC 5. ĐỒ THỊ + HOẠT HÌNH
// =====================================================================
if getscilabmode() <> "NWNI" then
    function luu(f, ten_file)
        if export_png then xs2png(f, out_dir + "/" + ten_file); end
    endfunction

    // --- Hình 1: kết quả quét (thang log) ---
    f1 = scf(1); clf(f1); f1.figure_size = [900 520];
    mau = ["blue", "green", "magenta", "orange", "black"];
    hh = []; nh = [];
    for i = 1:size(z_list,"*")
        k = find(R(:,2) == z_list(i));
        plot(R(k,1), R(k,6));
        e = gce(); e = e.children(1); e.foreground = color(mau(i));
        e.thickness = 2; e.mark_style = 9; e.mark_foreground = color(mau(i));
        hh = [hh; e]; nh = [nh; "zeta = " + string(z_list(i))];
    end
    kx = find(R(:,10) == 0);
    if kx <> [] then
        plot(R(kx,1), R(kx,6), "rx"); e = gce(); e = e.children(1);
        e.mark_size = 8; hh = [hh; e]; nh = [nh; "Không đạt"];
    end
    plot(wn_b, R(best,6), "rp"); e = gce(); e = e.children(1); e.mark_size = 16;
    hh = [hh; e]; nh = [nh; "Được chọn"];
    a1 = gca(); a1.log_flags = "nln";
    legend(hh, nh, 2);
    xgrid(); xlabel("wn (rad/s)"); ylabel("ITAE (nhỏ = tốt, thang log)");
    title(msprintf("Bước 3: quét PID - chọn wn = %g, zeta = %g", wn_b, z_b));
    luu(f1, "hinh1_quet_pid.png");

    // --- Hình 2: đáp ứng của bộ PID chọn ---
    [T, Y, F] = mo_phong(P, K, S);
    f2 = scf(2); clf(f2); f2.figure_size = [900 750];
    subplot(3,1,1); plot(T, Y(3,:)*180/%pi, "b"); xgrid();
    ylabel("theta (độ)"); title(msprintf("Bước 5: Kp = %.1f, Ki = %.1f, Kd = %.2f", Kp, Ki, Kd));
    subplot(3,1,2); plot(T, Y(1,:), "r"); xgrid(); ylabel("x (m)");
    subplot(3,1,3); plot(T, F, "k"); plot([0 S.tend], [Fmax Fmax], "r--");
    plot([0 S.tend], -[Fmax Fmax], "r--"); xgrid(); ylabel("F (N)"); xlabel("t (s)");
    luu(f2, "hinh2_dap_ung.png");

    // --- Hình 3: so sánh chậm / chuẩn / quá gắt ---
    f3 = scf(3); clf(f3); f3.figure_size = [900 520];
    so_wn = [6, wn_b, min(wn_b + 10, max(wn_list))];
    so_mau = ["green", "blue", "red"];  hh = [];
    for i = 1:3
        Ki_ = pid_dat_cuc(so_wn(i), z_b, pr, al, be);
        [Ti, Yi, Fi] = mo_phong(P, Ki_, S);
        plot(Ti, Yi(3,:)*180/%pi);
        e = gce(); e = e.children(1); e.foreground = color(so_mau(i)); e.thickness = 2;
        hh = [hh; e];
    end
    legend(hh, ["Chậm  wn = 6"; "Chọn  wn = " + string(wn_b); "Gắt  wn = " + string(so_wn(3))], 1);
    xgrid(); xlabel("t (s)"); ylabel("theta (độ)");
    title("So sánh 3 mức wn (cùng zeta)");
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
    title("Bước 4: cùng bộ PID khi thông số robot sai lệch");
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
