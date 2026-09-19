# LỘ TRÌNH CHI TIẾT TRIỂN KHAI ĐỒ ÁN (PROJECT ROADMAP)
## XE HAI BÁNH TỰ CÂN BẰNG TỐC ĐỘ CAO (HIGH-SPEED BALANCING ROBOT)
### Vi Điều Khiển STM32F411CEU6 • Cảm Biến Bosch BMI160 (GY-BMI160) • Điều Khiển Cascade PID

---

## 1. MỤC TIÊU CỐT LÕI CỦA ĐỒ ÁN
* **Tự cân bằng đứng yên vững chắc tại chỗ**:
  * Xe 2 bánh tự động giữ thăng bằng thẳng đứng tại chỗ, triệt tiêu mọi rung lắc cơ học và ma sát hộp số.
  * Tự động bù lệch trọng tâm tĩnh ($\theta_{\text{trim}}$) do sai số phân bổ linh kiện và vị trí pin, xe đứng bất động mà không bị trôi vị trí trên sàn bằng phẳng.
  * Khi có ngoại lực tác động (ngón tay đẩy nhẹ hoặc va chạm nhẹ), xe tự động tạo lực đẩy phản kháng, lùi/tiến để đỡ trọng tâm và trở về vị trí cân bằng cũ.
* **Điều khiển chạy đua tốc độ cao (High-Speed Racing & Maneuvering)**:
  * Nhận lệnh điều khiển từ xa qua Wi-Fi Web App / BLE từ gateway ESP32-S3 (Joystick ảo trên trình duyệt điện thoại/PC hoặc tay cầm ESP-NOW) với đáp ứng vận tốc tức thì.
  * Chủ động ngả thân xe về phía trước khi tăng tốc ($\theta_{\text{target}}$ mở rộng lên tới $\pm 15^\circ$) để đạt gia tốc đua lớn ($a_x > 2.5\text{ m/s}^2$).
  * Tích hợp thuật toán bẻ lái vi sai thích ứng vận tốc (Speed-Adaptive Steering) giúp xe vào cua gắt, lượn số 8 ở tốc độ cao mà thân xe vẫn đầm chắc, hoàn toàn không bị lật.
* **Vận hành tiện lợi, tối giản (Plug-and-Play)**:
  * Sử dụng pin LiPo 3S cắm trực tiếp giắc XT60: Chỉ cần sạc đầy pin là cắm chạy thử nghiệm, không rườm rà đo đạc pin qua ADC, tiết kiệm chân MCU và tinh giản phần mềm.
  * Bảo vệ phần cứng tự động: Ngắt động cơ tức thì ($< 5\text{ms}$) khi xe ngã quá $45^\circ$ hoặc kẹt bánh xe.

---

## 2. CHI TIẾT LỘ TRÌNH TRIỂN KHAI CÁC GIAI ĐOẠN

---

### GIAI ĐOẠN 1: CHUẨN BỊ PHẦN CỨNG & CẤU HÌNH NGOẠI VI TRÊN CUBEMX
> **Mục tiêu**: Thiết lập toàn bộ thông số phần cứng của chip STM32F411CEU6 trên phần mềm STM32CubeMX, đảm bảo không xảy ra xung đột chân (Conflict-free Pinout), kích hoạt bộ tăng tốc số thực FPU và xung nhịp tối đa 100MHz.

#### Danh sách đầu việc thực thi:
- [ ] **Thiết lập Xung nhịp hệ thống (RCC & Clock Tree)**:
  * Nguồn xung ngoài thạch anh HSE = 25MHz.
  * Cấu hình PLL nhân tần số hệ thống lên mức cực đại **SYSCLK = 100MHz**, APB1 = 50MHz, APB2 = 100MHz.
  * **Bắt buộc bật FPU (Floating Point Unit)**: Chọn `Hard ABI` để trình biên dịch tận dụng lệnh phần cứng của lõi ARM Cortex-M4F cho các phép tính số thực `float`.
