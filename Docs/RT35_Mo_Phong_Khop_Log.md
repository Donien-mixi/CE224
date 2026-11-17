# RT35 — Hiệu chỉnh mô phỏng theo log xe (1)(5)

Ngày 17/11/2026. **Chỉ sửa mô phỏng**, không đổi firmware — không cần nạp lại board.

## 1. Log dùng để hiệu chỉnh

`twip_balance_R12 (1)(5).txt` — build `BAL-20261117-R12-8DIR-RT34-CORNER` / gateway `…-NET35`:
8 652 mẫu / 173,0 s, `drops=0`, `dt` 4 999–5 000 µs, **0 lần `fault 4`** (không ngã), pack đo ~11,7 V.

Điểm đáng chú ý của log này:

| Điều cần kiểm | Kết quả |
|---|---|
| Sửa brownout của NET34 | **`reset=power_on`** — ba log trước đều `reset=brownout`, nay hết |
| Trần tốc độ do đường (RT34) | cờ 512 trong cua giảm còn **19–26 %** (trước 50–77 %) → bánh ngoài không còn bão hoà |
| Ổn định | 173 s, không ngã, không tự ngắt |
| Log Flash | vẫn `archive_error=1`, `archive_saved=0` → cần bấm “SỬA VÙNG LOG FLASH” một lần (khi xe đã tắt motor) |

## 2. Đường đặc tính bánh đo từ log

Chỉ dùng đoạn chạy thẳng, giữ lệnh, `|v| > 0,35`, `PWM > 150`; **bánh trái đã trừ 110 PWM** (phần
bù của RT32) để ra đúng PWM mà luật R12 yêu cầu:

| Bánh | gain (m/s trên 1 PWM) | intercept (m/s) | v @ PWM 2499 |
|---|---:|---:|---:|
| L+ (trái, tiến) | 0,000417 | −0,0332 | **1,010** |
| L− (trái, lùi) | 0,000396 | −0,0279 | 0,961 |
| R+ (phải, tiến) | 0,000394 | −0,0187 | 0,965 |
| R− (phải, lùi) | 0,000432 | −0,0593 | 1,019 |

Đường cũ trong mô phỏng (fit từ R7 W01–W09) cho 1,010 / 0,975 / 0,988 / 1,027 — tức **mô phỏng chỉ
lạc quan 1–2 %**. Đó là lý do mô hình vốn đã khá khớp; phần hiệu chỉnh chủ yếu nằm ở **intercept**
(mất mát/tải), không phải gain.

## 3. Đã thay gì

`Simulation/config/motor_pair_r8_20261011.sce`: thay `pair_no_load_curve` bằng 4 dòng đo được ở trên
(kèm comment nguồn). Không đổi `motor_vm_reference = 11,7` (đúng pack đang dùng), không đổi
`vehicle_baseline.sce`, không đổi luật điều khiển hay `CAT_VNL` của firmware.

## 4. Kết quả mô phỏng sau hiệu chỉnh

| Bộ ca | Trước (đường R7) | Sau (đo từ log (1)(5)) |
|---|---|---|
| `--stress` @ MAX 90 | 9/12 | **11/12** (chỉ còn ca 7: tiến, pin 11,2 V) |
| `--stress` @ MAX 80 | 12/12 | 12/12 |
| `--stress --edge` | 12/12 | 12/12 |
| `--stress --launch --launch50` | 12/12 | 12/12 |

Mô hình khớp xe hơn **và** bớt báo động giả: hai ca 11,2 V lùi và ca motor yếu 20 % trước đây ngã,
nay đạt — vì đường đặc tính thật của phía R−/L− mạnh hơn đường R7 cũ.

## 5. Giới hạn

- Intercept đo từ dữ liệu đang chuyển động (không phải thí nghiệm hãm riêng), sai số ước lượng
  ±0,03 m/s (~75 PWM). Muốn chính xác hơn cần một lượt đo riêng: cho xe chạy đều ở 3 mức PWM rồi
  ghi tốc độ, hoặc đo dòng từng bánh.
- Mô phỏng vẫn không có: trượt bánh, tụt áp theo tải, tư thế trung tính +1,5°, phản ứng cơ khí của
  khung. Vì vậy “khớp 1–2 %” chỉ đúng cho chuyển động đều trên sàn phẳng.
