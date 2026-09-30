# HƯỚNG DẪN NẠP VÀ VẬN HÀNH GATEWAY ESP32-S3 N16R8 (WEB HUD & IOT)

Tài liệu này cung cấp toàn bộ hướng dẫn kỹ thuật chi tiết từ việc lựa chọn cổng cáp, cấu hình phần mềm, nạp firmware qua cổng `COM` / `USB`, đến vận hành giao diện điều khiển thời gian thực (Cyber Dark Web HUD) cho xe cân bằng 2 bánh.

---

## 1. Yêu Cầu Phần Cứng & Đấu Nối Dây (Pinout Wiring)

Theo chuẩn hóa tại [Huong_Dan_Di_Day_Toan_Bo_He_Thong.md](file:///d:/DA_HTN/Huong_Dan_Di_Day_Toan_Bo_He_Thong.md#L215-L224):

| Chân ESP32-S3 N16R8 | Chân STM32F411CEU6 | Chức năng tín hiệu | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- |
| **`GPIO18`** *(RX)* | **`PB6`** *(TX)* | `ESP32 RX` $\leftarrow$ `STM32 TX (USART1)` | Nhận chuỗi Telemetry 20Hz (góc nghiêng, vận tốc, PWM, pin) |
| **`GPIO17`** *(TX)* | **`PB7`** *(RX)* | `ESP32 TX` $\rightarrow$ `STM32 RX (USART1)` | Truyền lệnh lái `$CMD`, lệnh dừng `$STOP`, thông số `$PID`, `$TRIM` |
| **`GND`** | **`GND`** | Nối chung đất qua Đường ray GND Breadboard | **Bắt buộc chung mốc 0V** với STM32 và nguồn hệ thống |
| **`5V`** *(hoặc `VIN`)*| **Đường ray 5V (+)** | Nhận 5.00V sạch từ Mạch Buck XL4015 (qua WAGO 2) | Có tụ hóa $1000\mu F$ trên ray để chống sụt áp khi phát sóng Wi-Fi |

> [!NOTE]
> Khi cắm cáp nạp Type-C từ máy tính vào ESP32, **không cần rút 4 dây trên**. Máy tính sẽ tự cấp nguồn nuôi tạm thời cho ESP32.

---

## 2. Phân Biệt 2 Cổng Type-C Trên Bo ESP32-S3 & Cách Chọn Cổng Nạp

Hầu hết các bo mạch ESP32-S3 N16R8 (như DevKitC-1) được trang bị **2 cổng cắm Type-C**:

```
                       ┌────────────────────────┐
                       │      ESP32-S3 BOARD    │
                       ├────────────┬───────────┤
                       │  CỔNG 1    │   CỔNG 2  │
                       │ [COM/UART] │   [USB]   │
                       └──────┬─────┴─────┬─────┘
                              │           │
                              ▼           ▼
        Qua chip cầu nối CH343 / CP2102   Ăn thẳng vào ruột chip ESP32-S3
        (Tự động reset, cực kỳ ổn định)   (Native USB CDC / OTG)
```

### So Sánh Kỹ Thuật:
1. **Cổng `COM` (hoặc `UART`) — KHUYẾN NGHỊ**:
   * Đi qua chip nạp chuyên dụng (CH343, CP2102 hoặc CH340).
   * **Ưu điểm**: Có mạch tự động kích hoạt chế độ nạp (`DTR`/`RTS` Auto-Reset). Khi bấm **Upload** trên máy tính, mạch tự động kéo chip vào bootloader mà **không cần giữ nút BOOT**. Cổng COM luôn cố định trên Windows, không bị mất kết nối khi chip khởi động lại.
   * **Serial Monitor**: Hiển thị log debug khởi động và địa chỉ IP `192.168.4.1` ngay lập tức.
2. **Cổng `USB` (Native USB)**:
   * Ăn trực tiếp vào chân GPIO19 (D-) và GPIO20 (D+) của vi điều khiển ESP32-S3.
   * Đòi hỏi cấu hình `USB CDC On Boot = Enabled` trong Arduino IDE. Đôi khi cần nhấn giữ nút `BOOT` + `RST` thủ công để vào chế độ nạp ROM nếu firmware cũ đang chiếm dụng cổng USB.

---

## 3. Cài Đặt Môi Trường Trên Arduino IDE

### Bước 3.1: Cài đặt gói Board ESP32
1. Mở **Arduino IDE** $\rightarrow$ Chọn **File** $\rightarrow$ **Preferences**.
2. Tại ô *Additional boards manager URLs*, dán liên kết:
   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```
3. Bấm **OK**. Vào **Tools** $\rightarrow$ **Board** $\rightarrow$ **Boards Manager**, tìm kiếm `esp32` và bấm **Install** (khuyên dùng bản mới nhất 2.0.14 trở lên hoặc 3.x).

### Bước 3.2: Cài đặt Thư viện WebSockets
1. Vào **Tools** $\rightarrow$ **Manage Libraries...** (hoặc tổ hợp phím `Ctrl + Shift + I`).
2. Gõ tìm kiếm: `WebSockets`.
3. Tìm đúng thư viện **WebSockets** của tác giả **Markus Sattler** $\rightarrow$ Nhấn **Install**.

---

## 4. Cấu Hình Nạp Code (Menu Tools)

Mở file [ESP32_S3_Gateway.ino](file:///d:/DA_HTN/ESP32_S3_Gateway/ESP32_S3_Gateway.ino) trong Arduino IDE, vào menu **Tools** và chọn chính xác các thông số:

* **Board**: `"ESP32S3 Dev Module"`
* **Port**: Chọn đúng cổng COM của bo mạch (kiểm tra trong *Device Manager* $\to$ *Ports (COM & LPT)*)
* **USB CDC On Boot**: `"Enabled"` *(Bắt buộc bật)*
* **CPU Frequency**: `"240MHz (WiFi)"`
* **Flash Mode**: `"QIO 80MHz"`
* **Flash Size**: `"16MB (128Mb)"`
* **Partition Scheme**: `"16M Flash (3MB APP/9.9MB FATFS)"`
* **PSRAM**: `"OPI PSRAM"`
* **Upload Mode**: `"UART0 / Hardware CDC"`

Bấm nút **Upload** (mũi tên $\rightarrow$).

> [!TIP]
> **XỬ LÝ LỖI "Failed to connect to ESP32-S3"**:
> Nếu phần mềm báo `Connecting........_____.....` rồi báo lỗi, bạn chỉ cần đưa chip về chế độ nạp thủ công:
> 1. Nhấn và **giữ nguyên** nút **`BOOT`** trên bo ESP32-S3.
> 2. Bấm nhả nút **`RST`** 1 cái (trong khi tay vẫn giữ `BOOT`).
> 3. **Thả tay** khỏi nút `BOOT`.
> 4. Bấm lại nút **Upload** trên Arduino IDE là code sẽ nạp vào 100% thành công.

---

## 5. Hướng Dẫn Vận Hành Web HUD & Điều Khiển Không Dây

### 5.1. Kết Nối Mạng Wi-Fi SoftAP
Khi được cấp nguồn, ESP32-S3 tự động phát mạng Wi-Fi riêng biệt (không cần internet):
* **Tên mạng (SSID)**: `TWIP_RACER_S3`
* **Mật khẩu (Password)**: `12345678`

Dùng điện thoại hoặc laptop kết nối vào mạng Wi-Fi này.

### 5.2. Mở Bảng Điều Khiển Cyber Dark HUD
Mở trình duyệt web bất kỳ (Chrome, Safari, Edge, Cốc Cốc...) và truy cập địa chỉ:
```text
http://192.168.4.1
```

Giao diện Web HUD lập tức kết nối WebSockets (Port 81) với độ trễ $<10\text{ms}$.

### 5.3. Các Tính Năng Trên Giao Diện Web HUD
1. **Thanh Trượt Chọn Tốc Độ**: Cho phép chọn vận tốc mục tiêu từ `0.10 m/s` đến `1.50 m/s`.
2. **Nút TIẾN (▲) / LÙI (▼)**: 
   * **Nhấn giữ**: Xe nhận lệnh chạy theo đúng tốc độ đã đặt.
   * **Thả tay**: Xe tự động gửi lệnh vận tốc về `0.0 m/s` để hãm xe đứng cân bằng tại chỗ.
3. **Nút DỪNG (■)**: Đưa vận tốc về 0 ngay lập tức mà vẫn giữ thăng bằng.
4. **Nút DỪNG KHẨN CẤP (E-STOP)**: Cắt hoàn toàn xung PWM động cơ, đưa xe về trạng thái ngã an toàn (`FALLEN`).
5. **Bảng Telemetry Thời Gian Thực (20Hz)**:
   * **Pitch**: Góc nghiêng thời gian thực kèm thanh đo màu sắc trực quan (ngưỡng ngã $\pm 40^\circ$).
   * **V_Act / V_Tgt**: Vận tốc thực tế từ Encoder và vận tốc đặt.
   * **PWM L/R**: Công suất kích xung 2 động cơ GA25.
   * **Gyro**: Tốc độ góc con quay hồi chuyển trục Y.
   * **Battery**: Điện áp Pin LiPo 3S giám sát trực tiếp.
6. **Thanh Chẩn Đoán Hệ Thống (Diagnostic Bar)**: Báo trạng thái FSM:
   * `STANDBY`: Đang chờ dựng xe thăng bằng.
   * `CALIBRATING`: Đang bù độ trôi con quay Gyro.
   * `BALANCING`: Xe đang giữ thăng bằng ổn định.
   * `FALLEN`: Xe đã nghiêng quá $40^\circ$, động cơ đã ngắt an toàn.
7. **Menu Rút Gọn "TINH CHỈNH THÔNG SỐ (PID & PITCH TRIM)"**:
   * **Đọc/Ghi 4 hệ số PID**: $K_{p1}, K_{d1}$ (Vòng góc nghiêng) và $K_{p2}, K_{i2}$ (Vòng vận tốc).
   * **Hiệu chuẩn Pitch Trim (`pitch_trim`)**: Bù lệch trọng tâm cơ học của xe theo bước $0.1^\circ$ giúp xe đứng yên hoàn toàn tại chỗ mà không bị trôi tới hay trôi lui.

---

## 6. Đặc Tả Giao Thức Khung Truyền Thông UART (Framing Protocol)

Tốc độ baud mặc định: **`115200 bps, 8N1`** giữa STM32 (`PB6/PB7`) và ESP32-S3 (`GPIO18/GPIO17`).

### A. Chiều STM32 $\rightarrow$ ESP32 $\rightarrow$ WebSockets (Telemetry 20Hz):
```text
$TEL,pitch,gyro_rate,v_actual,v_target,pwm_l,pwm_r,state,batt_voltage*
```
*Ví dụ*: `$TEL,1.2,-0.5,0.05,0.00,320,315,2,11.85*`

### B. Chiều WebSockets $\rightarrow$ ESP32 $\rightarrow$ STM32 (Lệnh Điều Khiển):
* **Lệnh chạy**: `$CMD,v_target,steer_cmd*` *(ví dụ: `$CMD,0.50,0.0*`)*
* **Lệnh ngắt khẩn cấp**: `$STOP*`
* **Lệnh nạp PID mới**: `$PID,kp1,kd1,kp2,ki2*` *(ví dụ: `$PID,55.0,2.2,25.0,0.8*`)*
* **Lệnh chỉnh lệch trọng tâm**: `$TRIM,trim_val*` *(ví dụ: `$TRIM,-0.50*`)*
* **Lệnh đọc lại PID hiện tại**: `$GETPID*`