- [ ] **Cấu hình Timer 1 (Điều khiển công suất động cơ qua Driver Dual A4950)**:
  * Kênh: `CH1` (PA8), `CH2` (PA9), `CH3` (PA10), `CH4` (PA11).
  * Chế độ: **Center-aligned Mode 1** (đối xứng tâm giúp triệt tiêu sóng hài động cơ).
  * Tần số PWM: $20\text{ kHz}$ (Prescaler = 0, ARR = 2499 với Clock Timer 100MHz). Vượt ngưỡng nghe của tai người, động cơ chạy hoàn toàn êm ái.
- [ ] **Cấu hình Timer 2 & Timer 3 (Đọc phản hồi Hall Encoder)**:
  * **TIM2** (Bánh Trái): `CH1` (PA0), `CH2` (PA1) $\rightarrow$ `Encoder Mode TI12` (đếm x4 cả 2 cạnh xung A và B).
  * **TIM3** (Bánh Phải): **Remap sang chân `PB4` (CH1) và `PB5` (CH2)** để giải phóng chân GPIO tự do. Cấu hình `Encoder Mode TI12`.
- [ ] **Cấu hình I2C1 (Giao tiếp cảm biến quán tính Bosch BMI160 / GY-BMI160)**:
  * Chế độ: `I2C Master`, tốc độ **Fast Mode 400kHz** (`I2C Speed Frequency = 400000 Hz`).
  * Chân kết nối: `SCL` (**PB8**), `SDA` (**PB9**).
  * Chân ngắt ngoài: `PB2` làm `GPIO_EXTI2` nối chân `INT1` của BMI160 để bắt sự kiện Data Ready.
  * Giải phóng toàn bộ cụm chân SPI1 (`PA4`..`PA7`) làm chân GPIO tự do phục vụ mở rộng phần cứng.
- [ ] **Cấu hình GPIO Còi Buzzer & LED trạng thái**:
  * Chân còi Active Buzzer: Cấu hình **`PB12`** làm `GPIO_Output` (chuyển sang PB12 để giải phóng PB8 cho I2C1_SCL).
  * Chân LED xanh on-board: Cấu hình **`PC13`** làm `GPIO_Output` (Active LOW).
- [ ] **Cấu hình USART1 (Truyền thông Gateway ESP32-S3 N16R8 & Telemetry)**:
  * **Remap chân**: Chuyển `TX` sang **`PB6`** (nối RX ESP32-S3) và `RX` sang **`PB7`** (nối TX ESP32-S3) để giải phóng PA9/PA10 cho Timer 1 PWM.
  * Baudrate: **115200 bps**, 8-N-1.
  * Kích hoạt chế độ **DMA Circular / Normal** kèm ngắt **Idle Line Detection** để nhận trọn vẹn chuỗi lệnh từ ESP32-S3 mà không làm nghẽn CPU.
- [ ] **Cấu hình Timer 4 (Tạo nhịp tim điều khiển 200Hz / 5ms)**:
  * Cấu hình ngắt định thời định kỳ: Prescaler = 99, ARR = 4999 $\rightarrow$ Thời gian ngắt chính xác đúng $5.000\text{ ms}$.
  * Mức ưu tiên ngắt (NVIC Priority): Đặt độ ưu tiên cao nhất (`Preemption Priority = 0`).
- [ ] **Kiểm tra điện áp an toàn trước khi cắm tải**:
  * Cắm pin LiPo 3S vào mạch Buck XL4015.
  * Dùng đồng hồ VOM đo ngõ ra mạch Buck: **Bắt buộc vặn chiết áp để điện áp ra đạt chuẩn đúng 5.0V** trước khi cắm vào chân 5V của STM32F411 Black Pill.

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 1**: Sinh code thành công từ CubeMX, mở project trong STM32CubeIDE biên dịch 0 Error, 0 Warning. Đo điện áp nuôi mạch số đạt 5.0V ổn định.

---

