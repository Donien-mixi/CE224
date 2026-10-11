# DỰ ÁN XE HAI BÁNH TỰ CÂN BẰNG TỐC ĐỘ CAO (TWIP ROBOT)
### Môn học: Thiết Kế Hệ Thống Nhúng (CE224) — ĐH Công Nghệ Thông Tin (UIT)

---

## 📂 CẤU TRÚC THƯ MỤC DỰ ÁN

Toàn bộ dự án được tổ chức thành 3 phân vùng độc lập, rõ ràng và chuẩn mực công nghiệp:

```
d:\DA_HTN\
│
├── 📁 Docs/                     # Toàn bộ tài liệu kỹ thuật của đồ án
│   ├── Phan_Cung_Va_Ket_Noi.md  # Phần cứng, danh mục linh kiện (BOM), ma trận Pinout, bố trí 2 tầng
│   ├── Thuat_Toan_Dieu_Khien.md # Động lực học TWIP, bộ lọc bù, Cascade PID & mã nguồn C 200Hz
│   ├── Roadmap_Do_An.md         # Roadmap 2 giai đoạn: tự cân bằng & điều hướng web, quy trình tune PID
│   └── linh kiện hệ thống nhúng.xlsx # File bảng tính dự trù chi phí & thông số
│
├── 📁 Xe_Can_Bang/              # Firmware điều khiển thời gian thực STM32F411 (Cortex-M4F)
│   ├── Xe_Can_Bang.ioc          # File cấu hình STM32CubeMX gốc
│   ├── Core/
│   │   ├── Inc/                 # Header ngoại vi CubeMX và Header Driver linh kiện
│   │   │   ├── motor.h          # Driver Dual A4950 (PWM 20kHz Center-aligned)
│   │   │   ├── encoder.h        # Driver Hall Encoder x4 (TIM2 + TIM3)
│   │   │   ├── bmi160.h         # Driver cảm biến IMU Bosch 6 trục (I2C1 Fast Mode 400kHz)
│   │   │   ├── filter.h         # Complementary Filter ước lượng góc Pitch
│   │   │   ├── pid.h            # Bộ điều khiển Cascade PID 2 vòng + Deadband
│   │   │   ├── esp32_comm.h     # Giao tiếp UART1 DMA Circular với ESP32-S3
│   │   │   ├── buzzer_led.h     # Điều khiển Còi PB12 và LED PC13 phi phong bế
│   │   │   └── robot_fsm.h      # Máy trạng thái hữu hạn FSM & ngắt cứng 200Hz (5ms)
│   │   └── Src/                 # Hiện thực mã nguồn C của các module tương ứng
│   └── Drivers/                 # STM32 HAL Driver và CMSIS Cortex-M4
│
└── 📁 ESP32_S3_Gateway/         # Firmware Gateway Không dây ESP32-S3 N16R8 (Arduino IDE)
    ├── ESP32_S3_Gateway.ino     # Cầu nối Wi-Fi SoftAP, WebSockets Server và Hardware Serial1
    ├── index_html.h             # Giao diện Cyber Dark HUD nhúng trực tiếp trong Flash
    └── README.md                # Hướng dẫn cài thư viện, nạp code và vận hành qua Web
```

---

## ⚡ HƯỚNG DẪN KHỞI CHẠY NHANH

1. **Firmware STM32**: Mở thư mục `Xe_Can_Bang/` bằng **STM32CubeIDE**. Cắm mạch nạp **ST-Link V2** vào 4 chân `3.3V`, `GND`, `SWDIO (PA13)`, `SWCLK (PA14)` của Black Pill *(không cần bấm BOOT0, không cần rút dây PA11)*, rồi nhấn **Run/Debug** để build và nạp (`0 errors, 0 warnings`).
2. **Firmware ESP32-S3**: Mở file `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` bằng **Arduino IDE**, cài thư viện `WebSockets` của Markus Sattler, chọn Board `ESP32S3 Dev Module` và nạp vào board.
3. **Điều Khiển & Tuning PID**: Kết nối Wi-Fi `TWIP_RACER_S3` (mật khẩu `12345678`), mở trình duyệt truy cập `http://192.168.4.1`. Dùng **chế độ Test Bàn** để quay thử 2 bánh, chọn tốc độ bằng thanh trượt rồi **giữ nút TIẾN/LÙI** (hoặc phím `W`/`S`) để xe chạy, theo dõi bảng Telemetry, chẩn đoán FSM, tinh chỉnh trực tiếp tham số PID và bấm **XUẤT FILE TXT** sau mỗi lượt chạy để tải log đo đạc về máy tính/điện thoại.
