# PHẦN CỨNG & KẾT NỐI HỆ THỐNG — XE CÂN BẰNG 2 BÁNH

Tài liệu hợp nhất toàn bộ thông số kỹ thuật, danh mục linh kiện (BOM), sơ đồ đấu nối chân (Pinout) và bố trí cơ khí 2 tầng cho dự án Xe Cân Bằng 2 Bánh Tốc Độ Cao.

---

## 1. THÔNG SỐ KỸ THUẬT CỐT LÕI

| Thông số | Giá trị | Ghi chú |
| :--- | :---: | :--- |
| **Vi điều khiển chính** | STM32F411CEU6 Black Pill | ARM Cortex-M4F @ 100MHz, FPU Hard ABI |
| **Cảm biến góc nghiêng** | Bosch BMI160 (GY-BMI160) | 6-DOF, Gyro $\pm 2000^\circ/s$, Accel $\pm 8g$, I2C 400kHz |
| **Chu kỳ điều khiển (Loop time)** | 5.0 ms (200 Hz) | Định thời ngắt cứng TIM4 (NVIC Priority 0) |
| **Động cơ & Hộp số** | 2x GA25-370 12V | Giảm tốc kim loại 1:30, tốc độ không tải ~300 RPM |
| **Encoder phản hồi** | Hall đĩa từ 2 kênh A/B | 11 xung $\times$ 30 $\times$ 4 = **1320 xung/vòng bánh xe** |
| **Mạch công suất** | Dual A4950 Driver | Cầu H MOSFET 3.5A, PWM 20kHz Center-aligned, Slow Decay |
| **Kết nối không dây** | ESP32-S3 N16R8 | Wi-Fi SoftAP, WebSockets, giao diện Web HUD (Joystick ảo) |
| **Nguồn cấp** | Pin LiPo 3S 11.1V (25C) | Mạch Buck XL4015 hạ áp cố định 5.0V (5A) nuôi mạch số |
| **Góc nghiêng cho phép** | $\pm 8^\circ$ (thường) / $\pm 15^\circ$ (đua) | Tự ngắt PWM bảo vệ khi nghiêng quá $\pm 45^\circ$ |

---

## 2. DANH MỤC LINH KIỆN (BILL OF MATERIALS - BOM)

| STT | Tên linh kiện | Số lượng | Thông số & Mục đích sử dụng |
| :---: | :--- | :---: | :--- |
| 1 | **STM32F411CEU6 Black Pill** | 01 | Bộ não xử lý trung tâm, ngắt 200Hz, tính Cascade PID bằng FPU |
| 2 | **Mạch nạp ST-Link V2** | 01 | Nạp và gỡ lỗi chương trình qua cổng SWD (SWDIO, SWCLK) |
| 3 | **Module Bosch BMI160** | 01 | Đo gia tốc và vận tốc góc Pitch, dán đệm xốp chống rung |
| 4 | **Động cơ GA25-370 (12V)** | 02 | Động cơ DC giảm tốc 1:30 kèm Hall Encoder đĩa từ 11 xung |
| 5 | **Bánh xe cao su 65mm** | 02 | Đường kính 65mm ($C \approx 0.2042\text{m}$), kèm khớp nối lục giác 4mm |
| 6 | **Mạch Driver Dual A4950** | 01 | Mạch công suất cầu H MOSFET 3.5A, điều khiển 2 motor |
| 7 | **Board ESP32-S3 N16R8** | 01 | Gateway không dây: phát Wi-Fi AP, chạy Web HUD điều khiển |
| 8 | **Pin LiPo 3S 11.1V (XT60)** | 01 | Cấp nguồn động lực 12V cho động cơ qua giắc XT60 |
| 9 | **Mạch Buck XL4015 (5A)** | 01 | Hạ áp từ 12V về 5.0V cấp nguồn cho STM32, BMI160, ESP32 |
| 10| **Khung Mica 2 tầng + Cọc đồng** | 01 bộ | 2 tấm mica DIY, 2 gá chữ L (25mm), 4 cọc đồng M3 dài 6cm |
| 11| **Còi Buzzer + Công tắc nguồn** | 01 bộ | Còi chíp 5V (PB12), công tắc bập bênh ngắt nguồn pin |