### GIAI ĐOẠN 2: LẬP TRÌNH BỘ ĐIỀU KHIỂN THIẾT BỊ ĐỘC LẬP (DEVICE DRIVERS)
> **Mục tiêu**: Viết và kiểm thử riêng rẽ từng hàm điều khiển cho từng linh kiện. Khẳng định 100% phần cứng hoạt động hoàn hảo trước khi ghép vào giải thuật cân bằng.

#### Danh sách đầu việc thực thi:
- [ ] **Lập trình Driver Động cơ & Dual A4950 (`motor.c / motor.h`)**:
  * Viết hàm `A4950_Set_Duty(int16_t pwm_l, int16_t pwm_r)` với đầu vào từ `-2499` đến `+2499`.
  * Vận hành theo chế độ **Slow Decay**:
    * Quay tiến ($PWM > 0$): Chân IN1 phát xung PWM, chân IN2 kéo xuống GND.
    * Quay lùi ($PWM < 0$): Chân IN1 kéo xuống GND, chân IN2 phát xung PWM.
    * Thả trôi/Phanh ($PWM = 0$): Cả 2 chân IN1 và IN2 kéo xuống GND.
  * *Kiểm thử*: Nạp code cho 2 động cơ tự động quay tới trong 2 giây, lùi trong 2 giây, dừng 2 giây. Quan sát hướng quay bánh xe đúng chiều quy ước.
- [ ] **Lập trình Driver Đọc Encoder (`encoder.c / encoder.h`)**:
  * Viết hàm `Encoder_Update(float dt)`: Đọc giá trị chênh lệch thanh ghi `TIM2->CNT` và `TIM3->CNT`, tính số xung quay được trong khoảng thời gian $dt$.
  * Quy đổi ra số vòng và vận tốc tịnh tiến $v$ (m/s):
    $$v = \frac{\Delta \text{Count} \times \pi \times D}{1320 \times dt} \quad (\text{với } D = 0.065\text{ m})$$
  * *Kiểm thử*: Lấy tay quay bánh xe đúng 1 vòng tròn cơ học, kiểm tra biến đếm xung trên IDE Watch Window phải đạt xấp xỉ đúng $1320 \text{ xung}$ ($\pm 2\%$).
- [ ] **Lập trình Driver Cảm biến IMU Bosch BMI160 (`bmi160.c / bmi160.h`)**:
  * Địa chỉ I2C của BMI160: `0x68` (địa chỉ 8-bit trên STM32 HAL: `0xD0` khi ghi, `0xD1` khi đọc).
  * Kiểm tra định danh cảm biến: Đọc thanh ghi `CHIP_ID` (địa chỉ `0x00`), giá trị trả về bắt buộc phải là **`0xD1`**.
  * Khởi tạo & Đánh thức BMI160 qua thanh ghi lệnh `CMD` (`0x7E`):
    * Ghi `0x11` vào `CMD (0x7E)` để đưa Accelerometer vào Normal Mode (chờ delay $5\text{ms}$).
    * Ghi `0x15` vào `CMD (0x7E)` để đưa Gyroscope vào Normal Mode (chờ delay $50\text{ms}$ để bộ dao động ổn định).
  * Cấu hình dải đo:
    * Dải đo Gyro: $\pm 2000^\circ/\text{s}$ (Thanh ghi `GYR_RANGE 0x43`, ghi `0x00`, hệ số chia $16.4\text{ LSB}/(^\circ/\text{s})$).
    * Dải đo Accel: $\pm 8\text{g}$ (Thanh ghi `ACC_RANGE 0x41`, ghi `0x08`, hệ số chia $4096\text{ LSB}/\text{g}$) hoặc $\pm 4\text{g}$ (ghi `0x05`, $8192\text{ LSB}/\text{g}$).
    * Bộ lọc số nội ODR/BWP: Cấu hình thanh ghi `ACC_CONF (0x40)` và `GYR_CONF (0x42)` về ODR 200Hz hoặc 400Hz, lọc Normal Bandwidth để triệt tiêu sóng rung cơ học từ hộp số GA25.
  * Viết hàm đọc Burst 12 bytes liên tục (`BMI160_Read_All()`): Thu thập đồng thời $G_x, G_y, G_z, A_x, A_y, A_z$ từ thanh ghi `0x0C` (`DATA_0`) đến `0x17` trong một lần truyền I2C duy nhất ($< 350\text{ µs}$).
