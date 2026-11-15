# R12 RT22-SYM — tiến/lùi tăng tốc như nhau, đúng mốc tốc độ, bẻ hướng gắt hơn

Bản dựng 14/11/2026: STM32 `BAL-20261114-R12-8DIR-RT22-SYM`, ESP32
`TWIP-WEB-20261114-BAL-R12-8DIR-RT22-SYM-NET23`. Artifact:
`.build/balance_release_r12_rt22_sym_20261114/` kèm `SHA256.json`, `SOURCE_SHA256.json`.
Log phân tích: `twip_balance_R12 (1).txt` (RT21-ANTISTALL/NET21, 7 779 mẫu, 169,6 s,
2 lần ARM, CRC/thiếu gói/reset STM32 = 0). **Chưa nạp RT22, chưa có log RT22.**

## 1. Triệu chứng người dùng và số đo trong log RT21

"Xe lùi nhạy và chạy nhanh hơn tiến": đúng, và đo được:

| Cùng lệnh 80 cm/s, đi thẳng | Tiến | Lùi |
|---|---|---|
| Thời gian `v_ref` đạt 95 % (2,26–4,56 s ở tiến) | **2,26 / 2,38 / 3,12 / 4,56 s** | **0,82 / 1,80 / 1,82 / 1,94 s** |
| Tốc độ cuối của lượt | 0,61–0,79 m/s | 0,77–0,81 m/s |
| PWM p50 | 1 531–1 864 | 1 894–1 993 |
| % mẫu bị governor ghìm | **15 / 27 / 30 / 35 / 40 / 70 / 96 / 100 %** | **1 / 2 / 6 / 10 %** |
| Tư thế khi đứng yên | **+1,45…+1,58°** (p50 +1,96…+2,00) | như nhau |
| Thời gian ghìm dài nhất | **4,78 s** (`v_ref` giữ 0,68 trong khi xe ở 0,57) | 0,58 s |

Đối chiếu từng mẫu: 1 867 mẫu có cờ 16384; với `equilibrium=0` thì quy tắc nghiêng giải
thích 20 % lúc bật, phần còn lại là **giữ** do điều kiện mở quá chặt:

- `predicted = direction*(pitch + 0,1·gyro) > 5°` — nhưng tư thế trung tính của xe là
  **+1,5°**, và lúc tăng tốc tới thân thật sự nghiêng **+5,8…+8,7°**. Ngưỡng 5° tuyệt đối
  nghĩa là **tiến gần như luôn vượt**, còn lùi (nghiêng âm) thì `predicted` bị đảo dấu
  nên hiếm khi vượt.
- Khi áp lực bật, `target=old` **đóng băng `v_ref`**; muốn mở phải có `|pitch|<3°`,
  `|gyro|<15`, `|v_ref−v|<0,08` liên tục 300 ms. Xe đang chạy 0,7 m/s với thân nghiêng
  3,2° và trễ 0,10 m/s **không bao giờ** đạt cả ba — nên ghìm 0,5–4,8 s. Đó chính là
  "khựng lại khi đang điều khiển".
- Bẻ hướng khi đang chạy: `lateral_cap = 0,4/|v|` → ở 0,8 m/s chỉ còn **0,50 rad/s**
  (bán kính 1,6 m), trong khi lệnh chéo là 0,55–0,80; cờ 512 bật 20–100 % thời gian.

## 2. Đã sửa

### 2.1 Governor (`balance_governor.h`)

| Thông số | RT21 | RT22 |
|---|---|---|
| Ngưỡng nghiêng thường | 5° | **9°** |
| Ngưỡng nghiêng lúc khởi hành | 8° | **11°** |
| Mở khi nghiêng / gyro (thường) | 3° / 15 °/s | **5° / 30 °/s** |
| Mở khi khởi hành | predicted<5,5; 6°; 25 °/s | **predicted<9; 11°; 45 °/s** |
| Khi có áp lực | `target=old` (đóng băng) | **giới hạn gia tốc 0,06 m/s²** (bò, không khựng) |
| Học tư thế trung tính | 1 s + 100 mẫu | **0,5 s + 60 mẫu**, giữ qua STOP/ARM |

`duty>0,90`, quy tắc trễ bám tốc (0,30 khi tăng tốc / 0,12 khi giữ), `retreat`, việc chặn
`x_ref` khi có áp lực và mọi giới hạn cứng **không đổi**. Ngưỡng 9°/11° vẫn nằm dưới
REC1 (12°) và stop góc (25°), nên nó chỉ bỏ qua độ nghiêng *bình thường của một cú tăng
tốc* chứ không bỏ bảo vệ.

### 2.2 Bẻ hướng (`balance_turn.h`)

| Thông số | RT21 | RT22 |
|---|---|---|
| Quay tại chỗ (50 / 72-80) | 1,2 / 2,2 rad/s | **1,3 / 2,4 rad/s** |
| Chéo khi đang chạy (50 / 72-80) | 0,80 / 0,55 rad/s | **0,90 / 1,15 rad/s** |
| Gia tốc yaw | 3,0 rad/s² | **4,5 rad/s²** |
| Trần gia tốc ngang | 0,4 m/s² | **0,9 m/s²** |

