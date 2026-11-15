# RT21-QUICKSTART — hiệu chuẩn gyro lúc cấp nguồn còn ~2 s

> **ĐÃ KHÔI PHỤC ngày 14/11/2026 — KHÔNG nạp bản này.** Theo yêu cầu, firmware trở
> lại cửa sổ **500 mẫu (~5,15 s)**: cặp hiện hành là **RT21-ANTISTALL / NET21**
> (`.build/balance_release_r12_rt21_antistall_net21_20261113`, web `?v=RT21-NET21`,
> STM32 `BAL-20261111-...RT21-ANTISTALL`). Binary sau khi khôi phục **trùng hash**
> bản 13/11 (`a1509a52…` cho STM32, `23eb1154…` cho app ESP32). Tài liệu này giữ
> lại số đo 5,15 s → 2,05 s và lập luận `1/√N` như hồ sơ thí nghiệm, không phải
> hướng dẫn nạp.

Bản thí nghiệm 14/11/2026 (nay đã khôi phục): STM32
`BAL-20261114-R12-8DIR-RT21-ANTISTALL-QUICKSTART`, ESP32
`TWIP-WEB-20261114-BAL-R12-8DIR-RT21-ANTISTALL-QUICKSTART-NET21` (gồm cả thay đổi
NET21). Artifact thí nghiệm: `.build/balance_release_r12_rt21_quickstart_net21_20261114/`
— **giữ để đối chiếu, không nạp**. **Chưa nạp xe, chưa có log trên xe** ở cả hai bản.
Luật điều khiển R12, gain/observer/phân bổ lực, encoder, REC1 và khung 122 byte
không đổi.

## 1. Con số

| | Trước (RT21-ANTISTALL) | Nay (QUICKSTART) |
|---|---|---|
| Mẫu đo bias lúc boot | 500 | **200** |
| Thời gian đo (mỗi mẫu `HAL_Delay(10)` + 1 lần đọc I2C ~0,3 ms) | **5,15 s** | **2,05 s** |
| Frame telemetry đầu tiên sau cấp nguồn | ~5,3 s | **~2,2 s** |
| `ready` (cho phép BẬT) sau đó | +0,2 s | +0,2 s |
| ĐO LẠI GYRO trên web | 500 mẫu / 5,15 s | **giữ nguyên 500 / 5,15 s** |

Đo bằng mã: `BMI160_Calibrate_Gyro(samples)` lặp `samples` lần, mỗi lần đọc 12 byte
qua I2C rồi `HAL_Delay(10)`; ODR gyro đặt 100 Hz nên 10 ms/mẫu là nhịp tối thiểu để
không đếm trùng một lần cập nhật. 500 × 10,3 ms = 5,15 s; 200 × 10,3 ms = 2,06 s.

Trong suốt cửa sổ đó TIM4 **chưa chạy** (phải dừng để tránh đọc I2C chồng chéo) nên
STM32 không gửi frame nào; trang web hiện *"Chưa nhận STM32 đang hoạt động"* và nút
BẬT bị khóa. Vì vậy **cái chờ không phải Wi-Fi**: cắm điện → 5,3 s không có dữ liệu →
0,2 s ready → mới bấm BẬT được.

## 2. Vì sao 200 mẫu là đủ

Sai số của giá trị trung bình giảm theo `1/√N`. Với BMI160 ở 100 Hz, độ lệch chuẩn
một mẫu cỡ 0,05–0,12 °/s:

- 500 mẫu → sai số bias ≈ 0,002–0,005 °/s
- 200 mẫu → sai số bias ≈ 0,004–0,008 °/s

Cả hai đều nhỏ hơn cổng chấp nhận của chính firmware (gyro mỗi trục < 6 °/s để ARM) và
nhỏ hơn ngưỡng phương sai 0,04 (độ lệch 0,2 °/s). Quan trọng hơn: `200` **đã là mức sàn
mà hàm hiệu chuẩn tự chấp nhận** (`calibration_ok = valid==samples && samples>=200 &&
!moved`), nên không phải nới tiêu chí nào — chỉ dùng đúng biên đã có.

## 3. Những gì KHÔNG đổi (bảo toàn chức năng)

- Tiêu chí chấp nhận giữ nguyên: đủ mẫu, không mẫu nào > 3 °/s (`moved`), phương sai
  mỗi trục ≤ 0,04; sai thì `calibration_ok=false` → fault 128 → khóa ARM như cũ.
- `BMI160_Calibrate_Gyro` không sửa một dòng nào (chỉ đổi số mẫu truyền vào).
- ĐO LẠI GYRO (`$CALIB`) vẫn 500 mẫu; trang vẫn chờ 9 s và báo kết quả như cũ.
- TIM4 vẫn chỉ chạy sau hiệu chuẩn; nhịp 200 Hz, cửa sổ ready 200 ms, mọi cổng ARM
  (±60°, gyro < 6 °/s, bánh < 0,05 m/s, norm, chu kỳ 4–6 ms), lease 350 ms, STOP,
  REC1, governor, R12: không đổi.