- [ ] **Lập trình Giao tiếp Gateway ESP32-S3 N16R8 & Telemetry (`telemetry.c / telemetry.h`)**:
  * Định nghĩa giao thức gói tin nhận từ ESP32-S3 (nguồn từ Web UI / BLE / App):
    * Lệnh lái: `$CMD,v,steer*` (Ví dụ: `$CMD,0.5,-0.2*`).
    * Lệnh tune thông số PID trực tiếp: `$PID,kp1,kd1,kp2,ki2*`.
  * Định nghĩa gói tin Telemetry gửi từ STM32 sang ESP32-S3 (tần số 20Hz):
    * `$TEL,pitch,pitch_target,velocity,pwm_l,pwm_r\r\n`
  * Lập trình firmware ESP32-S3 (phát Wi-Fi AP, chạy Web Server / WebSockets hoặc BLE Serial để forward dữ liệu sang STM32).
  * Kết nối phần mềm đồ thị **VOFA+** trên máy tính (qua TCP/Wi-Fi hoặc BLE) và giao diện Web trên điện thoại để kiểm tra hiển thị dạng sóng và điều khiển.

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 2**: Động cơ quay đúng lệnh, Encoder đo đúng 1320 xung/vòng, Bosch BMI160 trả về CHIP_ID = 0xD1, đồ thị VOFA+ và Web Dashboard nhận thông số mượt mà từ ESP32-S3.

---

### GIAI ĐOẠN 3: XỬ LÝ TÍN HIỆU & ƯỚC LƯỢNG TRẠNG THÁI (STATE ESTIMATION)
> **Mục tiêu**: Chuyển hóa dữ liệu thô từ cảm biến thành các đại lượng vật lý chuẩn xác, khử trôi và triệt tiêu rung động cơ học.

#### Danh sách đầu việc thực thi:
- [ ] **Lắp đệm xốp FPV triệt tiêu rung động cơ học**:
  * Cảm biến IMU tuyệt đối không được bắt vít cứng trực tiếp vào khung xe kim loại.
  * Dán cảm biến lên lớp băng keo xốp đệm chuyên dụng (FPV Foam Tape) để hấp thụ xung lực rung từ mặt đường và cặp động cơ GA25.
- [ ] **Hiệu chuẩn độ trôi tĩnh con quay hồi chuyển (Zero-rate Gyro Bias Calibration)**:
  * Khi bật nguồn, đặt xe nằm yên tĩnh trên mặt sàn.
  * Đọc 500 mẫu dữ liệu con quay hồi chuyển trục Y ($G_y$) trong vòng $2.5\text{ giây}$.
  * Tính giá trị trung bình làm giá trị bù lệch tĩnh (Offset $G_{y\_\text{bias}}$):
    $$G_{y\_\text{actual}} = G_{y\_\text{raw}} - G_{y\_\text{bias}}$$
- [ ] **Cài đặt Bộ lọc bù góc nghiêng (Complementary Filter)**:
  * Tính góc nghiêng tĩnh từ gia tốc kế:
    $$\theta_{\text{acc}} = \text{atan2f}(A_x, A_z) \times 57.29578^\circ$$
  * Tích hợp với vận tốc góc từ Gyroscope theo công thức bù tần số:
    $$\theta_k = 0.98 \times (\theta_{k-1} + G_{y\_\text{actual}} \times \Delta t) + 0.02 \times \theta_{\text{acc}}$$
  * Đảm bảo: Không bị trễ pha khi xe lắc nhanh (nhờ Gyro), không bị trôi dạt góc dài hạn (nhờ Accel).
