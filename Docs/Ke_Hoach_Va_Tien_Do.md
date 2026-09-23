# KẾ HOẠCH & TIẾN ĐỘ THỰC HIỆN CHI TIẾT — XE CÂN BẰNG 2 BÁNH TỐC ĐỘ CAO (TWIP ROBOT)

Tài liệu quản trị dự án toàn diện: Theo dõi tiến độ 6 giai đoạn, mô tả chi tiết toàn bộ các đầu việc cũ (đã hoàn thành/đang chạy) và các đầu việc phần mềm mới bổ sung, bảng phân công nhiệm vụ cho nhóm và checklist nghiệm thu kỹ thuật môn học Thiết kế Hệ thống Nhúng (CE224).

---

## 1. BẢNG TIẾN ĐỘ TỔNG QUAN & PHÂN BỔ TRẠNG THÁI

| Giai đoạn | Tên phân đoạn công việc | Trạng thái | Đầu việc cũ (Cốt lõi) | Đầu việc mới (Nâng cấp & Mở rộng) |
| :---: | :--- | :---: | :--- | :--- |
| **GĐ 1** | **Cấu hình phần cứng trên CubeMX** | **100% HOÀN THÀNH** | SYSCLK 100MHz, FPU Hard ABI, Timer 1-4, I2C1, UART1 DMA | Kiểm tra tính tương thích vector ngắt NVIC và tối ưu bộ đệm DMA |
| **GĐ 2** | **Lập trình Driver linh kiện & ESP32-S3** | **100% HOÀN THÀNH** | Driver Motor, Encoder, BMI160, Filter, Cascade PID, FSM, Web HUD | Chuẩn hóa cấu trúc gói tin NMEA và bộ lọc bù Complementary |
| **GĐ 3** | **Lắp ráp cơ khí & Kiểm thử độc lập** | **ĐANG TRIỂN KHAI** | Test từng cụm: Buck 5V, BMI160, GA25 Motor, Encoder, lắp khung 2 tầng | Hiệu chuẩn offset tĩnh cảm biến và kiểm tra hướng trục tọa độ |
| **GĐ 4** | **Phát triển tính năng mới & Căn chỉnh PID** | **SẴN SÀNG TRIỂN KHAI** | Tune PD vòng góc $\to$ Tune PI vòng vận tốc $\to$ Bù Deadband $\pm 250$ | **Bù trọng tâm tĩnh $\theta_{\text{trim}}$, Cân bằng động 2 bánh, Lưu Flash STM32, Vẽ Chart Real-time & Xuất CSV** |
| **GĐ 5** | **Tối ưu chế độ đua & Bẻ lái tốc độ cao** | **CHƯA BẮT ĐẦU** | Mở rộng góc $\pm 15^\circ$, bù lái thích ứng $\beta=1.2$, an toàn ngã $>45^\circ$ | **Phát hiện kẹt bánh (Stall Detection), Preset Profile tốc độ, Nút Trim $\pm 0.1^\circ$ trên Web** |
| **GĐ 6** | **Thực nghiệm đo đạc & Hoàn thiện đồ án** | **CHƯA BẮT ĐẦU** | Thu thập đồ thị đáp ứng bước, quay video clip thực tế, báo cáo CE224 | **Script Python tự động tính $M_p, t_s, e_{ss}$ từ CSV, Slide thuyết trình & Kịch bản demo** |

---

## 2. CHI TIẾT TỪNG GIAI ĐOẠN & MÔ TẢ ĐẦU VIỆC (MỚI & CŨ)

```
SƠ ĐỒ LUỒNG PHÁT TRIỂN & TÍCH HỢP HỆ THỐNG
┌────────────────────────┐      ┌────────────────────────┐      ┌────────────────────────┐
│  GĐ 1 & 2: HOÀN THÀNH  │ ───> │  GĐ 3: TEST PHẦN CỨNG  │ ───> │  GĐ 4: NÂNG CẤP SW &   │
│ Driver C & CubeMX 100% │      │ Lắp ráp & Test linh kiện│     │ Căn chỉnh Cascade PID  │
└────────────────────────┘      └────────────────────────┘      └───────────┬────────────┘
                                                                            │
┌────────────────────────┐      ┌────────────────────────┐                  │
│ GĐ 6: ĐO ĐẠC & BÁO CÁO │ <─── │ GĐ 5: ĐUA & AN TOÀN    │ <────────────────┘
│ Python Chart, Slide, BC│      │ Bẻ lái cao tốc, Failsafe│
└────────────────────────┘      └────────────────────────┘
```

---

