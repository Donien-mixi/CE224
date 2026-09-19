# HƯỚNG DẪN NẠP VÀ VẬN HÀNH ESP32-S3 N16R8 TRÊN ARDUINO IDE

Thư mục này chứa toàn bộ mã nguồn firmware của Gateway điều khiển không dây tốc độ cao cho dự án Xe Tự Cân Bằng.

---

## 1. Yêu Cầu Phần Cứng & Đấu Nối Dây (Pinout Wiring)

| Chân ESP32-S3 N16R8 | Chân STM32F411CEU6 | Chức năng tín hiệu | Ghi chú |
| :--- | :--- | :--- | :--- |
| **GPIO18** | **PB6** | `ESP32 RX` $\leftarrow$ `STM32 TX (USART1)` | Truyền Telemetry góc nghiêng, vận tốc |
| **GPIO17** | **PB7** | `ESP32 TX` $\rightarrow$ `STM32 RX (USART1)` | Truyền lệnh lái Joystick, thông số PID |
| **GND** | **GND** | Nối chung đất (Common Ground) | **Bắt buộc nối chung đất để có mốc điện áp** |
| **5V / VIN** | **5V (Buck LM2596)**| Nguồn cấp 5V ổn định (>1A) | Không cấp chung với nguồn động cơ để tránh sụt áp |

---

## 2. Cài Đặt Trên Arduino IDE

### Bước 2.1: Cài đặt Board ESP32 (nếu chưa có)
1. Mở **Arduino IDE** $\rightarrow$ Chọn menu **File** $\rightarrow$ **Preferences**.
2. Tại mục *Additional boards manager URLs*, dán đường dẫn sau:
   ```
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Nhấn **OK**. Vào menu **Tools** $\rightarrow$ **Board** $\rightarrow$ **Boards Manager**, tìm kiếm `esp32` và bấm **Install** (chọn phiên bản mới nhất, từ 2.0.x hoặc 3.x).

### Bước 2.2: Cài đặt Thư viện WebSockets
1. Vào menu **Tools** $\rightarrow$ **Manage Libraries...** (hoặc nhấn tổ hợp phím `Ctrl + Shift + I`).
2. Nhập từ khóa: `WebSockets`.
3. Tìm thư viện **WebSockets** của tác giả **Markus Sattler**.
4. Nhấn nút **Install**.

---

## 3. Cấu Hình Thông Số Nạp Board ESP32-S3

Trước khi bấm nút nạp (Upload), vào menu **Tools** và chọn chính xác các thông số sau:

* **Board**: `"ESP32S3 Dev Module"`
* **Port**: Chọn cổng COM tương ứng của ESP32-S3 (ví dụ `COM3`, `COM5`,...)
* **USB CDC On Boot**: `"Enabled"` *(Rất quan trọng để in log ra Serial Monitor qua cổng Type-C)*
* **CPU Frequency**: `"240MHz (WiFi)"`
* **Flash Mode**: `"QIO 80MHz"`
* **Flash Size**: `"16MB (128Mb)"`
* **Partition Scheme**: `"16M Flash (3MB APP/9.9MB FATFS)"`
* **PSRAM**: `"OPI PSRAM"`
* **Upload Mode**: `"UART0 / Hardware CDC"`

---

## 4. Hướng Dẫn Vận Hành & Điều Khiển Qua Web HUD

1. Sau khi nạp thành công, ESP32-S3 sẽ tự động phát mạng Wi-Fi:
   * **Tên mạng (SSID)**: `TWIP_RACER_S3`
   * **Mật khẩu (Password)**: `12345678`
2. Sử dụng điện thoại hoặc máy tính kết nối vào mạng Wi-Fi trên.
3. Mở trình duyệt web (Chrome, Safari, Edge) và truy cập vào địa chỉ IP:
   ```
   http://192.168.4.1
   ```
4. Giao diện điều khiển cảm ứng tối tân (Cyber Dark HUD) sẽ hiển thị:
   * **Kéo cần ảo (Virtual Joystick)**: Đẩy lên để chạy tiến, kéo xuống để lùi, gạt trái/phải để bẻ lái mượt mà.
   * **Nút Đua Tốc Độ (Race)**: Kích hoạt góc ngả tối đa $15^\circ$ cho xe bứt tốc cực đại.
   * **Nút Lấy Zero IMU**: Tự động lấy lại mốc thăng bằng ban đầu.
   * **Nút DỪNG GẤP**: Cắt điện động cơ ngay lập tức nếu có sự cố.
   * **Menu Tinh chỉnh PID**: Cho phép thay đổi thông số $K_p, K_d, K_{p\text{vel}}, K_{i\text{vel}}$ trực tiếp từ màn hình điện thoại mà không cần cắm nạp lại code!
