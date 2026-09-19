# TÀI LIỆU ĐỌC HIỂU: MÔ TẢ & CÁCH SỬ DỤNG CÁC LINH KIỆN TRONG ĐỒ ÁN
### (Hướng Dẫn Bản Chất Vận Hành, Vai Trò, Kết Nối & Lưu Ý Kỹ Thuật Từng Linh Kiện)

> [!NOTE]
> * **Mục đích tài liệu**: Giúp bạn và người đọc hiểu tường tận **từng linh kiện trong đồ án làm nhiệm vụ gì, nó hoạt động ra sao, vi điều khiển giao tiếp với nó như thế nào và cần lưu ý những cạm bẫy kỹ thuật gì** trong bài toán xe 2 bánh tự cân bằng tốc độ cao.
> * **Định dạng**: Trực quan, giải thích bản chất vật lý và phương pháp vận hành, không đi vào chi tiết cấu trúc thư mục hay mã nguồn phức tạp.

---

## 1. BỨC TRANH TỔNG THỂ VỀ DÒNG CHẢY DỮ LIỆU & NĂNG LƯỢNG

Trước khi đi vào từng linh kiện, hãy nhìn vào cách các linh kiện phối hợp với nhau tạo thành một cỗ máy hoàn chỉnh:

```
                  ┌───────────────────────────────────────────────┐
                  │          PIN LiPo 3S (11.1V - 12.6V)          │
                  └───────┬───────────────────────────────┬───────┘
                          │ (Nguồn công suất 12V)         │
                          │                               ▼
                          │                     [Mạch Hạ Áp Buck XL4015]
                          │                     Hạ từ 12V xuống 5V (5A)
                          │                               │ (Nguồn nuôi số 5V)
                          ▼                               ▼
                [Driver Dual A4950]            [STM32F411CEU6 Black Pill]
                Cầu H công suất MOSFET         "Bộ não" Cortex-M4F 100MHz
                  ▲               ▲               ▲      ▲          ▲
      (PWM 20kHz) │               │ (PWM 20kHz)   │      │ (I2C1)   │ (UART)
                  │               │               │      │ PB8/PB9  │ PB6/PB7
           [Motor Trái]       [Motor Phải]        │      │          │
           GA25-370 12V       GA25-370 12V        │   [Bosch BMI160] [ESP32-S3 N16R8]
                   │               │               │   (GY-BMI160)   Wi-Fi/BLE Web UI
                   │               │               │   Đo góc/Gyro Lái xe & Tune PID
                  ▼ (Xung Encoder)▼ (Xung Encoder)│
             [Hall A/B]      [Hall A/B] ──────────┘
             (TIM2 x4)       (TIM3 x4)
```

---

## 2. VI ĐIỀU KHIỂN STM32F411CEU6 BLACK PILL ("BỘ NÃO" TRUNG TÂM)

### 2.1. Vai trò trong đồ án
STM32F411CEU6 là trung tâm điều khiển toàn bộ robot. Nó thu thập dữ liệu từ cảm biến quán tính (IMU) và các encoder, tính toán giải thuật điều khiển cân bằng (Cascade PID) ở tần số cao, và điều khiển trực tiếp mạch công suất động cơ.

### 2.2. Tại sao lại chọn STM32F411?
* **Có bộ tính toán số thực FPU (Floating Point Unit)**: Trong xe cân bằng, các phép toán lượng giác (`atan2`), tích phân và bộ lọc số đều xử lý trên số thực `float`. Chip dòng Cortex-M0/M3 (như STM32F103 Blue Pill) không có FPU, phải tính số thực bằng phần mềm mất hàng chục chu kỳ máy. STM32F411 (Cortex-M4F) tính `float` trong **đúng 1 chu kỳ máy**, giúp giải thuật chạy cực nhanh (< 1ms).
* **Tốc độ xung nhịp 100MHz**: Đủ mạnh để chạy vòng lặp điều khiển thời gian thực **200Hz (5ms)** mà CPU vẫn rảnh rỗi hơn 80% để truyền telemetry và nhận lệnh từ ESP32-S3.
* **Hệ thống Timer phần cứng cực mạnh**: Tích hợp các bộ đếm giải mã xung Encoder và Timer phát xung PWM đối xứng tâm (Center-aligned) mà không tốn tài nguyên CPU.

