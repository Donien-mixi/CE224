# Bản RT45 — dọn code (không đổi hành vi điều khiển)

| Board | Build ID |
|---|---|
| STM32F411 | `BAL-20261121-R12-8DIR-RT45-CLEAN` |
| ESP32-S3 | `TWIP-WEB-20261121-BAL-R12-8DIR-RT45-CLEAN-NET45` |

RT45 **không phải bản sửa lỗi**: đây là bản dọn code (tách module `.c`, gom thông số vào `tuning.h`,
bỏ comment lịch sử, tách hàm vòng 200 Hz). **Luật điều khiển, gain R12, observer, khung 122 B, các chốt
an toàn và toàn bộ thông số chỉnh được giữ nguyên giá trị** — hành vi phải giống RT44.

## 1. Kiểm chứng trước khi đóng gói

| Cổng | Kết quả |
|---|---|
| Host test | **38/38 bộ PASS** (42 tệp, 4 tệp rỗng) + 9 probe hằng số cũ phải fail |
| `test_uart_schedule.py`, `test_build_id_sync` | PASS |
| Mô phỏng 3 bộ × 12 ca | **73/73 tệp trùng** với trước khi dọn (chỉ `scilab_console.txt` khác đường dẫn) |
| Bước 1–3 của refactor | `.bin` **giống từng byte** bản trước (SHA256 `3921ebd5…`) |
| Bước 4 (`robot_fsm.c`) | Đối chiếu câu lệnh: **9 236 ký tự chuẩn hoá khớp** với bản cũ; build 0 lỗi 0 cảnh báo |
| Build | STM32 `text 84 080 / data 500 / bss 7 572`; ESP32 1 094 155 B (34 % flash) |

## 2. Nội dung gói

| Tệp | Việc |
|---|---|
| `STM32_BAL_R12_RT45_CLEAN.hex` | nạp STM32 qua ST-LINK (`-w ... -v -rst`) |
| `STM32_BAL_R12_RT45_CLEAN.bin` | cùng bản, dạng binary (đối chiếu/khôi phục) |
| `STM32_BAL_R12_RT45_CLEAN.elf` | kèm ký hiệu để soi lỗi |
| `ESP32_RT45_CLEAN_NET45.bootloader.bin` | nạp ở `0x0` |
| `ESP32_RT45_CLEAN_NET45.partitions.bin` | nạp ở `0x8000` |
| `ESP32_RT45_CLEAN_NET45.boot_app0.bin` | nạp ở `0xe000` |
| `ESP32_RT45_CLEAN_NET45.app.bin` | nạp ở `0x10000` |
| `SHA256.json` / `SOURCE_SHA256.json` | hash 7 binary + 20 tệp nguồn |
| `nap_rt45.bat` | nạp cả hai board, hỏi COM của ESP32 |

## 3. Nạp

Nháy đúp `nap_rt45.bat` → nhập COM của ESP32 (ví dụ `COM3`). **Phải nạp cả hai board** vì build ID
đổi ở cả hai đầu và ESP32 kiểm tra `$BINFO` của STM32.

Sau khi nạp, kiểm:

1. Banner web: **`RT45-CLEAN-NET45`**, ba nút **50 / 80 / MAX 90**, nút ĐO LẠI GYRO bấm được.
2. Serial ESP32: `[UART] … @ 921600 bps` và `[NET41] … -> AP chN`.
3. Trang `/ui/r12` mở được, log mới có `crc_errors=0`, `reset=power_on`.
4. BINFO trong log phải là `BAL-20261121-R12-8DIR-RT45-CLEAN`.

## 4. Lượt chạy nghiệm thu (quan trọng)

`robot_fsm.c` **không có test tự động** — 38 test và mô phỏng chỉ dùng header, nên bước tách hàm vòng
200 Hz **chưa có log chạy thật**. Lượt chạy đầu là bài nghiệm thu:

| Bước | Việc | Đạt khi |
|---|---|---|
| 1 | Kê cao bánh hoặc đứng yên, ARM | bánh không tự chạy, `state=3`, `fault=0` |
| 2 | Mỗi mốc 50/80 tiến 3 s → nhả → lùi 3 s → nhả | `v` bám `v_ref`, không cắt motor, `fault=0` |
| 3 | MAX 90 chỉ khi pin ≥ 12,0 V, chạy thẳng 5 s | `duty` p95 < 0,95, không REC1 |
| 4 | Rẽ 4 hướng ở 50/80 (mỗi hướng ≥ 2 s) | yaw 1,05–1,20 rad/s ở 80, không ngã |
| 5 | Đẩy nhẹ khi đứng yên, rồi giữ tay 6–8 s | vào REC1 rồi tự thoát, **không** cắt motor |
| 6 | STOP giữa lúc chạy | dừng ngay, `fault=0` |

Sau mỗi lượt: STOP → chờ ~2 s → tải log. Log chỉ dùng được khi mở đầu bằng `BINFO` có
`RT45-CLEAN` và `crc_errors=0`.

## 5. Quay lui

Bản đang chạy tốt: `Firmware/RT44_TURN_20261120` (hash đã kiểm, `nap_rt44.bat`). Nếu RT45 có bất
thường, nạp lại RT44 **cả hai board** rồi ghi lại hiện tượng + log RT45 để đối chiếu.

## 6. Giới hạn

- Không đổi thông số nào, nên **không kỳ vọng** xe chạy khác RT44. Nếu thấy khác, đó là hồi quy do
  refactor → quay lui và báo lại, kèm log hai bản cùng điều kiện.
- `.bin` RT45 khác byte RT44 (địa chỉ/độ inline đổi); so sánh phải dựa trên log cùng kịch bản, không
  so hash.
