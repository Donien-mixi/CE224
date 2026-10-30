# Công cụ còn dùng

- `build_stm32.ps1`: biên dịch STM32 hiện tại, không tự nạp. ESP32 được biên dịch/nạp bằng Arduino IDE theo README của gateway.
- `Tai_Log_Cuu_Ho.*`: tải log khi giao diện gặp sự cố.
- `test_drive8_*`, `test_balance_*`, `test_net1_routes.cjs`: kiểm tra hồi quy điều khiển và log.
- `test_rt8_*`: kiểm tra sửa lỗi RT8; replay dùng dữ liệu trong `Simulation/data/raw/`.
- `test_hardware_boot.cjs` và `lib/`: nguồn mock/phụ thuộc của bộ kiểm thử; cần giữ.
- `test_uart_schedule.py`: kiểm tra logic tiếp nhận UART.

Kết quả kiểm thử tự sinh trong `.build/`, có thể xóa sau khi chạy. `.build/balance_release_r12_rt8_20261024` là bản nạp hiện tại đã giữ lại; RT7 là bản dự phòng. Chạy mô phỏng theo `Simulation/README.md`, không cần các bản build cũ.

## Điều kiện chạy trên máy khác

Chạy CJS bằng Node.js từ gốc repo. Các test dùng GCC/G++ hiện trỏ `C:/msys64/ucrt64/bin/`; cần cài đúng hoặc điều chỉnh đường dẫn theo máy. `build_stm32.ps1` là helper riêng cho laptop, cần STM32CubeIDE 2.1.1 và cấu trúc Debug/HAL đã được build; trên máy mới hãy build toàn project bằng CubeIDE trước.

`test_rt8_replay.cjs` cần `Simulation/data/raw/twip_balance_R12_27_replay.csv` trên máy tác giả. File dữ liệu đo này không đưa lên Git; bộ 12 mô phỏng tổng quan và các test còn lại không cần replay27.