### 2.3. Cách hệ thống sử dụng STM32F411
1. **Tạo nhịp tim ngắt 200Hz**: Cấu hình Timer 4 đếm định thời chính xác cứ mỗi $5.000\text{ ms}$ thì phát ngắt CPU một lần để thực thi vòng lặp cân bằng.
2. **Đọc Encoder phần cứng**: Cấu hình Timer 2 (bánh trái) và Timer 3 (bánh phải) ở chế độ `Encoder Mode TI12`. Phần cứng tự đếm xung của động cơ, CPU chỉ việc đọc thanh ghi số đếm.
3. **Phát xung PWM 20kHz**: Cấu hình Timer 1 phát 4 kênh xung điều khiển chiều và tốc độ động cơ.
4. **Đọc cảm biến tốc độ cao**: Dùng bộ I2C1 tốc độ Fast Mode 400kHz (chân PB8 - SCL, PB9 - SDA) để đọc đồng thời 12 thanh ghi gia tốc và con quay hồi chuyển từ Bosch BMI160 trong khoảng $\approx 300 - 350\text{ µs}$.
5. **Truyền nhận dữ liệu qua ESP32-S3**: Dùng bộ UART1 kết hợp DMA (chân PB6 - TX, PB7 - RX) để gửi dữ liệu telemetry lên đồ thị PC/Web và nhận lệnh điều khiển mà không làm đơ/nghẽn hệ thống.

### 2.4. Lưu ý & Cạm bẫy kỹ thuật
* **Bẫy xung đột chân (Pinout Conflict)**: Chân mặc định của USART1 (PA9, PA10) trùng với chân PWM của TIM1. Bắt buộc phải chuyển (remap) USART1 sang chân `PB6` và `PB7`. Timer 3 Encoder dùng `PB4` và `PB5`. Cảm biến Bosch BMI160 dùng I2C1 chân `PB8` (SCL) và `PB9` (SDA). Chân còi Buzzer được chuyển sang `PB12` để giải phóng hoàn toàn PB8 cho I2C1.
* **Bật phần cứng FPU**: Nếu quên bật FPU trong cấu hình CubeMX (`Hard ABI`), trình biên dịch sẽ sinh mã mô phỏng phần mềm chậm gấp 20 lần.

---

## 3. CẢM BIẾN QUÁN TÍNH BOSCH BMI160 / MODULE GY-BMI160 (IMU 6 TRỤC)

### 3.1. Vai trò trong đồ án
Cảm biến quán tính Bosch BMI160 (Module GY-BMI160) là "tai trong / mắt thần" của xe, liên tục đo lường:
1. Thân xe đang bị ngả về phía trước hay phía sau một góc bao nhiêu độ ($\theta$).
2. Thân xe đang ngã nhanh hay chậm (vận tốc góc $\omega$).

### 3.2. Đặc điểm & Lý do lựa chọn Bosch BMI160 (GY-BMI160)
* **Thế hệ cảm biến tiên tiến từ Bosch Sensortec (Đức)**: Khác với các dòng cảm biến đời cũ, Bosch BMI160 là cảm biến quán tính 16-bit siêu ít nhiễu (Ultra-low Noise), độ trôi nhiệt cực thấp, được thiết kế chuyên biệt cho drone đua, robot tự hành và các thiết bị đòi hỏi độ chính xác cao.
* **Tích hợp 6 trục chuyển động (6-DOF)**: Tích hợp 3 trục gia tốc kế (Accelerometer) và 3 trục con quay hồi chuyển (Gyroscope) trong một cấu trúc MEMS nguyên khối cứng vững, chịu rung sốc cơ học vượt trội.
* **Bộ lọc số phần cứng ODR & Bandwidth nội thông minh**: Cho phép cấu hình tốc độ lấy mẫu (Output Data Rate) lên tới 200Hz - 400Hz và chế độ lọc số Normal/Oversampling để triệt tiêu trực tiếp nhiễu rung cao tần từ hộp số GA25 trước khi xuất ra dữ liệu.
* **Hỗ trợ đa giao tiếp I2C & SPI**: Module GY-BMI160 hỗ trợ cả I2C và SPI. Trong dự án này, hệ thống sử dụng chuẩn **I2C1 Fast Mode 400kHz** (chân PB8/PB9), vừa đảm bảo thời gian đọc nhanh dưới $350\text{ µs}$, vừa giữ nguyên sơ đồ chân không xung đột cho STM32F411.
* **Tiêu thụ điện năng cực thấp**: Dòng tiêu thụ $< 1\text{ mA}$ khi chạy toàn tải 6 trục, không phát nhiệt làm trôi cảm biến.

