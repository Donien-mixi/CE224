> Bản 8 hướng: nạp cả hai board cùng phiên bản `SPEED2-8DIR`. Xem [hướng dẫn](../README.md).

# ESP32-S3 Gateway — R12 SPEED2

Mở `ESP32_S3_Gateway.ino` trong Arduino IDE. Cấu hình đã biên dịch: ESP32 core 3.3.11, thư viện WebSockets 2.7.2 của Markus Sattler.

- Board: ESP32S3 Dev Module (N16R8).
- USB CDC On Boot: Enabled.
- Flash Size: 16 MB.
- Partition Scheme: `app3M_fat9M_16MB` (3 MB APP).
- PSRAM: OPI PSRAM, bắt buộc để lưu log.

UART 115200 8N1: GPIO18 RX nối PB6 USART1 TX; GPIO17 TX nối PB7 USART1 RX; chung GND. Nạp STM32 cùng phiên bản giao thức.

Wi-Fi/AP được cấu hình trong sketch. Mở http://192.168.4.1/ui/r12 . Web hiển thị trạng thái, lỗi, góc, tốc độ, PWM, encoder và tải log. Điều khiển giữ nút TIẾN/LÙI, nhả để giảm tốc. Mặc định 40 cm/s, tùy chọn 60 cm/s đang cần thử thực tế. Không có giới hạn tổng quãng đường khi điều khiển liên tục; các giới hạn phục hồi và mất liên lạc vẫn được giữ.

Khi đổi tốc độ hoặc sau hủy mục tiêu do kẹt/phục hồi, cần nhả rồi giữ nút lại. Va chạm nghiêm trọng dừng motor, cần dựng xe và bật cân bằng lại. Khi bộ nhớ log đầy, tải log rồi xóa khi xe đã dừng trước lượt tiếp theo.
