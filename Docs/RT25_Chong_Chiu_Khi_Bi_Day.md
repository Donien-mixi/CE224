# RT25 — Chống chịu khi bị đẩy

Log: `twip_balance_R12 (3) (3).txt` (1 200 mẫu, 24 s, **ngã 1 lần**) và
`twip_balance_R12 (4) (3).txt` (2 654 mẫu, 53 s, không ngã), cả hai build
`BAL-…-RT23-SYM` + `NET24`, không có lệnh lái (`dir = turn = 0` suốt), CRC/gaps/reset = 0.

Cặp bản nạp: **`BAL-20261114-R12-8DIR-RT25-CATCH`** + **`…-RT25-CATCH-NET26`**,
thư mục `.build/balance_release_r12_rt25_catch_20261114/`
(`STM32_BAL_R12_RT25_CATCH.bin` sha256 `89248532…`, `ESP32_RT25_CATCH_NET26.merged.bin`
`b61e00fe…`), trang `http://192.168.4.1/ui/r12?v=RT25-CATCH-NET26`.

## 1. Vì sao xe ngã

Diễn biến (log 3, t = 21,5 → 22,0 s, `pitch` dương = nghiêng mũi xuống):

| t | pitch | gyro | v | pwm | ghi chú |
|---|---|---|---|---|---|
| 21,56 | +1,7° | +16 | 0,00 | −217 | bắt đầu bị đẩy |
| 21,70 | +4,1° | +28 | 0,21 | +1295 | xe chạy tới để đỡ |
| 21,82 | +1,5° | −60 | 0,69 | **+2499** | vẫn đang đẩy tới, bão hoà |
| 21,92 | −9,4° | −149 | 0,93 | **+2353** | thân đã ngả **về sau**, vẫn đẩy tới |
| 22,00 | −23,2° | −121 | 0,78 | +1321 | hãm, nhưng quá muộn |
| 22,02 | −25,5° | −116 | 0,70 | **0/0** | **fault 4** (vượt góc 25°) → ngã |

Ba nguyên nhân phần mềm:

**(a) Tham chiếu vị trí/vận tốc chống lại cú đỡ.** Luật R12:
`F = Kx·(x−x_ref) + Kv·(v−v_ref) + Kθ·θ + Kω·ω`, K = (9,15; 6,13; 14,33; 1,16), clip ±9,565 N.
Tại t = 21,88 s (pitch −4,25°, gyro −116 °/s, v 0,88, x 12 cm):

| Số hạng | Giá trị |
|---|---|
| Kx·(x−x_ref) | +1,1 N |
| Kv·(v−v_ref) | **+5,4 N** |
| Kθ·θ | −1,1 N |
| Kω·ω | −2,4 N |
| **Tổng** | **+1,5 N (vẫn đẩy TỚI)** |

Kv·v là số hạng lớn nhất, nên khi bị đẩy xe **được đẩy tiếp** thay vì được đỡ. Nếu hai tham
chiếu bám theo thân (`x_ref := x`, `v_ref := v`) thì cùng trạng thái đó yêu cầu **−4,6 N
(phanh)** và trong cú đẩy đạt tới **−9,57 N**.

**(b) REC1 bị "mù" 0,5 s sau mỗi lần phục hồi.** REC1 (vùng phục hồi, cho phép góc 50° thay
vì 25°) thoát ở t = 21,42 s và cần **100 mẫu yên (0,5 s)** để nạp lại `primed`; cú đẩy đến
sau đó **0,14 s** nên REC1 không kích hoạt được → chỉ còn giới hạn thường 25° → fault 4.
Bằng chứng: `phase = 0` suốt từ 21,42 s đến lúc ngã (không có phase 2).

**(c) Giới hạn 25° cắt motor đúng lúc cần motor nhất.** Ở 25° xe chưa chắc đã ngã, nhưng
`BalanceStop(4)` đặt PWM 0/0 → chắc chắn ngã. Cần thêm khoảng đệm trước khi bỏ cuộc.

## 2. Thay đổi (RT25)

Chỉ tầng **giám sát** — không sửa gain/observer/allocator R12, luật chuyển động, luật bẻ
hướng, governor hay khung 122 B.

1. **`balance_catch.h` (mới) — chế độ CATCH.** Kích hoạt khi **|gyro pitch| ≥ 60 °/s liên
   tục 20 ms** (một cú đẩy thật) **hoặc |pitch − tư thế trung tính| ≥ 10°** (ngã chậm).
   Trong lúc catch: `x_ref := vị trí đo`, `v_ref := vận tốc đo` (bỏ hai số hạng chống lại),
   **bỏ yêu cầu bẻ hướng** (dồn toàn bộ lực cho cân bằng), **nâng trần góc lên 40°**.
   Thoát khi yên 150 ms (|lean| ≤ 4° và |gyro| ≤ 40 °/s), sau đó ramp phanh bình thường
   đưa xe dừng tại chỗ mới. Trạng thái hiện trên web/log là **`control_phase = 3`**.
   Trong lúc ARM/dựng xe (startup capture) catch **bị khoá**, để không giành tham chiếu với
   pha dựng.