- [ ] **Lọc thông thấp vận tốc bánh xe (Low-Pass Filter)**:
  * Tín hiệu vận tốc tính từ sai phân xung Encoder thường có bậc thang và nhiễu do lượng tử hóa.
  * Áp dụng bộ lọc IIR thông thấp bậc 1 với $\beta = 0.75$:
    $$v_{\text{filtered}}[k] = 0.25 \times v_{\text{raw}}[k] + 0.75 \times v_{\text{filtered}}[k-1]$$

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 3**: Đồ thị góc $\theta$ trên VOFA+ bám sát góc nghiêng thực tế khi cầm tay lắc xe, giá trị góc tĩnh đứng yên ở mức sai số $< 0.05^\circ$, không bị trôi theo thời gian.

---

### GIAI ĐOẠN 4: CĂN CHỈNH THUẬT TOÁN ĐIỀU KHIỂN CASCADE PID
> **Mục tiêu**: Hiện thực hóa giải thuật Cascade 2 vòng lồng nhau, đưa xe vào trạng thái tự giữ thăng bằng tại chỗ và bù lệch trọng tâm tự động.

```
       ┌────────────────────────────────────────────────────────────────────────┐
       │                   QUY TRÌNH TUNE THÔNG SỐ 4 BƯỚC                       │
       └──────────────────────────────────┬─────────────────────────────────────┘
                                          ▼
   [BƯỚC 4.1: TẮT VÒNG VẬN TỐC (Kp2=0, Ki2=0) -> CHỈ DÒ VÒNG GÓC ANGLE PD]
   • Tăng Kp1 từ từ -> Xe có phản lực đẩy tay khi bị nghiêng nhưng dao động.
   • Tăng Kd1 -> Dập tắt dao động, xe đứng cứng vững nhưng bị trôi từ từ trên sàn.
                                          ▼
   [BƯỚC 4.2: TÍCH HỢP BÙ VÙNG CHẾT MA SÁT HỘP SỐ GA25 (DEADBAND)]
   • Cài đặt Deadband = 250 tick.
   • Triệt tiêu hoàn toàn hiện tượng rung lắc li ti (Limit Cycle) quanh điểm 0.
                                          ▼
   [BƯỚC 4.3: KÍCH HOẠT VÒNG VẬN TỐC (VELOCITY PI)]
   • Bật Kp2 -> Xe chống lại chuyển động trôi tịnh tiến.
   • Bật Ki2 -> Sinh góc nghiêng bù Theta_trim, xe đứng bất động tại chỗ.
                                          ▼
   [BƯỚC 4.4: KHÓA ANTI-WINDUP & CLAMP GÓC ĐẶT]
   • Kẹp tích phân Ki2 (+-10 độ) và kẹp Theta_target (+-8 độ).
   • Xe không bị "vọt lố tích phân" khi người dùng giữ tay cản trở bánh xe.
```

#### Danh sách đầu việc thực thi:
- [ ] **Hiện thực hóa hàm tính toán `Cascade_PID_Compute_5ms()` trong ngắt TIM4**:
  * Vòng ngoài (Velocity Loop): Tính sai số $e_v = v_{\text{target}} - v_{\text{actual}}$, xuất ra $\theta_{\text{target}}$.
  * Vòng trong (Angle Loop): Tính sai số $e_\theta = \theta_{\text{target}} - \theta_{\text{actual}}$, xuất ra $\text{PWM}_{\text{base}} = K_{p1} e_\theta - K_{d1} G_y$.
- [ ] **Căn chỉnh Vòng Trong (Angle Loop PD)**:
  * Tạm đặt $K_{p2} = 0, K_{i2} = 0, v_{\text{target}} = 0$.
  * Tăng dần $K_{p1}$: Thử các dải giá trị từ $150 \rightarrow 350 \rightarrow 550$. Quan sát lực phản kháng ở trục bánh xe.
  * Tăng dần $K_{d1}$: Thử dải giá trị từ $4.0 \rightarrow 8.0 \rightarrow 14.0$. Điều chỉnh đến khi thả tay ra, xe có thể tự đứng khoảng 5 - 10 giây trước khi từ từ trôi ngã.