### 3.3. Nguyên lý hoạt động thực tế & Sự kết hợp 2 cảm biến
Bên trong Bosch BMI160 có 2 cảm biến hoạt động theo 2 nguyên lý vật lý bổ trợ hoàn hảo cho nhau:

| Cảm biến thành phần | Đo lường đại lượng gì? | Ưu điểm cốt lõi | Nhược điểm chí mạng |
| :--- | :--- | :--- | :--- |
| **Gia tốc kế (Accelerometer)** | Đo vectơ trọng lực $g$ của Trái Đất theo 3 trục ($A_x, A_y, A_z$). | **Không bị trôi theo thời gian**. Khi xe đứng yên, tính góc nghiêng tĩnh $\arctan2(A_x, A_z)$ cực kỳ chuẩn xác. | **Cực kỳ nhạy cảm với rung chấn và lực quán tính**. Khi xe chạy nhanh hoặc rung lắc, gia tốc kế sẽ đọc sai góc. |
| **Con quay hồi chuyển (Gyroscope)** | Đo vận tốc góc quay quanh trục ngã ($\omega_y$, đơn vị $^\circ/\text{s}$). | **Phản hồi tức thời cực nhanh**, không bị ảnh hưởng bởi rung chấn mặt sàn. | **Bị trôi tĩnh (Zero-rate Drift)**. Nếu chỉ tích phân $\int \omega dt$ để tính góc, sau một khoảng thời gian góc sẽ bị trôi lệch dần. |

$$\implies \textbf{Giải pháp: BỘ LỌC BÙ (Complementary Filter)}$$
Phần mềm kết hợp cả 2: Lấy ưu thế phản ứng nhanh tức thời của Gyroscope ($98\%$) kết hợp với ưu thế giữ chuẩn dài hạn của Accelerometer ($2\%$) để tạo ra một góc nghiêng $\theta$ vừa mượt, vừa không có độ trễ, vừa không bị trôi!

### 3.4. Lưu ý & Cạm bẫy kỹ thuật sống còn khi sử dụng Bosch BMI160
1. **Trình tự đánh thức cảm biến qua thanh ghi lệnh CMD (0x7E)**:
   * Sau khi cấp nguồn, chip BMI160 mặc định ở chế độ ngủ (Suspend Mode). Bạn không thể đọc dữ liệu ngay mà bắt buộc phải gửi lệnh đánh thức:
     * Ghi `0x11` vào thanh ghi `CMD (0x7E)` để đưa Accelerometer vào Normal Mode $\rightarrow$ Chờ delay tối thiểu $5\text{ms}$.
     * Ghi `0x15` vào thanh ghi `CMD (0x7E)` để đưa Gyroscope vào Normal Mode $\rightarrow$ Chờ delay tối thiểu $50\text{ms}$ để bộ dao động Gyro ổn định.
2. **Kiểm tra định danh chip (Chip ID)**:
   * Đọc thanh ghi `CHIP_ID (0x00)`. Giá trị trả về chuẩn mực của Bosch BMI160 bắt buộc phải là **`0xD1`**. Nếu đọc ra `0x00` hoặc `0xFF` là bị lỗi tiếp xúc phần cứng I2C.
3. **Cấu hình dải đo & Bộ lọc số**:
   * Gyroscope: Cấu hình thanh ghi `GYR_RANGE (0x43)` về giá trị `0x00` tương ứng dải đo $\pm 2000^\circ/\text{s}$ (độ phân giải 16-bit, hệ số chuyển đổi $16.4\text{ LSB}/(^\circ/\text{s})$).
   * Accelerometer: Cấu hình thanh ghi `ACC_RANGE (0x41)` về giá trị `0x08` tương ứng dải đo $\pm 8\text{g}$ (độ phân giải 16-bit, hệ số chuyển đổi $4096\text{ LSB}/\text{g}$) hoặc $\pm 4\text{g}$ (`0x05`, $8192\text{ LSB}/\text{g}$).
   * Cấu hình ODR 200Hz hoặc 400Hz tại thanh ghi `ACC_CONF (0x40)` và `GYR_CONF (0x42)` để khớp chính xác với chu kỳ điều khiển ngắt 5ms của hệ thống.