### GIAI ĐOẠN 1: CẤU HÌNH PHẦN CỨNG TRÊN STM32CUBEMX (100% Hoàn thành)
*Mục tiêu: Thiết lập toàn bộ cây ngoại vi trên vi điều khiển STM32F411CEU6 không xung đột tài nguyên, cấu hình ngắt cứng tối ưu.*

#### A. Đầu việc đã hoàn thành (Cốt lõi):
- [x] **Cấu hình Clock cây hệ thống**: Thạch anh ngoài HSE 25MHz qua PLL nhân lên SYSCLK = 100MHz; APB1 = 50MHz (Timer clock 100MHz); APB2 = 100MHz.
- [x] **Kích hoạt FPU phần cứng**: Cờ biên dịch `-mfloat-abi=hard -mfpu=fpv4-sp-d16`, bổ sung flag `-u _printf_float` trong Linker để in số thực ra UART.
- [x] **Timer 1 (PWM Motor)**: Chế độ Center-aligned Mode 1, ARR = 2499, tần số sóng mang 20kHz điều khiển mạch công suất Dual A4950 qua 4 kênh PA8..PA11.
- [x] **Timer 2 & Timer 3 (Encoder)**: Chế độ TI12 (Encoder Mode x4), lọc số ngõ vào `ICxFilter = 6` khử rung nhiễu cơ khí. TIM2 đọc bánh trái (PA0/PA1), TIM3 đọc bánh phải remap (PB4/PB5).
- [x] **I2C1 Fast Mode 400kHz**: Chân PB8 (SCL), PB9 (SDA), giao tiếp cảm biến góc Bosch BMI160.
- [x] **USART1 DMA Circular**: Baudrate 115200 bps, PB6 (TX - DMA2 Stream 7), PB7 (RX - DMA2 Stream 2 vòng lặp liên tục).
- [x] **Timer 4 (Nhịp tim hệ thống)**: Định thời ngắt cứng chu kỳ chính xác 5.0ms (tần số lấy mẫu $f_s = 200\text{Hz}$), NVIC Priority 0 (ưu tiên cao nhất).
- [x] **GPIO Ngoại vi**: PB12 điều khiển Còi Buzzer Active; PC13 điều khiển LED xanh on-board (Active LOW).
- [x] **Biên dịch thử nghiệm**: File dự án [`Xe_Can_Bang.ioc`](file:///d:/DA_HTN/Xe_Can_Bang/Xe_Can_Bang.ioc) đã sinh mã nguồn và biên dịch thành công `0 errors, 0 warnings`.

---

### GIAI ĐOẠN 2: LẬP TRÌNH DRIVER LINH KIỆN & FIRMWARE ESP32-S3 (100% Hoàn thành)
*Mục tiêu: Xây dựng toàn bộ các tầng module driver phần mềm độc lập, kiến trúc phi phong bế và Gateway không dây.*

#### A. Đầu việc đã hoàn thành (Cốt lõi):
- [x] **Driver Động cơ ([`motor.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/motor.h) / [`motor.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/motor.c))**:
  * Hiện thực chế độ Slow Decay: 1 chân PWM, 1 chân 0V giúp lực kéo tuyến tính.
  * Giới hạn công suất cực đại kẹp biên trong dải $[-2499, +2499]$.
- [x] **Driver Encoder ([`encoder.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/encoder.h) / [`encoder.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/encoder.c))**:
  * Đếm xung x4: $11\text{ xung} \times 30 \times 4 = 1320\text{ xung/vòng bánh xe}$.
  * Xử lý tràn số tự động qua ép kiểu `int16_t`.
  * Bộ lọc thông thấp LPF bậc 1 ($\beta = 0.75$) khử nhiễu lượng tử hóa vận tốc $m/s$.
- [x] **Driver Bosch BMI160 ([`bmi160.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/bmi160.h) / [`bmi160.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/bmi160.c))**:
  * Xác thực thanh ghi `CHIP_ID (0x00)` bắt buộc trả về `0xD1`.
  * Khởi tạo nguồn: Đánh thức Accel (`0x11`) và Gyro (`0x15`). Cấu hình dải đo Gyro $\pm 2000^\circ/s$, Accel $\pm 8g$.
  * Đọc Burst đồng thời 12 bytes thanh ghi từ `0x0C` đến `0x17` chỉ trong $< 350\mu\text{s}$.
  * Hàm `BMI160_Calibrate_Gyro(500)` lấy 500 mẫu trung bình tĩnh để trừ trôi góc tĩnh (Zero-rate Bias).
- [x] **Bộ lọc bù Complementary Filter ([`filter.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/filter.h) / [`filter.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/filter.c))**:
  * Lọc bù góc Pitch: $\theta[k] = 0.98 \times (\theta[k-1] + \omega_{\text{gyro}} \Delta t) + 0.02 \times \theta_{\text{acc}}$.
- [x] **Bộ điều khiển Cascade PID ([`pid.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/pid.h) / [`pid.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/pid.c))**:
  * Vòng trong Angle Loop PD: $\text{PWM} = K_{p1} \cdot e_\theta - K_{d1} \cdot \omega_{\text{gyro}}$ (chống hiện tượng Derivative Kick).
  * Vòng ngoài Velocity Loop PI: Tính sai số vận tốc sinh ra góc ngả đặt $\theta_{\text{target}}$, tích hợp chống bão hòa tích phân Anti-windup kẹp $\pm 10^\circ$.
  * Bù vùng chết ma sát hộp số (Deadband $\pm 250$) và suy giảm lái thích ứng theo vận tốc $\frac{1}{1 + 1.2|v|}$.
- [x] **Giao tiếp UART1 DMA ([`esp32_comm.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/esp32_comm.h) / [`esp32_comm.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/esp32_comm.c))**:
  * Giải mã luồng byte từ Circular DMA buffer, nhận các gói `$CMD`, `$PID`, `$RACE`, `$CALIB`, `$STOP`.
  * Cơ chế an toàn Failsafe timeout 1000ms: Tự xóa ga/lái về 0 nếu mất kết nối.
  * Truyền Telemetry định kỳ 20Hz: `$TEL,pitch,gyro,v_act,v_tgt,pwm_l,pwm_r,state,batt\r\n`.
- [x] **Còi & LED phi phong bế ([`buzzer_led.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/buzzer_led.h) / [`buzzer_led.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/buzzer_led.c))**:
  * LED PC13: Standby (1Hz), Calibrating (5Hz), Balancing (Heartbeat nháy đúp), Fallen/Alarm (10Hz).
  * Còi PB12: Beep theo thời gian bất đồng bộ không gây trễ CPU.
- [x] **Máy trạng thái FSM ([`robot_fsm.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/robot_fsm.h) / [`robot_fsm.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/robot_fsm.c))**:
  * Điều phối 7 trạng thái vận hành; tự động kích hoạt cân bằng khi dựng xe $|\theta| < 2.5^\circ$; cắt ngắt an toàn khi ngã $> 45^\circ$.
- [x] **Firmware Gateway ESP32-S3 ([`ESP32_S3_Gateway/`](file:///d:/DA_HTN/ESP32_S3_Gateway/))**:
  * [`ESP32_S3_Gateway.ino`](file:///d:/DA_HTN/ESP32_S3_Gateway/ESP32_S3_Gateway.ino): SoftAP `TWIP_RACER_S3` (192.168.4.1), Web Server port 80, WebSockets port 81, Serial1 115200 bps.
  * [`index_html.h`](file:///d:/DA_HTN/ESP32_S3_Gateway/index_html.h): Giao diện Cyber Dark HUD, Virtual Joystick cảm ứng, chỉnh thông số PID trực tiếp từ Web.

---

### GIAI ĐOẠN 3: LẮP RÁP CƠ KHÍ & KIỂM THỬ ĐỘC LẬP LINH KIỆN (Đang triển khai)
*Mục tiêu: Đảm bảo 100% linh kiện phần cứng hoạt động hoàn hảo độc lập trước khi ráp thành hệ thống hoàn chỉnh.*

#### A. Đầu việc kiểm thử phần cứng độc lập (Hardware Bring-up):
- [ ] **Kiểm tra và cân chỉnh nguồn Buck XL4015**:
  * Cắm nguồn Pin LiPo 3S (11.1V - 12.6V).
  * Dùng đồng hồ VOM đo ngõ ra, vặn chiết áp vi chỉnh về **chính xác 5.0V** ($\pm 0.05\text{V}$) trước khi cắm dây cấp nguồn cho STM32 và ESP32-S3 để chống quá áp làm cháy chip.
- [ ] **Kiểm tra độc lập Cảm biến Bosch BMI160**:
  * Đấu nối chân I2C (PB8 SCL, PB9 SDA) lên mạch nạp hoặc Arduino Uno để xác thực thanh ghi `CHIP_ID = 0xD1`.
  * Nghiêng cảm biến, kiểm tra góc Pitch biến thiên liên tục, không bị đơ hoặc nhảy giá trị rác.
- [ ] **Kiểm tra cụm Động cơ GA25 & Mạch Driver Dual A4950**:
  * Nạp chương trình test cơ bản kích xung PA8..PA11.
  * Kiểm tra chiều quay: Bánh trái và bánh phải phải quay cùng chiều tiến khi nhận giá trị PWM dương.
  * Đảm bảo không bị chạm chập đường nguồn động lực 12V.
- [ ] **Kiểm tra phản hồi 2 Hall Encoder**:
  * Dùng tay quay chậm bánh xe đúng 1 vòng ($360^\circ$).
  * Quan sát số xung trên STM32 Live Watch: Cả 2 kênh phải đếm xấp xỉ $\approx 1320\text{ xung}$ (dung sai $\pm 20$ xung).
  * Chiều đếm: Bánh quay tiến thì số xung phải tăng dương.
- [ ] **Kiểm tra Gateway ESP32-S3**:
  * Nạp code qua Arduino IDE, kết nối điện thoại vào mạng Wi-Fi `TWIP_RACER_S3`.
  * Truy cập `http://192.168.4.1`, kiểm tra giao diện HUD hiển thị đầy đủ, không bị treo.

#### B. Lắp ráp hoàn thiện khung xe 2 tầng:
- [ ] Lắp 2 gá nhôm chữ L và 2 động cơ GA25 vào mặt dưới tầng 1, siết ốc chặt, **2 trục bánh xe phải thẳng hàng 100%**.
- [ ] Bố trí Pin LiPo 3S sát trục bánh xe ở tầng 1 để giảm quán tính xoay; Driver A4950 ở giữa; Buck XL4015 ở mép sau.
- [ ] Lắp 4 cọc đồng M3 (cao 6cm) cố định tầng 2.
- [ ] **Dán cảm biến BMI160 lên đệm xốp 3M / FPV dày 2-3mm** tại chính giữa tâm hình học ngay trên trục bánh xe ở tầng 2 để triệt tiêu rung động cơ học từ hộp số.
- [ ] Đi dây nguồn và tín hiệu theo đúng nguyên tắc **Star Ground** (đấu chung mass Pin, Buck, Driver, STM32, ESP32 về một điểm).

---

### GIAI ĐOẠN 4: PHÁT TRIỂN TÍNH NĂNG MỚI & CĂN CHỈNH CASCADE PID (Sẵn sàng triển khai)
*Mục tiêu: Bổ sung các tính năng phần mềm nâng cao trên STM32 và Web HUD; thực hiện quy trình tune Cascade PID thực tế giúp xe đứng vững.*

#### A. CÁC ĐẦU VIỆC PHẦN MỀM MỚI BỔ SUNG (NEW SOFTWARE WORK PACKAGES):

##### 1. Phần mềm STM32:
- [ ] **Nhiệm vụ 1.1: Bù góc trọng tâm tĩnh thực tế ($\theta_{\text{trim}}$)**
  * *Vấn đề*: Khung xe sau khi lắp đặt pin và dây nối sẽ có độ lệch trọng tâm cơ khí nhẹ (ví dụ $+0.7^\circ$). Nếu lấy mốc $0.0^\circ$ xe sẽ bị trôi từ từ.
  * *Hiện thực*: Bổ sung biến `pitch_trim` vào cấu trúc điều khiển; công thức sai số góc: $e_\theta = (\theta_{\text{target}} + \theta_{\text{trim}}) - \theta_{\text{actual}}$. Nhận gói tin `$TRIM,value*` từ Web để tinh chỉnh mốc này từ xa.
  * *File*: [`pid.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/pid.h), [`pid.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/pid.c), [`esp32_comm.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/esp32_comm.c).
- [ ] **Nhiệm vụ 1.2: Cân bằng động học 2 động cơ (Motor Trim Gain)**
  * *Vấn đề*: Hai động cơ DC có ma sát hộp số khác nhau nhẹ khiến xe bị nhao lái sang một bên khi tiến/lùi.
  * *Hiện thực*: Bổ sung hệ số bù tỉ lệ `k_left` và `k_right` vào hàm xuất xung `Motor_SetDuty()`. Hoặc tích hợp khâu bù vận tốc góc Yaw $G_z$ khi lệnh lái bằng 0 để xe tự động khóa hướng chạy thẳng tắp.
  * *File*: [`motor.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/motor.c), [`robot_fsm.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/robot_fsm.c).
- [ ] **Nhiệm vụ 1.3: Lưu trữ thông số PID vào bộ nhớ Flash STM32 (Flash Emulation)**
  * *Vấn đề*: Hiện tại tắt nguồn xe là mất thông số vừa tune trên Web, phải chỉnh lại từ đầu.
  * *Hiện thực*: Viết module đọc/ghi Sector 7 (`0x08060000`) của bộ nhớ Flash STM32F411. Khi nhận lệnh `$SAVE*` từ Web, lưu bộ tham số $K_{p1}, K_{d1}, K_{p2}, K_{i2}, \theta_{\text{trim}}$ vào Flash. Khi khởi động xe tự động load bộ thông số này.
  * *File*: Thêm module mới `flash_storage.c/.h` vào [`Xe_Can_Bang/Core/`](file:///d:/DA_HTN/Xe_Can_Bang/Core/).
- [ ] **Nhiệm vụ 1.4: Bảo vệ chống kẹt bánh / trượt bánh (Slip & Stall Protection)**
  * *Vấn đề*: Xe bị chướng ngại vật chặn hoặc ngã đè bánh khiến motor ăn dòng cực đại dễ nổ MOSFET A4950.
  * *Hiện thực*: Nếu PWM $> 80\%$ liên tục quá $500\text{ms}$ nhưng Encoder đo được $v \approx 0\text{ m/s}$, lập tức ngắt động cơ chuyển sang `ROBOT_STATE_EMERGENCY` và hú còi cảnh báo.
  * *File*: [`robot_fsm.c`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Src/robot_fsm.c).

##### 2. Phần mềm Web HUD & ESP32-S3:
- [ ] **Nhiệm vụ 2.1: Nhúng đồ thị đường thời gian thực (Real-time Canvas Line Chart)**
  * *Mục tiêu*: Giúp quan sát trực quan sóng dao động của góc nghiêng và vận tốc khi đang tune PID.
  * *Hiện thực*: Nhúng thư viện Canvas siêu nhẹ chạy offline (như *uPlot* hoặc vẽ Canvas HTML5 thuần) trực tiếp trong [`index_html.h`](file:///d:/DA_HTN/ESP32_S3_Gateway/index_html.h). Hiển thị đồng thời 3 đường: Góc đặt $\theta_{\text{target}}$ (Vàng), Góc thực $\theta_{\text{actual}}$ (Cyan), Vận tốc thực $v_{\text{act}}$ (Xanh lá).
- [ ] **Nhiệm vụ 2.2: Tính năng Ghi dữ liệu & Xuất file CSV (Telemetry Data Logger)**
  * *Mục tiêu*: Thu thập dữ liệu đáp ứng bước phục vụ vẽ biểu đồ báo cáo khoa học.
  * *Hiện thực*: Thêm nút **"Bắt đầu Ghi"** và **"Xuất CSV"** trên Web HUD. Trình duyệt lưu $500 - 1000$ mẫu Telemetry và cho phép tải file `telemetry_run.csv` về máy.
- [ ] **Nhiệm vụ 2.3: Bộ điều khiển Profile PID & Nút Trim góc nhanh**
  * *Hiện thực*: Thêm các nút chọn chế độ cấu hình mẫu: *Cân bằng êm (Smooth)*, *Đua tốc độ (Race)*; thêm 2 nút **`Trim +0.1°`** và **`Trim -0.1°`** trên giao diện để cân chỉnh trọng tâm chỉ với 1 chạm.
- [ ] **Nhiệm vụ 2.4: Chỉ báo chất lượng sóng & Độ trễ Ping (Ping / RTT Indicator)**
  * *Hiện thực*: ESP32 gửi gói ping phản hồi, Web tính độ trễ Round Trip Time hiển thị lên Header (ví dụ: `Ping: 6ms - Ổn định`).

---

#### B. QUY TRÌNH CĂN CHỈNH CASCADE PID THỰC TẾ:

```
CHIẾN THUẬT TUNE CASCADE PID 2 VÒNG LỒNG NHAU:
┌────────────────────────────────────────────────────────┐
│ BƯỚC 1: VÒNG TRONG - GÓC NGHIÊNG (PD)                  │
│ Tắt vòng vận tốc (Kp2 = 0, Ki2 = 0)                   │
│ 1. Tăng Kp1 (200 -> 350) đến khi xe có lực phản kháng │
│ 2. Tăng Kd1 (4.0 -> 8.5) để dập tắt rung giật lò xo   │
│ -> Kết quả: Xe tự đứng cân bằng trên 2 bánh           │
└──────────────────────────┬─────────────────────────────┘
                           │
┌──────────────────────────▼─────────────────────────────┐
│ BƯỚC 2: BÙ VÙNG CHẾT MA SÁT HỘP SỐ (DEADBAND)          │
│ Chỉnh MOTOR_DEADBAND (200 -> 250 -> 300)               │
│ -> Kết quả: Động cơ phản hồi tức thì với góc lệch nhỏ │
└──────────────────────────┬─────────────────────────────┘
                           │
┌──────────────────────────▼─────────────────────────────┐
│ BƯỚC 3: VÒNG NGOÀI - VẬN TỐC (PI)                      │
│ Bật vòng vận tốc với v_target = 0                      │
│ 1. Tăng Kp2 (0.8 -> 1.2) tạo lực kéo chống trôi       │
│ 2. Tăng Ki2 (0.05 -> 0.15) học góc nghiêng bù trọng tâm│
│ -> Kết quả: Xe đứng bất động tại chỗ, đẩy tự lùi lại   │
└────────────────────────────────────────────────────────┘
```

- [ ] **Bước 1: Căn chỉnh Vòng Góc nghiêng (Angle Loop PD)**:
  * Đặt $K_{p2} = 0, K_{i2} = 0$ trên Web HUD.
  * Tăng dần $K_{p1}$ từ $150 \to 250 \to 350$: Cảm nhận lực chống ngã của motor khi lấy tay nghiêng xe. Tăng đến ngưỡng bắt đầu xuất hiện dao động rung thì giảm nhẹ lại $10\%$.
  * Tăng dần $K_{d1}$ từ $3.0 \to 6.0 \to 8.5$: Đóng vai trò giảm chấn ảo, dập tắt các dao động rung của $K_{p1}$.
  * *Nghiệm thu Bước 1*: Xe tự đứng được trên 2 bánh nhưng bị trôi tự do theo quán tính.
- [ ] **Bước 2: Căn chỉnh Bù Deadband ma sát**:
  * Kiểm tra xem khi xe nghiêng góc nhỏ ($0.5^\circ$) bánh xe đã nhích quay chưa. Nếu bị trơ, tăng `MOTOR_DEADBAND` trong [`motor.h`](file:///d:/DA_HTN/Xe_Can_Bang/Core/Inc/motor.h) từ $200 \to 250 \to 300$.
- [ ] **Bước 3: Căn chỉnh Vòng Vận tốc (Velocity Loop PI)**:
  * Đặt vận tốc mong muốn $v_{\text{target}} = 0\text{ m/s}$.
  * Tăng dần $K_{p2}$ từ $0.5 \to 1.2$: Giúp xe sinh lực đẩy tiến/lùi khi bánh xe bắt đầu di chuyển vị trí.
  * Tăng dần $K_{i2}$ từ $0.02 \to 0.15$: Khâu tích phân sai số vận tốc sẽ tự động tích lũy và nghiêng thân xe một góc bù trọng tâm.
  * *Nghiệm thu Bước 3*: Xe tự đứng bất động tại một điểm trên sàn. Lấy tay đẩy nhẹ vào thân xe, xe tự chống trả và quay lại đúng vị trí ban đầu.

---

### GIAI ĐOẠN 5: TỐI ƯU CHẾ ĐỘ ĐUA & BẺ LÁI TỐC ĐỘ CAO (Chưa bắt đầu)
*Mục tiêu: Đạt hiệu năng di chuyển tối đa, bứt tốc mạnh mẽ và vào cua an toàn không lật xe.*

#### A. Đầu việc cần triển khai:
- [ ] **Kiểm tra đáp ứng điều khiển Joystick**: Gạt cần ảo trên Web HUD, kiểm tra xe tiến/lùi mượt mà theo đúng vận tốc đặt $v_{\text{target}}$ ($0 \to 1.2\text{ m/s}$).
- [ ] **Tinh chỉnh thuật toán bẻ lái thích ứng theo vận tốc**:
  $$\Delta\text{PWM}_{\text{steer}} = \frac{\text{SteerCmd}}{1.0 + \beta \cdot |v_{\text{actual}}|}$$
  * Tinh chỉnh hệ số $\beta = 1.2$: Khi xe đứng yên ($v \approx 0$), xe xoay tròn tại chỗ nhanh; khi xe chạy tốc độ cao, biên độ bẻ lái tự thu nhỏ lại để vào cua êm ái, triệt tiêu lực ly tâm gây lật xe.
- [ ] **Thử nghiệm Chế độ Đua Tốc độ cao (Racing Mode)**:
  * Nhấn nút "Race Mode" (`$RACE,1*`): Mở rộng giới hạn góc ngả cực đại từ $\pm 8^\circ$ lên $\pm 15^\circ$.
  * Kiểm tra gia tốc bứt tốc $a_x \approx g \cdot \tan(15^\circ) \approx 2.6\text{ m/s}^2$, đạt vận tốc tối đa $\approx 2.0\text{ m/s}$.
- [ ] **Kiểm tra các cơ chế an toàn Fail-safe toàn diện**:
  * Xe ngã quá $45^\circ$: Cắt PWM động cơ trong $< 5\text{ms}$, hú còi báo động.
  * Dựng xe đứng thẳng lại: Tự động xóa tích phân và kích hoạt cân bằng trở lại êm ái.
  * Tắt Wi-Fi hoặc mất sóng trên điện thoại: Sau 1.0s Failsafe tự kích hoạt đưa vận tốc và lực lái về 0, giữ xe tự đứng yên an toàn.

---

### GIAI ĐOẠN 6: THỰC NGHIỆM ĐO ĐẠC & HOÀN THIỆN BÁO CÁO (Chưa bắt đầu)
*Mục tiêu: Xử lý số liệu khoa học, phân tích đồ thị đáp ứng thực nghiệm và hoàn thiện hồ sơ nghiệm thu môn CE224.*

#### A. CÁC ĐẦU VIỆC MỚI BỔ SUNG (PHÂN TÍCH & BÁO CÁO):
- [ ] **Nhiệm vụ 6.1: Script Python tự động phân tích đồ thị đáp ứng bước (Step-Response Analyzer)**
  * *Nội dung*: Viết script Python (dùng thư viện `pandas`, `matplotlib`, `scipy`) đọc file CSV xuất từ Web HUD.
  * *Chỉ số tính toán*:
    * Độ vọt lố Overshoot ($M_p < 15\%$).
    * Thời gian xác lập Settling Time ($t_s < 0.8\text{s}$).
    * Sai số góc tĩnh Steady-State Error ($e_{ss} \le \pm 0.3^\circ$).
  * *Sản phẩm*: Đồ thị đáp ứng chất lượng cao phục vụ chèn vào tài liệu thuyết minh.
- [ ] **Nhiệm vụ 6.2: Xây dựng Kịch bản Demo & Quay Video Clip Nghiệm thu**
  * *Cảnh 1: Thử thách ngoại lực*: Xe đứng thăng bằng tại chỗ, dùng thước hoặc tay đẩy xe 3 lần liên tiếp, xe không ngã.
  * *Cảnh 2: Lái xe qua sa bàn*: Điều khiển bằng Joystick điện thoại bẻ lái zíc-zắc qua hàng cốc/chướng ngại vật.
  * *Cảnh 3: Bứt tốc chế độ đua*: Chuyển sang Race Mode, xe tăng tốc nhanh trên đường thẳng.
  * *Cảnh 4: An toàn ngã xe*: Cố tình gạt xe ngã để chứng minh cơ chế cắt điện tức thì chống cháy động cơ.
- [ ] **Nhiệm vụ 6.3: Hoàn thiện Báo cáo Thuyết minh Đồ án CE224**
  * Chương 1: Đặt vấn đề, mục tiêu và yêu cầu kỹ thuật.
  * Chương 2: Cơ sở lý thuyết TWIP và mô hình hóa toán học con lắc ngược.
  * Chương 3: Thiết kế phần cứng (Sơ đồ khối, sơ đồ nguyên lý, BOM, ma trận Pinout).
  * Chương 4: Thiết kế phần mềm (Cấu trúc FSM, ngắt 200Hz, Cascade PID, Gateway ESP32-S3 Web HUD).
  * Chương 5: Kết quả thực nghiệm và đánh giá số liệu đo đạc.
  * Chương 6: Kết luận và hướng mở rộng.
- [ ] **Nhiệm vụ 6.4: Thiết kế Slide Thuyết trình Báo cáo Hội đồng**
  * Slide 15 - 20 trang tóm tắt điểm sáng công nghệ: ARM Cortex-M4F FPU Hard, ngắt cứng 200Hz, Cascade PID 2 vòng, WebSockets HUD thời gian thực.

---

## 3. BẢNG PHÂN CÔNG CÔNG VIỆC GỢI Ý CHO NHÓM ĐỒ ÁN (4-5 THÀNH VIÊN)

| Thành viên | Trách nhiệm chính | Chi tiết nhiệm vụ đảm nhận | Sản phẩm bàn giao nghiệm thu |
| :--- | :--- | :--- | :--- |
| **Thành viên 1**<br>*(Trưởng nhóm / Firmware Lead)* | Kiến trúc điều khiển & Thuật toán STM32 | • Quản lý mã nguồn STM32, tối ưu ngắt 200Hz.<br>• Hiện thực góc bù trọng tâm tĩnh $\theta_{\text{trim}}$ (Nhiệm vụ 1.1).<br>• Cân bằng động 2 động cơ (Nhiệm vụ 1.2).<br>• Lưu cấu hình PID vào Flash STM32 (Nhiệm vụ 1.3).<br>• Bảo vệ kẹt bánh xe Stall Protection (Nhiệm vụ 1.4). | • Firmware STM32 hoàn chỉnh, nạp chạy ổn định, không lỗi bộ nhớ. |
| **Thành viên 2**<br>*(Web & IoT Lead)* | Giao diện Web HUD & Firmware ESP32-S3 | • Nâng cấp giao diện [`index_html.h`](file:///d:/DA_HTN/ESP32_S3_Gateway/index_html.h).<br>• Tích hợp biểu đồ Canvas Real-time (Nhiệm vụ 2.1).<br>• Lập trình bộ Data Logger xuất file CSV (Nhiệm vụ 2.2).<br>• Thêm nút Trim $\pm 0.1^\circ$ và Preset Profiles (Nhiệm vụ 2.3).<br>• Tối ưu độ trễ WebSockets (Nhiệm vụ 2.4). | • Web HUD hiển thị mượt mà trên smartphone, xuất được CSV chuẩn. |
| **Thành viên 3**<br>*(Hardware & Tuning Lead)* | Lắp ráp cơ khí & Căn chỉnh PID thực tế | • Hoàn thành kiểm thử độc lập linh kiện GĐ 3 (Buck, IMU, Motor, Encoder).<br>• Lắp ráp cơ khí 2 tầng, dán đệm xốp chống rung BMI160.<br>• Đấu nối dây Star GND.<br>• Chủ trì buổi căn chỉnh PID thực tế GĐ 4 ($K_{p1}, K_{d1} \to K_{p2}, K_{i2}$). | • Khung xe hoàn thiện chắc chắn, xe tự đứng thăng bằng không ngã. |
| **Thành viên 4**<br>*(Data Analyst & Documentation)* | Xử lý số liệu & Viết thuyết minh đồ án | • Viết script Python phân tích CSV tính $M_p, t_s, e_{ss}$ (Nhiệm vụ 6.1).<br>• Xuất các đồ thị Step-Response khoa học.<br>• Soạn thảo Thuyết minh đồ án môn học CE224 (Nhiệm vụ 6.3).<br>• Chuẩn bị Slide báo cáo hội đồng (Nhiệm vụ 6.4). | • File báo cáo Word/PDF hoàn chỉnh, bộ biểu đồ số liệu thực nghiệm. |
| **Cả nhóm phối hợp** | Nghiệm thu chung & Quay video | • Chạy thử chế độ đua GĐ 5.<br>• Quay video clip thực tế theo kịch bản 4 cảnh.<br>• Tập dượt thuyết trình vấn đáp đồ án. | • Video clip demo 1080p, hoàn thành bảo vệ đồ án đạt kết quả cao. |

---

## 4. TIÊU CHÍ NGHIỆM THU KỸ THUẬT CUỐI CÙNG (PROJECT ACCEPTANCE CRITERIA)

| Hạng mục kiểm tra | Tiêu chuẩn đạt | Phương pháp kiểm chứng |
| :--- | :--- | :--- |
| **Tự đứng thăng bằng tĩnh** | Đứng yên tại chỗ $> 60\text{s}$ không trôi quá $10\text{cm}$ | Đặt trên mặt sàn gạch phẳng, quan sát thực tế |
| **Khả năng dập tắt va chạm** | Thời gian dập tắt $t_s < 0.8\text{s}$, không ngã | Lấy tay đẩy một lực vừa phải vào đầu xe |
| **Sai số góc tĩnh ($e_{ss}$)** | $\le \pm 0.3^\circ$ so với phương thẳng đứng | Đo đạc qua gói Telemetry `$TEL` xuất ra CSV |
| **Vận tốc tối đa chế độ đua** | Đạt $\ge 1.8\text{ m/s}$ trong chế độ đua | Đo bằng số xung Encoder theo thời gian |
| **Khả năng bẻ lái linh hoạt** | Bo cua vòng cua bán kính $< 0.4\text{m}$, không lật | Lái zíc-zắc qua 5 chai nước đặt cách nhau 1m |
| **Độ trễ truyền nhận không dây** | Ping RTT $< 20\text{ms}$ qua WebSockets Wi-Fi | Hiển thị trực tiếp trên thanh trạng thái Web HUD |
| **Cơ chế an toàn ngắt điện** | Cắt PWM trong $< 5\text{ms}$ khi nghiêng $> 45^\circ$ | Làm nghiêng xe quá góc ngưỡng, nghe tiếng còi báo |
