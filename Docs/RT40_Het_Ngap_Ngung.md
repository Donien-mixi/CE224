# RT40 — Hết ngập ngừng khi phanh/đổi hướng, không cắt motor khi bị giữ

Người vận hành gửi **8 log** của bản RT39 và mô tả ba nhóm lỗi:

1. **Chống chịu:** đẩy xe thì **động cơ bị ngắt**; đẩy nhẹ thì xe trôi đi, **không về vị trí ban đầu**.
2. **Tới/lui:** xe **ngập ngừng, chưa thực thi ngay** khi nhận lệnh; khi **lùi**, ngắt lệnh rồi xe vẫn chạy
   thêm một chút mới dừng.
3. **Rẽ:** chuyển hướng liên tục giữa 4 hướng chưa mượt, **có lần đã ngã**.

Yêu cầu: chạy mượt, vài giây đầu có thể chậm nhưng **phải đạt đúng mốc**; **thực thi tức thời** khi có
lệnh; **không được ngã** ở bất kỳ mốc nào.

## 1. Kiểm kê 8 log (cùng một build → so sánh được)

| # | Tệp | Mẫu | Thời lượng | ARM | Preset | Nhận xét |
|---|---|---:|---:|---:|---|---|
| 1 | `Test chống chịu.txt` | 844 | 16,9 s | 2 | 50 | đẩy tay, không lệnh |
| 2 | `Chống chịu.txt` | 415 | 8,3 s | 2 | 50 | đẩy tay, không lệnh |
| 3 | `Tới lui mốc 80…` | 3 131 | 62,6 s | 2 | 80/50 | 12 lượt tiến/lùi + cua |
| 4 | `Chạy tới lui 50…` | 2 437 | 48,7 s | 2 | 50 | 9 lượt |
| 5 | `Tới lui mốc 90…` | 3 098 | 61,9 s | 2 | 90/50 | 12 lượt |
| 6 | `Chạy rẽ trái và phải ở cả 3 mốc cho tiến` | 6 424 | 128,5 s | 2 | 50/80/90 | 30 lượt rẽ |
| 7 | `Chạy 3 mốc kết hợp 4 hướng rẽ` | 8 029 | 160,6 s | 2 | 50/80/90 | 41 lượt |
| 8 | `Rẽ trái phải 3 mốc khi lùi và đã ngã` | 7 644 | 188,0 s | **4** | 50/80/90 | **ngã 3 lần** |

Tất cả: `crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0`, `drops=0`, `dt` 4 991–5 014 µs,
`execution_us` p95 647–653 µs, `age` p95 28 ms. **Đường truyền và vòng điều khiển sạch** — mọi hiện
tượng đều nằm trong luật giám sát. (`archive_saved=0` + `archive_error=1` như mọi buổi trước: log chỉ
nằm trong RAM của ESP32.)

## 2. Ba nguyên nhân đo được

### 2.1 Nghiêng do phanh bị coi là mất cân bằng (15/28 cước REC1)

Đếm toàn bộ 8 log: **28 cước vào REC1**, trong đó **15 cước do `|pitch| ≥ 12°` khi đang giữ lệnh** —
nhưng pitch lúc đó chỉ **12,1–15,1°** và **gyro 1–61 °/s** (nghiêng chậm, đúng là độ ngả của một cú
phanh), không phải mất cân bằng:

| Tệp | t (s) | pitch | gyro | Kẹt sau đó |
|---|---:|---:|---:|---:|
| 03 | 307,43 | 12,1° | 1 | **1,62 s** |
| 03 | 324,03 | 12,4° | 12 | **1,40 s** |
| 04 | 126,53 | 13,0° | −6 | **1,62 s** |
| 05 | 474,25 | 12,8° | 23 | **2,20 s** |
| 05 | 480,41 | 12,5° | 13 | 0 |
| 05 | 494,07 | 12,3° | 2 | 0 |
| 06 | 686,61 | 12,7° | 23 | 0 |
| 07 | 1209,27 | 12,2° | −9 | 0 |
| 07 | 1284,17 | 13,5° | 45 | 0 |
| 07 | 1286,67 | 13,1° | 13 | 0 |
| 07 | 1349,59 | −12,2° | −4 | **1,52 s** |
| 07 | 1353,71 | 12,5° | 15 | 0 |
| 08 | 989,73 | 12,4° | 9 | 0 |

