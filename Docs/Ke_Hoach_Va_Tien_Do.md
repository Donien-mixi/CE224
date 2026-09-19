# KẾ HOẠCH & TIẾN ĐỘ THỰC HIỆN — XE CÂN BẰNG 2 BÁNH

Tài liệu theo dõi tiến độ thực hiện, checklist nghiệm thu 6 giai đoạn và quy trình căn chỉnh PID thực nghiệm cho dự án Xe Cân Bằng 2 Bánh Tốc Độ Cao.

---

## 1. BẢNG TIẾN ĐỘ TỔNG QUAN

| Giai đoạn | Nội dung công việc | Trạng thái | Nhiệm vụ chính |
| :---: | :--- | :---: | :--- |
| **GĐ 1** | Chuẩn bị phần cứng & Cấu hình CubeMX | **100% HOÀN THÀNH** | SYSCLK 100MHz, FPU Hard ABI, Timer 1-4, I2C1, UART1 DMA |
| **GĐ 2** | Lập trình Driver linh kiện & ESP32-S3 | **100% HOÀN THÀNH** | Viết xong toàn bộ driver C STM32 & Firmware Arduino ESP32 |
| **GĐ 3** | Lắp ráp cơ khí & Kiểm thử phần cứng | **ĐANG TRIỂN KHAI** | Lắp khung 2 tầng, chỉnh áp Buck 5V, kiểm tra quay motor/encoder |
| **GĐ 4** | Căn chỉnh Cascade PID thực nghiệm | **CHƯA BẮT ĐẦU** | Tune $K_{p1}, K_{d1}$ (vòng góc) $\to$ $K_{p2}, K_{i2}$ (vòng vận tốc) |
| **GĐ 5** | Tối ưu chế độ đua & Bẻ lái tốc độ cao | **CHƯA BẮT ĐẦU** | Mở rộng góc $\pm 15^\circ$, bù deadband ma sát, bẻ lái thích ứng |
| **GĐ 6** | Thực nghiệm số liệu & Hoàn thiện báo cáo | **CHƯA BẮT ĐẦU** | Thu thập đồ thị qua Web HUD, quay clip chạy biểu diễn |

---

## 2. CHI TIẾT TỪNG GIAI ĐOẠN & CHECKLIST

### GIAI ĐOẠN 1: CẤU HÌNH PHẦN CỨNG TRÊN STM32CUBEMX
*Mục tiêu: Cấu hình toàn bộ ngoại vi STM32F411CEU6 không xung đột, kích hoạt FPU phần cứng.*

- [x] Cấu hình Clock: HSE 25MHz $\to$ SYSCLK = 100MHz, APB1 = 50MHz, APB2 = 100MHz.
- [x] Kích hoạt FPU: `-mfloat-abi=hard -mfpu=fpv4-sp-d16` và thêm `-u _printf_float` trong Tool Settings.
- [x] Cấu hình Timer 1 (PWM 20kHz Center-aligned Mode, CH1..CH4: PA8..PA11) điều khiển Dual A4950.
- [x] Cấu hình Timer 2 (PA0/PA1) & Timer 3 remap (PB4/PB5) ở chế độ Encoder TI12 (đếm x4).
- [x] Cấu hình I2C1 Fast Mode 400kHz (PB8 SCL, PB9 SDA) kết nối Bosch BMI160.
- [x] Cấu hình USART1 DMA Circular (PB6 TX, PB7 RX) baudrate 115200 bps kết nối ESP32-S3.
- [x] Cấu hình Timer 4 ngắt định thời nhịp tim 200Hz (5.0ms), NVIC Priority 0.
- [x] Cấu hình GPIO: PB12 (Còi Buzzer), PC13 (LED xanh on-board, Active LOW).
- [x] Sinh code dự án vào `Xe_Can_Bang/`, biên dịch thành công `0 errors, 0 warnings`.

---

### GIAI ĐOẠN 2: LẬP TRÌNH DRIVER LINH KIỆN & FIRMWARE ESP32-S3
*Mục tiêu: Hiện thực các module độc lập theo kiến trúc phân tầng, tích hợp ngắt 200Hz và xây dựng Gateway không dây.*

