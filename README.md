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
│   ├── Ke_Hoach_Va_Tien_Do.md   # Tiến độ 6 giai đoạn, checklist nghiệm thu & hướng dẫn tune PID
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

1. **Firmware STM32**: Mở thư mục `Xe_Can_Bang/` bằng **STM32CubeIDE**, nhấn `Ctrl + B` để build project (`0 errors, 0 warnings`) và nạp qua mạch nạp ST-Link V2.
2. **Firmware ESP32-S3**: Mở file `ESP32_S3_Gateway/ESP32_S3_Gateway.ino` bằng **Arduino IDE**, cài thư viện `WebSockets` của Markus Sattler, chọn Board `ESP32S3 Dev Module` và nạp vào board.
3. **Điều Khiển**: Kết nối Wi-Fi `TWIP_RACER_S3` (mật khẩu `12345678`), mở trình duyệt truy cập `http://192.168.4.1` để xem Telemetry thời gian thực và lái xe bằng Joystick ảo.