Trần bánh (`2*(1,05−|v|)/0,262`), giảm theo độ nghiêng `(12°−|pitch|)/6`, giảm theo
trôi tâm, phát hiện kẹt quay (200 ms) và mốc hướng chốt sau 0,6 s **giữ nguyên**.

## 3. Kiểm chứng trên máy

Hai test mới, mỗi test đều **tự chứng minh độ nhạy** bằng cách biên dịch lại với hằng số
RT21 và yêu cầu nó **fail**:

- `tools/test_rt22_sym.cjs` — phát lại đúng đoạn thân xe thật trong log (78 mẫu 40 ms,
  nghiêng tới 8,54°), lùi là **ảnh gương** của tiến quanh tư thế +1,5°:
  - tiến và lùi đạt 95 % mốc trong **2,21 s** như nhau, kết thúc **đúng 0,8000 m/s**;
  - mốc 0,50 / 0,72 / 0,80 đều đạt **đúng từng số** ở cả hai chiều;
  - không có khoảng đóng băng nào (bản RT21 fail ngay: tiến không tới đích);
  - quá tải thật (nghiêng 14°, PWM bão hoà) vẫn bị ghìm: 0,059 → 0,119 m/s trong 1 s.
- `tools/test_rt22_turn.cjs`:
  - quay tại chỗ **2,40 rad/s sau 0,6 s** (RT21: 2,2 sau 0,8 s);
  - chéo ở 0,8 m/s đạt **1,12 rad/s** (RT21: 0,50) — gấp 2,2 lần;
  - ở 1,04 m/s trần bánh vẫn ghìm còn **0,10 rad/s**;
  - nghiêng 10° vẫn giảm còn **0,80 rad/s**;
  - kẹt quay thật vẫn hủy; nhả lệnh → `w_ref=0` ngay.
- Hồi quy: **24/24** test host đang dùng pass (thêm 2 test mới); 6 test
  `test_drive8_*`/`test_hardware_boot`/`test_load_speed` vẫn fail như trước phiên.
- Mô phỏng: **36 lượt** (stress 12 + edges 12 + launch50 12) đều 20,000/20 s, không fault.
  So với RT21: **edges 12 ca đều đạt**, nhưng chỉ 6 ca đầu trùng số; ca 7–12 khác vì bẻ
  hướng nhanh hơn (yaw cuối đổi tới ~115°, `max|pitch|` tăng nhiều nhất +0,88°); stress
  đi xa hơn một chút vì không còn bị ghìm. Bộ `--launch50` cho `summary.csv` **trùng
  byte** stress nên không tính là bằng chứng độc lập.
  *Đính chính:* lần chạy edges đầu tiên dùng `--edge` trần nên chỉ có **6 ca**; số 12 ca
  ở trên là lần chạy lại đúng (`--stress --edge`).
- Build: STM32 text **81 624 B** (RT21: 81 520), ESP32 app **1 086 992 B**.

## 4. Bảo toàn

- Khung 122 B, BINFO, `$MOVE8`/lease 350 ms, epoch/sequence, STOP, REC1, ARM ±60°,
  cửa sổ ready 200 ms, hiệu chuẩn gyro 500 mẫu: không đổi.
- Gain/observer/phân bổ lực R12, encoder, dấu bánh: không đổi.
- Vẫn **chỉ chạy khi cân bằng**: mọi lệnh vẫn qua `motion_ready`, cổng góc/tốc độ/ngã
  giữ nguyên; "tuân thủ lệnh" chỉ áp dụng trong biên an toàn.

## 5. Nạp và thử

1. Nạp **cả hai**: `STM32_BAL_R12_RT22_SYM.bin/.hex` và
   `ESP32_RT22_SYM_NET23.merged.bin` (hoặc nạp sketch).
2. `http://192.168.4.1/ui/r12?v=RT22-SYM-NET23`; trang phải hiện `BAL-20261114-...RT22-SYM`.
3. Thử theo thứ tự: 50 tiến/lùi từ đứng yên (phải tăng tốc giống nhau), rồi 72, rồi 80.
   Quan sát bảng Chi tiết: `vận tốc đặt` phải lên đúng mốc ở **cả hai chiều**.
4. Thử bẻ hướng: quay tại chỗ, rồi tiến + trái/phải ở 72–80; phải vào cua nhanh hơn,
   bánh không trượt, xe không khựng.
5. DỪNG, chờ 2 s, tải log gửi lại — log RT22 phải có `build BAL-20261114-...RT22-SYM`.

## 6. Giới hạn

- Toàn bộ số liệu trên là **log RT21 + test host + mô phỏng**, chưa có log RT22 trên xe.
- Mô hình mô phỏng **không có góc nghiêng trung tính +1,5°**; test host mới là nơi kiểm
  tra tính đối xứng dưới độ lệch đó.
- Yaw vẫn có thể vượt tham chiếu ~20 % khi vào cua tại chỗ (đo trên RT21); RT22 tăng gia
  tốc yaw nên độ vọt này có thể lớn hơn một chút — cần log RT22 để xác nhận, không tự
  giảm gain yaw khi chưa có số.
- Chưa thử trên sàn nghiêng/dốc; chưa thử va chạm mạnh sau khi đổi ngưỡng.