---

## 3. MA TRẬN KẾT NỐI CHÂN (PINOUT MATRIX)

Bảng đấu nối chi tiết, không trùng lặp và không xung đột ngoại vi trên STM32F411:

```
                  SƠ ĐỒ ĐẤU DÂY GIỮA CÁC MODULE
┌───────────────────────────┐           ┌───────────────────────────┐
│     STM32F411CEU6         │           │        ESP32-S3 N16R8     │
│ PB6 (USART1_TX)           ├──────────>│ GPIO18 (Serial1 RX)       │
│ PB7 (USART1_RX)           │<──────────┤ GPIO17 (Serial1 TX)       │
│ PB8 (I2C1_SCL) ────────┐  │           │ GND (Nối chung đất)       │
│ PB9 (I2C1_SDA) ─────┐  │  │           └───────────────────────────┘
│ PB2 (EXTI2) ─────┐  │  │  │
│ PB12 (Buzzer)    │  │  │  │           ┌───────────────────────────┐
│ PC13 (Status LED)│  │  │  │           │       BOSCH BMI160        │
└───────┬──────────┼──┼──┼──┘           │ SCL ──< Nối PB8 (I2C1)    │
        │          │  │  │              │ SDA ──< Nối PB9 (I2C1)    │
        │          └──┼──┼─────────────>│ INT1 ─< Nối PB2 (EXTI2)   │
        │             └──┼─────────────>│ GND, 3.3V/5V              │
        │                └─────────────>└───────────────────────────┘
        ▼ PWM 20kHz
┌───────────────────────────┐           ┌───────────────────────────┐
│     DRIVER DUAL A4950     │           │     HALL ENCODER 2 BÁNH   │
│ PA8  ──> IN1 (Motor L+)   │           │ PA0 (TIM2_CH1) <── Kênh A │ Trái
│ PA9  ──> IN2 (Motor L-)   │           │ PA1 (TIM2_CH2) <── Kênh B │
│ PA10 ──> IN3 (Motor R+)   │           │ PB4 (TIM3_CH1) <── Kênh A │ Phải
│ PA11 ──> IN4 (Motor R-)   │           │ PB5 (TIM3_CH2) <── Kênh B │
└───────────────────────────┘           └───────────────────────────┘
```

| Chân MCU | Ngoại vi ánh xạ | Kết nối phần cứng | Ghi chú kỹ thuật |
| :---: | :--- | :--- | :--- |
| **PA8** | `TIM1_CH1` (PWM) | Driver A4950: IN1 (Motor Trái) | PWM 20kHz đối xứng tâm (Center-aligned) |
| **PA9** | `TIM1_CH2` (PWM) | Driver A4950: IN2 (Motor Trái) | Chế độ Slow Decay (1 chân PWM, 1 chân 0V) |
| **PA10**| `TIM1_CH3` (PWM) | Driver A4950: IN3 (Motor Phải) | PWM 20kHz đối xứng tâm |
| **PA11**| `TIM1_CH4` (PWM) | Driver A4950: IN4 (Motor Phải) | Chế độ Slow Decay (1 chân PWM, 1 chân 0V) |
| **PA0** | `TIM2_CH1` (Encoder) | Encoder Trái: Kênh Hall A | Đếm 4 sườn xung (TI12 Mode) |
| **PA1** | `TIM2_CH2` (Encoder) | Encoder Trái: Kênh Hall B | |
| **PB4** | `TIM3_CH1` (Encoder) | Encoder Phải: Kênh Hall A | Remap sang PB4/PB5 giải phóng chân tự do |
| **PB5** | `TIM3_CH2` (Encoder) | Encoder Phải: Kênh Hall B | |
| **PB8** | `I2C1_SCL` (Fast Mode)| Cảm biến BMI160: Chân SCL | Tốc độ 400kHz, trở kéo 4.7k trên module |
| **PB9** | `I2C1_SDA` (Fast Mode)| Cảm biến BMI160: Chân SDA | |
| **PB2** | `GPIO_EXTI2` (Ngắt) | Cảm biến BMI160: Chân INT1 | Bắt sự kiện dữ liệu IMU sẵn sàng |
| **PB6** | `USART1_TX` (DMA) | ESP32-S3: Chân GPIO18 (RX) | Baudrate 115200 bps |
| **PB7** | `USART1_RX` (DMA) | ESP32-S3: Chân GPIO17 (TX) | Circular DMA buffer nhận lệnh điều khiển |
| **PB12**| `GPIO_Output` | Còi chíp Active Buzzer (+) | Báo trạng thái FSM và cảnh báo ngã |
| **PC13**| `GPIO_Output` | LED xanh on-board Black Pill | Active LOW: Báo Calib / Run / Lỗi |

