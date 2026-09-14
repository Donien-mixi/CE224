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
      (PWM 20kHz) │               │ (PWM 20kHz)   │      │ (SPI1)   │ (UART)
                  │               │               │      │          │
           [Motor Trái]       [Motor Phải]        │   [ICM-20602] [Bluetooth HC-05]
           GA25-370 12V       GA25-370 12V        │   Cảm biến IMU Điện thoại/PC
                  │               │               │   Đo góc & Gyro Lái xe & Tune PID
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
* **Tốc độ xung nhịp 100MHz**: Đủ mạnh để chạy vòng lặp điều khiển thời gian thực **200Hz (5ms)** mà CPU vẫn rảnh rỗi hơn 80% để truyền telemetry và nhận lệnh Bluetooth.
* **Hệ thống Timer phần cứng cực mạnh**: Tích hợp các bộ đếm giải mã xung Encoder và Timer phát xung PWM đối xứng tâm (Center-aligned) mà không tốn tài nguyên CPU.

### 2.3. Cách hệ thống sử dụng STM32F411
1. **Tạo nhịp tim ngắt 200Hz**: Cấu hình Timer 4 đếm định thời chính xác cứ mỗi $5.000\text{ ms}$ thì phát ngắt CPU một lần để thực thi vòng lặp cân bằng.
2. **Đọc Encoder phần cứng**: Cấu hình Timer 2 (bánh trái) và Timer 3 (bánh phải) ở chế độ `Encoder Mode TI12`. Phần cứng tự đếm xung của động cơ, CPU chỉ việc đọc thanh ghi số đếm.
3. **Phát xung PWM 20kHz**: Cấu hình Timer 1 phát 4 kênh xung điều khiển chiều và tốc độ động cơ.
4. **Đọc cảm biến tốc độ cao**: Dùng bộ SPI1 tốc độ 6.25MHz để đọc dữ liệu gia tốc và con quay hồi chuyển từ ICM-20602 trong chưa đầy 400 micro-giây.
5. **Truyền nhận Bluetooth**: Dùng bộ UART1 kết hợp DMA để gửi dữ liệu telemetry lên đồ thị PC và nhận lệnh điều khiển mà không làm đơ/nghẽn hệ thống.

### 2.4. Lưu ý & Cạm bẫy kỹ thuật
* **Bẫy xung đột chân (Pinout Conflict)**: Chân mặc định của USART1 (PA9, PA10) trùng với chân PWM của TIM1. Bắt buộc phải chuyển (remap) USART1 sang chân `PB6` và `PB7`. Tương tự, Timer 3 Encoder phải dùng `PB4` và `PB5` để không đè lên chân SPI1 của IMU.
* **Bật phần cứng FPU**: Nếu quên bật FPU trong file khởi tạo, trình biên dịch sẽ sinh mã mô phỏng phần mềm chậm gấp 20 lần.

---

## 3. CẢM BIẾN QUÁN TÍNH ICM-20602 (IMU 6 TRỤC)

### 3.1. Vai trò trong đồ án
ICM-20602 là "tai trong / mắt thần" của xe, liên tục đo lường:
1. Thân xe đang bị ngả về phía trước hay phía sau một góc bao nhiêu độ ($\theta$).
2. Thân xe đang ngã nhanh hay chậm (vận tốc góc $\omega$).

### 3.2. Tại sao chọn ICM-20602 thay vì MPU6050?
* MPU6050 là cảm biến đời cũ, cực kỳ nhạy cảm với rung động cơ học. Khi lắp lên xe chạy động cơ công suất lớn, độ rung từ bánh xe truyền vào làm MPU6050 bị nhiễu loạn, dẫn đến xe bị ngã khi vừa tăng tốc.
* ICM-20602 là cảm biến chuyên dụng cho **FPV Racing Drone (drone đua tốc độ cao)**. Nó có cấu trúc cơ vi cơ điện tử (MEMS) cứng vững hơn, dải đo rộng hơn ($\pm 2000^\circ/\text{s}$ và $\pm 8\text{g}$) và tích hợp bộ lọc số thông thấp nội (DLPF) kháng rung cực tốt.

### 3.3. Nguyên lý hoạt động thực tế & Sự kết hợp 2 cảm biến
Bên trong ICM-20602 có 2 cảm biến hoạt động theo 2 nguyên lý vật lý khác nhau:

| Cảm biến thành phần | Đo lường đại lượng gì? | Ưu điểm cốt lõi | Nhược điểm chí mạng |
| :--- | :--- | :--- | :--- |
| **Gia tốc kế (Accelerometer)** | Đo vectơ trọng lực $g$ của Trái Đất theo 3 trục ($A_x, A_y, A_z$). | **Không bị trôi theo thời gian**. Khi xe đứng yên, tính góc nghiêng tĩnh $\arctan2(A_x, A_z)$ cực kỳ chuẩn xác. | **Cực kỳ nhạy cảm với rung chấn và lực quán tính**. Khi xe chạy nhanh hoặc rung lắc, gia tốc kế sẽ đọc sai góc. |
| **Con quay hồi chuyển (Gyroscope)** | Đo vận tốc góc quay quanh trục ngã ($\omega_y$, đơn vị $^\circ/\text{s}$). | **Phản hồi tức thời cực nhanh**, không bị ảnh hưởng bởi rung chấn mặt sàn. | **Bị trôi tĩnh (Zero-rate Drift)**. Nếu chỉ tích phân $\int \omega dt$ để tính góc, sau 10 giây góc sẽ bị trôi lệch hàng chục độ. |

