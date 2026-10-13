# Gateway ESP32-S3 N16R8

## Build và nạp

Mở `ESP32_S3_Gateway.ino` trong Arduino IDE. Bộ thư viện đã dùng để build: **ESP32 core3.3.11**, **WebSockets2.7.2** của Markus Sattler; WiFi/WebServer đi kèm ESP32 core.

- Board: ESP32S3 Dev Module.
- USB CDC On Boot: Enabled.
- Flash Size: 16MB.
- Partition Scheme: 16M Flash, 3MB APP / khoảng9,9MB FATFS.
- PSRAM: **OPI PSRAM**; cần cho lưu log. Firmware chặn biên dịch nếu thiếu cấu hình PSRAM.
- Chọn cổng COM tương ứng bo thực tế, Upload. Không có số COM cố định trong repo.

Cấu hình tương đương cho Arduino CLI:

```text
esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi
```

## Giao tiếp STM32

| ESP32 | STM32 | Chức năng |
|---|---|---|
| GPIO18 RX | PB6 USART1 TX | Nhận dữ liệu |
| GPIO17 TX | PB7 USART1 RX | Gửi lệnh |
| GND | GND | Chung mốc điện áp |

UART115200,8N1. Cần nạp cảhai board với giao thức tương thích. Binary frame/CRC dùng `balance_wire.h`, job đo dùng `wheel_measurement.h`; hai bản header tương ứng trong STM32 phải đồng bộ.

## Vận hành hiện hành

Kết nối Wi-Fi `TWIP_RACER_S3`, mật khẩu `12345678`, mở `http://192.168.4.1`.

- **Xe đang kê cao:** cố định thân, bánh quay tự do, xác nhận rồi chọn trái/phải/cảhai. Mỗi lần PWM+600 tối đa1,5giây; đây không phải tự cân bằng.
- **Xe đặt trên nền:** giữ nhẹ thân, xác nhận và bấm BẬT trước. Web chờ tối đa20giây, thanh góc hướng dẫn dựng xe. STM32 kiểm tra±2°, gyro<3°/s, bánh<0,02m/s, góc lọc/accel lệch≤0,75°, giữ300ms. Khi xác nhận ĐANG CHẠY CÂN BẰNG, thả nhẹ tay.
- Gyro được đo lúc khởi động. Không cần đo lại trước mỗi lượt; nếu đo lại thì cố định5giây và chờ thêm1,5giây. Không lấy tư thế lúc đo làm mốc góc0°.
- **DỪNG MOTOR NGAY** ngắt motor. Xe còn tự ngắt theo góc25°, quãng encoder30cm, tốc độ0,8m/s, lỗi cảm biến/chu kỳ hoặc mất nhịp điều khiển.
- Giữ trang điều khiển sáng/ở phía trước. Chuyển ứng dụng, khóa màn hình hoặc rời trang có thể gửi STOP. Dùng thiết bị khác quay video.

Sau motor dừng, đợi2giây rồi tải TXT. Một lượt mộtfile; kiểm tra đã tải rồi mới xóa. ESP32 lưu trong RAM, mất nguồn sẽ mất log chưa tải. Log không ghi đè khi đầy; thêm tối đa1giây trước ARM và2giây sau dừng. Chưa đo điện áp pin/dòng motor thực tế.

Trang đo nâng cao `/calibration` và `/wheel-tests` giữ các bài đo linh kiện. Đọc chuẩn bị của từng bài trước khi chạy. Sau vào chế độ đo khóa, khởi động lại STM32 để trở về trang chạy cân bằng.

## Các file chính

- `dashboard_html.h`: trang vận hành chính.
- `balance_web.h`: lệnh vận hành, trạng thái và xuất log.
- `control_session.h`: chủ điều khiển, heartbeat, nguồn STOP và chống lệnh cũ.
- `balance_preroll.h`: cửa sổ log trước ARM, bộ nhớ cố định.
- `measurement_web.h`, `job_log_store.h`, `wheel_tests_html.h`: bài đo và lưu log.
- `index_html.h`: giao diện nâng cao còn được các route sử dụng; cần giữ khi build.
- `firmware_build.h`: phiên bản web và đường dẫn trang.

Thông số PID/hiệu chuẩn được chốt trong STM32. Các lệnh PID/trim/đua cũ không được coi là có hiệu lực ở bản cân bằng hiện hành; xem bộ phân tích lệnh trong `Core/Src/esp32_comm.c` trước khi mở lại tính năng.