---

## 4. HƯỚNG DẪN CẤU HÌNH & LƯU Ý TỪNG LINH KIỆN

### 4.1. STM32F411CEU6 Black Pill
* **Cấu hình xung**: HSE = 25MHz $\to$ SYSCLK = 100MHz.
* **Cờ biên dịch STM32CubeIDE**: Bắt buộc chọn `Float ABI = Hard` (`-mfloat-abi=hard -mfpu=fpv4-sp-d16`) và thêm `-u _printf_float` trong Linker Flags để in số thực ra Telemetry.

### 4.2. Cảm biến quán tính Bosch BMI160
* **Địa chỉ I2C**: `0x68` (HAL: `0xD0` khi ghi, `0xD1` khi đọc). Tốc độ: **400kHz**.
* **Trình tự khởi tạo**:
  1. Đọc `CHIP_ID (0x00)`: Bắt buộc trả về **`0xD1`**.
  2. Ghi `0x11` vào `CMD (0x7E)` $\to$ Đánh thức Accel (delay 5ms).
  3. Ghi `0x15` vào `CMD (0x7E)` $\to$ Đánh thức Gyro (delay 50ms để bộ dao động ổn định).
* **Dải đo**: Gyro $\pm 2000^\circ/s$ (`16.4 LSB/dps`), Accel $\pm 8g$ (`4096 LSB/g`).
* **Đọc Burst 12 bytes**: Đọc liên tục thanh ghi `0x0C` đến `0x17` trong 1 frame I2C ($< 350\text{µs}$).

### 4.3. Động cơ GA25-370 & Encoder
* **Độ phân giải**: $11\text{ xung} \times 30 \times 4 = 1320\text{ xung/vòng bánh}$.
* **Chu vi bánh xe**: Đường kính 65mm $\to C \approx 0.2042\text{m}$.
* **Lọc vận tốc**: Vận tốc tức thời qua bộ lọc LPF ($\beta = 0.75$) để khử nhiễu lượng tử hóa ở tốc độ chậm.

### 4.4. Mạch công suất Dual A4950
* **Chế độ Slow Decay**: 1 chân cấp PWM 20kHz, chân còn lại nối 0V.
* **Bù vùng chết ma sát hộp số (Deadband)**: Cộng/trừ $250$ xung vào PWM để thắng lực ma sát tĩnh của hộp số GA25.

