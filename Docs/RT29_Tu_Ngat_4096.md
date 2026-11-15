# RT29 — Log (12)(3): ba lần xe tự ngắt và lỗi 4096

Log: `twip_balance_R12 (12) (3).txt`, build **RT28-ANCHOR + NET29**, 7 345 mẫu / **158,4 s**,
`crc_errors=0`, `sequence_gaps=0`, `STM32_resets=0`, `archive_error=1`, `boot=426`,
3 lần ARM (`epoch` 1,2,3).

## 1. Bao nhiêu lần tự ngắt: **3 lần**, không phải 2

| # | t (s) | mã lỗi | trạng thái trước |
|---|---|---|---|
| 1 | **11,52** | **4096** | `state=3` (đang cân bằng) |
| 2 | **85,50** | **4096** | `state=3` |
| 3 | **156,48** | **4096** | `state=3` |

Không có mã nào khác trong log (chỉ 4096, đúng 297 mẫu = 3 × 99 mẫu đuôi 2 s sau mỗi lần
ngắt). Vậy lần thứ ba (156,5 s) có thể bạn chưa để ý, hoặc nó xảy ra khi bạn đã ngừng quan sát.

## 2. 4096 này là lỗi gì

`4096` = **"quá 4 s"**. Có **hai** nguồn cùng mã 4096:

1. REC1 (vùng phục hồi) quá 4 s — trong `balance_recovery.h`.
2. **`BalanceMotion_Fault`: phanh không hoàn tất trong 4 s** — trong `balance_motion.h`.

Log cho thấy đây là **nguồn thứ 2**: mỗi lần ngắt cách một lần **vào REC1 đúng 4,00 s**:

| lần ngắt | vào REC1 (`phase 0→2`) | chênh |
|---|---|---|
| 11,52 | 7,52 | **4,00 s** |
| 85,50 | 81,50 | **4,00 s** |
| 156,48 | 152,48 | **4,00 s** |

Cơ chế: khi REC1 bắt đầu, `BalanceTurn` gọi `BalanceMotion_BeginBrakeHold` → đặt
`braking=true` và mốc thời gian phanh. Phanh chỉ "hoàn tất" khi
`|v| < 0,06 m/s` liên tục **20 tick (200 ms)** và `|v_ref|<0,01`, `|pitch|≤6°`, `|gyro|<25`.
Trong khi bạn **đẩy liên tục**, xe không bao giờ đứng yên 200 ms → phanh không hoàn tất →
đủ 4 s → **ngắt motor**.

## 3. Ngắt lúc đó là ngắt oan

Trạng thái ngay trước mỗi lần ngắt (mẫu cuối cùng còn `state=3`):

| # | pitch | gyro | v | vị trí so mốc |
|---|---|---|---|---|
| 1 | **−0,22°** | +3 °/s | +0,11 m/s | −0,084 m |
| 2 | **+1,12°** | −15 °/s | +0,22 m/s | −0,192 m |
| 3 | **−0,86°** | +8 °/s | +0,13 m/s | −0,114 m |

Xe **đang dựng thẳng, đang cân bằng, chỉ trôi nhẹ 0,1–0,2 m/s** — cắt motor ở đây làm xe
mất thăng bằng ngay, không phải để bảo vệ gì. Và mốc vị trí chỉ lệch 8–19 cm (RT28 hoạt động
đúng: `x_ref` không nhảy trong các episode này).

## 4. Sửa (RT29): bỏ hạn 4 s cho **phanh do nhiễu**

Trong `balance_motion.h`:

```c
if(fabsf(position-s->cancel_origin)>(s->braking?1.25f:1.0f))return 8U;   // giữ: chặn chạy xa
if(s->braking&&s->hold_anchor)return 0;   // RT29: phanh do nhiễu không có hạn 4s
return (uint32_t)(now-s->cancel_started_ms)>=4000U?4096U:0U;             // giữ cho nhả ga
```

- **Giữ**: phanh khi **nhả ga có lệnh** vẫn có hạn **4 s / 1,25 m** (hành vi RT19).
- **Giữ**: biên **1,25 m** cho cả phanh do nhiễu (xe chạy xa thật vẫn bị cắt).
- **Giữ**: timeout 4 s riêng của REC1 (nguồn 4096 còn lại) — nếu vùng phục hồi không tự
  thoát thì vẫn ngắt.
- Chỉ bỏ hạn 4 s cho đúng loại phanh "do bị xô" (cờ `hold_anchor`).

`tools/test_rt29_nocut.cjs`:
- phanh do nhiễu chạy **12 s** ở 0,20 m → **không fault**;
- phanh nhả ga vẫn fault **4096 đúng tại 5,0 s** so với mốc 1,0 s;
- phanh do nhiễu vẫn fault **8** khi lệch 1,30 m.

**30/30 host test pass** (6 test của bản RT8/RT9 đã chuyển sang `tools/legacy/`); `test_build_id_sync` pass.

## 5. Bản nạp

`Firmware/RT29_NOCUT_20261115/` — **`STM32_BAL_R12_RT29_NOCUT.bin`**
(`16effa54…`) + **`ESP32_RT29_NOCUT_NET30.app.bin`** (`0581361f…`), trang
`?v=RT29-NOCUT-NET30`. **Nạp cả hai board. Đã push Git 15/11/2026; chưa có log xe cho RT29.**

## 6. Kỳ vọng và việc cần kiểm

- Đẩy liên tục >4 s: xe **không còn tự ngắt**; motor vẫn chạy, xe tự về mốc trong giới hạn lực.
- Nhả ga sau khi chạy: phanh vẫn dừng trong 4 s / 1,25 m như cũ.
- Nếu còn tự ngắt, gửi log: xem `fault` và **thời điểm so với lần vào REC1** — nếu đúng 4 s
  sau REC1 thì là nguồn còn lại (timeout REC1), cần xử lý riêng.
