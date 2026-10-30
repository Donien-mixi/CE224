# RT8 / NET5 — sửa ba lỗi cảm nhận từ log 27

Đầu vào: `twip_balance_R12 (27).txt` chạy trên RT7 (`BAL-20261024-R12-8DIR-RT7`), 7.585 mẫu, 151,68 giây, 1 lần STOP cuối. Ba phản hồi của người vận hành:

1. Đường gồ ghề thì xe ngừng nhận điều khiển để lấy lại cân bằng.
2. Đi lùi chậm hơn đi tiến nhiều.
3. Xoay trái chậm hơn xoay phải rõ rệt.

Các phân tích dưới đây là giả thuyết từ một log RT7, không phải xác nhận nguyên nhân vật lý đầy đủ hay kết quả RT8 trên xe.

## 1. Phục hồi bật vì gai gyro khi qua chỗ xóc

- Có **4 lần phục hồi**: t = 5,08 s · 29,42 s · 44,36 s · 130,16 s; mỗi lần 0,6–1,0 s, tổng 3,32 s (2,2% thời gian chạy).
- Cả 4 lần đều do **gyro vượt 60 °/s** (82,9 · 62,7 · 58,4→63,7 · −62,4 °/s) trong khi **pitch chỉ 2,4–4,7°** — đúng dạng gai của hệ treo khi bánh lăn qua mấp mô, không phải mất thăng bằng.
- Ngoài phục hồi, gyro lớn nhất là 146,5 °/s; **không có mẫu nào ≥ 150 °/s**. Ngưỡng 60 cũ bắt nhầm, ngưỡng 150 không bắt nhầm mẫu nào trong cả lượt chạy.

**Sửa:** ngưỡng gyro của REC1 **60 → 150 °/s** (`balance_recovery.h`). Hai đường kích hoạt còn lại giữ nguyên: góc 8°/12° và sai lệch tốc độ 0,35 m/s (có lọc 60 ms).

## 2. Đi lùi bị bóp bởi ngưỡng bám 10 cm

| | Tiến | Lùi |
|---|---|---|
| v_ref (đặt) | +0,500 | −0,369 |
| v đo | +0,506 | −0,348 |
| sai số vị trí (x_ref − x) | **+0,008 m** | **−0,096 m** |
| tỉ lệ mẫu bị `tracking_limited` | **0%** | **27%** |

- Mọi đoạn lùi đều dừng ở **x_ref − x ≈ −0,095…−0,097 m**, tức sát ngưỡng chặn 0,10 m.
- Trong log có lúc xe lùi thật tới **−0,478 m/s** với v_ref −0,5 (t = 89–94 s), cho thấy bộ chặn tham chiếu có ảnh hưởng; dữ liệu này chưa đủ loại trừ thiếu mô-men dưới tải ở điều kiện khác.
- Cơ chế: khi đảo chiều, thân vượt tham chiếu ~10 cm; bộ chặn đặt `target = 0`, v_ref tụt, xe chậm, rồi lại tăng — tạo dao động quanh −0,3 m/s suốt đoạn lùi.

**Sửa:** ngân sách bám **0,10 → 0,20 m**; ngưỡng bằng chứng kẹt **0,18 → 0,30 m** (`balance_motion.h`). Xe vẫn bị huỷ nếu kẹt thật (đâm tường) vì bộ đếm kẹt dùng 30 cm và 60 tick, cộng điều kiện “đứng yên mà lệch 6 cm”.

## 3. Xoay trái bị bộ điều tiết tâm quay bóp tốc độ

- Ở trạng thái **ổn định** (|w_ref| > 1,05 rad/s), trái và phải gần như bằng nhau: **1,152 vs 1,116 rad/s** (96% và 93% yêu cầu). Động cơ và dấu chiều **không** sai.
- Khác biệt nằm ở **quá độ**: lượt xoay trái có **55% mẫu bị cờ 512** (bộ điều tiết tâm quay can thiệp) so với **13%** ở lượt phải; w_ref trung vị 1,105 so với 1,2; độ trôi tâm 0,062 m so với 0,021 m.
- Nguyên nhân: bộ điều tiết cũ dùng công thức `max(.20,min(1,(.16-abs(x_ref-x))/.10))`: bắt đầu giảm sau 6 cm, chạm sàn 20% ở khoảng 14 cm. Lượt trái trôi tới ~0,11 m nên bị cắt xuống còn ~49% tốc độ — người lái cảm nhận “xoay trái chậm”.

**Sửa:** điểm gãy **0,16 → 0,22 m**, sàn **20% → 40%** (`balance_turn.h`). Vẫn giảm tốc khi tâm trôi thật (đi bộ ngang), nhưng không cắt một cú xoay bình thường.

## 4. Kiểm tra điều khiển động cơ (theo yêu cầu)

- **Dấu chiều đúng 100%**: mọi mẫu tiến có `v > 0` và `PWM > 0`; mọi mẫu lùi có `v < 0` và `PWM < 0`. Không có mẫu nào ngược dấu.
- **Hai bánh cân**: khi đi thẳng, chênh tốc độ trung vị **−0,0009 m/s**, p90 0,056 m/s (do lượng tử encoder và mặt đường), không phải lệch hiệu chuẩn.
- **Bản đồ động cơ** (tốc độ theo |PWM|, chỉ đoạn thẳng):

