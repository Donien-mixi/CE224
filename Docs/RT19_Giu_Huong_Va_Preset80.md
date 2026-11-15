# R12 RT19-HOLD80 / NET18 — giữ hướng sau quay, chốt mốc sau phanh, preset 80

Bản dựng ngày 08/11/2026: STM32 `BAL-20261108-R12-8DIR-RT19-HOLD80`, ESP32 `TWIP-WEB-20261108-BAL-R12-8DIR-RT19-HOLD80-NET18`. Đã build cả hai, chạy 14 bộ kiểm thử host và 36 ca mô phỏng. **Chưa nạp xuống xe.** Bản nạp để ở `.build/balance_release_r12_rt19_hold80_20261108/` kèm `SHA256.json` và `SOURCE_SHA256.json`.

## Nguồn: log xe thật `twip_balance_R12 (13) (2).txt`

Log đầu tiên của RT18: 2 813 mẫu, 54,3 s cân bằng, một lần ARM, toàn bộ ở preset MAX95, dt 4 998–5 014 µs, xử lý 635–662 µs, CRC lỗi 1, mất gói 2. Người vận hành báo bốn hiện tượng; phân tích cho ra ba lỗi hệ thống và một giới hạn vật lý.

| Hiện tượng | Bằng chứng đo được | Kết luận |
|---|---|---|
| Chọn 95 chỉ đạt 50–60 cm/s | `v_ref` không bao giờ vượt **0,746 m/s**; 64,5 % thời gian chạy tiến tham chiếu bị đóng băng; `x_ref − x = +0,1000 m` chính xác suốt lượt (governor anchor); 82 % mẫu tốc độ cao có cờ 16384 | Governor ghìm tham chiếu, **cộng** giới hạn vật lý |
| Phản ứng chậm | v_ref đạt 50 % sau **1,9–2,1 s**, 80 % sau 2,6–3,7 s; tuổi lệnh tại STM32 chỉ 13–15 ms (p50) | Trễ nằm ở bộ tạo tham chiếu, không phải Wi-Fi |
| Lệnh thẳng nhưng xe lệch trái | Lượt 225,33–230,27 s: encoder +59,4°, gyro +69,4°; `psi_ref` bám `psi_enc` từng mẫu; bit 2048 bật **100 %**; `w_ref=0`, tích phân hướng=0 | Mốc hướng bị bám theo thân khi còn braking; bộ giữ hướng không bao giờ ra tay |
| Dừng không đúng vị trí | Sau phanh xe lăn thêm 1–20 cm rồi đứng cách mốc **7–10 cm**; PWM giữ **+240** mà xe không nhích | Mốc vị trí chốt quá sớm, trước khi hết lăn ngược |

Kiểm chứng encoder từ 12 bài roll 10 vòng trong `56 bài test/` (kind4): PPR trái 898,32, phải 897,58 tick/vòng (lệch 0,08 %), dấu thô L+/R− đúng như firmware giả định. Từ log: `x` dựng từ tick khớp telemetry trong 1e-6 m; lượng tử 1 tick = 0,0114 m/s ở 20 ms; hai bánh tại cùng PWM lệch 1,5 % (trái 0,5064 / phải 0,5141 m/s).

## Thay đổi

### 1. Mốc hướng sau quay có giới hạn 0,6 s — `balance_turn.h`

Luật cũ bám `psi_ref = yaw` cho tới khi `|yaw_rate| < 0,08 rad/s` liên tục 100 ms, **không có giới hạn trên**. Với drivetrain lệch nhẹ, vận tốc quay dư có thể ở trên ngưỡng rất lâu, nên mốc bám theo thân suốt lượt chạy thẳng kế tiếp.

Nay: bám tối đa **120 tick (0,6 s)** rồi **đóng băng mốc**, để vòng giữ hướng dùng sai số hướng thật. Không đổi gain, không đổi dấu yaw, không đổi cơ chế phanh.

### 2. Chốt lại mốc vị trí 200 ms sau khi phanh yên — `balance_motion.h`

Luật cũ chốt `x_ref = position` ngay khi `|speed|<0,06` đủ 100 ms — lúc đó thân còn ngả sau và bánh còn lăn ngược. Mốc đóng băng tại chỗ đó tạo sai số vĩnh viễn.

Nay: sau khi phanh kết thúc, nếu xe thật sự yên (`|v_ref|<0,0001`, `|speed|<0,02`, `|pitch|≤6°`, `|gyro|<20°/s`) đủ 100 ms thì **chốt lại mốc một lần** tại vị trí đang đứng. Chỉ cửa sổ `stop_ticks` 20..40 mới chốt lại, nên một lượt giữ bình thường vẫn bị phản hồi vị trí giữ, không "tha" trôi chậm.

### 3. Preset cao nhất 95 → 80 cm/s — `balance_motion.h`, web, build ID

Fit vùng tốc độ cao từ chính log: PWM 2499 cho **0,964 m/s (trái) / 0,979 m/s (phải)** trên sàn. Muốn 0,95 m/s cần 97–99 % duty, không còn dự trữ để cân bằng. Chọn 0,80 m/s (~83 % duty, còn 17 % dự trữ). Nhãn web, `$BINFO`, `/firmware-info`, header log và build ID hai MCU đã đồng bộ.