4. **Đọc Burst 12 bytes liên tục từ thanh ghi DATA (0x0C đến 0x17)**:
   * BMI160 sắp xếp dữ liệu Gyro trước rồi đến Accel:
     * `0x0C - 0x11`: $G_x\text{ (LSB, MSB)}, G_y\text{ (LSB, MSB)}, G_z\text{ (LSB, MSB)}$.
     * `0x12 - 0x17`: $A_x\text{ (LSB, MSB)}, A_y\text{ (LSB, MSB)}, A_z\text{ (LSB, MSB)}$.
   * Chỉ cần 1 lần kéo đọc I2C Burst 12 bytes liên tục là lấy trọn vẹn toàn bộ trạng thái chuyển động của xe trong vòng $< 350\text{ µs}$.
5. **Bắt buộc hiệu chuẩn độ trôi tĩnh (Zero-rate Bias Calibration)**:
   * Khi vừa bật nguồn, xe phải được đặt nằm yên tĩnh trên sàn trong 2 - 3 giây để STM32 lấy 500 mẫu Gyroscope tính giá trị bù lệch tĩnh (Offset) và trừ ra khỏi mọi phép đo sau đó.
6. **Đệm xốp chống rung cơ học**:
   * Dán module GY-BMI160 lên lớp băng dính xốp đệm 3M / đệm FPV trên sàn mica để cách ly rung chấn cơ học tần số cao từ động cơ GA25.

---

## 4. ĐỘNG CƠ GA25-370 CÓ GẮN HALL ENCODER

### 4.1. Vai trò trong đồ án
* **Động cơ GA25-370 (12V)**: Cơ cấu chấp hành sinh lực đẩy bánh xe tiến/lùi để đỡ thân xe không bị ngã và tạo gia tốc đua.
* **Đĩa từ Hall Encoder ở đuôi**: Cảm biến đo vận tốc quay và vị trí bánh xe, cung cấp dữ liệu cho vòng điều khiển vận tốc (Velocity Loop).

### 4.2. Nguyên lý đo vận tốc của Encoder
* Đuôi trục động cơ gắn một đĩa nam châm vĩnh cửu có các cực từ xen kẽ. Khi motor quay, 2 cảm biến Hall A và B đặt lệch nhau $90^\circ$ sẽ sinh ra 2 chuỗi xung vuông lệch pha.
* Độ phân giải: Đĩa từ sinh ra **11 xung/vòng**.
* Hộp số kim loại giảm tốc **1:30**: Cứ 30 vòng trục motor thì bánh xe mới quay được 1 vòng.
* STM32 đếm cả 2 cạnh lên và xuống của cả kênh A và B (Chế độ x4):
  $$\text{Số xung / 1 vòng bánh xe} = 11 \times 30 \times 4 = 1320\text{ xung}$$
* Bánh xe đường kính $65\text{ mm} \implies$ Chu vi $C \approx 0.2042\text{ m}$.
  $$\implies 1\text{ xung đếm được tương đương xe di chuyển } \approx 0.155\text{ mm}$$

### 4.3. Cách phần mềm sử dụng động cơ & Encoder
* Mỗi chu kỳ 5ms, STM32 đọc số xung mà Timer đếm được, chia cho thời gian để tính ra vận tốc tức thời $v$ (m/s).
* Phần mềm áp dụng bộ lọc thông thấp (LPF) để làm mịn vận tốc, tránh hiện tượng vận tốc nhảy bậc gây giật ở tốc độ chậm.

### 4.4. Lưu ý & Cạm bẫy kỹ thuật
* **Hiện tượng vùng chết ma sát (Deadband / Stiction)**: Hộp số bánh răng kim loại có lực ma sát tĩnh. Nếu xuất PWM nhỏ (dưới 10% công suất), động cơ không đủ lực để quay $\rightarrow$ Xe bị rung lắc liên tục quanh điểm 0. Phần mềm bắt buộc phải có thuật toán **bù vùng chết** (tự động cộng thêm một lượng PWM tối thiểu để thắng ma sát).
* **Độ rơ khớp nối trục D 4mm**: Nếu ốc lục giác bắt bánh xe vào trục motor bị lỏng, bánh xe sẽ có độ rơ cơ khí. Đây là nguyên nhân số 1 khiến xe không bao giờ tự đứng yên được. Bắt buộc phải siết chặt và nhỏ keo khóa ren.

