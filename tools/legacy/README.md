# Test của các bản firmware đã bỏ

Sáu tệp ở đây thuộc các bản **RT8 / RT9 / đo tốc độ bằng tay** — những tính năng đã bị gỡ khỏi
firmware hiện hành, nên các test này **không còn chạy được** và không phản ánh lỗi của bản RT29:

| Tệp | Vì sao hết hiệu lực | Test tương đương hiện dùng |
|---|---|---|
| `test_drive8_controller.cjs` | Dựng mock từ `Core/Inc/load_speed_test.h` — tệp đã xoá cùng tính năng đo tốc độ. | `test_production_controller.cjs`, `test_rt22_sym.cjs` |
| `test_drive8_dashboard.cjs` | Đọc các `id` của trang web bản RT8 (`fixture`, bảng đo cũ) — trang hiện tại đã đổi hẳn. | `test_production_dashboard.cjs` |
| `test_drive8_gateway.cjs` | Gọi `archiveFormatRequest()` — hàm đã đổi tên thành `archiveFormatNow()`. | `test_production_gateway.cjs`, `test_net21_archive.cjs` |
| `test_drive8_session.cjs` | Kỳ vọng chuỗi lệnh MOVE8 của bản RT8 (thiếu `drive_epoch`). | `test_production_session.cjs` |
| `test_hardware_boot.cjs` | Dựng mock cho `manual_job`, `test_stage`, `TestSample()` — khối đo tốc độ bằng tay đã gỡ. | `test_rt29_nocut.cjs`, `test_rt23_interlock.cjs` (đường cắt motor) |
| `test_load_speed.cjs` | Cần `Core/Inc/load_speed_test.h` — không còn trong firmware. | `tools/run_load_speed_sim.cjs` |

Ghi chú: `tools/test_net1_routes.cjs` **có** đọc `legacy/test_drive8_gateway.cjs` để lấy khung
mock C (fake filesystem) — chỉ tái dùng phần mock, không dùng assertion nào ở đây.

`factory/` giữ các header đã gỡ khỏi firmware (đo tốc độ bằng tay, hiệu chuẩn nâng cao, LOADTEST,
trang đo cũ của ESP32: `esp32/`, `stm32/`). Đây là dữ liệu đối chiếu lịch sử, **không** được include
hay cấp phát bộ nhớ trong bản vận hành.

Muốn chạy lại để đối chiếu lịch sử thì cần khôi phục đúng firmware RT8/RT9 từ Git
(commit `b95cd71` và trước đó), không chạy trực tiếp trên bản hiện hành.
