# RT45 — log chạy thử đầu tiên trên xe (21/11/2026)

## 0. Nguồn

| Mục | Giá trị |
|---|---|
| Tệp | `twip_balance_R12 (2) (6).txt` (7,07 MB) |
| STM32 | `BAL-20261121-R12-8DIR-RT45-CLEAN` |
| ESP32 | `TWIP-WEB-20261121-BAL-R12-8DIR-RT45-CLEAN-NET45` |
| Mẫu | **30 000** dòng, `state=3` toàn bộ, 1 `drive_epoch` |
| Thời gian | `t = 38,70 → 638,68 s` (600 s, log đầy bộ nhớ) |
| Header | `crc_errors=0 sequence_gaps=0 overflow=2568 STM32_resets=0`; `reset=power_on`, `heap_min=195832`, `archive_saved=0 archive_error=1` |
| Radio | `ch=6 rssi=-37 phy=11n ws_joins=2 ws_leaves=1 ap_joins=1` |
| STOP | `GSTOP` ở `last_seen=688,06 s` — **sau** cửa sổ log, nên file này không có nhịp STOP |
| Phân tích | `output/RT45/analysis/analyze_rt45.py`, `analyze2_rt45.py`, `probe3.py`, `metrics.json`, `metrics2.json` |

## 1. Toàn vẹn (kiểm trước khi đọc hành vi)

| Chỉ số | Giá trị | Đạt |
|---|---|---|
| `seq` liên tục | 29 999 bước, **tất cả = 1** | ✅ |
| `crc_errors` / `sequence_gaps` / `STM32_resets` / `drops` | 0 / 0 / 0 / 0 | ✅ |
| `dt_us` | 20,0 ms đúng; thô p50 5 000, max **5 015 µs** | ✅ (max vượt 2 µs, không đáng kể) |
| `execution_us` | p50 **658**, p95 **660**, max **679 µs** | ✅ (hạn cắt 4 000 µs) |
| Encoder | `x` so `0,5·(ticks_l−ticks_r)·MPT` lệch max **1,03e-6 m** | ✅ |
| `fault` khi `state=3` | **0 ở cả 30 000 mẫu** | ✅ |

`execution_us` p95 660 µs so với 648 µs của log RT37: **+12 µs (+1,9 %)** — đúng mức tăng của việc gọi 9 hàm
thay vì inline, vẫn cách hạn 4 ms gần 6 lần. Đây là bằng chứng realtime của bước tách `robot_fsm.c`.

## 2. Đường lệnh

| Chỉ số | Giá trị |
|---|---|
| `age = t_ms − drive_rx_ms` (chỉ mẫu có lệnh) | n = 2 723; p50 **13 ms**, p95 **28 ms**, max **55 ms** |
| Số mẫu `age > 350 ms` khi có lệnh | **0** |

## 3. Tư thế trung tính

`pitch` khi không lệnh, `|v|<0,05`, `|v_ref|<0,01`: n = 23 971 → mean **+2,50°**, p50 **+2,54°**.
`BINFO` báo `equilibrium=2,000°, qualified=1` — khớp, tức bộ học tư thế đang chạy đúng trên RT45.

## 4. Hành vi theo nhóm

| Nhóm | n mẫu | Kết quả đo |
|---|---:|---|
| Đứng yên / giữ | ~26 000 | `pitch` p50 +2,6°; `x` trôi p-p trong 1 s < 1 cm; `fault=0` |
| Mốc **80** (preset 1) | 538 s | thẳng: `v_ref` 0,71 → `v` 0,67; chéo: bám 1,13–1,15 rad/s |
| Mốc **MAX 90** (preset 2) | 56,6 s | thẳng dài nhất: `v_ref` **0,900**, `v` **0,883** (98 %), duty p50 **0,66**, p95 **0,915**, `max_envelope=1` 100 % mẫu |
| Mốc **50** (preset 0) | 5,4 s | **0 mẫu có lệnh** → chưa lái thử mốc 50 |
| Cua | 9 episode | chéo: `w_ref` 1,12–1,15 → `w_meas` 1,16–1,36 (tỉ lệ **1,01–1,39**); quay tại chỗ: 2,40 → **2,93** (1,22) |
| Bão hoà duty (một bánh ≥2 499) | 82 mẫu (**0,27 %**) | 89 % số mẫu đó là đang cua; `v` max 0,96; **0 fault** |
| Governor `limited` (bit 16384) | 40 episode, **14,0 s (2,3 %)**, dài nhất **1,70 s** | episode dài nhất ở t=79,7 s: cua chéo MAX 90, `v_ref` 0,65 / `v` 0,63, duty 0,61 → **không khựng** (lệch 0,02 m/s) |
| REC1 (`control_phase=2`) | **5 episode, 11,1 s (1,85 %)** | xem mục 5 |
| Nhả lệnh từ 0,63–0,78 m/s | 3 lần | về `|v|<0,1` sau 0,98–1,20 s, quãng 0,23–0,64 m |