---

## 5. MẠCH DRIVER CÔNG SUẤT DUAL A4950

### 5.1. Vai trò trong đồ án
Vi điều khiển STM32 chỉ xuất được tín hiệu điều khiển điện áp nhỏ 3.3V và dòng điện vài mili-ampe (mA), trong khi động cơ GA25 khi tăng tốc hoặc phanh gấp cần điện áp 12V và dòng điện lên tới **2A - 3A**. Mạch Driver Dual A4950 đóng vai trò là "cơ bắp", khuếch đại tín hiệu điều khiển từ STM32 để cấp năng lượng cho 2 động cơ.

### 5.2. Tại sao chọn Dual A4950 thay vì L298N?
* L298N dùng transistor BJT cổ điển, sụt áp trên mạch rất lớn (mất 2V - 3V khiến động cơ yếu đi rõ rệt), tỏa nhiệt rất lớn và kích thước cồng kềnh.
* A4950 sử dụng transistor MOSFET hiện đại: Nội trở dẫn cực thấp, chịu dòng đỉnh 3.5A, xả năng lượng cực nhanh khi đảo chiều (rất quan trọng để phanh gấp cho xe đua cân bằng), kích thước siêu nhỏ gọn.

### 5.3. Cách điều khiển: Chế độ Slow Decay (Bắt buộc)
Mỗi động cơ được điều khiển bởi 2 chân: `IN1` và `IN2`.
* Khi muốn xe **Tiến**: Xuất xung PWM vào chân `IN1`, giữ chân `IN2` ở mức `0` (GND).
* Khi muốn xe **Lùi**: Giữ chân `IN1` ở mức `0` (GND), xuất xung PWM vào chân `IN2`.
* **Tại sao phải dùng Slow Decay?**: Khi xung PWM ngắt (mức LOW), dòng điện trong cuộn cảm động cơ sẽ tự tuần hoàn qua 2 MOSFET dưới của cầu H. Điều này giúp mô-men lực của motor biến thiên **tuyến tính và êm ái**, xe không bị giật cục và driver không bị nóng.

### 5.4. Lưu ý & Cạm bẫy kỹ thuật
* **Tần số PWM 20kHz**: Phải phát xung ở tần số 20kHz (trên ngưỡng nghe của tai người) để xe chạy hoàn toàn êm ái, không phát ra tiếng rít chói tai của cuộn cảm.
* **Cài đặt dòng qua chân VREF**: Trên mạch A4950 có chân VREF để đặt ngưỡng giới hạn dòng điện tối đa. Cần kiểm tra biến trở/điện trở phân áp trên mạch để driver không ngắt dòng quá sớm khi xe cần gia tốc mạnh.

---

## 6. BOARD GATEWAY KHÔNG DÂY ESP32-S3 N16R8 (WI-FI & BLUETOOTH 5 LE)

### 6.1. Vai trò trong đồ án
Module ESP32-S3 N16R8 đóng vai trò là "Trung tâm truyền thông không dây & IoT Gateway" của robot. Nó kết nối cầu nối với STM32F411 qua kênh UART DMA tốc độ cao, đồng thời mở ra các phương thức giao tiếp không dây hiện đại (Wi-Fi 2.4GHz và Bluetooth 5 LE) để điều khiển xe từ xa và hiển thị đồ thị telemetry theo thời gian thực.

### 6.2. Tại sao nâng cấp lên ESP32-S3 N16R8 thay vì module Bluetooth HC-05 cổ điển?
* **Cấu hình phần cứng vượt trội**:
  * Chip ESP32-S3: Lõi kép 32-bit Xtensa® LX7 xung nhịp lên tới **240 MHz**, tích hợp tập lệnh tăng tốc tính toán vector AI.
  * Bộ nhớ cực lớn: **16MB Flash (N16)** và **8MB Octal PSRAM (R8)**, cho phép lưu trữ toàn bộ mã nguồn Web Dashboard (HTML/CSS/JS nhúng) và bộ đệm telemetry dung lượng lớn trực tiếp trên chip mà không lo thiếu RAM.
