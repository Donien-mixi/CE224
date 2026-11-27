> **Trạng thái đồ án · 21/11/2026.** Workspace ở **RT47-SLIM / NET47**: **38/38 host test** chạy được
> pass (38 tệp `test_*.cjs`); mô phỏng native **12/12 ca × 3 bộ** đủ 20/20 s.
> Firmware nạp bằng IDE từ source — xem [Docs/Nap_Bang_IDE](../Docs/Nap_Bang_IDE.md).
> Xem [trạng thái kỹ thuật](../HANDOFF.md) và [tổng quan](../Docs/Tong_Quan_Do_An.md).

# Công cụ kiểm thử và build

## Chạy toàn bộ test

```powershell
Get-ChildItem tools/test_*.cjs | ForEach-Object { node $_.FullName }
```

Mỗi test tự biên dịch phần C/C++ tương ứng vào `.build/<tên_test>/` rồi chạy. Không test nào nạp
firmware hay mở cổng COM.

## Test đang dùng (38 tệp chạy được)

| Nhóm | Tệp | Kiểm cái gì |
|---|---|---|
| Luật điều khiển R12 | `test_production_controller`, `test_balance_governor`, `test_rt12_replay72`, `test_rt11_response`, `test_rt16_supervisor` | Quan sát 14 trạng thái, phân bổ lực, governor dự phòng |
| Quỹ đạo và giám sát | `test_rt17_motion`, `test_rt18_launch`, `test_rt19_hold`, `test_rt20_post_rec`, `test_rt21_antistall`, `test_rt22_sym`, `test_rt22_turn` | Khởi hành, mốc giữ, giới hạn sau REC1, chống kẹt, đối xứng tiến/lùi, bẻ hướng |
| Các lần sửa gần nhất | `test_rt23_interlock`, `test_rt24_sym`, `test_rt27_push`, `test_rt28_anchor`, `test_rt29_nocut` | Trần tốc độ có trễ, taper nghiêng, không đẩy theo vận tốc, giữ mốc khi bị đẩy, miễn hạn 4 s cho phanh do nhiễu |
| Bản chốt RT34 | `test_rt32_set`, `test_rt34_corner` | Ba mức 50/80/90, trần 0,90, bù bánh trái 110 PWM có kẹp; cua hạ tham chiếu (RT36: 0,749), thẳng không đổi — đều có probe phải fail với hằng số cũ |
| RT36 (18/11) | `test_rt36_anchor`, `test_rt36_lateral`, `test_rt36_hold`, `test_rt36_brake` | Neo mốc chỉ khi duty > 0,95; trần ngang theo tốc độ lệnh (w_ref 1,15, 512=0); mốc chảy về vị trí khi đứng yên; phanh nhả 0,90 → 0 trong 1,3 s — mỗi test có probe hằng số cũ phải fail |
| RT37 (18/11) | `test_rt37_smooth` | Ngưỡng nghiêng governor 10°/13°: lệch 11,2° khi xuất phát không còn áp lực, 13,5° vẫn ghim; 10,0° khi chạy không ghim, 10,5° ghim; probe 9/11 phải fail |
| RT38 (18/11) | `test_rt38_continuous`, `test_rt38_recovery` | Đổi hướng 3 gói neutral không phanh; mốc vượt 20 cm giữ tốc độ; REC1 do gyro cần 60 ms khi có lệnh — probe hằng số cũ phải fail |
| Gateway | `test_production_gateway`, `test_production_dashboard`, `test_production_session`, `test_net1_routes`, `test_net21_archive`, `test_net21_link` | Giao thức UART, trang web, phiên điều khiển, route log/archive |
| Driver | `test_bmi160_driver`, `test_encoder_runtime`, `test_balance_recovery` | Đọc IMU, đếm xung, REC1 |
| Dựng lại log cũ | `test_rt8_ab`, `test_rt8_replay`, `test_rt8_tracking` | Phát lại dữ liệu log RT8 |
| Đồng bộ bản | `test_build_id_sync` | Mọi literal build ID trong gateway/tools phải trùng `BALANCE_BUILD` của STM32 |
| Khác | `test_uart_schedule.py`, `test_balance_journal.cpp` | Lịch nhận UART, journal log (chạy tay, không nằm trong vòng `test_*.cjs`) |

Mỗi test sửa hành vi đều có **old-constant probe**: tự biên dịch lại với hằng số của bản trước và
**yêu cầu fail**, để chứng minh test bắt đúng lỗi chứ không phải chỉ chạy qua.

## Dữ liệu / tiện ích dùng chung

- `lib/gateway_mock.cjs`: mock C của gateway (nguồn bản RT8) để `test_net1_routes` kiểm route/CRC.
- `lib/legacy_headers.cjs`: dựng bản header hằng số cũ cho 3 test phát lại log RT8.
- 6 test RT8/RT9 cũ (cần khôi phục firmware RT8 từ git mới chạy) đã xoá ở RT47; còn trong git history.

## Script

- `run_governor_sim.cjs`: chạy mô phỏng native (xem `../Simulation/README.md`).
  `--stress` = 12 ca tốc độ, `--edge` = 12 ca biên, `--launch --launch50` = 12 ca khởi hành 50 cm/s.
- `Tai_Log_Cuu_Ho.*`: tải log qua cổng cứu hộ khi trang web không mở được.
- `log_manifest_template.csv`: mẫu ghi điều kiện từng lượt chạy xe.

## Điều kiện chạy trên máy khác

Node.js (đã kiểm với 24.19) từ gốc repo. Các test dùng GCC/G++ trỏ `C:/msys64/ucrt64/bin/`; cần cài
đúng hoặc chỉnh đường dẫn trong từng test. Bản build do CubeIDE tạo `Debug/` ở lần **Build** đầu tiên.

`test_rt8_replay.cjs` cần `Simulation/data/raw/twip_balance_R12_27_replay.csv` (721 kB). Tệp này
**có** trong Git — `.gitignore` chỉ mở đúng một ngoại lệ cho nó; mọi dữ liệu đo khác không lên Git.