Tổng **8,4 s “có lệnh mà xe không chạy”** trong 8 log, chỉ vì ngưỡng 12°. Ví dụ log 05 t=473,4–476,0:
phanh từ −0,90 m/s, thân ngả tới 16,6°, REC1 bật ở 12,8° và `v_ref` **bị ghim 0,00 trong 1,0 s** dù
người lái vẫn giữ lệnh tiến.

### 2.2 Cắt motor khi bị giữ (fault 4096)

`02_chong_chiu` bị **`fault=4096` đúng 4 s** sau khi vào REC1, lúc xe **vẫn đứng thẳng**:
pitch 7,6°, gyro 15 °/s, |v| 0,20 m/s. Người thử chỉ giữ xe lâu hơn hạn 4 s → motor ngắt → xe đổ.
Đây chính là “động cơ bị ngắt lúc đẩy”.

### 2.3 Bánh ghim 100 % duty khi cua → ba lần ngã ở mốc 90

Cả ba lần ngã (log 08, preset 90, lùi) đều có **một bánh ghim duty 1,00 trong lúc đang cua**:

| t (s) | Bánh trái | Bánh phải | Ghi chú |
|---|---:|---:|---|
| 1112,4–1113,1 | −2499 (100 %), v_l −0,88 | −0,45 → −0,85 | cua lùi-phải ở 0,70 m/s |
| 1050,7–1050,9 | −2380 → −2499, v_l −0,88 | −1518 → −1712 | cua lùi-phải ở 0,72 m/s |

Khi nhả lệnh, phanh yaw phải kéo bánh phải từ −0,45 lên −0,95 m/s trong 300 ms → **trục vọt
−0,72 → −0,95 m/s** → thân quật ngược −56° → ngã. Nguyên nhân vật lý: lệnh cua 0,719 + 1,15·0,131 =
**0,87 m/s bánh ngoài**, mà bánh **trái cần ~1,11 lần duty của bánh phải** (đo trên chính 8 log này:
|duty/v| trái 1,080–1,135 so với phải 0,996–1,079), nên bánh ngoài chạm 97–100 % và không còn dự trữ
cho phanh yaw.

## 3. Sửa gì (chỉ lớp giám sát, không đổi R12)

| # | Tệp | Thay đổi |
|---|---|---|
| 1 | [`balance_recovery.h`](../Xe_Can_Bang/Core/Inc/balance_recovery.h) | Ngưỡng nghiêng **khi có lệnh 12° → 16°** (đỉnh đo được của cú phanh là 15,1°). Giữ nguyên: 8° khi không có lệnh, gyro 150 °/s, sai bám 0,35 m/s, cắt góc 25°, REC1 cho 50° |
| 2 | `balance_recovery.h` | Hạn một cước REC1 **4 s → 8 s**: giữ tay lâu hơn 4 s không còn cắt motor khi xe vẫn đứng. Vẫn bị chặn bởi 50° / 1,0 m / interlock |
| 3 | [`balance_turn.h`](../Xe_Can_Bang/Core/Inc/balance_turn.h) + [`robot_fsm.c`](../Xe_Can_Bang/Core/Src/robot_fsm.c) | **Trần cua theo duty còn dư**: đang cua mà có bánh ≥ 96 % duty thì hạ tham chiếu tuyến tính (hệ số 0,004/nhịp, sàn 0,72), hồi lại khi ≤ 90 %. Chỉ tác dụng khi đang cua, nên chạy thẳng vẫn đạt đúng mốc |

Ba hằng số đều có `#ifndef` để test dùng làm **old-constant probe**.

## 4. Kiểm chứng