### 4.5. Gateway không dây ESP32-S3 N16R8
* **Firmware**: Thư mục `d:\DA_HTN\ESP32_S3_Gateway\`.
* **Mạng Wi-Fi**: SoftAP `TWIP_RACER_S3` (mật khẩu `12345678`), truy cập `http://192.168.4.1`.
* **Giao tiếp**: Web Server port 80, WebSockets port 81.
* **Gói tin UART**:
  * Nhận: `$CMD,v_tgt,steer*`, `$PID,kp1,kd1,kp2,ki2*`, `$RACE,1/0*`, `$CALIB*`, `$STOP*`.
  * Gửi: `$TEL,pitch,gyro,v_act,v_tgt,pwm_l,pwm_r,state,batt\r\n` (chu kỳ 50ms / 20Hz).

### 4.6. Nguồn Pin LiPo 3S & Buck XL4015
* **Pin LiPo 3S 11.1V - 12.6V**: Cấp nguồn động lực trực tiếp cho Driver A4950 qua giắc XT60.
* **Mạch Buck XL4015**: **Bắt buộc đo chỉnh chiết áp ngõ ra về đúng 5.0V** trước khi cắm vào STM32/ESP32.
* **Quy trình Plug-and-Play**: Sạc đầy pin $\to$ Cắm giắc XT60 bật công tắc $\to$ Dựng xe chạy $\to$ Rút giắc khi xong.

---

## 5. BỐ TRÍ CƠ KHÍ KHUNG DIY 2 TẦNG & CHỐNG NHIỄU

### 5.1. Sơ đồ bố trí linh kiện 2 tầng
```
                             [TẦNG 2: XỬ LÝ & CẢM BIẾN]
        ┌─────────────────────────────────────────────────────────────┐
        │  [Công tắc nguồn]   [STM32F411]     [GY-BMI160]   [ESP32-S3]│
        │                     (Black Pill)   (Đệm xốp 3M)   (N16R8)   │
        │                                     [Buzzer PB12]           │
        └──────────────────────────────┬──────────────────────────────┘
                                       │ 4 cọc đồng M3 (dài 6cm)
                             [TẦNG 1: ĐỘNG LỰC & NGUỒN]
        ┌──────────────────────────────┴──────────────────────────────┐
        │   [Pin LiPo 3S 11.1V]   [Driver A4950]    [Buck XL4015 5A]  │
        └───┬─────────────────────────────────────────────────────┬───┘
            │                                                     │
    [Gá kim loại L] ── [Motor Trái GA25]         [Motor Phải GA25] ── [Gá kim loại L]
    [Khớp đồng lục giác 4mm]                     [Khớp đồng lục giác 4mm]
    [Bánh cao su 65mm]                           [Bánh cao su 65mm]
```

* **Tầng 1 (Đáy)**:
  * Mặt dưới: 2 gá kim loại chữ L giữ motor GA25, **2 trục bánh xe phải nằm thẳng hàng 100%**.
  * Mặt trên: Pin LiPo đặt sát trục bánh để hạ thấp quán tính xoay; Driver A4950 đặt giữa 2 motor; Buck XL4015 ở mép sau.
* **Tầng 2 (Đỉnh)**:
  * **BMI160**: Đặt tại **chính giữa tâm hình học** ngay trên trục bánh xe. **Dán lên đệm xốp 3M / FPV dày 2-3mm** để hấp thụ rung cơ học từ hộp số.
  * **STM32F411**: Đặt cạnh BMI160 để dây I2C ngắn ($< 5\text{cm}$), hướng cổng Type-C ra ngoài để cắm nạp code.
  * **ESP32-S3**: Đặt ở mép ngoài, hướng ăng-ten ra ngoài để sóng Wi-Fi không bị cọc đồng che khuất.

### 5.2. Quy tắc đi dây chống nhiễu
1. Không bó chung dây động cơ (dòng lớn, PWM 20kHz) với dây tín hiệu I2C của BMI160.
2. Dây Encoder 2 kênh A/B nên xoắn đôi (twisted-pair).
3. Đấu chung mass GND của Pin, Buck, Driver, STM32 và ESP32 về một điểm chung (Star GND).
4. Dây nối giữa 2 tầng để độ chùng vừa phải, tránh kéo căng làm truyền rung động lên cảm biến.
