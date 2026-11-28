# Tối ưu realtime — danh sách công việc

> **Baseline (đã chốt để so sánh):** commit **`44f8e74`** — STM32 RT47-SLIM `text 84 152 B`,
> ESP32 NET47 `1 094 155 B`, 38/38 host test. Tag git: `baseline-44f8e74`.
>
> **Quy tắc làm việc (theo yêu cầu 28/11):**
> 1. Mỗi mục dưới đây là **một commit riêng**, không gộp nhiều mục.
> 2. Chỉ `git push` khi mục đó **xong**: build 0 lỗi 0 cảnh báo + **38/38 host test**;
>    nếu chạm điều khiển thì kèm log xe hoặc kết quả mô phỏng trước/sau.
> 3. Không push khi đang làm dở. Nếu một mục thất bại, revert commit đó rồi mới làm mục khác.
> 4. Giữ nguyên build ID RT47/NET47 (theo yêu cầu "không tạo bản firmware mới"); mỗi commit ghi rõ mục nào.

## 0. Đã làm trong đợt này

### [x] A3 — Nới ngưỡng "sẵn sàng điều khiển" (STM32)

- Trước: pitch ≤5°, gyro <25, |v| <0,15, neo 8 cm, 100 nhịp (0,5 s) → sau khi ARM/tông phải chờ lâu, có log 24,8 s không nhận được lệnh.
- Nay: pitch ≤6° (hysteresis reset >7,5°), gyro <35 (reset >45), |v| <0,20 (reset ≥0,25), neo 15 cm, **60 nhịp (0,3 s)**.
- Test `test_rt18_launch` cập nhật theo hợp đồng mới (vẫn chặn pitch 8°, gyro 50, v 0,25).

## 1. Hai điểm hành vi còn thiếu

### [ ] A1 — Dừng và giữ đúng vị trí khi ngắt tín hiệu (STM32)

- **Hiện tượng:** sau khi thả tay, xe còn trôi thêm (120 ms giữ hướng cũ + quãng phanh + relax 2 cm/s).
- **Sửa:** trong `Core/Src/balance_motion.c`
  - đặt `x_ref = position` ngay tại nhịp thả (không chờ đến khi dừng);
  - khoá relax trong **2 s** đầu sau khi dừng (macro mới `BALANCE_MOTION_HOLD_LOCK_MS`),
    sau đó mới relax 2 cm/s như cũ.
- **Chấp nhận:** thả ở 0,5 và 0,8 m/s → quãng trôi **sau khi dừng ≤ 5 cm/10 s**; không lắc thân;
  vẫn qua 38/38 test. Nếu lắc: giảm `DECEL` hoặc nới khoá về 1 s.
- **Đo:** log `x - x_ref` sau khi `drive_direction=0`; đếm số mẫu `|v|>0,05` khi không lệnh.
- **Thử 28/11:** giữ mốc tại điểm thả làm `test_rt19_hold` fail (hợp đồng cũ: chốt mốc tại **vị trí dừng**). Cần quyết định: đổi hợp đồng + test, hoặc chỉ giảm quãng dừng (`DECEL`). Đã revert thử nghiệm.

### [x] A2 — Đổi mốc 50/80/90 không nhả hướng (ESP32)

- **Hiện tượng:** đổi mốc gọi `releaseDrive()` → nhả lệnh, chờ 120 ms rồi mới chạy mốc mới → khựng.
- **Sửa:** trong `ESP32_S3_Gateway/dashboard_html.h`, `selectDriveSpeed()`: nếu đang giữ hướng
  thì gửi ngay `$MOVE8` với `preset` mới, **không** nhả hướng, không cần ARM lại.
- **Chấp nhận:** giữ 50 → bấm 80 khi đang chạy: xe tăng tốc liền, không dừng, `drive_sequence` liên tục.

## 2. Đường truyền — giảm độ trễ lệnh

### [x] P1.1 — Tick lệnh 20 → 10 ms (ESP32)

- `setInterval(driveTick,20)` → `10`.
- **Chấp nhận:** `age` p50 ≤ 10 ms, p95 ≤ 20 ms (đã đo được 7/15 ms ở lần thử trước).

### [ ] P1.4 — Parse UART bằng IDLE-DMA trên STM32

- `Core/Src/esp32_comm.c`: dùng `HAL_UARTEx_ReceiveToIdle_DMA` (hoặc cờ IDLE) để parse ngay khi
  khung kết thúc, thay vì chờ main loop.
- **Chấp nhận:** `age` max không tăng; `exec` p95 ≤ 700 µs; 38/38 test.

### [x] P1.5 — SoftAP DTIM = 1, beacon 100 ms (ESP32)

- Giảm trễ hướng xuống do điện thoại ngủ theo DTIM.
- **Chấp nhận:** `age` p95/max giảm; `ws_leaves=0`.

### [x] P2.1 — Gzip trang + ETag (ESP32)

- Đã từng làm (commit `baa330f`): 57 → 19 KB, 304 khi không đổi.
- **Chấp nhận:** `loop_gap_max_ms` < 20 ms (hiện 100–147 ms khi tải trang); trang mở bình thường.

### [ ] P2.2 — Giãn ping WS 200 ms → 1 s

- **Chấp nhận:** `ws_leaves=0`, `age` không đổi.

### [ ] P2.3 — Lệnh nhị phân 12 B thay ASCII 35 B

- Sửa `Drive8_Parse`, trang gửi `ArrayBuffer`.
- **Chấp nhận:** lệnh đúng 100 %, `age` giảm ~0,2 ms; 38/38 test + 1 test mới cho gói nhị phân.

### [ ] P2.4 — UART 1,5 Mbaud

- `usart.c` + `.ioc` + `STM32_BAUD`.
- **Chấp nhận:** `crc_errors=0` sau 5 phút chạy; `age` giảm ~0,2 ms.

## 3. Không khuyến nghị (chỉ khi cần cực đoan)

- **[ ] P3.1** Kênh lệnh UDP: giảm head-of-line blocking của TCP nhưng phải tự làm tin cậy/thứ tự/lease.
- **[ ] P3.2** Tách WS sang core khác: `WebSocketsServer` không thread-safe.

## 4. Cách đo sau mỗi mục

| Chỉ số | Lấy ở đâu | Kỳ vọng |
|---|---|---|
| `age` p50/p95/max | log | p50 ≤ 10, p95 ≤ 20, max ≤ 50 ms |
| `loop_gap_max_ms` | `/network-status` | < 20 ms |
| `exec` p95 | log | ≤ 700 µs |
| Quãng trôi khi `dir=0` | log | ≤ 5 cm/10 s (A1) |
| Đổi mốc khi đang chạy | quan sát + log | không dừng, `v` tăng liền (A2) |
| `crc_errors/gaps/ws_leaves` | log | 0 |

## 5. Lịch sử commit sau baseline

| Commit | Mục | Kết quả |
|---|---|---|
| `A2` | Đổi mốc không nhả hướng | ESP32 1 094 235 B, 38/38 test |
| `P1.1` | Tick lệnh 10 ms | ESP32 1 094 235 B, 38/38 test |
| `P2.1` | Gzip 54,8 → 18,1 KB + ETag/304 | ESP32 1 059 107 B, 39/39 test |
| `A3` | Nới ngưỡng sẵn sàng 0,3 s | STM32 84 224 B, 39/39 test |
| `P1.5` | DTIM=1, beacon 100 ms | ESP32 1 059 143 B, 39/39 test |