* **Vượt trội hoàn toàn so với HC-05**:
  * *HC-05 cổ điển*: Chỉ hỗ trợ Bluetooth 2.0 SPP (Serial Port Profile) lạc hậu, tốc độ thấp, không thể kết nối trực tiếp với iPhone/iOS (do Apple chặn Bluetooth Classic SPP), bắt buộc người dùng phải cài app Android của bên thứ ba.
  * *ESP32-S3 N16R8*:
    1. **Tạo Web Dashboard điều khiển không cần cài App**: ESP32-S3 tự phát Wi-Fi Access Point (ví dụ: `TWIP-Robot-AP`), chạy Web Server & WebSockets nhúng. Người dùng dùng bất kỳ thiết bị nào (iPhone, Android, Laptop, iPad) chỉ cần kết nối Wi-Fi và mở trình duyệt web là có thể điều khiển xe bằng Joystick cảm ứng ảo và xem đồ thị thời gian thực siêu mượt.
    2. **Đa giao thức không dây**: Hỗ trợ đồng thời Wi-Fi (WebSockets / HTTP / TCP Socket), Bluetooth Low Energy (BLE 5.0 - tương thích hoàn hảo cả iOS và Android), và giao thức **ESP-NOW** (độ trễ siêu thấp $< 2\text{ms}$ phục vụ đua xe tốc độ cao với tay cầm điều khiển chuyên dụng).
    3. **Băng thông truyền dữ liệu cao**: Cho phép truyền dữ liệu Telemetry tốc độ cao mà không làm nghẽn kênh truyền UART của STM32.

### 6.3. Cách sử dụng & Sơ đồ giao tiếp với STM32F411
* **Giao tiếp liên chip (Inter-Chip UART)**:
  * STM32F411 `PB6` (USART1_TX) $\longrightarrow$ ESP32-S3 Chân `RX` (ví dụ GPIO18 / RXD1).
  * STM32F411 `PB7` (USART1_RX) $\longleftarrow$ ESP32-S3 Chân `TX` (ví dụ GPIO17 / TXD1).
  * Chân `GND` của ESP32-S3 bắt buộc phải nối chung với `GND` của STM32F411.
* **Cấp nguồn cho ESP32-S3**:
  * Cấp nguồn 5.0V sạch từ mạch Buck XL4015 (5A) vào chân `5V` (hoặc `VIN`) của board ESP32-S3. Mạch Buck 5A dư sức gánh đỉnh dòng phát sóng Wi-Fi (~300 - 500mA) của ESP32-S3 mà không gây sụt áp cho STM32.
* **Tốc độ truyền dữ liệu**: Cài đặt **115200 bps** (hoặc nâng lên 460800 bps khi cần xuất đồ thị siêu mịn).
* **Luồng dữ liệu 2 chiều**:
  * *Uplink (Điều khiển)*: Web UI / App BLE $\xrightarrow{\text{Wi-Fi/BLE}}$ ESP32-S3 $\xrightarrow{\text{UART DMA}}$ STM32 (`$CMD,v,steer*`, `$PID,kp1,kd1,kp2,ki2*`).
  * *Downlink (Telemetry)*: STM32 $\xrightarrow{\text{UART DMA (20Hz)}}$ ESP32-S3 $\xrightarrow{\text{WebSockets/TCP}}$ Web Dashboard / PC VOFA+ (`$TEL,pitch,pitch_target,velocity,pwm_l,pwm_r\r\n`).

### 6.4. Lưu ý & Cạm bẫy kỹ thuật khi sử dụng ESP32-S3
1. **Dòng khởi động và phát sóng Wi-Fi**: Khi phát sóng RF Wi-Fi, ESP32-S3 có các xung dòng đỉnh ngắn lên tới 400mA - 500mA. Do đó, bắt buộc phải lấy nguồn 5V từ ngõ ra của mạch Buck XL4015 (5A), tuyệt đối không lấy nguồn từ chân 3.3V của STM32F411 vì sẽ gây sụt áp làm reset chip STM32.
2. **Chọn chân GPIO UART trên ESP32-S3**: ESP32-S3 cho phép ma trận GPIO Matrix gán bất kỳ chân nào cho UART. Tránh các chân Strapping Pin (như GPIO0, GPIO45, GPIO46) hoặc các chân nối sẵn với Flash/PSRAM Octal (GPIO33 đến GPIO37) để không làm treo bootloader. Khuyến nghị gán UART1 qua cặp chân GPIO17 (TX) và GPIO18 (RX).
3. **Phân bổ nhân xử lý (Dual-core FreeRTOS)**: Nên chạy tác vụ nhận/truyền UART với STM32 trên Core 1, còn tác vụ Web Server / Wi-Fi / BLE chạy trên Core 0 để đảm bảo không bị nghẽn ngắt dữ liệu.