- Không lưu bias qua lần tắt nguồn: đây là thứ sẽ phải bỏ nếu muốn "0 giây", và nó
  không an toàn (nhiệt độ/thay board làm bias lệch; log RT16 từng có metadata bias
  6 °/s khi xe bị rung lúc hiệu chuẩn).

## 4. Lộ trình từ cắm điện đến lúc xe chạy

| Mốc | Thời điểm (điển hình) |
|---|---|
| Cấp nguồn → HAL/clock/I2C init | ~0,1 s |
| Hiệu chuẩn gyro 200 mẫu | ~2,05 s (trước: 5,15 s) |
| Đọc IMU, cấu hình, `Filter_Init`, TIM4 chạy | ~2,2 s → **có frame đầu tiên** |
| `ready` (xe đứng yên 200 ms, gyro < 6 °/s, bánh < 0,05 m/s) | ~2,4 s |
| Bấm BẬT → STM32 nhận `$ARM` | ≤ 1 nhịp 100 ms của trang + HTTP |
| `motion_ready` sau ARM | ~0,2–0,5 s |
| Bấm hướng → xe chạy | ngay sau đó |

Tổng thực tế **~2,5–3 s** từ cắm điện, so với ~5,5–6 s trước đây. Nếu xe **đã cấp nguồn
và đã hiệu chuẩn** thì chỉ cần mở lại trang: không hiệu chuẩn lại, chỉ chờ BINFO (≤1 s,
gửi 1 Hz) và một mẫu tươi (≤0,5 s) là bấm BẬT được.

Không thể về 0 giây: muốn ước lượng bias phải có tối thiểu ~1 s dữ liệu đứng yên. Nếu
sau này cần nhanh hơn nữa, mức khả thi là 100 mẫu (~1,03 s) nhưng phải hạ sàn
`samples>=200`, tức đổi tiêu chí chấp nhận — chưa làm.

## 5. Kiểm chứng

- `tools/test_production_controller.cjs` được thêm hai assert: boot gọi hiệu chuẩn
  **200** mẫu, `$CALIB` gọi **500** mẫu. Đã chứng minh test **fail trước khi sửa** bằng
  cách tạm đặt lại hằng số 500 → `Assertion failed: cal_n==1&&cal_counts[0]==200`; trả
  về 200 → PASS.
- 22/22 test host đang dùng pass; `test_uart_schedule.py` pass; `test_balance_journal` pass.
- 36 lượt mô phỏng sau khi đổi hằng số: **stress 12** và **edges 12** cho kết quả y hệt
  RT21 (20,000/20 s, không fault); bộ thứ ba (nhãn `launch50`, 12 ca) có file
  `summary.csv` **trùng byte** với stress, tức cờ `--launch50` không đổi kịch bản trong
  lần chạy này — không tính nó là bằng chứng độc lập. Điều cần chứng minh là luật điều
  khiển không đổi, và header điều khiển/hash đều trùng RT21.
- Build STM32: text 81 528 B (RT21: 81 520 B, +8 B do hai hằng số). ESP32 app 1 086 736 B.

## 6. Nạp và thử

1. Nạp `STM32_BAL_R12_RT21_QUICKSTART.bin/.hex` và `ESP32_RT21_QUICKSTART_NET21.merged.bin`
   (hoặc nạp sketch bằng Arduino IDE). Phải nạp **cả hai** vì web kiểm build ID.
2. `http://192.168.4.1/ui/r12?v=RT21-QUICKSTART-NET21`; trang phải hiện **MAX 80** và
   `BAL-20261114-...QUICKSTART`.
3. Bấm đồng hồ: từ lúc cắm pin đến khi nút **BẬT CÂN BẰNG** bấm được phải ~2–2,5 s.
4. Giữ xe yên khi cấp nguồn (như cũ). Nếu cố tình rung, hiệu chuẩn sẽ bị từ chối và
   fault 128 khóa ARM — đúng như thiết kế.
5. Nếu thấy xe trôi góc khi đứng yên (bias kém), bấm **ĐO LẠI GYRO** để chạy bản 500 mẫu
   rồi chạy lại. Ghi lại bias trong BINFO (3 số `%.6f` sau PPR/hằng số deadband) để so
   giữa các lần cấp nguồn.

## 7. Giới hạn

- Chưa đo trên xe: chưa có bằng chứng bias 200 mẫu ổn định bằng 500 trên chính board này.
  Bằng chứng hiện có là lý thuyết `1/√N`, mức sàn 200 của chính firmware, và test host.
- Chưa đo thời gian thật từ cấm điện đến frame đầu tiên bằng đồng hồ; con số 2,2 s là
  tính từ mã (200 × 10,3 ms + init).
- Nếu log NET sau khi nạp cho thấy `pitch` trôi > 0,1 °/s khi xe đứng yên, quay lại 500
  mẫu hoặc bật lại đường "đo dài khi đứng yên trước khi ARM".