- [x] **Driver Động cơ (`motor.h` / `motor.c`)**: PWM 20kHz Slow Decay, kẹp công suất cực đại 2499.
- [x] **Driver Encoder (`encoder.h` / `encoder.c`)**: Đếm xung x4, lọc LPF vận tốc $m/s$ ($\beta = 0.75$).
- [x] **Driver Bosch BMI160 (`bmi160.h` / `bmi160.c`)**: Đọc Chip ID `0xD1`, đánh thức CMD `0x7E`, đọc Burst 12 bytes $< 350\text{µs}$, hiệu chuẩn 500 mẫu bù trôi Gyro.
- [x] **Bộ lọc bù góc nghiêng (`filter.h` / `filter.c`)**: Complementary Filter ($\alpha = 0.98$).
- [x] **Bộ điều khiển Cascade PID (`pid.h` / `pid.c`)**: Vận tốc PI (Anti-windup) $\to$ Góc nghiêng PD $\to$ Bù lái thích ứng $\to$ Bù vùng chết ma sát hộp số (Deadband $\pm 250$).
- [x] **Còi & LED (`buzzer_led.h` / `buzzer_led.c`)**: Điều khiển phi phong bế theo trạng thái FSM.
- [x] **Giao tiếp ESP32 UART DMA (`esp32_comm.h` / `esp32_comm.c`)**: Nhận vòng lặp DMA Circular buffer, Failsafe timeout 1s, phát Telemetry 20Hz.
- [x] **Máy trạng thái FSM & Vòng 200Hz (`robot_fsm.h` / `robot_fsm.c`)**: Quản lý an toàn (ngã $> 45^\circ$ cắt PWM, dựng đứng xe tự kích hoạt cân bằng), móc vào `HAL_TIM_PeriodElapsedCallback()` trong `main.c`.
- [x] **Firmware Gateway ESP32-S3 (`ESP32_S3_Gateway/`)**:
  * [`ESP32_S3_Gateway.ino`](file:///d:/DA_HTN/ESP32_S3_Gateway/ESP32_S3_Gateway.ino): Mở Wi-Fi SoftAP `TWIP_RACER_S3` (IP 192.168.4.1), WebSockets (Port 81), cầu nối Hardware Serial1.
  * [`index_html.h`](file:///d:/DA_HTN/ESP32_S3_Gateway/index_html.h): Giao diện Web HUD nhúng với Joystick ảo, đồ thị HUD thời gian thực và bảng tune PID.
  * [`README.md`](file:///d:/DA_HTN/ESP32_S3_Gateway/README.md): Hướng dẫn nạp code trên Arduino IDE.

---

### GIAI ĐOẠN 3: LẮP RÁP CƠ KHÍ & KIỂM TRA PHẦN CỨNG THỰC TẾ (Hiện Tại)
*Mục tiêu: Lắp ráp hoàn thiện khung 2 tầng, kiểm tra độc lập từng cụm phần cứng trước khi chạy giải thuật.*

- [ ] Lắp ráp cơ khí 2 tầng Mica theo đúng [`Docs/Phan_Cung_Va_Ket_Noi.md`](file:///d:/DA_HTN/Docs/Phan_Cung_Va_Ket_Noi.md).
- [ ] Dán cảm biến Bosch BMI160 bằng băng keo xốp đệm 3M / FPV tại tâm sàn tầng 2.
- [ ] Dùng đồng hồ VOM đo và chỉnh chiết áp Buck XL4015 về **đúng 5.0V** trước khi cắm tải.
- [ ] Đấu nối dây theo đúng bảng Pinout trong [`Docs/Phan_Cung_Va_Ket_Noi.md`](file:///d:/DA_HTN/Docs/Phan_Cung_Va_Ket_Noi.md).
- [ ] Test động cơ: Nạp lệnh chạy thử, kiểm tra chiều quay tiến/lùi của 2 bánh xe.
- [ ] Test Encoder: Quay tay bánh xe 1 vòng, kiểm tra số xung tăng xấp xỉ 1320 xung trên debug window.
- [ ] Test IMU: Đọc thanh ghi `CHIP_ID` qua I2C1, kiểm tra giá trị trả về đúng `0xD1`.
- [ ] Nạp firmware ESP32-S3 qua Arduino IDE, kết nối Wi-Fi trên điện thoại mở `http://192.168.4.1`.

---

### GIAI ĐOẠN 4: HƯỚNG DẪN CĂN CHỈNH CASCADE PID THỰC TẾ
*Mục tiêu: Tìm bộ thông số $K_p, K_d, K_i$ tối ưu giúp xe tự đứng thăng bằng bất động tại chỗ.*

#### Bước 1: Căn chỉnh Vòng Trong (Angle Loop PD)
* Tạm thời tắt vòng vận tốc ($K_{p2} = 0, K_{i2} = 0$).
* **Tăng dần $K_{p1}$** (khởi điểm 200 $\to$ 350): Đến khi dùng tay nghiêng xe thì cảm nhận được lực motor đẩy ngược lại để chống ngã. Nếu tăng quá lớn xe sẽ bị rung tần số cao $\to$ lùi lại một chút.
* **Tăng dần $K_{d1}$** (khởi điểm 4.0 $\to$ 8.5): Khâu vi sai đóng vai trò giảm xóc, dập tắt các dao động rung giật của thân xe do khâu $K_{p1}$ gây ra.
* *Kết quả nghiệm thu*: Xe tự đứng thăng bằng được trên 2 bánh, nhưng sẽ trôi tự do nếu mặt sàn nghiêng.

#### Bước 2: Căn chỉnh Vòng Ngoài (Velocity Loop PI)
* Bật vòng vận tốc, đặt vận tốc mong muốn $v_{\text{target}} = 0$.
* **Tăng dần $K_{p2}$** (khởi điểm 0.8 $\to$ 1.2): Giúp xe sinh ra phản xạ tiến/lùi khi xe bắt đầu trôi vị trí.
* **Tăng dần $K_{i2}$** (khởi điểm 0.05 $\to$ 0.15): Tích lũy sai số vận tốc sinh ra góc ngả bù trọng tâm tĩnh $\theta_{\text{trim}}$.
* *Kết quả nghiệm thu*: Xe tự đứng bất động tại một chỗ, khi bị lấy tay đẩy nhẹ thì tự chống trả và quay lại vị trí cũ.

---

### GIAI ĐOẠN 5: TỐI ƯU CHẾ ĐỘ ĐUA & BẺ LÁI TỐC ĐỘ CAO
*Mục tiêu: Tối ưu khả năng vận hành khi di chuyển, vào cua gắt và bứt tốc đua.*

- [ ] Gạt Joystick trên Web HUD kiểm tra đáp ứng bám vận tốc đặt $v_{\text{target}}$ khi tiến/lùi.
- [ ] Tinh chỉnh hệ số bẻ lái thích ứng $\beta = 1.2$: Xe xoay tại chỗ linh hoạt khi đứng yên và bo cua mượt mà không bị lật khi chạy tốc độ cao.
- [ ] Thử nghiệm chế độ đua (`$RACE,1*`): Mở rộng góc ngả lên $\pm 15^\circ$, kiểm tra gia tốc bứt tốc.
- [ ] Kiểm thử cơ chế an toàn: Xe ngã quá $45^\circ$ lập tức cắt PWM trong $< 5\text{ms}$; dựng thẳng xe tự động cân bằng trở lại.

---

### GIAI ĐOẠN 6: THỰC NGHIỆM ĐO ĐẠC & HOÀN THIỆN BÁO CÁO
*Mục tiêu: Đo đạc chỉ số kỹ thuật thực tế, thu thập đồ thị và hoàn thành báo cáo đồ án.*

- [ ] Đo đạc các chỉ số: Sai số góc tĩnh ($\le \pm 0.3^\circ$), thời gian dập tắt va chạm ($< 0.8\text{s}$), tốc độ tối đa đạt được.
- [ ] Ghi lại đồ thị đáp ứng thực nghiệm (Step Response) hiển thị trên Web HUD / VOFA+.
- [ ] Quay video clip thực tế xe tự cân bằng tại chỗ và chạy đua vượt chướng ngại vật.
- [ ] Hoàn thành báo cáo thuyết minh đồ án môn học Thiết kế Hệ thống nhúng (CE224).