## 5. REC1 — 5 episode, tất cả tự thoát, không cắt motor

| t₀ (s) | dài (s) | lệnh | pitch max | gyro max | v max | v_ref đầu→cuối |
|---:|---:|---|---:|---:|---:|---|
| 47,14 | 4,32 | tiến (dir +1) | 10,6° | 210 °/s | 0,27 | +0,19 → +0,18 |
| 51,84 | 1,50 | không | 8,3° | 71 °/s | 0,34 | +0,30 → 0 |
| 56,46 | 2,20 | không | **16,6°** | **305 °/s** | 0,52 | +0,62 → 0 |
| 60,76 | 1,46 | không | 9,7° | 84 °/s | 0,33 | +0,27 → 0 |
| 118,28 | 1,74 | không | 13,1° | 61 °/s | 0,47 (lùi) | −0,10 → 0 |

Đúng thiết kế REC1: vào khi bị đập/đẩy (gyro 210–305 °/s hoặc nghiêng 8–16,6°), nới trần có hạn, tự thoát
khi yên, **0 fault 4096, 0 lần cắt motor, không ngã**. Đây là 4/5 episode đúng bài "đẩy nhẹ rồi giữ" của
quy trình nghiệm thu.

## 6. Đối chiếu với kỳ vọng RT44

- Cùng luật: mốc 0,50/0,80/0,90; cua 1,3/2,4 rad/s (chéo 0,90/1,15); trần cua 0,87 − |w|·0,131; trần MAX
  theo duty; REC1 8°/12°/150 dps.
- Log này **đạt** các mốc đã thử: 80 đạt 0,67–0,71; MAX 90 thẳng đạt 0,883/0,900 với duty p95 0,915 (< 0,96
  nên trần duty không phải can thiệp mạnh); bão hoà chỉ 0,27 %; cua 4 hướng không ngã, `%512` = 0 ở phần
  lớn thời gian.
- **Không có log RT44 cùng điều kiện** (cùng mốc, cùng hướng, cùng pin) trong repo để so trực tiếp. So với
  log RT42/RT41 (chéo MAX 90, `turns.txt`): tỉ lệ `w_meas/w_ref` 0,78–1,20; log RT45: 1,01–1,39 — cao hơn
  ở 2 episode, nhưng (a) khác pin/mặt sàn, (b) luật cua đã được chứng minh **trùng từng token** so với
  RT44 và mô phỏng trùng 73/73 tệp, nên không phải thay đổi do refactor.

## 7. Điểm cần nói rõ

1. **Chưa thử mốc 50**: 5,4 s ở preset 0 nhưng **không mẫu nào có lệnh** → mốc 50 cm/s chưa được kiểm.
2. **MAX 90 thẳng chỉ ~2,6 s** ở `|v|>0,6`; chưa có lượt giữ 0,90 m/s ≥ 5 s để kiểm trần duty ở trạng thái
   ổn định (đề nghị chạy khi pin ≥ 12,0 V).
3. **STOP không nằm trong file**: `GSTOP` ở 688 s, log đóng ở 638,7 s. Cần một log có nhịp STOP để kiểm
   "dừng ngay".
4. `overflow=2568` trong header nhưng **30 000 dòng lưu có `seq` liên tục** → không có lỗ hổng trong cửa
   sổ này; đây là bộ đếm phía gateway (bộ đệm hiển thị), không phải khung telemetry bị mất.
5. `archive_error=1`, `archive_saved=0`: vùng log Flash vẫn chưa format — cần bấm **SỬA VÙNG LOG FLASH**
   một lần khi xe đã dừng (việc còn mở số 2 trong HANDOFF).
6. `dt_us` max 5 015 µs: vượt dải 4 995–5 013 đúng 2 µs, một mẫu — bỏ qua.

## 8. Kết luận

**Log sạch, hành vi đúng thiết kế trên mọi thứ đã thử; bản refactor RT45 không để lại hồi quy quan sát được.**
Cụ thể: 600 s liên tục, 0 lỗi CRC/gap/reset/fault, `age` p95 28 ms, `exec` p95 660 µs, REC1 5/5 tự thoát
không cắt motor, MAX 90 đạt 0,883 m/s với duty p95 0,915.

Còn **3 khoảng trống** cần một buổi đo nữa nếu muốn chốt đủ: mốc 50, MAX 90 thẳng ≥ 5 s, và một lượt có
STOP. Không có việc sửa code nào phát sinh từ log này.
