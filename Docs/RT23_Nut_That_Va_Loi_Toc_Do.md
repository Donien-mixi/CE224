# RT23 — Nút thắt tốc độ quay và lỗi ngắt motor vì vượt tốc độ

Ngày đo: 2026-11-14. Cặp bản cần nạp (không nạp bằng tay agent):

| Board | Build | Tệp |
|---|---|---|
| STM32F411 | `BAL-20261114-R12-8DIR-RT23-SYM` | `STM32_BAL_R12_RT23_SYM.bin` (82 676 B, sha256 `3000e363…`) |
| ESP32-S3 | `TWIP-WEB-20261114-BAL-R12-8DIR-RT23-SYM-NET24` | `ESP32_RT23_SYM_NET24.merged.bin` (sha256 `6cb6822e…`) |

Trang: `http://192.168.4.1/ui/r12?v=RT23-SYM-NET24`. Thư mục phát hành:
`.build/balance_release_r12_rt23_sym_20261114/` (kèm `SHA256.json`, `SOURCE_SHA256.json`).

## 1. Kiểm kê 8 log (điều kiện so sánh)

Cả 8 tệp trong `D:\DA_HTN\log test\`:

- build STM32 `BAL-20261114-R12-8DIR-RT22-SYM`, gateway `…-NET23`, `boot=384`,
  `reset=power_on`, `heap_min=181724`.
- `crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0` → dữ liệu dùng được.
- **`archive_saved=0`, `archive_error=1` trên cả 8 tệp** → vùng log Flash chưa gắn
  được, nên bản log nằm trong RAM. Đây là lý do 3 lượt chạy thẳng (tiến/lùi ở 50/72/80)
  không còn dữ liệu: ESP32 khởi động lại giữa buổi (`watchdog ngắt` = `ESP_RST_INT_WDT`
  trong ảnh người dùng gửi) là mất luôn buffer RAM.
- Mọi tệp có 2 s đuôi `state=5, fault=1` sau khi DỪNG — đó là bản ghi STOP bình
  thường, **không phải lỗi**.

## 2. Bảng lượt chạy (tách theo dir × turn × preset)

Cột `yaw` là tốc độ quay đo được từ encoder: `(v_r − v_l)/0.262`.

| Preset | Hướng quay | n mẫu | w_ref yêu cầu | w_ref đạt (median) | Tỉ lệ | drift | pitch median |
|---|---|---|---|---|---|---|---|
| 50 | trái (`turn=+1`) | 509, 449 | 1.30 | 1.15, 1.27 | 0.88–0.98 | 0.122–0.132 m | +4.4…+4.6° |
| 50 | phải (`turn=−1`) | 392, 365 | 1.30 | 1.30, 1.30 | **1.00** | 0.017–0.028 m | −0.5…+0.2° |
| 72 | trái | 320, 534, 723 | 2.40 | 2.11, 1.57, 1.63 | 0.65–0.88 | 0.132–0.151 m | +5.4…+5.9° |
| 72 | phải | 527, 404 | 2.40 | 2.40, 2.40 | **1.00** | 0.064–0.092 m | −2.4…−3.2° |
| 80 | trái | 719, 469, 497 | 2.40 | 1.50, 1.75, 1.55 | 0.62–0.73 | 0.147–0.156 m | +5.5…+5.9° |
| 80 | phải | 237, 387, 412, 174 | 2.40 | 2.40 | **1.00** | 0.061–0.098 m | −2.6…−4.1° |

Quay trái luôn ra 62–88 % tốc độ yêu cầu, quay phải luôn 100 %. Đối xứng bị lệch
đúng như người dùng mô tả, và lệch **tăng theo preset**.

Chéo (vừa tiến/lùi vừa quay):

| Preset | w_ref yêu cầu | w_ref đạt | flag 512 |
|---|---|---|---|
| 72 | 1.15 | 0.51–0.69 | 63–100 % thời gian |
| 80 | 1.15 | 1.15 | 0–15 % |

## 3. Ba nút thắt tìm được

### 3.1 Quay trái bị ghìm vì taper "tâm quay trôi" (lỗi chính của đối xứng)

`balance_turn.h` giảm tốc độ quay theo độ trôi tâm: `center=(0.22−|x_ref−x|)/0.10`,
sàn 40 %, tức là bắt đầu ghìm khi tâm trôi quá 12 cm.

Số đo: khi quay **trái**, thân xe ngả pitch **+4.8…+5.9°**, R12 phải giữ độ ngả đó
để thắng ma sát xoay, nên trục bò 2–3 cm/s và trôi **dừng ở 15–17 cm** (log 80: trôi
0.108 → 0.165 m trong 2 s rồi đứng yên, không chạy tiếp). Khi quay **phải** pitch chỉ
−2.6…−4.2° nên trôi 6–10 cm.

Hệ quả số học: trôi 0.146 m → `center=0.74` → `w_ref = 2.40 × 0.74 = 1.78 rad/s`
(đúng số đo 1.78), trôi 0.108 m → `center=1.0` → `w_ref = 2.40` (đúng số đo ở lần
bắt đầu). Vì vậy tốc độ quay **giảm dần trong lúc đang quay** và chỉ xảy ra ở phía
trái.

Sửa: hạ tầng ghìm xuống làm chốt an toàn — knee 22 cm → **45 cm**, sàn 40 % → **55 %**.
Trôi 15 cm là bình thường của một cú xoay tại chỗ, còn trượt thật (tâm chạy đi) vẫn bị
ghìm.

### 3.2 Preset 72 mất 40–60 % tốc độ quay khi vừa tiến/lùi vừa quay

`wheel_cap = 2·((preset==2 ? 1.05 : 0.80) − |v_đo|)/0.262` dùng **vận tốc đo được**
so với **trần trục 0.80 m/s** trong khi 0.80 là trần *trục*, không phải trần *bánh*.
Ở preset 72 trục chạy 0.72–0.79 m/s, nên:

- `wheel_cap = 2·(0.80 − 0.77)/0.262 = 0.23 rad/s` — trong khi yêu cầu là 1.15;
- cap còn **nhảy theo nhiễu đo** nên `w_ref` dao động, không chỉ thấp.

Bằng chứng mô phỏng (cùng một ca, cùng cây trồng, cùng hạt nhiễu):

| Ca 6 (tiến + quay, preset 80, 3–15 s) | 4–6 s | 6–8 s | 8–10 s | 10–12 s | 12–14 s |
|---|---|---|---|---|---|
| RT22 (trước) | 0.92 | 0.75 | **0.12** | **0.10** | **0.10** |
| RT23 (sau) | 1.06 | 1.08 | **1.13** | **1.10** | **1.12** |

Ca 9 (lùi + quay) tương tự: RT22 sụp về 0.08–0.09 rad/s, RT23 giữ 1.10–1.15 rad/s.

Sửa: trần bánh theo từng preset (`50: 0.80`, `72: 0.98`, `80: 1.05` m/s mỗi bánh) và
đánh giá trên **vận tốc lệnh** `v_ref`, cộng 10 cm/s dung sai để nhiễu vòng kín không
lọt vào cap. Bánh ngoài khi quay chéo ở preset 72 là 0.72 + 1.15·0.131 = **0.87 m/s**,
vẫn dưới 0.98 — đúng với tốc độ bánh xe thật đã chạy trong log (0.86 m/s).

### 3.3 `fault 16` ngắt cả hai motor khi trục chỉ vượt trần thoáng qua → xe ngã

Đây là lỗi làm xe ngã, và cũng là dòng chữ trong ảnh người dùng gửi
("Vượt tốc độ của pha đang chạy…").

Hai log preset 72 ngã ở đúng tick **nhả lệnh quay**:

| Log | t | Sự kiện | Trục | fault | PWM | Pitch sau đó |
|---|---|---|---|---|---|---|
| 72 ngã xe 1 | 38.94 s | `turn −1 → 0` | 0.74 → **0.795** m/s | 0 (còn chạy) | 2295/2117 | 2.66° |
| 72 ngã xe 1 | 39.00 s | 1 tick sau | 0.79 m/s | **16** | **0/0** | 1.00° |
| 72 ngã xe 1 | 39.06→39.20 s | xe đổ | — | 16 | 0/0 | **6.8 → 90.3°** |
| 72 ngã xe 2 | 33.70 s | `dir 1 → 0` | 0.73 → 0.79 m/s | 0 | 2399/2023 | 2.60° |
| 72 ngã xe 2 | 33.80 s | 1 tick sau | 0.79 m/s | **16** | **0/0** | 0.16° |
| 72 ngã xe 2 | 33.84→34.06 s | xe đổ | — | 16 | 0/0 | 0.8 → **105.3°** |

Cơ chế: khi nhả lệnh quay, bộ điều khiển yaw hãm tốc độ quay thật → hai bánh tăng tốc
(0.74 → 0.86 m/s) để hãm. Trục vượt 0.80 m/s đúng **một mẫu**. `BalanceStop(16)` cắt
cả hai PWM, thân xe đang 2° không còn mô-men giữ → đổ 0 → 105° trong **0.24 s**. Xe
đứng yên được nhờ motor, nên cắt motor ở 0.79 m/s chính là nguyên nhân ngã, không phải
biện pháp bảo vệ.

Cùng cơ chế giải thích 3 log chạy thẳng bị mất: ở 72 trần chỉ hơn tốc độ chạy 0.08 m/s,
một gờ sàn là đủ vượt; ảnh chụp cho thấy `fault 16`, PWM 0/0, pitch −13.63°.

Sửa (chỉ tầng giám sát, không đụng luật R12):

- trần = `max(trần preset/recovery, |v_ref| + 0.12)` — preset 72 chạy 0.72–0.79 nên
  0.80 là quá sát;
- chỉ ngắt khi vượt trần **liên tục 200 ms** (40 tick 200 Hz), không ngắt theo một mẫu;
- vượt **1.45 m/s** thì ngắt ngay (bảo vệ runaway thật, vẫn giữ);
- reset bộ đếm khi ARM và khi STOP.

### 3.4 Ghi nhận thêm (chưa sửa, cần đo trên xe)

- **Trôi khi xoay tại chỗ**: quay trái bò lùi ~2–3 cm/s tới 15–17 cm rồi dừng; quay
  phải chỉ 6–10 cm. Đây là bất đối xứng **cơ khí** (pitch trái +5.5° so với phải −3°
  ở cùng tốc độ quay) chứ không phải luật điều khiển. RT23 đã bỏ việc *giảm tốc độ quay*
  vì trôi này, nhưng độ trôi vẫn còn. Muốn bỏ hẳn phải đo thêm dòng/nhiệt từng motor.
- **Hai lần "ngã" ở log 80 không phải lỗi điều khiển**: t=63.8 s và t=83.0 s xe đổ
  **sau khi đã DỪNG** (`state=5`, `fault=1`, PWM 0/0) — motor đã tắt thì xe hai bánh
  không tự đứng được. Không tính vào lỗi.
- **ESP32 reset `watchdog ngắt` + `archive_error=1`**: chưa chứng minh được nguyên
  nhân trên bàn. RT23/NET24 chỉ sửa phần chắc chắn sai: (a) dò "vùng trống" lúc boot
  chỉ đọc 64 kB đầu thay vì cả 9.9 MB (20 224 lần đọc flash mỗi lần boot khi mount
  lỗi); (b) định dạng vùng log không còn chạy trên tác vụ web — lệnh được xếp hàng,
  tác vụ lưu thực hiện, trang trả lời ngay (trước đây `f_mkfs` 9.9 MB giữ request hàng
  chục giây, bấm lại có thể chồng 2 lần định dạng). **Chưa kiểm trên phần cứng.**

## 4. Đã kiểm bằng gì

- **25/25 host test đạt** (`node tools/test_*.cjs`), gồm 2 test mới:
  - `tools/test_rt23_interlock.cjs` — 40 ms vượt 0.86 m/s ở 72 **không** ngắt; chạy
    5 s ở 0.79 m/s **không** ngắt; runaway 0.95 m/s ngắt sau đúng 40 tick (200 ms);
    1.50 m/s ngắt ngay; mẫu không hợp lệ vẫn ngắt. Probe `-DBALANCE_SPEED_TRIP_TICKS=1`
    (luật RT22) **phải và đã** fail.
  - `tools/test_rt22_turn.cjs` mở rộng — preset 72 chéo (lệnh 0.72, đo 0.79) đạt
    1.14 rad/s; trôi 15 cm vẫn 2.40 rad/s; trượt 60 cm bị ghìm còn ≤1.34; lean 10°
    vẫn 0.80; blockage vẫn hủy. Probe `-DBALANCE_TURN_WHEEL_ENV_72=0.80 -DCENTER_KNEE=0.22
    -DSPAN=0.10 -DFLOOR=0.40` (luật RT22) **phải và đã** fail.
- **36 ca mô phỏng Scilab đều đạt 20/20 s, không fault**: stress 12 (pitch y hệt RT22
  từng ca), edge 12, launch50 12 (pitch ≤ 10.72°).
- **Chưa kiểm trên xe**: mọi kết luận hành vi đều từ log + mô phỏng; bản RT23 chưa chạy
  trên sàn.

## 5. Việc cần làm trên xe (theo `twip-oncar-test`)

1. Nạp STM32 RT23 **và** ESP32 NET24 (cả hai, nếu lệch build trang sẽ báo "Cần nạp đúng
   phiên bản STM32").
2. Trước khi chạy: bấm **SỬA VÙNG LOG FLASH** một lần khi xe đứng yên, đợi
   `archive_ready=true` (hoặc `archive_format=4` là lỗi — ghi lại `reset_reason`). Không
   có bước này thì log vẫn mất khi ESP32 reset.
3. Ma trận tối thiểu, mỗi lượt một tệp log và ghi rõ trong tên tệp:
   - 50/72/80 × **thẳng tiến** 5 s, **thẳng lùi** 5 s (đây là 3 lượt đã mất lần trước);
   - 50/72/80 × **quay trái** 8 s, **quay phải** 8 s (đo đối xứng);
   - 72 × bốn hướng, chuyển hướng **đột ngột** (tiến+trái → lùi+phải) 6 lượt.
4. Đạt khi: `fault` không có 16; `w_ref` trái ≈ phải trong ±10 %; chéo 72 đạt ≥1.05 rad/s;
   không lượt nào pitch > 30° mà không có `fault 4/8`.