- [ ] **Áp dụng hàm bù vùng chết ma sát hộp số (Deadband Compensation)**:
  * Viết hàm `Apply_Deadband(pwm, 250, 2499)`.
  * Đảm bảo khi bộ điều khiển yêu cầu lực nhỏ, điện áp ngõ ra lập tức nhảy qua mức $1.2\text{V}$ để thắng ma sát tĩnh bánh răng GA25.
- [ ] **Căn chỉnh Vòng Ngoài (Velocity Loop PI)**:
  * Tăng dần $K_{p2}$: Thử dải giá trị từ $0.8 \rightarrow 1.5 \rightarrow 2.5$. Xe bắt đầu có phản xạ tự lùi bánh lại khi thân xe bị trôi tới.
  * Tăng dần $K_{i2}$: Thử dải giá trị từ $0.05 \rightarrow 0.15 \rightarrow 0.35$. Thành phần $I_v$ tích lũy điện áp bù góc lệch trọng tâm $\theta_{\text{trim}}$.
- [ ] **Cài đặt Anti-windup & Kẹp góc**:
  * Giới hạn thành phần tích phân vận tốc: $[-10^\circ, +10^\circ]$.
  * Giới hạn góc đặt thăng bằng thông thường: $[-8^\circ, +8^\circ]$.

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 4**: Xe tự đứng thăng bằng ổn định trên sàn nhà bằng phẳng liên tục $> 5\text{ phút}$. Khi lấy ngón tay đẩy thân xe, xe tự động chống cự, lùi lại và trở về vị trí đứng yên ban đầu.

---

### GIAI ĐOẠN 5: TÍNH NĂNG TỐC ĐỘ CAO, ĐUA XE & MÁY TRẠNG THÁI (RACING & FSM)
> **Mục tiêu**: Mở rộng dải vận hành của robot để đạt tốc độ cao, vào cua ổn định mà không bị lật xe, và bảo vệ phần cứng bằng máy trạng thái hữu hạn.

#### Danh sách đầu việc thực thi:
- [ ] **Cài đặt Thuật toán Bù vi sai lái thích ứng theo vận tốc (Speed-Adaptive Steering)**:
  * Giảm hệ số bẻ lái khi xe đang phóng nhanh để tránh lật xe:
    $$\text{steer}_{\text{adaptive}} = \frac{\text{steer}_{\text{cmd}}}{1.0 + 1.2 \times |v_{\text{actual}}|}$$
    $$\text{PWM}_{\text{Left}} = \text{PWM}_{\text{base}} + \text{steer}_{\text{adaptive}}, \quad \text{PWM}_{\text{Right}} = \text{PWM}_{\text{base}} - \text{steer}_{\text{adaptive}}$$
- [ ] **Cài đặt Chế độ Đua (Racing Mode)**:
  * Khi nhận lệnh `$RACE,1*` từ ESP32-S3 (Web UI / BLE): Mở rộng góc nghiêng cho phép từ $\pm 8^\circ$ lên **$\pm 15^\circ$**.
  * Cho phép gia tốc tịnh tiến $a_x$ đạt cực đại:
    $$\tan(15^\circ) \approx 0.268 \implies a_x \approx 0.268 \times 9.81 \approx \mathbf{2.63\text{ m/s}^2}$$
    Xe bốc đầu lao vút về phía trước với tốc độ đua ấn tượng.
- [ ] **Hoàn thiện Máy trạng thái hữu hạn FSM (Finite State Machine)**:
  * Lập trình cấu trúc trạng thái với 6 mode:
    1. `STATE_INIT`: Cấp nguồn, khởi tạo phần cứng.
    2. `STATE_CALIBRATING`: Đứng yên lấy mẫu Gyro Bias (2.5 giây).
    3. `STATE_STANDBY`: Chờ người dùng dựng xe vào góc an toàn $|\theta| < 3^\circ$.
    4. `STATE_BALANCING`: Khóa vòng PID, xe tự cân bằng.
    5. `STATE_RACING`: Chế độ đua tốc độ cao.
    6. `STATE_FALLEN`: Ngã xe $> 45^\circ$, cắt ngay PWM về 0 trong $< 5\text{ms}$ để bảo vệ động cơ.
    7. `STATE_EMERGENCY`: Khóa hệ thống khi kẹt bánh $> 500\text{ms}$ (PWM cực đại nhưng tốc độ $\approx 0$) hoặc lỗi giao tiếp I2C với BMI160.