## Kiểm chứng

### Kiểm thử host

Thêm `tools/test_rt19_hold.cjs`: tái hiện đúng hai lỗi — quay nhanh rồi nhả, vận tốc quay dư vẫn lớn, và phanh xong lăn ngược 9 cm. Test khẳng định mốc hướng đóng băng trong ≤ 0,6 s và không bám tiếp, mốc vị trí được chốt lại sau khi yên và không bị dời bởi trôi chậm về sau.

Kết quả: **18/18 bộ đạt** — 11 bộ cũ (recovery, NET1 routes, RT8 tracking/replay/A-B, governor, RT18, RT17, RT16, BMI160, encoder) + 4 bộ production (controller/dashboard/gateway/session) + RT11 response + RT12 replay + `test_rt19_hold`. Bốn test `test_drive8_*` cũ và `test_load_speed` vẫn lỗi thời từ trước (dùng header LOAD đã gỡ); không phải hồi quy của RT19.

### Mô phỏng Scilab (header sản phẩm, cùng plant/seed)

| Bộ | Kết quả | Điều kiện |
|---|---:|---|
| `rt19_stress` | **12/12** | 50/72/80, hai hướng, đảo chiều, chéo, VM 11,2/11,7 V, lệch motor/tải, 20 s |
| `rt19_edges12` | **12/12** | đứng yên, lệch mốc ±2°, CG 3 mm, quay hai chiều, nhấn–nhả ngắn |
| `rt19_launch50` | **12/12** | 50 cm/s, lệch mốc ±2/±3/±4°, nhấn 0,8 s; có kiểm tra đã chạy đúng hướng |

Không còn ca dừng sớm hay cờ bảo vệ. Góc thân lớn nhất trong bộ stress 11,0° (ca đổi chiều), dưới ngưỡng REC1 khi có maneuver (12°).

Hai chỉnh sửa chỉ thuộc **công cụ mô phỏng**, không vào firmware: (a) trần tốc độ nghiệm thu của preset2 dùng envelope 1,12 m/s như firmware thay vì 0,8 m/s; (b) `--launch` phải chạy kèm `--stress` vì vector lệch mốc có 12 phần tử.

## Còn mở — đọc trước khi kết luận

- **Governor chưa đổi.** RT19 vẫn giữ nguyên `balance_governor.h` của RT17/RT18. Vì vậy trên xe, preset 80 **có thể vẫn bị ghìm quanh 0,55–0,75 m/s** như log (13)(2). Muốn đạt đúng 80 cần bước F4/F5: bỏ điều kiện `v_ref − v > 0,12` khi đang tăng tốc, hạ dwell nhả 300→100 ms, nới cap gia tốc — và phải chạy lại đủ 36 ca cùng bộ xô 1–4 N.
- **Chênh lệch gyro/encoder ~15 % chưa xử lý.** Năm lượt quay thuần cho track hiệu dụng 226 ± 6 mm so với 262 mm của mô hình. Người dùng xác nhận hình học trong mô phỏng Scilab là đúng, nên **không đổi hằng số track**; cần đo trực tiếp hoặc kiểm tra trượt bánh khi quay tại chỗ trước khi kết luận.
- **Chưa chạy lại bộ xô 1–4 N.** Bộ này vốn có ca không đạt ở RT17/RT18; không có kết quả mới thì không được nói đã cải thiện chịu nhiễu.
- **Chưa có log xe thật RT19.** Mọi kết quả trên là mô phỏng/host, không phải chứng nhận trên xe.
- Kho lưu Flash vẫn báo `filesystem_unavailable_preserved`; tải log RAM trước khi cắt nguồn.

## Nạp và thử

1. Nạp STM32 bằng CubeIDE (hoặc `STM32_BAL_R12_RT19_HOLD80.bin/.hex`) và ESP32 bằng Arduino (hoặc `ESP32_RT19_NET18.merged.bin` nếu nạp cả phân vùng). Cần đúng cả hai vì web kiểm tra build ID.
2. Wi-Fi `TWIP_RACER_S3`, mở `http://192.168.4.1/ui/r12?v=RT19-NET18`. Trang phải hiện MAX 80 và xác nhận `BAL-20261108-R12-8DIR-RT19-HOLD80`.
3. Đo gyro khi cố định. Sàn trống, bật cân bằng, thả tay 2–3 s.
4. Thử mức 50 trước: tiến/lùi 2–3 s, nhả, để xe dừng hẳn và **đứng yên 3 s** — quan sát xe có tự bò về mốc cũ không. Sau đó thử quay, nhả, rồi bấm tiến ngay: xe phải giữ hướng mới, không xoay tiếp.
5. Nếu 50 ổn mới thử 72 rồi 80. Ghi lại tốc độ thực đạt được để quyết định bước F4.
6. DỪNG, chờ 2 s, tải log trước khi cắt nguồn.
