# RT26 — Chống chịu khi bị đẩy: giữ vị trí, không trôi

Log dùng để sửa: `twip_balance_R12 (5) (3).txt` (1 493 mẫu, 29,8 s) và
`(6) (1).txt` (1 436 mẫu, 28,7 s), build **RT25-CATCH + NET26**, **không có lệnh lái**
(`dir = turn = 0` toàn bộ), CRC/gaps/reset = 0, không fault trong lúc cân bằng.

Cặp bản nạp: **`BAL-20261114-R12-8DIR-RT26-PUSH`** + **`…-RT26-PUSH-NET27`**
(`.build/balance_release_r12_rt26_push_20261114/`, `STM32_BAL_R12_RT26_PUSH.bin`
sha256 `898a7642…`, `ESP32_RT26_PUSH_NET27.merged.bin` `6579e074…`), trang
`http://192.168.4.1/ui/r12?v=RT26-PUSH-NET27`.

## 1. RT25 sai ở đâu (số đo trên chính 2 log này)

| Chỉ số | Log (5)(3) | Log (6)(1) |
|---|---|---|
| Tổng quãng `x` đi được | 1,28 m (x −1,01…+0,27) | **2,51 m** (x −0,74…+1,76) |
| `x_ref` bị **kéo** tổng cộng | **3,66 m** | **4,56 m** |
| Số mẫu `control_phase=3` (catch) | 33 | 11 |
| REC1 (`phase=2`) | 631 mẫu | 726 mẫu |
| Trần di chuyển REC1 (`dist_limit`) | tới 0,91 m | tới 0,92 m |

Nguyên nhân: `balance_catch.h` (RT25) đặt **`x_ref := vị trí thân`** mỗi tick khi đang
catch. Chỉ cần **11–33 mẫu** catch là mốc vị trí bị kéo 3,7–4,6 m; mỗi cú đẩy kết thúc ở
một chỗ mới và xe **không còn mốc cũ để quay về**. Cộng thêm hai điều làm nó nặng hơn:
ngưỡng catch quá thấp (**60 °/s trong 20 ms** — một cái huých tay cũng đủ), và REC1 cho
phép đi **+0,8 m** tính từ vị trí lúc vào (nay còn **+0,35 m**).

## 2. RT26 làm gì

Chỉ tầng **tham chiếu** (không đụng gain/observer/allocator R12, luật chuyển động/bẻ
hướng, governor, khung 122 B). Logic đặt trong `BalanceMotion8_Update` để **mô phỏng
native cũng chạy đúng đoạn này**, không chỉ firmware.

1. **`balance_push.h` (thay `balance_catch.h`)** — CATCH **giữ nguyên mốc vị trí**:
   chụp `x_hold = x_ref` lúc vào và **khôi phục lại mỗi tick** (chặn cả đường tích phân
   mốc), chỉ thả **tham chiếu vận tốc** (`v_ref := vận tốc đo`) để luật R12 dùng hết lực
   đỡ thân. Ngưỡng: **|gyro pitch| ≥ 80 °/s trong 20 ms** hoặc **|lean| ≥ 12°** — trên
   xa mọi thứ khi lái bình thường (cú xoay 2,40 rad/s chỉ ~7 °/s trên trục pitch, lean
   ≤ 8,9°).
2. **RETURN** — khi *không có lệnh lái*, xe đã `ready`, không phanh, không lean
   (`> 6°`), gần như đứng yên (`|v| < 0,25`): đặt `v_ref = 1,6·(x_ref − x)`, **giới hạn
   ±0,30 m/s**, **deadband 4 cm**. Xe tự lùi/tiến về đúng mốc cũ rồi dừng.
3. **REC1**: quãng cho phép sau vị trí vào **0,8 → 0,35 m**; vẫn giữ `primed` sau khi
   thoát để nạp lại ngay (từ RT25).
4. Pha catch vẫn hiện là **`control_phase = 3`**; trần góc khi catch **40°**.