---

## 7. PIN LIPO 3S (11.1V - 12.6V) & MẠCH HẠ ÁP BUCK XL4015 (5A)

### 7.1. Vai trò trong đồ án
* **Pin LiPo 3S**: Khối pin gồm 3 cell mắc nối tiếp (Điện áp khi sạc đầy là $12.6\text{V}$, điện áp danh định $11.1\text{V}$). Pin LiPo có dòng xả lớn (25C - 35C), cung cấp dòng xả tức thời mạnh mẽ cho 2 động cơ GA25 khi tăng tốc đua hoặc thắng gấp mà không bị nghẽn nguồn.
* **Mạch Buck XL4015 (5A)**: Đảm nhiệm hạ áp từ 12V của pin xuống đúng **5.0V** ổn định, công suất dư dả để cấp nguồn nuôi số cho vi điều khiển STM32F411, module GY-BMI160 và board ESP32-S3 N16R8.

### 7.2. Phương thức cấp nguồn & Vận hành Plug-and-Play (Không đo đạc pin)
* **Đặc thù thực nghiệm của xe**: Xe tự cân bằng hoạt động trong các phiên thực nghiệm, biểu diễn thuật toán hoặc chạy đua ngắn (từ vài phút đến 15 phút mỗi lần), không cần vận hành liên tục nhiều giờ liền.
* **Quy trình vận hành tối giản**:
  1. **Sạc đầy pin trước khi chạy**: Sử dụng bộ sạc cân bằng chuyên dụng (như B3 hoặc B6) để sạc đầy khối pin 3S lên mức $12.6\text{V}$.
  2. **Cắm giắc XT60 là chạy (Plug-and-Play)**: Cắm nguồn pin trực tiếp vào xe qua giắc XT60 chống cắm ngược, bật công tắc nguồn xe để hệ thống khởi động, dựng đứng xe và điều khiển.
  3. **Ngắt pin khi kết thúc**: Sau khi hoàn thành phiên chạy hoặc biểu diễn, người dùng chỉ cần rút giắc XT60 để ngắt nguồn hoàn toàn.
* **Lợi ích của việc loại bỏ khối đo đạc kiểm thử pin**:
  * Không cần hàn thêm mạch cầu chia điện trở ngoài phần cứng, giúp mạch gọn gàng, giảm dây nối chằng chịt và loại bỏ nguy cơ chập mạch.
  * Tiết kiệm chân phần cứng trên vi điều khiển STM32 cho các mục đích mở rộng khác.
  * Giải phóng tài nguyên tính toán của CPU: Không cần chạy bộ chuyển đổi ADC DMA, mã nguồn tinh gọn và tập trung 100% vào giải thuật điều khiển cân bằng tốc độ cao.

---

## 8. KẾT CẤU CƠ KHÍ KHUNG XE DIY & PHỤ KIỆN TRUYỀN ĐỘNG

> [!NOTE]
> Khung cơ khí của xe được chế tạo theo dạng **DIY (Do-It-Yourself)** linh hoạt dựa trên các vật tư đã mua trong danh sách linh kiện, không gò bó theo khuôn mẫu thương mại đúc sẵn.

### 8.1. Các vật tư cơ khí phục vụ lắp ráp DIY:
1. **2 Tấm nhựa Mica trong suốt**: Làm 2 tầng sàn xe (tầng 1 đặt động lực/driver/pin, tầng 2 đặt mạch điều khiển và IMU).
2. **Cọc đồng M3 (4 trục cái - cái 6cm & 4 trụ đực - cái 5cm)**: Ghép nối và định cự ly chiều cao giữa các tầng sàn.
3. **2 Gá kim loại động cơ GA25 (25mm)**: Bắt vít cố định cặp động cơ GA25 vào sàn đáy, đảm bảo 2 trục bánh xe thẳng hàng.
4. **2 Khớp nối đồng lục giác trục D 4mm & 2 Bánh xe cao su 65mm**: Truyền mô-men xoắn từ hộp số 1:30 ra mặt sàn với độ bám đường cao.

---



