> ESP32 hiện là `8DIR-RT4-NET1`, tương thích STM32 `8DIR-RT4`. Nếu STM32 đã là RT4 thì chỉ cần nạp lại ESP32. Xem [hướng dẫn](../README.md).

Đo lại gyro khóa các thao tác bật/thử bánh tới khi có dữ liệu xác nhận hoàn tất. STM32 hủy lệnh motor đang chờ và yêu cầu kiểm tra đứng yên lại đủ 200 ms; hiệu chuẩn không tự bật motor.

# ESP32-S3 Gateway — R12 RT4 NET1

NET1 bổ sung chẩn đoán reset và log Flash dự phòng, không đổi điều khiển STM32. Web `http://192.168.4.1/ui/r12?v=rt4-net1`. SSID vẫn `TWIP_RACER_S3`, AP kênh6/HT20, tắt power-save và đặt TX17dBm; không phát lại luồng TEL thừa cho dashboard R12. Lệnh giữ/nhả gửi ngay qua WebSocket, làm mới40ms; cập nhật giao diện200ms, bảng chi tiết500ms. Mất kết nối không tự chạy lại và không xóa log RAM.

Mục **Kết nối và log dự phòng** hiển thị số phiên khởi động, `esp_reset_reason`, bộ nhớ thấp nhất, số lần rời Wi-Fi và khoảng ngắt vòng web. Nút **TẢI CHẨN ĐOÁN KẾT NỐI** xuất JSON. `brownout` là ESP32 báo sụt áp; `power_on` cũng có thể do mất nguồn hoàn toàn; chỉ nhìn biểu tượng5G không đủ xác định nguyên nhân.

Log dự phòng dùng phân vùng FAT có sẵn của cấu hình16MB; không đổi phân vùng. Mount không phá dữ liệu: chỉ tự khởi tạo khi toàn bộ phân vùng còn0xFF. Nếu phân vùng có dữ liệu mà không mount được, giữ nguyên và báo lỗi. Tác vụ riêng chép tối đa50frame mỗi đợt, kiểm CRC và flush; sau reset chỉ xuất các đợt hợp lệ. Không giữ captureMutex khi ghi Flash hoặc gửi mạng. Tối đa4file, không tự xóa/ghi đè. Đầy thì báo rõ và dừng ghi dự phòng; log RAM và điều khiển vẫn hoạt động.

Khi dừng và chờ2s, bấm **TẢI LOG FLASH** ở file tương ứng. Sau khi đã lưu, có thể xóa bản cũ. Để xóa bản của phiên RAM hiện tại, tải trước rồi bấm xóa log RAM; điều này không xóa bản Flash. Ghi Flash là dự phòng tốt nhất có thể, không phải nguồn dự phòng: mất nguồn giữa đợt có thể mất phần cuối, hoặc làm hỏng FAT. Không chọn **Erase All Flash** khi muốn giữ log qua lần nạp firmware.

Trên iPhone, thử bật chế độ máy bay rồi bật lại Wi-Fi để loại việc tự chuyển sang mạng di động; giữ trang điều khiển phía trước. Mạng nội bộ không có Internet là bình thường. Nếu AP biến mất/xuất hiện lại và log RAM về0, kiểm tra lý do reset; đo5V tại chính chân cấp nguồn ESP32 khi motor khởi động/đảo chiều, không chỉ đo đầu ra XL4015 lúc không tải. Kiểm tra3V3, dây5V/GND và đường hồi dòng motor. Không tắt brownout/watchdog để che lỗi nguồn. Ăng-ten cần thoáng, tránh kim loại/dây động lực sát phần ăng-ten.

Mở `ESP32_S3_Gateway.ino` trong Arduino IDE. Cấu hình đã biên dịch: ESP32 core 3.3.11, thư viện WebSockets 2.7.2 của Markus Sattler.

- Board: ESP32S3 Dev Module (N16R8).
- USB CDC On Boot: Enabled.
- Flash Size: 16 MB.
- Partition Scheme: `app3M_fat9M_16MB` (3 MB APP).
- PSRAM: OPI PSRAM, bắt buộc để lưu log.

UART 115200 8N1: GPIO18 RX nối PB6 USART1 TX; GPIO17 TX nối PB7 USART1 RX; chung GND. Nạp STM32 cùng phiên bản giao thức.

Wi-Fi/AP được cấu hình trong sketch. Mở http://192.168.4.1/ui/r12?v=rt4-net1 . Web hiển thị trạng thái, lỗi, góc, tốc độ, PWM, encoder và tải log. Điều khiển giữ một trong8nút; giữ thì thực hiện liên tục, thả thì bỏ mục tiêu cũ và hãm về cân bằng tại điểm mới. Tiến/lùi có quãng đường hãm; quay đặt về0 ngay khi nhận neutral. Không xếp hàng lệnh giữ khi mạng chậm. Mặc định 50 cm/s, tùy chọn 72 cm/s đang cần thử thực tế. Không có giới hạn tổng quãng đường khi điều khiển liên tục; các giới hạn phục hồi và mất liên lạc vẫn được giữ.

Khi đổi tốc độ hoặc sau hủy mục tiêu do kẹt/phục hồi, cần nhả rồi giữ nút lại. Va chạm nghiêm trọng dừng motor, cần dựng xe và bật cân bằng lại. Khi bộ nhớ log đầy, tải log rồi xóa khi xe đã dừng trước lượt tiếp theo.