**Phạm vi đã chọn (và vì sao):** catch chỉ nhận tham chiếu khi **không đang giữ lệnh lái**
(`fresh && (word&3)!=0` ⇒ nhường REC1). Lý do: giữ nguyên hành vi REC1 "phanh khi đang giữ
lệnh" mà các test RT16/RT17 đã ghim; khi đang giữ lệnh, vỏ bọc REC1 (50°, 1,2 m/s, drive
inhibited) vẫn bảo vệ, và RETURN sẽ kéo xe về sau khi nhả lệnh.

## 3. Kiểm chứng

- `tools/test_rt26_push.cjs` (dùng **đúng 26 mẫu log của cú ngã RT24**):
  - Catch **chỉ bật 1 lần** trên cửa sổ đó (mẫu 20), lực **−4,59 N** so với **+0,98 N**
    của luật cũ (Δ **−5,57 N**), phanh đỉnh **−8,88 N**; và **x_ref không đổi một
    micromet** trong suốt cửa sổ (chính là lỗi RT25).
  - Trigger: lean 5,5°/7 °/s và 3 gai 15 ms **không** kích hoạt.
  - RETURN: 25 cm phía trước → −0,300 m/s (đúng giới hạn); 60 cm phía sau → +; trong
    deadband 4 cm / đang lean / đang có lệnh / đang catch → **không** can thiệp.
  - Probe tắt catch (`-DBALANCE_PUSH_GYRO=1e9 -DBALANCE_PUSH_LEAN=1e9`) **fail** đúng.
- **27/27 host test pass**; 6 test `test_drive8_*`/`test_hardware_boot`/`test_load_speed`
  fail có sẵn từ trước.
- 24/24 ca mô phỏng (stress 12 + edge 12) chạy xong 20/20 s.
- **Trung thực về mô phỏng:** bộ `--disturbance` (xô 1–4 N) **đã ngã 9/12 ca ngay cả khi
  tắt toàn bộ chính sách RT26** → bộ đó **không dùng được** để chứng minh chống chịu; nó
  chỉ cho thấy chính sách mới **giảm pitch** ở 3 ca (14,5→12,8°, 18,9→15,8°) chứ không
  cứu được cú xô mà mô hình coi là quá lớn.

## 4. Giới hạn / chưa kiểm

- **Chưa nạp RT26 lên xe.** RT25 đã cho thấy bài test trên bàn không thay được xe thật.
- RETURN chỉ chạy khi **không giữ lệnh lái** và xe `ready`. Nếu va chạm xảy ra *đang giữ
  ga*, xe dựa vào vỏ REC1; sau khi nhả ga RETURN mới kéo về. Đây là điểm cần đo trên xe.
- Không thể hứa "không bao giờ ngã": ở 0,9–1,0 m/s sức phản điện động chỉ còn ~1,5 N kéo
  tới. RT26 giảm quãng trôi và tăng lực đỡ, không vượt được giới hạn vật lý đó.
- `archive_error=1` vẫn còn trong cả 2 log ⇒ phải bấm **SỬA VÙNG LOG FLASH** trước buổi đo.

## 5. Việc cần làm trên xe

1. Nạp STM32 RT26-PUSH + ESP32 NET27; bấm SỬA VÙNG LOG FLASH, đợi `archive_ready`.
2. Đứng yên, huých nhẹ 5 lần: đo **|x − x_ref| lớn nhất** và **vị trí cuối** — kỳ vọng
   quãng trôi ≤ 15–20 cm và **xe quay về quanh mốc cũ** (sai số ≤ 5 cm).
3. Huých mạnh hơn, ghi lại `control_phase=3` và pitch lớn nhất; nếu vẫn ngã, gửi log kèm
   mốc thời gian.
4. Đang chạy 50/72/80 thì va nhẹ: kiểm xe **không mất ga lâu** và sau khi thôi va vẫn bám
   lệnh đang giữ.