| | 200–400 | 400–700 | 700–1000 | 1000–1400 |
|---|---|---|---|---|
| Trái tiến | 0,039 | 0,134 | 0,277 | 0,490 |
| Trái lùi | 0,065 | 0,200 | 0,296 | 0,422 |
| Phải tiến | 0,071 | 0,135 | 0,295 | 0,487 |
| Phải lùi | 0,053 | 0,208 | 0,305 | 0,443 |

  Ở vùng PWM lớn, chiều lùi cần thêm ~5% PWM để đạt cùng tốc độ; ở vùng nhỏ thì ngược lại. Đây là phi tuyến của động cơ/hộp số, và bộ phân bổ R12 đã bù riêng từng bánh từng chiều (`CAT_VNL`, `CAT_FRICTION`, `CAT_START`) — không phải lỗi đấu dây hay đảo dấu. Không chỉnh các hệ số đã đóng băng này khi chưa có phép đo tải riêng.

## Thay đổi

| File | Thay đổi |
|---|---|
| `Core/Inc/balance_recovery.h` | ngưỡng gyro 60 → 150 °/s |
| `Core/Inc/balance_motion.h` | ngân sách bám 0,10 → 0,20 m; bằng chứng kẹt 0,18 → 0,30 m |
| `Core/Inc/balance_turn.h` | gãy tâm quay 0,16 → 0,22 m; sàn 20% → 40% |
| `Core/Inc/balance_profile.h` | build `BAL-20261024-R12-8DIR-RT8` |
| `ESP32_S3_Gateway/balance_web.h` | mô tả ngưỡng trong header log |
| Bản web | `TWIP-WEB-20261024-BAL-R12-8DIR-RT8-NET5` |

Không đổi: luật lực R12/observer/PWM, ARM 60°, giới hạn góc/tốc độ/quãng trôi, impact 8192, dead-man 10 s (từ RT7), chính sách đóng web không dừng xe.

## Kiểm chứng

- **Replay đúng log 27** (`tools/test_rt8_replay.cjs`): nạp 7.585 mẫu cảm biến thật qua ba lớp phục hồi + chuyển động + xoay, với ngưỡng RT8:
  - **0 lần phục hồi** trong lúc đang cân bằng; phép replay hiện hành in legacy=3, RT8=0. Con số 4 ở phân tích sự kiện gốc phía trên thuộc phạm vi/trạng thái khởi tạo khác, không phải cùng phép replay,
  - **0 mẫu bị huỷ** vì kẹt,
  - w_ref trung vị **+1,200** (trái) và **−1,200** (phải) — cả hai đạt đủ tốc độ.
- **Test tổng hợp** (`tools/test_rt8_tracking.cjs`): với thân lệch 12 cm, lùi giữ **−0,500 m/s** (ngân sách cũ 0,10 sẽ bóp); với tâm quay trôi 12 cm, xoay giữ **1,200 rad/s** (góc gãy cũ 0,16 sẽ còn 0,48).
- Bộ test hiện hành: `test_drive8_controller`, `test_drive8_dashboard`, `test_drive8_gateway`, `test_drive8_session`, `test_balance_recovery`, `test_net1_routes` + 2 test RT8: **PASS 8/8**.
- Biên dịch STM32 (arm-none-eabi `-Wall`) và ESP32 (Arduino CLI, PSRAM OPI): xem log build.
- **Chưa chạy trên xe thật.**

## Thử trên xe

1. Nạp cả hai board RT8/NET5. Mở `http://192.168.4.1/ui/r12?v=rt8-net5`.
2. **Đường gồ ghề**: chạy 50 cm/s qua đoạn xóc. Xe phải **không ngắt điều khiển**; nếu có ngắt, ghi lại thời điểm và tải log — xem `control_phase` và gyro quanh đó.
3. **Lùi**: giữ lùi 3–5 giây trên nền phẳng; tốc độ phải tương đương tiến (≈50 cm/s), không giật từng nhịp.
4. **Xoay**: giữ trái 2 giây rồi phải 2 giây; hai chiều phải gần bằng nhau, không bị “khựng” giữa cú xoay.
5. Dừng 2 giây, tải log TXT gửi lại. Cần xem: `control_phase` (kỳ vọng ít/không có 2), `motion_flags` bit 8/64, `v_ref` khi lùi, `w_ref` khi xoay.

## Giới hạn

- Ngưỡng gyro 150 °/s bỏ sót một cú đẩy rất mạnh nhưng ngắn; khi đó đường góc 8°/12° và sai lệch tốc độ vẫn bắt (chậm hơn 60 ms).
- Ngân sách 20 cm cho phép thân chậm hơn tham chiếu tới 20 cm trước khi bị bóp; các chặn cứng 1 m/4 s và kẹt 30 cm vẫn giữ.
- Nới bộ điều tiết tâm quay nghĩa là khi xoay mà tâm trôi mạnh, xe có thể dịch ngang nhiều hơn trước một chút (bắt đầu giảm sau 12 cm và chạm sàn 40% ở khoảng 18 cm).
- Các con số trên là từ **một** lượt chạy; cần log RT8 để xác nhận lại.

## Kiểm tra khi đồng bộ Git ngày 30/10/2026

Đã chạy thành công 9 CJS: controller, dashboard, gateway, session, recovery, net1 routes, RT8 tracking, RT8 replay và RT8 A/B. Replay có 7.585 mẫu, legacy=3 lần REC, RT8=0; đây là replay dữ liệu cố định, không phải mô phỏng khép kín quỹ đạo RT8 mới. STM32 và thư viện native đã build lại thành công; kết quả ESP32 ghi theo lần build thực tế khi bàn giao. Không nạp xe và chưa có log RT8 mới trên xe.