$$\implies \textbf{Giải pháp: BỘ LỌC BÙ (Complementary Filter)}$$
Phần mềm sẽ kết hợp cả 2: Lấy ưu thế phản ứng nhanh ngắn hạn của Gyroscope ($98\%$) kết hợp với ưu thế giữ chuẩn dài hạn của Accelerometer ($2\%$) để tạo ra một góc nghiêng $\theta$ vừa mượt, vừa không có độ trễ, vừa không bị trôi!

### 3.4. Lưu ý & Cạm bẫy kỹ thuật
* **Bắt buộc hiệu chuẩn độ trôi tĩnh (Static Calibration)**: Khi vừa bật nguồn, xe phải được đặt nằm yên tuyệt đối trong 2 - 3 giây để STM32 lấy 500 mẫu Gyroscope tính ra giá trị sai số tĩnh (Bias) và trừ ra khỏi mọi phép đo sau đó.
* **Đệm xốp chống rung**: Bắt buộc phải gắn cảm biến qua một lớp đệm xốp hoặc cao su chống rung FPV, không bắt ốc trực tiếp lên khung cứng để triệt tiêu sóng rung cao tần từ hộp số.

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

## 6. MODULE BLUETOOTH HC-05 (GIAO TIẾP KHÔNG DÂY)

### 6.1. Vai trò trong đồ án
1. **Truyền dữ liệu lên đồ thị thời gian thực (Telemetry Downlink)**: Gửi góc nghiêng, góc đặt, vận tốc và công suất động cơ về máy tính để vẽ đồ thị theo thời gian thực (dùng phần mềm VOFA+ hoặc Python). Nhờ đó, bạn nhìn thấy rõ đáp ứng bước (Step Response) của xe để biết xe đang thiếu hay thừa $K_p, K_d$.
2. **Nhận lệnh điều khiển từ xa (Command Uplink)**: Nhận lệnh từ joystick điện thoại để lái xe tiến, lùi, rẽ trái, rẽ phải, bật chế độ đua.
3. **Hiệu chỉnh PID trực tiếp (Wireless PID Tuning)**: Cho phép gửi lệnh đổi $K_p, K_i, K_d$ ngay khi xe đang đứng cân bằng mà không cần cắm dây nạp lại firmware.

### 6.2. Cách sử dụng & Giao thức truyền thông
* Kết nối: Chân TX của HC-05 nối vào RX của STM32 (`PB7`), chân RX của HC-05 nối vào TX của STM32 (`PB6`).
* Tốc độ truyền: Cài đặt tốc độ cao **115200 bps** (dùng lệnh AT trước khi lắp vào xe).
* Cơ chế nhận dữ liệu: Dùng chế độ **DMA + Idle Line Interrupt** trên STM32 để nhận trọn vẹn chuỗi lệnh từ điện thoại mà không làm chậm nhịp tim điều khiển 200Hz.
* Tần số gửi Telemetry: Đặt ở mức **20Hz (mỗi 50ms gửi 1 gói tin)**. Tuyệt đối không gửi ở tần số 200Hz vì sẽ làm nghẽn băng thông truyền UART.

---

## 7. PIN LIPO 3S (11.1V - 12.6V) & MẠCH HẠ ÁP BUCK XL4015 (5A)

### 7.1. Vai trò trong đồ án
* **Pin LiPo 3S**: Khối pin gồm 3 cell mắc nối tiếp (Điện áp khi sạc đầy là $12.6\text{V}$, điện áp danh định $11.1\text{V}$). Pin LiPo có dòng xả lớn (25C - 35C), cung cấp dòng xả tức thời mạnh mẽ cho 2 động cơ GA25 khi tăng tốc đua hoặc thắng gấp mà không bị nghẽn nguồn.
* **Mạch Buck XL4015 (5A)**: Đảm nhiệm hạ áp từ 12V của pin xuống đúng **5.0V** ổn định, công suất dư dả để cấp nguồn nuôi số cho vi điều khiển STM32F411, cảm biến ICM-20602 và module Bluetooth HC-05.

### 7.2. Phương thức cấp nguồn & Vận hành Plug-and-Play (Không đo đạc pin)
* **Đặc thù thực nghiệm của xe**: Xe tự cân bằng hoạt động trong các phiên thực nghiệm, biểu diễn thuật toán hoặc chạy đua ngắn (từ vài phút đến 15 phút mỗi lần), không cần vận hành liên tục nhiều giờ liền.
* **Quy trình vận hành tối giản**:
  1. **Sạc đầy pin trước khi chạy**: Sử dụng bộ sạc cân bằng chuyên dụng (như B3 hoặc B6) để sạc đầy khối pin 3S lên mức $12.6\text{V}$.
  2. **Cắm giắc XT60 là chạy (Plug-and-Play)**: Cắm nguồn pin trực tiếp vào xe qua giắc XT60 chống cắm ngược, bật công tắc nguồn xe để hệ thống khởi động, dựng đứng xe và điều khiển.
  3. **Ngắt pin khi kết thúc**: Sau khi hoàn thành phiên chạy hoặc biểu diễn, người dùng chỉ cần rút giắc XT60 để ngắt nguồn hoàn toàn.
* **Lợi ích của việc loại bỏ khối đo đạc kiểm thử pin**:
  * Không cần hàn thêm mạch cầu chia điện trở (10k/2.2k) ngoài phần cứng, giúp mạch gọn gàng, giảm dây nối chằng chịt và loại bỏ nguy cơ chập mạch.
  * Tiết kiệm chân phần cứng `PA2` (ADC1_IN2) trên vi điều khiển STM32 cho các mục đích mở rộng khác.
  * Giải phóng tài nguyên tính toán của CPU: Không cần chạy bộ chuyển đổi ADC DMA, không cần thuật toán lọc trung bình trượt Moving Average 16 mẫu loại trừ sụt áp, mã nguồn trở nên tinh gọn và tập trung 100% vào giải thuật điều khiển cân bằng tốc độ cao.