- [ ] **Cài đặt tín hiệu phản hồi người dùng (Buzzer & LED)**:
  * LED nhấp nháy theo tần số tương ứng trạng thái (Calib sáng liên tục, Run nhấp nháy 1Hz, Error nháy nhanh 10Hz).
  * Còi Buzzer (PB12) phát tiếng bíp khi chuyển mode và hú liên tục khi rơi vào trạng thái khẩn cấp `STATE_EMERGENCY`.

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 5**: Xe nhận lệnh từ Web UI / App qua ESP32-S3 chạy tiến/lùi mượt mà, ôm cua ngọt ở tốc độ cao không bị lật. Khi xô ngã xe, motor tự ngắt tức thì, dựng đứng xe lại xe tự động đứng thăng bằng trở lại.

---

### GIAI ĐOẠN 6: THỰC NGHIỆM, THU THẬP SỐ LIỆU & HOÀN THIỆN BÁO CÁO
> **Mục tiêu**: Kiểm thử các chỉ tiêu kỹ thuật thực tế, thu thập đồ thị thực nghiệm để đưa vào tài liệu đồ án tốt nghiệp, đóng gói toàn bộ dự án.

#### Danh sách đầu việc thực thi:
- [ ] **Thực nghiệm đo đạc các chỉ tiêu kỹ thuật chất lượng điều khiển**:
  * *Độ lệch góc tĩnh*: Đo bằng cảm biến Bosch BMI160 khi xe đứng yên ($\le \pm 0.3^\circ$).
  * *Thời gian hồi phục sau va chạm (Recovery Time)*: Tác động xung lực đẩy xe lệch $10^\circ$, đo thời gian xe lấy lại thăng bằng ($t_{\text{settling}} < 0.8\text{ s}$).
  * *Vận tốc tối đa đạt được*: Thử nghiệm chạy đua thẳng đo qua Encoder ($v_{\max} \ge 1.2\text{ m/s}$).
  * *Góc nghiêng dốc tối đa*: Cho xe leo dốc nghiêng thử nghiệm ($> 15^\circ$).
- [ ] **Thu thập đồ thị thực nghiệm từ VOFA+ đưa vào báo cáo đồ án**:
  * Đồ thị so sánh giữa góc gia tốc thô và góc qua bộ lọc bù Complementary Filter.
  * Đồ thị đáp ứng bước (Step Response) của vòng góc Angle Loop.
  * Đồ thị bám vận tốc đặt và đáp ứng thực tế của vòng Velocity Loop.
  * Đồ thị điều khiển vi sai bẻ lái ở các dải tốc độ khác nhau.
- [ ] **Quy chuẩn hóa quy trình vận hành Plug-and-Play**:
  * Hướng dẫn sạc pin LiPo bằng sạc chuyên dụng B3/B6.
  * Hướng dẫn cắm giắc XT60, bật công tắc, dựng xe khởi động và ngắt nguồn an toàn khi kết thúc buổi thực nghiệm.
- [ ] **Hoàn thiện tài liệu thuyết minh đồ án và Slide bảo vệ**:
  * Tổng hợp các chương: Giới thiệu đề tài $\rightarrow$ Cơ sở lý thuyết con lắc ngược $\rightarrow$ Thiết kế phần cứng ngoại vi $\rightarrow$ Thuật toán điều khiển Cascade PID $\rightarrow$ Kết quả thực nghiệm và kết luận.

> [!IMPORTANT]
> **Tiêu chuẩn nghiệm thu Giai đoạn 6**: Hoàn thành toàn bộ cuốn báo cáo thuyết minh đồ án kèm các đồ thị chứng minh thực tế; quay video clip xe tự cân bằng chạy đua biểu diễn thành công.