- **41/41 host test pass**. Test mới `test_rt40_hold`:
  - **676 nhịp (200 Hz) phát lại 13 cửa sổ thật** của các cú nghiêng-phanh → **0 nhịp vào REC1**;
    probe `TILT_MANEUVER=12` **fail**.
  - Giữ xe 6 s (pitch 7°, gyro 15, |v| 0,20) → REC1 vẫn active, **không fault**; probe
    `TIMEOUT_MS=4000` **fail**.
  - duty 0,99 khi cua → hệ số 0,72, tham chiếu cua 0,518 m/s; thả ra hồi về 1,0; chạy thẳng về 1,0
    ngay; probe `HEADROOM_FLOOR=1.0` **fail**.
  - Cập nhật `test_balance_recovery`, `test_rt17_motion` theo hai hằng số mới.
- **Mô phỏng Scilab** (12 ca/bộ):

| Bộ | RT40 | RT39 |
|---|---|---|
| `--stress` @MAX 80 | **12/12**, mọi ca 20 s | 12/12 |
| `--stress --edge` | **12/12** | 12/12 |
| `--stress --launch --launch50` | **12/12**, pitch trùng RT39 từng ca | 12/12 |
| `--stress` @MAX 90 | 11/12 — ca 7 ngã ở 8,87 s (RT39: 8,95 s) | 11/12 |

- **Build**: STM32 `text 83 320 B` → bin 83 824 B, 0 lỗi 0 cảnh báo; ESP32 1 091 072 B. Build lại trong
  workspace `D:\CubeIDE_ws` ra hex **trùng SHA256** bản đóng gói (`d4a60fdf…`).

## 5. Cần log gì để xác nhận

Một buổi đo **đúng RT40** (`$BINFO` có `RT40-HOLD`), theo thứ tự:

1. **Chống chịu:** đứng yên 30 s; đẩy nhẹ 1 lần (không giữ) → xe phải về chỗ cũ; đẩy và **giữ 6–8 s**
   → **không được cắt motor**, không ngã; thả ra → về chỗ cũ.
2. **Tới/lui:** mỗi mốc 50/80, tiến 3 s → nhả → lùi 3 s → nhả; đo thời gian từ lúc nhả tới lúc |v| < 0,05.
3. **Đổi hướng:** tiến 0,8 → lùi ngay (không dừng giữa) ở 50/80.
4. **Rẽ:** 4 hướng ở 80 và 90, mỗi hướng giữ ≥ 2 s rồi nhả.

| Chỉ số | RT39 (8 log) | Kỳ vọng RT40 |
|---|---:|---:|
| REC1 do nghiêng khi có lệnh | **15 cước**, kẹt 8,4 s | **0 cước** |
| Cắt motor khi bị giữ (fault 4096) | 1 | **0** |
| Ngã | **3** | **0** |
| duty p95 khi cua | 1,00 | **< 0,95** |
| `v_ref` ghim 0 khi đang giữ lệnh | tới 2,2 s | **< 0,3 s** |

## 6. Giới hạn

- **Chưa kiểm trên xe.** Toàn bộ số đo trên là của 8 log RT39.
- Mốc **90** vẫn là giới hạn vật lý của pack 3S: log 05 chạy 0,90 nhưng chỉ đạt p50 0,79 m/s; cần pin
  đo **≥ 12,0 V** và bánh trái đang kéo ~11 % duty nhiều hơn bánh phải (cơ khí: bạc đạn/belt/motor).
- Trần cua động có thể làm cua chậm hơn ở mốc 90 khi bánh đã sát trần — đó là chủ ý để không ngã.
- Cơ chế **vọt trục khi nhả phanh yaw** vẫn tồn tại về mặt vật lý; RT40 chỉ giữ dư duty để nó không
  đẩy bánh vào bão hoà. Nếu log mới vẫn thấy vọt > 0,15 m/s khi nhả cua, bước tiếp theo là giới hạn
  tốc độ nhả yaw theo duty (đo trực tiếp thay vì suy từ duty bánh).