2. **REC1 giữ `primed` sau khi thoát** (`balance_recovery.h`) — chỉ thoát khi đã yên 300 ms,
   nên cú đẩy kế tiếp nạp lại được ngay; timeout 4 s vẫn giữ nguyên.

## 3. Kiểm chứng

- `tools/test_rt25_catch.cjs`:
  - **Phát lại đúng cửa sổ ngã** (26 mẫu thật, t = 21,50…22,00 s) qua `BalanceCatch_Step` +
    đúng công thức lực R12: catch bật ở mẫu 19 (pitch −4,25°, gyro −116 °/s, v 0,88) và cho
    **−4,59 N** ngay tại mẫu đó, tới **−9,57 N**; tham chiếu RT24 vẫn **+1,53 N** (Δ −6,12 N).
  - Không kích hoạt với lean 5,5°/gyro 7 °/s (đúng cú xoay trái đã đo ở RT24) và với 3 gai
    15 ms; thoát đúng sau 150 ms yên.
  - **Probe tắt catch** (`-DBALANCE_CATCH_GYRO=1e9 -DBALANCE_CATCH_LEAN=1e9`) **fail** — tức
    test thật sự bắt được hành vi cũ.
- `tools/test_balance_recovery.cjs` cập nhật theo chính sách mới (giữ primed, nạp lại tức thì).
- **27/27 host test pass**; 6 test `test_drive8_*`/`test_hardware_boot`/`test_load_speed` fail
  **có sẵn từ trước** (mock cũ), không liên quan.
- **36 ca mô phỏng** (stress 12 / edge 12 / launch50 12) **trùng từng số với RT24** — đúng như
  kỳ vọng vì catch nằm ở `robot_fsm.c` (không có trong đường mô phỏng). Nghĩa là: **36 ca này
  KHÔNG kiểm chứng catch**; catch chỉ được kiểm bằng test phát lại log thật ở trên.
- Ước lượng trên hai log (50 Hz, nên là cận trên): cửa sổ catch chiếm 20–36 % thời gian
  *trong lúc đang bị đẩy liên tục*, với lực phanh tối đa −9,57 N so với −3,7 N (log 3) và
  số mẫu mà luật cũ vẫn đẩy tới: 50/391 (log 3), 468/724 (log 4).

## 4. Giới hạn vật lý — không hứa được nhiều hơn

- Ở 0,95 m/s, **sức phản điện động** chỉ còn cho phép ~1,5 N lực kéo tới; phanh thì mạnh hơn
  nhưng vẫn hữu hạn. Không firmware nào đỡ được cú đẩy vượt quá khả năng này — chỉ có thể
  phản ứng **sớm hơn** và **dùng toàn bộ lực** (đó là điều RT25 làm).
- PWM ~1000 mà bạn thấy trên web là **bình thường khi giữ thăng bằng** (chỉ cần ~2–3 N). Khi
  bị đẩy, log cho thấy PWM **đã lên 2499** — nghĩa là motor đã dùng hết biên. Cái thiếu không
  phải biên PWM mà là **hướng lực** (mục 1a). RT25 chỉ đổi hướng/thời điểm dùng lực.
- Vì vậy yêu cầu "không bao giờ ngã" chỉ đạt được trong giới hạn lực/ma sát thực tế: cú đẩy
  làm thân vượt 40° hoặc làm bánh trượt thì vẫn ngã.

## 5. Việc cần làm trên xe

1. Nạp **cả hai** bản RT25-CATCH + NET26. Kiểm `control_phase` = 3 xuất hiện trên log khi bị
   đẩy (và **không** xuất hiện khi chỉ chạy thẳng/quay bình thường).
2. Lặp lại bài đẩy: đẩy nhẹ → vừa → mạnh, mỗi lần 3 lượt; ghi lại mức đẩy nào làm ngã.
3. Đẩy khi **đang chạy** (50/72/80, tiến/lùi/quay) để kiểm catch có trả lại lệnh lái đúng sau
   khi hết xô (xe phải tiếp tục chạy theo lệnh đang giữ, không bị "mất ga" lâu).
4. Nếu còn ngã: gửi log — chú ý `phase` (3 = catch), `pitch` lúc fault, và `pwm` xem đã 2499 chưa.
