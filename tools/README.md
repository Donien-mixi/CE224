> **Trạng thái đồ án · 18/11/2026.** Workspace ở **RT34-CORNER / NET35**: **32/32 host test pass**,
> mô phỏng native 36/36 ca đủ 20/20 s khi mức ≤ 80 và **35/36** ở MAX 90 (ca trượt là pin 11,2 V).
> Firmware phát hành ở [`Firmware/RT34_CORNER_20261117/`](../Firmware/RT34_CORNER_20261117/README.md).
> Xem [trạng thái kỹ thuật](../HANDOFF.md) và [tổng quan](../Docs/Tong_Quan_Do_An.md).

# Công cụ kiểm thử và build

## Chạy toàn bộ test

```powershell
Get-ChildItem tools/test_*.cjs | ForEach-Object { node $_.FullName }
```

Mỗi test tự biên dịch phần C/C++ tương ứng vào `.build/<tên_test>/` rồi chạy. Không test nào nạp
firmware hay mở cổng COM.

## Test đang dùng (32)

| Nhóm | Tệp | Kiểm cái gì |
|---|---|---|
| Luật điều khiển R12 | `test_production_controller`, `test_balance_governor`, `test_rt12_replay72`, `test_rt11_response`, `test_rt16_supervisor` | Quan sát 14 trạng thái, phân bổ lực, governor dự phòng |
| Quỹ đạo và giám sát | `test_rt17_motion`, `test_rt18_launch`, `test_rt19_hold`, `test_rt20_post_rec`, `test_rt21_antistall`, `test_rt22_sym`, `test_rt22_turn` | Khởi hành, mốc giữ, giới hạn sau REC1, chống kẹt, đối xứng tiến/lùi, bẻ hướng |
| Các lần sửa gần nhất | `test_rt23_interlock`, `test_rt24_sym`, `test_rt27_push`, `test_rt28_anchor`, `test_rt29_nocut` | Trần tốc độ có trễ, taper nghiêng, không đẩy theo vận tốc, giữ mốc khi bị đẩy, miễn hạn 4 s cho phanh do nhiễu |
| Bản chốt RT34 | `test_rt32_set`, `test_rt34_corner` | Ba mức 50/80/90, trần 0,90, bù bánh trái 110 PWM có kẹp; cua ở MAX hạ tham chiếu về ~0,80 mượt, thẳng không đổi — đều có probe phải fail với hằng số cũ |
| Gateway | `test_production_gateway`, `test_production_dashboard`, `test_production_session`, `test_net1_routes`, `test_net21_archive`, `test_net21_link` | Giao thức UART, trang web, phiên điều khiển, route log/archive |
| Driver | `test_bmi160_driver`, `test_encoder_runtime`, `test_balance_recovery` | Đọc IMU, đếm xung, REC1 |
| Dựng lại log cũ | `test_rt8_ab`, `test_rt8_replay`, `test_rt8_tracking` | Phát lại dữ liệu log RT8 |
| Đồng bộ bản | `test_build_id_sync` | Mọi literal build ID trong gateway/tools phải trùng `BALANCE_BUILD` của STM32 |
| Khác | `test_uart_schedule.py`, `test_balance_journal.cpp` | Lịch nhận UART, journal log (chạy tay, không nằm trong vòng `test_*.cjs`) |

Mỗi test sửa hành vi đều có **old-constant probe**: tự biên dịch lại với hằng số của bản trước và
**yêu cầu fail**, để chứng minh test bắt đúng lỗi chứ không phải chỉ chạy qua.

## Test đã ngừng dùng

`tools/legacy/` giữ 6 test của các bản RT8/RT9 và khối đo tốc độ bằng tay đã bị gỡ — không chạy được
trên firmware hiện hành. Danh sách và test tương đương ở [`legacy/README.md`](legacy/README.md).

## Script

- `build_stm32.ps1`: biên dịch STM32 hiện tại, không tự nạp. ESP32 build/nạp bằng Arduino IDE hoặc
  `arduino-cli` theo `../ESP32_S3_Gateway/README.md`.
- `run_governor_sim.cjs`: chạy mô phỏng native (xem `../Simulation/README.md`).
  `--stress` = 12 ca tốc độ, `--edge` = 12 ca biên, `--launch --launch50` = 12 ca khởi hành 50 cm/s.
- `run_load_speed_sim.cjs`: mô phỏng thử tải/đo tốc độ, không dùng để kết luận thông số.
- `Tai_Log_Cuu_Ho.*`: tải log qua cổng cứu hộ khi trang web không mở được.
- `log_manifest_template.csv`: mẫu ghi điều kiện từng lượt chạy xe.

## Điều kiện chạy trên máy khác

Node.js (đã kiểm với 24.19) từ gốc repo. Các test dùng GCC/G++ trỏ `C:/msys64/ucrt64/bin/`; cần cài
đúng hoặc chỉnh đường dẫn trong từng test. `build_stm32.ps1` cần STM32CubeIDE và cấu trúc `Debug/`
đã được build một lần; máy mới hãy build toàn project bằng CubeIDE trước.

`test_rt8_replay.cjs` cần `Simulation/data/raw/twip_balance_R12_27_replay.csv` (721 kB). Tệp này
**có** trong Git — `.gitignore` chỉ mở đúng một ngoại lệ cho nó; mọi dữ liệu đo khác không lên Git.
